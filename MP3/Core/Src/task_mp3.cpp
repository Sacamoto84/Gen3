#include "global.h"
#include "mp3dec.h"
#include "fatfs.h"
#include "playerTask.h"
#include "debugTask.h"
#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

#include "timber.h"
#include "TFT_convert.h"

extern "C" uint32_t get_newlib_heap_free(void);	// свободно в newlib-куче (sysmem.c)

#define MP3_GAIN 1.8F

static volatile bool taskMP3_terminate;


extern volatile uint32_t dma_tc_sequence;
extern volatile uint32_t dma_just_completed;
extern volatile uint32_t dac_underruns;

#define MP3_FILEBUFF_SIZE	4096		// размер файлового буфера

#define MONO_SUPPORT

extern void stopError(void);
extern void DAC_setSampleRate(int sample = 44100);
extern void DAC_DMA_Pause(void);
extern void init_MP3_DAC_DMA(void);
extern void DAC_DMA_Play(void);
extern void DAC_DMA_ClearBuffer(void);

const osThreadAttr_t myTaskMP3_attributes = {
	  .name = "TaskMP3",
	  .stack_size = 512 * 4,
	  .priority = (osPriority_t) osPriorityNormal,
	};

osThreadId_t myTaskMP3Handle;

static void MP3_Deinit(void);

static osSemaphoreId_t mp3DoneSem;	// сигнал о завершении задачи-декодера

//Команда переключения паузы от HMI (1 = переключить) и фактическое состояние.
//Единственный владелец DAC/DMA - задача декодера: HMI только запрашивает.
volatile bool    mp3_paused = false;
volatile uint8_t mp3_cmd_pause = 0;

decodeStatistic_t decodeStatistic;
debug_mode_t      debug_mode;		    // отладочные флаги

extern int16_t   outBuff[2][1152*2] RAM_16; // буфер выходного потока

/// результат чтения потока MP3 из файла для функции
typedef enum { READ_OK =  0, READ_ERROR	= -1, READ_END_DATA	= -2} read_result_t;

typedef struct
{

	// файловые переменные
	//FIL     *mp3_file;					    // указатель на играемый файл

	uint32_t fileAddr;						// адрес первого непрочитанного из файла байта

	// переменные для декодера
	HMP3Decoder  hMP3Decoder;				// указатель на ОЗУ декодера
	MP3FrameInfo mp3FrameInfo;				// параметры фрейма

	// буферы
	uint32_t  t[3];
	uint8_t   inputBuf[MP3_FILEBUFF_SIZE];	// буфер для входного потока

	uint32_t  outBuffPtr;					// указатель на свободный буфер

	uint8_t   * inputDataPtr;	// указатель начала данных в файловом буфере
	int32_t   bytes_left;		// указатель количества данных в файловом буфере
	 int32_t  samprate;
	uint32_t  frameCNT;			// счётчик декодированных фреймов

	// локальные переменные для управления ЦАП и задачей проигрывателя
	//cmd_t	 	player_cmd;
	//OS_FlagID 	dac_DataReqFlag;
	//OS_FlagID 	player_CmdMailBox;
	//dac_cmd_i 	* DAC_interface;

	uint32_t	summaryDecodeTime_mks;
	uint32_t	DecodeTime_Ticks;

	float gain; //Усиление

} mp3DecoderState_t;

//*-----------------------------------------------------------------------------------------------
//*			Переменные
//*-----------------------------------------------------------------------------------------------

//malloc
static mp3DecoderState_t * mp3DecoderState;	// переменная состояния, для которой выделяется место в куче

// выделить память для декодера
//mp3DecoderState->hMP3Decoder = MP3InitDecoder();

void MP3Task(void);

/*************************************************************************************************
 * @brief	Инициализация задачи-декодера MP3
 *
 * @param 	decoder_params - указатель на интерфейс управления проигрывателем
 * @return	указатель на функцию деинициализации декодера
 ************************************************************************************************/
void mp3_player_init(void)
{
	timber.info ("Свободно памяти %d", xPortGetFreeHeapSize());
	timber.info ("Свободно newlib heap %d", (int)get_newlib_heap_free());

	// выделить память для задачи
	mp3DecoderState = (mp3DecoderState_t*)malloc(sizeof(mp3DecoderState_t));

	if (mp3DecoderState == NULL)
	{
		timber.print("Ошибка выделения памяти для mp3DecoderState\n");
		stopError();
	}
	timber.successful("OK выделения памяти для mp3DecoderState\n");
	timber.info ("Свободно памяти %d", xPortGetFreeHeapSize());

	// выделить память для декодера
	mp3DecoderState->hMP3Decoder = MP3InitDecoder();
	if (mp3DecoderState->hMP3Decoder == NULL)
	{
		timber.print("Ошибка выделения памяти для MP3InitDecoder\n");
		stopError();
	}
	timber.successful("OK выделения памяти для MP3InitDecoder");
	timber.info ("Свободно памяти %d", xPortGetFreeHeapSize());

	//mp3DecoderState->mp3_file = &SDFile;

	mp3DecoderState->inputDataPtr = NULL;
	mp3DecoderState->bytes_left = 0;
	mp3DecoderState->frameCNT = 0;
	mp3DecoderState->outBuffPtr = 0;
	mp3DecoderState->samprate = 0;
	mp3DecoderState->fileAddr = 0;

	mp3DecoderState->DecodeTime_Ticks = HAL_GetTick();

}


static read_result_t ReadMP3buff(mp3DecoderState_t * mp3DecoderState)
{
	// если буфер полный, ничего не делать
	if (mp3DecoderState->bytes_left >= MP3_FILEBUFF_SIZE)
		return READ_OK;

	// если в файловом буфере остались данные, переместить их в начало буфера
	// (inputDataPtr указывает внутрь inputBuf - области перекрываются,
	// memmove корректен при перекрытии, memcpy - UB)
	if (mp3DecoderState->bytes_left > 0)
	{
		memmove(mp3DecoderState->inputBuf, mp3DecoderState->inputDataPtr, mp3DecoderState->bytes_left);
	}
	assert_param(mp3DecoderState->bytes_left >= 0 && mp3DecoderState->bytes_left <= MP3_FILEBUFF_SIZE);

	// прочитать очередную порцию данных
	uint32_t bytes_to_read = MP3_FILEBUFF_SIZE - mp3DecoderState->bytes_left;
	uint32_t bytes_read;

	FRESULT result = f_read(&SDFile,
							(BYTE *)mp3DecoderState->inputBuf + mp3DecoderState->bytes_left,
							bytes_to_read,
							(UINT*)&bytes_read);

	if (result != FR_OK)
	{
		return READ_ERROR;
	}

	//Реальная позиция в файле (не "запрошено", а "прочитано")
	mp3DecoderState->fileAddr = f_tell(&SDFile);

	mp3DecoderState->inputDataPtr = mp3DecoderState->inputBuf;
	mp3DecoderState->bytes_left += bytes_read;

	if (bytes_read == bytes_to_read)
		return READ_OK;
	else
		return READ_END_DATA;
}

/*************************************************************************************************
 * @brief	Задача декодера MP3
 * @param	pdata - указатель на данные (не используется)
 ***********************************************************************************************/
//Завершение задачи-декодера: освобождение ресурсов, сигнал play() и выход
static void mp3TaskExit(void)
{
	f_close(&SDFile);
	DAC_DMA_Pause();
	DAC_DMA_ClearBuffer();
	MP3_Deinit();
	if (mp3DoneSem != NULL)
		osSemaphoreRelease(mp3DoneSem);
	osThreadExit();
}

//Выполнить запрошенные HMI команды. Вызывается только из контекста задачи-декодера,
//поэтому управление DAC/DMA не гоняется с декодированием.
static void serviceMp3Pause(void)
{
	if (mp3_cmd_pause)
	{
		mp3_cmd_pause = 0;
		if (mp3_paused)
		{
			DAC_DMA_Play();
			mp3_paused = false;
		}
		else
		{
			DAC_DMA_Pause();
			DAC_DMA_ClearBuffer();
			mp3_paused = true;
		}
	}
}

void MP3(char * mp3name)
{
	timber.info("MP3:mp3name %s", mp3name);
	debug_mode.showDecoderInfo = true;
	//debug_mode.showFrameDecodeTime = true;

	mp3_player_init();

	init_MP3_DAC_DMA();
    DAC_DMA_ClearBuffer(); //Очистить выходной буффер

    bool init        = true; //Init режим init
    int init_count   = 0;


    //DAC_DMA_Play();


    mString <64> fullPath;
    fullPath = globalPach;
    fullPath += "/";
    fullPath += mp3name;

	//fullPath собран в UTF-8 (список хранит UTF-8), а FatFs работает в OEM CP866
	char path_oem[_MAX_LFN + 1];
	if (ConvertStringUTF8to1251(fullPath.c_str(), path_oem, sizeof(path_oem)) == 0)
		strcpy(path_oem, fullPath.c_str());
	ConvertString1251ToDos(path_oem);

	//Открыть файл 'GIFtoBIN.exe'
	FRESULT result = f_open(&SDFile, path_oem , FA_READ);
	if (result != FR_OK)
	{
		timber.error("MP3:%s..Ошибка открытия файла", fullPath.c_str());
		mp3TaskExit();   // сигнал play() + освобождение ресурсов, без бесконечного декодирования
	}
	else
		timber.successful("MP3:%s..OK", fullPath.c_str());

	playerInfo = PlayerInfo{};
	int filesize = f_size(&SDFile);
	timber.info("MP3:Размер файла %d", filesize);
	playerInfo.filesize = filesize;

	//Текущее положение
	playerInfo.flseekcurrent = f_tell (&SDFile);
	playerInfo.calculatePersent(); //Расчет процента

	uint32_t t = uwTick;

	mp3DecoderState->gain = MP3_GAIN;

	playerInfo.filename = mp3name;
	playerInfo.filename.truncate(4);

	taskMP3_terminate =  false;
	mp3_cmd_pause = 0;   // не переносить команду от "простаивающего" нажатия на новый трек
	mp3_paused = false;

	uint32_t last_seq = 0;	// последний обработанный dma_tc_sequence

	for(;;)
	{


		if(taskMP3_terminate)
		{
			timber.warning("Завершение MP3 по внешнему запросу");
			taskMP3_terminate = false;
			mp3TaskExit();
		}

		//Командный гейт: пауза применяется до декодирования следующего фрейма
		serviceMp3Pause();
		if (mp3_paused)
		{
			osDelay(5);   // на паузе: ждём, пока HMI не запросит снятие
			continue;
		}


		if (init == false )
		{
		  //Ждём завершения очередного DMA-буфера (сравнение по счётчику, а не по флагу).
		  //Ожидание прерывается обработкой команд паузы.
		  for (;;)
		  {
			if (dma_tc_sequence != last_seq)
				break;
			osDelay(2);

			serviceMp3Pause();
			if (mp3_paused)
				break;   // ушли на паузу

			playerInfo.flseekcurrent = f_tell(&SDFile);
			playerInfo.calculatePersent(); //Расчет процента

		    if(taskMP3_terminate)
		    {
		    	timber.warning("Завершение MP3 по внешнему запросу");
		  			taskMP3_terminate = false;
		  			mp3TaskExit();
		    }
		  }
		  if (mp3_paused)
			continue;   // на паузу - следующий проход виснет в командном гейте

		  //Сколько передач прошло с нашей последней обработки
		  uint32_t missed = dma_tc_sequence - last_seq;
		  last_seq = dma_tc_sequence;
		  if (missed > 1)
		  {
			  dac_underruns++;
			  if ((dac_underruns % 100) == 1)
				  timber.warning("DAC underrun: пропущено %u буферов, всего %u",
						  (unsigned)(missed - 1), (unsigned)dac_underruns);
		  }
		}
		else
		  init_count++;


		playerInfo.timeCurrent += uwTick - t;
		t = uwTick;



		//
		  //останова
		//
		StartTimeMeasurement();	// начало контроля времени выполнения

		// прочитать порцию данных
		read_result_t fileReadResult = ReadMP3buff(mp3DecoderState);

		//Текущее положение
		playerInfo.flseekcurrent = f_tell (&SDFile);
		playerInfo.calculatePersent(); //Расчет процента
        playerInfo.gain = mp3DecoderState->gain;

		//rtt.print("Полное время проигрывания файла %u мс\n",(unsigned int)mp3DecoderState->DecodeTime_Ticks);

		////
		if (fileReadResult == READ_ERROR)
		{
			StopTimeMeasurement();
			mp3DecoderState->DecodeTime_Ticks = HAL_GetTick() - mp3DecoderState->DecodeTime_Ticks;

			if (debug_mode.showDecoderInfo)
			{
				timber.print("Чистое время декодирования файла %u мс\n",(unsigned int)mp3DecoderState->DecodeTime_Ticks);
			uint32_t temp = mp3DecoderState->summaryDecodeTime_mks/1000;
			timber.print("Длительность проигрывания файла %u мс\n",(unsigned int)temp);
			temp = mp3DecoderState->summaryDecodeTime_mks/10;
			temp /= mp3DecoderState->DecodeTime_Ticks;
			timber.print("Средняя загрузка контроллера %u%%\n",(unsigned int)temp);
				// отображается время выполнения только одной процедуры - MP3Decode

				timber.print("Декодировано %u фреймов\n", (unsigned int)mp3DecoderState->frameCNT);
			}

			//Ошибка чтения файла (не конец данных!) - завершаем задачу в любом случае,
			//иначе при выключенном debug был бы бесконечный цикл декодирования
			timber.error("Ошибка чтения файла, воспроизведение завершено\n");
			mp3TaskExit();
		}
		////

		if (fileReadResult == READ_END_DATA)
		{
			mp3DecoderState->DecodeTime_Ticks = HAL_GetTick() - mp3DecoderState->DecodeTime_Ticks;
			StopTimeMeasurement();
			if (debug_mode.showDecoderInfo)
			{
				uint32_t temp = mp3DecoderState->summaryDecodeTime_mks/1000;
				timber.print("Чистое время декодирования файла %u мс\n",(unsigned int)temp);
				timber.print("Полное время проигрывания файла %u мс\n",(unsigned int)mp3DecoderState->DecodeTime_Ticks);
				temp = mp3DecoderState->summaryDecodeTime_mks/10;
				temp /= mp3DecoderState->DecodeTime_Ticks;

				timber.print("Средняя загрузка контроллера %u%%",(unsigned int)temp);
				// отображается время выполнения только одной процедуры - MP3Decode

				timber.print("Декодировано %u фреймов\n", (unsigned int)mp3DecoderState->frameCNT);
				timber.info("Воспроизведение файла завершено полностью\n");
			}
			mp3TaskExit();
			continue;
		}

		// поиск синхрослова - начала фрейма
		int offset = MP3FindSyncWord(mp3DecoderState->inputDataPtr, mp3DecoderState->bytes_left);
		if (offset < 0)
		{
			timber.warning("синхро не найдено 1");
			// синхро не найдено, очистить буфер и прочитать из файла следующую порцию
			mp3DecoderState->bytes_left = 0;
			StopTimeMeasurement();
			continue;
		}

		// указатель на начало фрейма
		mp3DecoderState->inputDataPtr += offset;
		// пропустить ненужные данные в буфере (если есть)
		mp3DecoderState->bytes_left -= offset;

		// проверка валидности фрейма (тип, версия)
		int err = MP3GetNextFrameInfo(mp3DecoderState->hMP3Decoder, &mp3DecoderState->mp3FrameInfo, mp3DecoderState->inputDataPtr);
		if (err < 0
#ifndef MONO_SUPPORT
			|| mp3DecoderState->mp3FrameInfo.nChans != 2	// работаем пока только со стерео
#endif
			|| mp3DecoderState->mp3FrameInfo.layer != 3)
		{
			// поиск следующего фрейма
			mp3DecoderState->bytes_left -= 2;
			mp3DecoderState->inputDataPtr += 2;

			offset = MP3FindSyncWord(mp3DecoderState->inputDataPtr, mp3DecoderState->bytes_left);
			if (offset < 0)
			{
				timber.warning("синхро не найдено 2");
				// следующее синхро не найдено, очистить буфер
				mp3DecoderState->bytes_left = 0;
				StopTimeMeasurement();
				mp3DecoderState->frameCNT++;
				continue;
			}
			else
			{
				// найден следующий фрейм, повторить цикл
				mp3DecoderState->inputDataPtr += offset;
				mp3DecoderState->bytes_left -= offset;
				StopTimeMeasurement();
				mp3DecoderState->frameCNT++;
				continue;
			}

		}

		short * outbuf;

		if (init == false)
		{
			//Декодируем в буфер, который только что завершился (его уже не читает DMA)
			outbuf = outBuff[dma_just_completed];
		}
		else
		{
			// выбрать свободный буфер для декодированных данных
			outbuf = outBuff[mp3DecoderState->outBuffPtr];
			mp3DecoderState->outBuffPtr = mp3DecoderState->outBuffPtr ? 0: 1;
		}

		// декодирование фрейма
		uint32_t temp = GetCurrentTime();
		err = MP3Decode(mp3DecoderState->hMP3Decoder, &mp3DecoderState->inputDataPtr, (int *)&mp3DecoderState->bytes_left, outbuf, 0);
		temp = GetCurrentTime() - temp;
		mp3DecoderState->summaryDecodeTime_mks += temp;
		if (err < 0)
		{
		    DAC_DMA_ClearBuffer(); //Очистить выходной буффер
			playerInfo.error++; //Счетчик ошибок

			if (debug_mode.showDecoderInfo)
				timber.error("Ошибка (%d) декодирования фрейма %u, адрес чтения %u\n", (signed int)err,
					  (unsigned int)mp3DecoderState->frameCNT,
					  (unsigned int)mp3DecoderState->fileAddr);

			StopTimeMeasurement();

			// пропустить на 1 байт, чтобы не зависнуть на ошибочном фрейме
			// (если MP3Decode не заберёт ни одного байта из буфера inputBuf)
			if (mp3DecoderState->bytes_left > 0)	{ mp3DecoderState->bytes_left--; mp3DecoderState->inputDataPtr++; 	}
			mp3DecoderState->frameCNT++;
			continue;	// break;	- ошибка!
		}

		// обновить информацию о декодированном фрейме
		// нужно знать, сколько сэмплов декодировано и определить текущий битрейт
		MP3GetLastFrameInfo(mp3DecoderState->hMP3Decoder, &mp3DecoderState->mp3FrameInfo);

		// обновить частоту дискретизации
		//Буфер DMA вмещает 1152 стереопары за один фрейм. У LSF (MPEG-2/2.5) фрейм
		//содержит 576 сэмплов/канал и каждый сэмпл растягивается на 2 периода DAC
		//(zero-order hold), поэтому частота DAC для LSF удваивается - скорость верная.
		uint32_t samplesPerChan = (uint32_t)(mp3DecoderState->mp3FrameInfo.outputSamps / mp3DecoderState->mp3FrameInfo.nChans);
		uint32_t dacRate = (uint32_t)mp3DecoderState->mp3FrameInfo.samprate * ((samplesPerChan == 576) ? 2u : 1u);
		if (mp3DecoderState->samprate != (int32_t)dacRate)
		{
			timber.info("----------");
			timber.info("Битрейт %d", mp3DecoderState->mp3FrameInfo.bitrate);
			timber.info("Битность %d", mp3DecoderState->mp3FrameInfo.bitsPerSample);
			timber.info("outputSamps %d", mp3DecoderState->mp3FrameInfo.outputSamps);
			timber.info("Каналов %d", mp3DecoderState->mp3FrameInfo.nChans);
			timber.info("Set SampleRate %d (DAC %d)",mp3DecoderState->mp3FrameInfo.samprate, (int)dacRate);
			timber.info("layer %d",mp3DecoderState->mp3FrameInfo.layer);
			timber.info("version %d",mp3DecoderState->mp3FrameInfo.version);
			DAC_setSampleRate((int)dacRate);
			mp3DecoderState->samprate = (int32_t)dacRate;
		}



		// преобразовать данные в формат, понятный ЦАПу
		//Запомнить усиление до фрейма: если клиппинга не было (gain не менялся),
		//медленно восстанавливаем его к MP3_GAIN (AGC release).
		float gainBefore = mp3DecoderState->gain;
#ifdef MONO_SUPPORT
		if (mp3DecoderState->mp3FrameInfo.nChans == 2)
		{
			uint32_t outputSamps = mp3DecoderState->mp3FrameInfo.outputSamps;
			//Нормальный режим
			if (outputSamps == 2304)
			{
			 // для стереомузыки только сместить шкалу
			 for (uint32_t i = 0; i < outputSamps; i++)
			 {
				float f = ( outbuf[i])/65536.0F;

		 		int16_t sample = 2047 + (int16_t)(2047.0F  * mp3DecoderState->gain * f);
		 		if (sample > 4095)
		 		{
		 			sample = 4095;
		 			if (mp3DecoderState->gain > 0.1F) mp3DecoderState->gain -= 0.1F; else mp3DecoderState->gain = 0.1F;
		 		}
		 		if (sample < 0){
		 			sample = 0;
		 			if (mp3DecoderState->gain > 0.1F) mp3DecoderState->gain -= 0.1F; else mp3DecoderState->gain = 0.1F;
		 		}
		  		outbuf[i] = sample;
			 }
		}else
		//MPEG-2/LSF стерео: 576 пар L/R. Каждая пара удерживается 2 периода DAC
		//(zero-order hold x2, частота DAC удвоена) - скорость и стереокартина верные.
		//Проход обратный: расширение происходит in-place, вперёд нельзя.
			if (outputSamps == 1152)
			{
				 for (uint32_t i = outputSamps; i >= 2; i -= 2)
				 {
					float fL = ( outbuf[i - 2] )/65536.0F;
					float fR = ( outbuf[i - 1] )/65536.0F;
			 		int16_t sL = 2048 + (int16_t)(2047.0F * mp3DecoderState->gain * fL );
			 		int16_t sR = 2048 + (int16_t)(2047.0F * mp3DecoderState->gain * fR );
			 		if (sL > 4095 || sL < 0 || sR > 4095 || sR < 0)
			 		{
			 		  sL = (sL > 4095) ? 4095 : ((sL < 0) ? 0 : sL);
			 		  sR = (sR > 4095) ? 4095 : ((sR < 0) ? 0 : sR);
			 		  if (mp3DecoderState->gain > 0.1F) mp3DecoderState->gain -= 0.1F; else mp3DecoderState->gain = 0.1F;
			 		}
			 		uint32_t d = i * 2;	//пара i/2-1 занимает пары DAC d/2-2 и d/2-1
			  		outbuf[d - 4] = sL;
			  		outbuf[d - 3] = sR;
			  		outbuf[d - 2] = sL;
			  		outbuf[d - 1] = sR;
				 }
			}


		}
		else
		{
			uint32_t outputSamps = mp3DecoderState->mp3FrameInfo.outputSamps;
			if (outputSamps == 1152)
			{
				// для моно собрать семплы для левого и правого каналов из моно-сигнала
				short * sptr = &outbuf[mp3DecoderState->mp3FrameInfo.outputSamps - 1];
				short * dptr = &outbuf[mp3DecoderState->mp3FrameInfo.outputSamps*2 - 1];
				for (uint32_t i = 0; i < outputSamps; i++)
				{
					float f = (*sptr--)/65536.0F;
					int16_t sample = 2048 + (int16_t)(2047.0F * mp3DecoderState->gain * f);
			 		if (sample > 4095) {
			 		  sample = 4095;
			 		  if (mp3DecoderState->gain > 0.1F) mp3DecoderState->gain -= 0.1F; else mp3DecoderState->gain = 0.1F;
			 		}
			 		if (sample < 0) {
			 		  sample = 0;
			 		  if (mp3DecoderState->gain > 0.1F) mp3DecoderState->gain -= 0.1F; else mp3DecoderState->gain = 0.1F;
			 		}
					*dptr-- = sample ;//+ 0x8000;
					*dptr-- = sample ;//+ 0x8000;
				}
			}
			else
			if (outputSamps == 576)
			{
				// для моно собрать семплы для левого и правого каналов из моно-сигнала
				short * sptr = &outbuf[mp3DecoderState->mp3FrameInfo.outputSamps - 1];
				short * dptr = &outbuf[mp3DecoderState->mp3FrameInfo.outputSamps*4 - 1];
				for (uint32_t i = 0; i < outputSamps; i++)
				{
					float f = (*sptr--)/65536.0F;
					int16_t sample = 2048 + (int16_t)(2047.0F * mp3DecoderState->gain * f);
			 		if (sample > 4095) {
			 		  sample = 4095;
			 		  if (mp3DecoderState->gain > 0.1F) mp3DecoderState->gain -= 0.1F; else mp3DecoderState->gain = 0.1F;
			 		}
			 		if (sample < 0) {
			 		  sample = 0;
			 		  if (mp3DecoderState->gain > 0.1F) mp3DecoderState->gain -= 0.1F; else mp3DecoderState->gain = 0.1F;
			 		}
					*dptr-- = sample ;//+ 0x8000;
					*dptr-- = sample ;//+ 0x8000;
					*dptr-- = sample ;//+ 0x8000;
					*dptr-- = sample ;//+ 0x8000;
				}
			}

		}

#else
		for (uint32_t i = 0; i < mp3DecoderState->mp3FrameInfo.outputSamps; i++)
		{
			outbuf[i] += 0x8000;	// или проинвертировать старший бит, что то же самое
		}
#endif

		//AGC: если в этом фрейме клиппинга не было, плавно вернуть усиление к номиналу
		if (mp3DecoderState->gain == gainBefore)
		{
			mp3DecoderState->gain += 0.002F;
			if (mp3DecoderState->gain > MP3_GAIN)
				mp3DecoderState->gain = MP3_GAIN;
		}

		// отправить message ЦАПу
		//uint32_t len = mp3FrameInfo.outputSamps / mp3FrameInfo.nChans;	// делим на 2 канала
		//DAC_interface->SendData((uint32_t *)outbuf, len);

		mp3DecoderState->frameCNT++;
		uint32_t time = StopTimeMeasurement();

		if (debug_mode.showDecoderInfo && debug_mode.showFrameDecodeTime)
		{
			//printf("%u,%u\r\n", (unsigned int)mp3DecoderState->frameCNT, (unsigned int)time/100);
			// отобразить реальное время декодирования
			timber.print("%u,%u\n", (unsigned int)mp3DecoderState->frameCNT, (unsigned int)temp/100);
		}
		if (decodeStatistic.minCPUidle_mks > time)			decodeStatistic.minCPUidle_mks = time;
		if (decodeStatistic.maxCPUidle_mks < time)			decodeStatistic.maxCPUidle_mks = time;
		decodeStatistic.averageCPUidle_mks += time;

		if((init_count >= 2) && (init)){
			init = false;
		    DAC_DMA_Play();
		}

		//osDelay(1);

	}

	// аварийное завершение задачи недопустимо
	timber.print("Аварийное завершение задачи mp3Task\r\n");
	stopError();
}

static void MP3_Deinit(void)
{
	MP3FreeDecoder(mp3DecoderState->hMP3Decoder);
	free(mp3DecoderState);
	char str[64];
	sprintf(str,"MP3_Deinit: FreeRTOS %d, newlib %d",xPortGetFreeHeapSize(), (int)get_newlib_heap_free());
	timber.colorStringln(0, 183, str);
	timber.info("MP3 task stack HWM %d", (int)uxTaskGetStackHighWaterMark(NULL));

}

//FreeRTOS
void StartTaskMP3(void *argument)
{
  //Получаем имя запускаемого файла
	timber.successful("!!!Таска MP4 Запуск");
  MP3((char*)argument);
  for(;;)
  {
    osDelay(999);
  }
}




void play(char * name)
{
	timber.info("play %s", name);
	timber.info ("Свободно памяти play %d", xPortGetFreeHeapSize());

	if (mp3DoneSem == NULL)
		mp3DoneSem = osSemaphoreNew(1, 0, NULL);

	if (myTaskMP3Handle == NULL)
	{
		//osThreadState_t state = osThreadGetState (myTaskMP3Handle);
		//rtt.info("play NULL state %d", state);
		myTaskMP3Handle = osThreadNew(StartTaskMP3, (char *)name, &myTaskMP3_attributes);
	}
	else
	{

		timber.info("Ожидание завершения предыдущей задачи");

		//Запросить остановку декодера и дождаться её по семафору.
		//Поллинг osThreadGetState() по освобождённому TCB не используется.
		taskMP3_terminate = true;
		if (osSemaphoreAcquire(mp3DoneSem, 3000) != osOK)
		{
			timber.error("Таймаут ожидания завершения задачи MP3, трек не запущен");
			return;
		}

		osDelay(50);
		myTaskMP3Handle = osThreadNew(StartTaskMP3, (char *)name, &myTaskMP3_attributes);
		timber.info ("Свободно play запуск потока %d", xPortGetFreeHeapSize());
	}

}