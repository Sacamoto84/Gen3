using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;


namespace resurce_packer
{

    public partial class Form1 : Form
    {
        List<resItem> resCollection = new List<resItem>();
        int selectedIndex = -1; //Текущий выбранный индекс
        private readonly string workingDirectory;
        //UInt32 start_adress = 0x0800C000;





        public Form1()
        {
            InitializeComponent();
            workingDirectory = Environment.CurrentDirectory;
        }

        private string WorkingPath(string fileName)
        {
            return Path.Combine(workingDirectory, fileName);
        }

        private static void WriteFileAtomically(string destinationPath, Action<string> writeTemporaryFile)
        {
            string temporaryPath = destinationPath + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try
            {
                writeTemporaryFile(temporaryPath);
                if (File.Exists(destinationPath))
                    File.Replace(temporaryPath, destinationPath, null);
                else
                    File.Move(temporaryPath, destinationPath);
            }
            finally
            {
                if (File.Exists(temporaryPath))
                    File.Delete(temporaryPath);
            }
        }

        private bool HasSelectedResource()
        {
            if (selectedIndex >= 0 && selectedIndex < resCollection.Count)
                return true;

            MessageBox.Show("Сначала выберите ресурс в списке.", "Ресурс не выбран",
                MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return false;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            resCollection.Add(new resItem() { id = resCollection.Count, name = "+", start_adress = 0x80000 });
            Collection_to_list();
        }

        //Открытите
        private void Form1_Shown(object sender, EventArgs e)
        {
            //string jsonString = "";

            string collectionPath = WorkingPath("collection.json");
            if (File.Exists(collectionPath))
            {
                try
                {
                    resCollection = JsonConvert.DeserializeObject<List<resItem>>(File.ReadAllText(collectionPath))
                        ?? new List<resItem>();
                }
                catch (Exception ex) when (ex is IOException || ex is JsonException)
                {
                    resCollection = new List<resItem>();
                    LOG("Не удалось открыть collection.json: " + ex.Message);
                    MessageBox.Show("Файл collection.json повреждён или недоступен. Открыт пустой проект.",
                        "Ошибка загрузки", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }
            else
              LOG("json не найден");

            //if (resCollection == null)
            //{
                //resCollection.Add(new resItem() { id = 10, name = "+" });
            //}
            Collection_to_list();
            //CalculateSummBytes();

            tbAdress.Text = Properties.Settings.Default.adress;
         
        }

        private void Collection_to_list()
        {
            listBox1.Items.Clear();

            if (resCollection == null) return;

            int index = 0;

            foreach (resItem item in resCollection)
            {
                item.id = index;
                string str = "Индекс: " + index.ToString() + "  Тип: " + item.type +" bit:"+item.bit.ToString() + "  Имя: " + item.name ;
                listBox1.Items.Add(str);
                index++;
            }
            LOG("Обновить listbox");
        }

        //Выбрали строку в listbox1
        private void listBox1_SelectedIndexChanged(object sender, EventArgs e)
        {
            int index = listBox1.SelectedIndex;
            if (index == -1) return;
            //LOG(index.ToString());

            labelIndex.Text = index.ToString();
            selectedIndex = index;

            //Заполняем TextBox
            tbID.Text   = index.ToString();
            tbName.Text = resCollection[index].name;

            comboBoxBMPBit.Text = resCollection[index].bit.ToString();


            listBox2.Items.Clear();
            if (resCollection[index].type == "BMP")
            {
                listBox2.Items.Add("Высота : " + resCollection[index].H.ToString());
                listBox2.Items.Add("Ширина : " + resCollection[index].W.ToString());
                listBox2.Items.Add("Пикселей : " + resCollection[index].H * resCollection[index].W);
                listBox2.Items.Add("32b : " + resCollection[index].H * resCollection[index].W * 4);
                listBox2.Items.Add("24b : " + resCollection[index].H * resCollection[index].W * 3);
                listBox2.Items.Add("16b : " + resCollection[index].H * resCollection[index].W * 2);

                dataGridView1.Enabled = false;
            }
            if (resCollection[index].type == "FONT")
            {
                if (resCollection[index].img_byte_arr != null)
                {
                    dataGridView1.Enabled = true;
                    listBox2.Items.Add("Размер: " + resCollection[index].img_byte_arr.Length.ToString());
                    listBox2.Items.Add("Модуль 4: " + (resCollection[index].img_byte_arr.Length % 4).ToString());
                    try
                    {
                        Font_Smooth_Load();
                    }
                    catch (InvalidDataException ex)
                    {
                        dataGridView1.Enabled = false;
                        LOG("Некорректный шрифт: " + ex.Message);
                        MessageBox.Show(ex.Message, "Ошибка шрифта", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    }
                }
                else
                {
                    listBox2.Items.Add("Нет данных");
                }
            }

            //Перерисовка картинки
            if ((resCollection[index].img_byte_arr != null) && (resCollection[index].type == "BMP"))
            {
                try
                {
                    SetPreviewImage(BytesToImage(resCollection[index].img_byte_arr));
                }
                catch (ArgumentException ex)
                {
                    SetPreviewImage(null);
                    LOG("Некорректное изображение: " + ex.Message);
                }
            }
            else
            {
                SetPreviewImage(null);
            }

        }

        //Удаление записи
        private void bDelete_Click(object sender, EventArgs e)
        {
            if (selectedIndex < 0 || selectedIndex >= resCollection.Count)
                return;

            resCollection.RemoveAt(selectedIndex); //Удаляем выбранную запись
            selectedIndex = -1;
            Collection_to_list();
        }

        private void buttonSaveJson_Click(object sender, EventArgs e)
        {
            try
            {
                string jsonString = JsonConvert.SerializeObject(resCollection);
                WriteFileAtomically(WorkingPath("collection.json"), path => File.WriteAllText(path, jsonString));
                LOG("Сохранить json");
            }
            catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException)
            {
                LOG("Не удалось сохранить JSON: " + ex.Message);
                MessageBox.Show("Не удалось сохранить collection.json.", "Ошибка сохранения",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void buttonSaveItem_Click(object sender, EventArgs e)
        {
            //Сохранение данных из TextBox в структуре
            if (selectedIndex < 0 || selectedIndex >= resCollection.Count)
                return;

            int bit;
            if (!Int32.TryParse(comboBoxBMPBit.Text, out bit))
            {
                MessageBox.Show("bit должен быть целым числом.", "Некорректные данные",
                    MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            resCollection[selectedIndex].id = selectedIndex;
            resCollection[selectedIndex].name = tbName.Text;
            resCollection[selectedIndex].bit = bit;
            Collection_to_list();
        }



        /// <summary>
        /// Кнопка открыть картинку
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void buttonOpenPicture_Click(object sender, EventArgs e)
        {
            if (!HasSelectedResource())
                return;

            //#TagName7

            //При открытии#TagName помещаем в коллекцию массив
            using (var ofd = new OpenFileDialog())
            {
                ofd.RestoreDirectory = true;
                if (ofd.ShowDialog() == DialogResult.OK)
                {
                    using (var image = new Bitmap(ofd.FileName))
                    {
                        resCollection[selectedIndex].img_byte_arr = ImageToBytes(image);
                        resCollection[selectedIndex].H = image.Height;
                        resCollection[selectedIndex].W = image.Width;
                        resCollection[selectedIndex].type = "BMP";
                        SetPreviewImage(new Bitmap(image));
                    }
                }
            }
        }



        /// <summary>
        /// Кнопка Открыть фонт и записать в img_byte_arr
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void buttonOpenFont_Click(object sender, EventArgs e)
        {
            if (!HasSelectedResource())
                return;

            //При открытии помещаем в коллекцию массив
            using (var ofd = new OpenFileDialog())
            {
                ofd.RestoreDirectory = true;
                ofd.Filter = "Шрифты Processing (*.vlw)|*.vlw";
                ofd.FilterIndex = 1;
                if (ofd.ShowDialog() == DialogResult.OK)
                {
                    byte[] fileBytes = File.ReadAllBytes(ofd.FileName);
                    resCollection[selectedIndex].img_byte_arr = fileBytes;
                    resCollection[selectedIndex].type = "FONT";
                    LOG("Количество байт: " + fileBytes.Length.ToString());
                }
            }
            Collection_to_list();
        }

        /// <summary>
        /// Кнопка удалить глиф
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void buttonGlyphDelete_Click(object sender, EventArgs e)
        {
            if (dataGridView1.CurrentCell == null)
                return;

            int index = dataGridView1.CurrentCell.RowIndex;
            if (index < 0 || index >= GliphCollection.Count)
                return;

            Gliph deletedGlyph = GliphCollection[index];
            GliphCollection.RemoveAt(index); //Удаляем из коллекции
            if (deletedGlyph.bmp != null)
                deletedGlyph.bmp.Dispose();
            gFont.gCount = (ushort)GliphCollection.Count;

            //Перерисовка
            dataGridView1.Rows.Clear();
            foreach (Gliph g in GliphCollection)
            {
                g.data = new byte[g.gHeight * g.gWidth];
                Array.Copy(resCollection[selectedIndex].img_byte_arr, g.gBitmap, g.data, 0, g.gHeight * g.gWidth);
                dataGridView1.Rows.Add(new object[] { g.bmp, g.gUnicode.ToString(), String.Format("0X{0:X}", g.gUnicode) });
            }

        }

        /// <summary>
        /// Кнопка сохранить в коллекцию <br/>
        /// Проевращаем структуры фонта в массив байтов в resCollection 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void buttonSaveFont_Click(object sender, EventArgs e)
        {
            if (!HasSelectedResource())
                return;

            convertGliphToImageByteArray();
        }

        /// <summary>
        /// Кнопка удалить кучу мусора за раз
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void buttonGlyphDeleteArray_Click(object sender, EventArgs e)
        {
            if (!HasSelectedResource())
                return;

            ushort[] massiv = {0x400,0x402, 0x403, 0x404, 0x405, 0x406, 0x407, 0x408, 0x409, 0x40A, 0x40B,
                0x40C, 0x40D, 0x40E, 0x40F, 1104, 1251};
            for (int i = GliphCollection.Count - 1; i >= 0; i--)
            {
                if (seachUnicodeInArray(GliphCollection[i].gUnicode, massiv))
                {
                    if (GliphCollection[i].bmp != null)
                        GliphCollection[i].bmp.Dispose();
                    GliphCollection.RemoveAt(i);
                }
            }
            gFont.gCount = (ushort)GliphCollection.Count;

            //Перерисовка
            dataGridView1.Rows.Clear();
            foreach (Gliph g in GliphCollection)
            {
                dataGridView1.Rows.Add(new object[] { g.bmp, g.gUnicode.ToString(), String.Format("0X{0:X}", g.gUnicode) });
            }
        }

        /// <summary>
        /// Событие> Выбрана строка
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void dataGridView1_SelectionChanged(object sender, EventArgs e)
        {

            if (dataGridView1.CurrentCell == null)
                return;

            int index = dataGridView1.CurrentCell.RowIndex;
            if (index < 0 || index >= GliphCollection.Count)
                return;
            int selRowNum = Convert.ToInt32(dataGridView1[1, dataGridView1.CurrentCell.RowIndex].Value);

            //LOG("g " + index.ToString());


            SetPreviewImage(Font_Smooth_drawGlyph((ushort)Convert.ToInt32(selRowNum)));

            listBox3.Items.Clear();
            listBox3.Items.Add("Выслота: " + GliphCollection[index].gHeight.ToString());
            listBox3.Items.Add("Ширина: " + GliphCollection[index].gWidth.ToString());
            int i = GliphCollection[index].gWidth * GliphCollection[index].gHeight;
            listBox3.Items.Add("Размер: " + i.ToString());
            listBox3.Items.Add("Код: " + String.Format("#{0:X}", GliphCollection[index].gUnicode));
        }


        //Кнопка сохранить все в Bin
        private void buttonSaveAllToBin_Click(object sender, EventArgs e)
        {
            if (resCollection == null || resCollection.Count == 0)
            {
                LOG("Проект пустой");
                return;
            }

            if (resCollection.Count > Byte.MaxValue)
            {
                MessageBox.Show("Формат BIN поддерживает не более 255 ресурсов.", "Слишком много ресурсов",
                    MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }


            LOG("Создаем BIN файл");
            List<byte> BIN = new List<byte>();
            uint sum = 0;


            //Пробегаем по коллекции и создаем bindata для каждого елемента
            foreach(resItem item in resCollection)
            {
                if (item.type == "FONT")
                {
                    LOG("----------------------");
                    LOG("Это Font");
                    //Проверяем модуль item.img_byte_arr
                    if (item.img_byte_arr == null || item.img_byte_arr.Length == 0)
                    {
                        ShowInvalidResource(item, "у шрифта нет данных");
                        return;
                    }

                    int mod = item.img_byte_arr.Length % 4;
                    LOG("img_byte_arr.Length:" + item.img_byte_arr.Length.ToString());
                    LOG("Модуль:" + mod.ToString());
                    if (mod > 0)
                    {
                        List<byte> temp = new List<byte>();
                        temp.AddRange(item.img_byte_arr);
                        if (mod == 1) { temp.Add(0);temp.Add(0);temp.Add(0);}
                        if (mod == 2) { temp.Add(0); temp.Add(0);}
                        if (mod == 3) { temp.Add(0);}
                        item.setBinData(temp.ToArray());
                        LOG("temp.Count:"  +  temp.Count.ToString()) ;
                        LOG("Модуль:"      + (temp.Count % 4).ToString());
                        sum += item.getBinLength();
                    }
                    else
                    {
                        item.setBinData(item.img_byte_arr);
                        sum += item.getBinLength();
                    }         
                }

                if (item.type == "BMP")
                {
                   if (item.img_byte_arr == null || item.img_byte_arr.Length == 0)
                   {
                       ShowInvalidResource(item, "у картинки нет данных");
                       return;
                   }
                   if (item.bit != 16 && item.bit != 24 && item.bit != 32)
                   {
                       ShowInvalidResource(item, "допустимы только 16, 24 или 32 bit");
                       return;
                   }

                   LOG("----------------------");
                   LOG("Это BMP Имя: "+ item.name +" H=" + item.H.ToString() + " W=" + item.W.ToString() + " bit:" + item.bit.ToString());
                   try
                   {
                       using (Image image = BytesToImage(item.img_byte_arr))
                       {
                           if (item.H != image.Height || item.W != image.Width)
                           {
                               ShowInvalidResource(item, "размеры в проекте не совпадают с изображением");
                               return;
                           }
                           item.setBinData(ImageToBytesBit(image, item.bit));
                       }
                   }
                   catch (ArgumentException ex)
                   {
                       ShowInvalidResource(item, "не удаётся прочитать изображение: " + ex.Message);
                       return;
                   }
                   LOG("Размер Bin: " +item.getBinLength());
                   sum += item.getBinLength();
                }

                if (item.type != "FONT" && item.type != "BMP")
                {
                    ShowInvalidResource(item, "не выбран тип BMP или FONT");
                    return;
                }
            }

            LOG("----------------------");

            LOG("Общий размер Data " + sum.ToString());
            //Дата мы создали теперь собираем дескрипторы
            BIN.Add((byte)resCollection.Count);
            BIN.Add(0);BIN.Add(0);BIN.Add(0);
            UInt32 baseOffset = (UInt32)(4 + resCollection.Count * 16); //Начало данных
            
            foreach(resItem item in resCollection)
            {
                BIN.AddRange( writeInt32LitleEndian((UInt32)(item.W)));      // only BMP
                BIN.AddRange( writeInt32LitleEndian((UInt32)(item.H)));      // only BMP
                BIN.AddRange( writeInt32LitleEndian((UInt32)(item.bit)));    // only BMP
                BIN.AddRange( writeInt32LitleEndian((UInt32)(baseOffset)));  // Основные данные

                baseOffset += item.getBinLength();
            }

            foreach (resItem item in resCollection)
            {
                BIN.AddRange( item.getBinData());
            }

            LOG("Общий размер BIN = " + BIN.Count.ToString() +" байт");
            LOG("Модуль BIN:"      + (BIN.Count % 4).ToString());

            byte[] fileout = BIN.ToArray();

            LOG(fileout.Length.ToString());

            try
            {
                WriteFileAtomically(WorkingPath("res.bin"), path => File.WriteAllBytes(path, fileout));
                LOG("BIN сохранён: " + WorkingPath("res.bin"));
            }
            catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException)
            {
                LOG("Не удалось сохранить BIN: " + ex.Message);
                MessageBox.Show("Не удалось сохранить res.bin.", "Ошибка сохранения",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

        }




        /// <summary>
        /// R
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private async void buttonFlash_Click(object sender, EventArgs e)
        {
            uint address;
            if (!TryParseAddress(tbAdress.Text, out address))
            {
                MessageBox.Show("Адрес должен быть 32-битным числом, например 0x08030000.", "Некорректный адрес",
                    MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            if (!File.Exists(WorkingPath("res.bin")))
            {
                MessageBox.Show("Сначала создайте res.bin.", "Нет BIN-файла", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            buttonFlash.Enabled = false;
            try
            {
                string commandPath = WorkingPath("jlink.Command");
                File.WriteAllLines(commandPath, new[]
                {
                    "loadbin res.bin,0x" + address.ToString("X8"),
                    "verifybin res.bin,0x" + address.ToString("X8"),
                    "r",
                    "exit"
                }, new UTF8Encoding(false));

                using (Process process = new Process())
                {
                    process.StartInfo = new ProcessStartInfo
                    {
                        FileName = "jlink",
                        Arguments = "-device STM32F407VG -if SWD -speed 1000 -CommanderScript \"jlink.Command\"",
                        WorkingDirectory = workingDirectory,
                        UseShellExecute = false,
                        RedirectStandardOutput = true,
                        RedirectStandardError = true,
                        CreateNoWindow = true
                    };

                    process.Start();
                    Task<string> standardOutput = process.StandardOutput.ReadToEndAsync();
                    Task<string> standardError = process.StandardError.ReadToEndAsync();
                    await Task.Run(() => process.WaitForExit());
                    string output = await standardOutput;
                    string error = await standardError;

                    LOG("J-Link завершился с кодом " + process.ExitCode + ".");
                    if (!String.IsNullOrWhiteSpace(output)) LOG(output.Trim());
                    if (!String.IsNullOrWhiteSpace(error)) LOG(error.Trim());
                    if (process.ExitCode != 0)
                        MessageBox.Show("J-Link завершился с ошибкой. Подробности есть в журнале.", "Ошибка прошивки",
                            MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            catch (Exception ex) when (ex is System.ComponentModel.Win32Exception || ex is IOException || ex is UnauthorizedAccessException)
            {
                LOG("Не удалось запустить J-Link: " + ex.Message);
                MessageBox.Show("Не удалось запустить jlink. Проверьте, что J-Link установлен и доступен в PATH.",
                    "Ошибка прошивки", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                buttonFlash.Enabled = true;
            }
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
        }

        private void ShowInvalidResource(resItem item, string reason)
        {
            string name = String.IsNullOrWhiteSpace(item.name) ? "без имени" : item.name;
            string message = "Ресурс '" + name + "': " + reason + ".";
            LOG(message);
            MessageBox.Show(message, "Некорректный ресурс", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }

        private static bool TryParseAddress(string input, out uint address)
        {
            address = 0;
            if (String.IsNullOrWhiteSpace(input)) return false;
            string value = input.Trim();
            if (value.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
                value = value.Substring(2);
            return UInt32.TryParse(value, System.Globalization.NumberStyles.AllowHexSpecifier,
                System.Globalization.CultureInfo.InvariantCulture, out address);
        }


        private void buttonSaveAdress_Click(object sender, EventArgs e)
        {
            Properties.Settings.Default.adress = tbAdress.Text;
            Properties.Settings.Default.Save();
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }

        private void listBox3_SelectedIndexChanged(object sender, EventArgs e)
        {

        }

        private void tbID_TextChanged(object sender, EventArgs e)
        {

        }

        private void label2_Click(object sender, EventArgs e)
        {

        }

        private void label1_Click(object sender, EventArgs e)
        {

        }

        private void dataGridView1_CellContentClick(object sender, DataGridViewCellEventArgs e)
        {

        }

        private void tbName_TextChanged(object sender, EventArgs e)
        {

        }

        private void label3_Click(object sender, EventArgs e)
        {

        }

        private void label5_Click(object sender, EventArgs e)
        {

        }

        private void label4_Click(object sender, EventArgs e)
        {

        }

        private void buttonUP_Click(object sender, EventArgs e)
        {
            int index = listBox1.SelectedIndex;
            if (index == 0) return;

            resCollection = swap(resCollection, index, index - 1);
            Collection_to_list();

        }

        private void buttonDown_Click(object sender, EventArgs e)
        {
            int index = listBox1.SelectedIndex;
            if (index == resCollection.Count -1) return;
            resCollection = swap(resCollection, index, index + 1);
            Collection_to_list();
        }
    }




}
