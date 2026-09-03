using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text;
using System.Windows.Forms;
using System.Configuration;


namespace resurce_packer
{

    public partial class Form1 : Form
    {
        List<resItem> resCollection = new List<resItem>();
        int selectedIndex; //Текущий выбранный индекс
        //UInt32 start_adress = 0x0800C000;





        public Form1()
        {
            InitializeComponent();
           
        }

        private void button1_Click(object sender, EventArgs e)
        {
            resCollection.Add(new resItem() { id = 10, name = "+", start_adress = 0x80000 });
            Collection_to_list();
        }

        //Открытите
        private void Form1_Shown(object sender, EventArgs e)
        {
            //string jsonString = "";

            if (File.Exists("collection.json"))
            {
                resCollection = JsonConvert.DeserializeObject<List<resItem>>(File.ReadAllText("collection.json"));
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
                string str = "id: " + index.ToString() + "  Тип: " + item.type +" bit:"+item.bit.ToString() + "  Имя: " + item.name ;
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
                    Font_Smooth_Load();
                }
                else
                {
                    listBox2.Items.Add("Нет данных");
                }
            }

            //Перерисовка картинки
            if ((resCollection[index].img_byte_arr != null) && (resCollection[index].type == "BMP"))
                pBox.Image = BytesToImage(resCollection[index].img_byte_arr);
            else
            {
                Graphics g = pBox.CreateGraphics();
                g.Clear(Color.Magenta);
            }

        }

        //Удаление записи
        private void bDelete_Click(object sender, EventArgs e)
        {
            resCollection.RemoveAt(selectedIndex); //Удаляем выбранную запись
            Collection_to_list();
        }

        private void buttonSaveJson_Click(object sender, EventArgs e)
        {
            string jsonString = JsonConvert.SerializeObject(resCollection);
            File.WriteAllText("collection.json", jsonString);
            LOG("Сохранить json");
        }

        private void buttonSaveItem_Click(object sender, EventArgs e)
        {
            //Сохранение данных из TextBox в структуре
            resCollection[selectedIndex].id = Convert.ToInt32(tbID.Text);
            resCollection[selectedIndex].name = tbName.Text;
            resCollection[selectedIndex].bit = Convert.ToInt32(comboBoxBMPBit.Text);
            Collection_to_list();
        }



        /// <summary>
        /// Кнопка открыть картинку
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void buttonOpenPicture_Click(object sender, EventArgs e)
        {

            //#TagName7

            //При открытии#TagName помещаем в коллекцию массив
            var ofd = new OpenFileDialog();
            ofd.RestoreDirectory = true;
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                pBox.Image = Image.FromFile(ofd.FileName);
                Bitmap image = new Bitmap(ofd.FileName);
                resCollection[selectedIndex].img_byte_arr = ImageToBytes(image);
                resCollection[selectedIndex].H = image.Height;
                resCollection[selectedIndex].W = image.Width;
                resCollection[selectedIndex].type = "BMP";
                image.Dispose();
            }
        }



        /// <summary>
        /// Кнопка Открыть фонт и записать в img_byte_arr
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void buttonOpenFont_Click(object sender, EventArgs e)
        {
            //При открытии помещаем в коллекцию массив
            var ofd = new OpenFileDialog();

            ofd.RestoreDirectory = true;
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                byte[] fileBytes = File.ReadAllBytes(ofd.FileName);
                resCollection[selectedIndex].img_byte_arr = fileBytes;
                resCollection[selectedIndex].type = "FONT";
                LOG("Количество байт: " + fileBytes.Length.ToString());
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
            int index = dataGridView1.CurrentCell.RowIndex;

            GliphCollection.RemoveAt(index); //Удаляем из колекции
            gFont.gCount--;

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
            convertGliphToImageByteArray();
        }

        /// <summary>
        /// Кнопка удалить кучу мусора за раз
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void buttonGlyphDeleteArray_Click(object sender, EventArgs e)
        {

            ushort[] massiv = {0x400,0x402, 0x403, 0x404, 0x405, 0x406, 0x407, 0x408, 0x409, 0x40A, 0x40B,
                0x40C, 0x40D, 0x40E, 0x40F, 1104, 1251};
            int end = gFont.gCount;
            for (int ii = 0; ii < end; ii++)
            {
                for (int i = 0; i < GliphCollection.Count; i++)
                {
                    if (seachUnicodeInArray(GliphCollection[i].gUnicode, massiv) == true)
                    {
                        GliphCollection.RemoveAt(i);
                        gFont.gCount--;
                        break;
                    }
                }
            }

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

            int index = dataGridView1.CurrentCell.RowIndex;
            int selRowNum = Convert.ToInt32(dataGridView1[1, dataGridView1.CurrentCell.RowIndex].Value);

            //LOG("g " + index.ToString());


            pBox.Image = Font_Smooth_drawGlyph((ushort)Convert.ToInt32(selRowNum));

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
            if (resCollection == null)
            {
                LOG("Проект пустой");
                return;
            }


            LOG("Создаем BIN файл");
            List<byte> BIN = new List<byte>();
            uint sum = 0;


            //Пробегаем по коллекции и создаем bindata для каждого елемента
            foreach(resItem item in resCollection)
            {
                if (item.type == null) 
                {
                    byte [] t = { 0, 0, 0, 0 }; 
                    item.setBinData ( t );
                    sum += item.getBinLength();
                }

                //Пробегаем по коллекции
                if (item.type == "FONT")
                {
                    LOG("----------------------");
                    LOG("Это Font");
                    //Проверяем модуль item.img_byte_arr
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
                   LOG("----------------------");
                   LOG("Это BMP Имя: "+ item.name +" H=" + item.H.ToString() + " W=" + item.W.ToString() + " bit:" + item.bit.ToString());
                   item.setBinData(ImageToBytesBit(BytesToImage(item.img_byte_arr), item.bit));
                   LOG("Размер Bin: " +item.getBinLength());
                   sum += item.getBinLength();
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

            File.WriteAllBytes("res.bin", fileout);

        }




        /// <summary>
        /// R
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void buttonFlash_Click(object sender, EventArgs e)
        {

          StreamWriter Performing = new StreamWriter(Environment.CurrentDirectory + @"\Performing.bat");
          Performing.WriteLine("jlink -device STM32F407VG -if SWD -speed 1000 -CommanderScript jlink.Command", Encoding.UTF8);
          Performing.Close();

          StreamWriter Performing1 = new StreamWriter(Environment.CurrentDirectory + @"\jlink.Command");
          Performing1.WriteLine("loadbin   res.bin ,"+tbAdress.Text, Encoding.UTF8);
          Performing1.WriteLine("verifybin res.bin ,"+tbAdress.Text, Encoding.UTF8);
          Performing1.WriteLine("r", Encoding.UTF8);
          Performing1.WriteLine("exit", Encoding.UTF8);
          Performing1.Close();

          ProcessStartInfo startInfo = new ProcessStartInfo(Environment.CurrentDirectory + @"\Performing.bat");
          startInfo.WorkingDirectory = Application.StartupPath;
          Process.Start(startInfo);

          LOG("Прошивка");

        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            File.Delete("Performing.bat");
            File.Delete("jlink.Command");
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
