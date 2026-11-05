
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Windows.Forms;

namespace resurce_packer
{
    partial class Form1
    {

        UInt32 readInt32()
        {
            byte[] data = resCollection[selectedIndex].img_byte_arr;
            if (data == null || (UInt64)p + 4 > (UInt64)data.Length)
                throw new InvalidDataException("Файл шрифта обрывается внутри 32-битного поля.");

            UInt32 res = 0;
            res = (UInt32)data[p++] << 24;
            res |= (UInt32)data[p++] << 16;
            res |= (UInt32)data[p++] << 8;
            res |= (UInt32)data[p++];
            return res;
        }

         byte[] writeInt32(UInt32 _in)
        {
            byte[] res = new byte[4];

            res[0] = (byte)((_in >> 24) & 0xFF);
            res[1] = (byte)((_in >> 16) & 0xFF);
            res[2] = (byte)((_in >> 8) & 0xFF);
            res[3] = (byte)(_in & 0xFF);

            return res;
        }

        byte[] writeInt32LitleEndian(UInt32 _in)
        {
            byte[] res = new byte[4];

            res[3] = (byte)((_in >> 24) & 0xFF);
            res[2] = (byte)((_in >> 16) & 0xFF);
            res[1] = (byte)((_in >> 8) & 0xFF);
            res[0] = (byte)(_in & 0xFF);

            return res;
        }

        List<Gliph> GliphCollection = new List<Gliph>();  //Коллекция глифов

        fontMetrics gFont;
        UInt32 p;
        UInt32 p_start; //Стартовый индекс массива для выделения глифа

        void Font_Smooth_Load()
        {
            ClearGlyphCollection();

            byte[] data = resCollection[selectedIndex].img_byte_arr;
            if (data == null || data.Length < 24)
                throw new InvalidDataException("Файл VLW слишком короткий: ожидается заголовок из 24 байт.");

            p = 0;
            p_start = 0;
            UInt32 glyphCount = readInt32();
            if (glyphCount > UInt16.MaxValue)
                throw new InvalidDataException("В VLW заявлено слишком много глифов.");

            if ((UInt64)24 + (UInt64)glyphCount * 28 > (UInt64)data.Length)
                throw new InvalidDataException("В VLW отсутствует часть таблицы глифов.");

            gFont.gCount   = (UInt16)glyphCount;       // glyph count in file
            readInt32();                               // vlw encoder version - discard
            gFont.yAdvance = (UInt16)readInt32();      // Font size in points, not pixels
            readInt32();                               // discard
            gFont.ascent   = (Int16) readInt32();      // top of "d"
            gFont.descent  = (Int16) readInt32();      // bottom of "p""

            gFont.maxAscent  = (UInt16) gFont.ascent;   // Determined from metrics
            gFont.maxDescent = (UInt16) gFont.descent;  // Determined from metrics
            gFont.yAdvance   = (UInt16)(gFont.ascent + gFont.descent);
            gFont.spaceWidth = (UInt16)(gFont.yAdvance / 4);  // Guess at space width

            Font_Smooth_loadMetrics(gFont.gCount);
        }

        void Font_Smooth_loadMetrics(UInt16 gCount)
        {
            UInt32 bitmapPtr = 24 + (UInt32)gCount * 28;

            for (int i = 0; i < gCount; i++)
            {
                Gliph g = new Gliph();
                g.gUnicode = (UInt16)readInt32(); // Unicode code point value
                g.gHeight = (UInt16)readInt32(); // Height of glyph
                g.gWidth = (UInt16)readInt32(); // Width of glyph
                g.gxAdvance = (UInt16)readInt32(); // xAdvance - to move x cursor
                g.gdY = (Int16)readInt32(); // y delta from baseline
                g.gdX = (Int16)readInt32(); // x delta from cursor
                readInt32(); // ignored

                // Different glyph sets have different descent values not always based on "p", so get maximum glyph descent
                if (((Int16)g.gHeight - (Int16)g.gdY) > gFont.maxDescent)
                {
                    // Avoid UTF coding values and characters that tend to give duff values
                    if (((g.gUnicode > 0x20) && (g.gUnicode < 0xA0)
                            && (g.gUnicode != 0x7F)) || (g.gUnicode > 0xFF))
                    {
                        gFont.maxDescent = (UInt16)(g.gHeight - g.gdY);
                    }
                }

                UInt32 bitmapLength = checked((UInt32)g.gWidth * (UInt32)g.gHeight);
                if ((UInt64)bitmapPtr + bitmapLength > (UInt64)resCollection[selectedIndex].img_byte_arr.Length)
                    throw new InvalidDataException("В VLW отсутствуют данные изображения одного из глифов.");

                g.gBitmap = bitmapPtr;
                bitmapPtr += bitmapLength;
                GliphCollection.Add(g);
            }

            gFont.yAdvance = (UInt16)(gFont.maxAscent + gFont.maxDescent);
            gFont.spaceWidth = (UInt16)((gFont.ascent + gFont.descent) * 2 / 7); // Guess at space width

            foreach (Gliph g in GliphCollection)
            {
                //Записываем в дата данные глифа в коллекцию
                int glyphDataLength = checked((int)((UInt32)g.gHeight * (UInt32)g.gWidth));
                g.data = new byte[glyphDataLength];
                Array.Copy(resCollection[selectedIndex].img_byte_arr, (int)g.gBitmap, g.data, 0, glyphDataLength);

                //Создаем картинку для каждого глифа и помещаем к коллекцию
                g.bmp = Font_Smooth_drawGlyph(g.gUnicode);

                dataGridView1.Rows.Add(new object[] { g.bmp, g.gUnicode.ToString(), String.Format("0X{0:X}", g.gUnicode) });
            }
        }

        Bitmap Font_Smooth_drawGlyph(UInt16 code)
        {
            Int16 X = 2;
            Int16 Y = 5;

            Bitmap bmp = new Bitmap(64, 64);

            UInt32 p_gBitmap; //Указатель на графические данные
            Byte pixel;
            //Byte getColor;
            //getColor = 1;

            UInt16 gNum = 0;
            bool found;

            found = false;
            for (UInt16 i = 0; i < gFont.gCount; i++)
            {
                if (GliphCollection[i].gUnicode == code)
                {
                    gNum = i;
                    found = true;
                    break;
                }
            }

            Color fg = Color.White;
            Color bg = Color.Blue;
            using (Graphics g = Graphics.FromImage(bmp))
            using (Pen pixelBrush = new Pen(Color.White))
            {
                g.Clear(Color.Blue);
                if (found) //Если нашли символ
                {
                    if (X == 0) X -= GliphCollection[gNum].gdX;

                    p_gBitmap = p_start + GliphCollection[gNum].gBitmap; //Начало графических данных

                    Int16  xs = 0;
                    UInt32 dl = 0;

                    Int16 cy = (Int16)(Y + gFont.maxAscent - GliphCollection[gNum].gdY);
                    Int16 cx = (Int16)(X + GliphCollection[gNum].gdX);


                    for (int y = 0; y < GliphCollection[gNum].gHeight; y++)
                    {
                        for (int x = 0; x < GliphCollection[gNum].gWidth; x++)
                        {
                            pixel = resCollection[selectedIndex].img_byte_arr[p_gBitmap++];

                            if (pixel > 0)
                            {
                                if (pixel != 0xFF)
                                {
                                    if (dl > 0)
                                    {
                                        if (dl == 1)
                                            bmp.SetPixel(xs, y + cy, fg);
                                        else
                                            g.DrawLine(pixelBrush, xs, y + cy, xs + dl, y + cy);

                                        dl = 0;
                                    }

                                //if (getColor>0)
                                //  bg = bmp.GetPixel(x + cx, y + cy);

                                    bmp.SetPixel(x + cx, y + cy, alphaBlend(pixel, fg, bg));

                                }
                                else
                                { if (dl == 0) xs = (short)(x + cx); dl++; }
                            }
                            else
                            {
                                if (dl > 0)
                                {
                                    g.DrawLine(pixelBrush, xs, y + cy, xs + dl, y + cy);
                                    dl = 0;
                                }
                            }
                        }
                        if (dl > 0)
                        {
                            g.DrawLine(pixelBrush, xs, y + cy, xs + dl, y + cy);
                            dl = 0;
                        }
                    }
                    X += (short)(GliphCollection[gNum].gxAdvance);
                }
            }

            return bmp;
        }

        Color alphaBlend(byte alpha, Color fgc, Color bgc)
        {
            // For speed use fixed point maths and rounding to permit a power of 2 division
            byte fgR = fgc.R;
            byte fgG = fgc.G;
            byte fgB = fgc.B;

            byte bgR = bgc.R;
            byte bgG = bgc.G;
            byte bgB = bgc.B;

            byte r = (byte)(((fgR * alpha) + (bgR * (255 - alpha)) + 127) / 255);
            byte g = (byte)(((fgG * alpha) + (bgG * (255 - alpha)) + 127) / 255);
            byte b = (byte)(((fgB * alpha) + (bgB * (255 - alpha)) + 127) / 255);

            // Combine RGB565 colours into 16 bits
            return Color.FromArgb(r, g, b);
        }


        /// <summary>
        /// Превращаем структуры фонта в массив байтов в resCollection (для кнопки сохранить глиф)
        /// </summary>
        private void convertGliphToImageByteArray()
        {
            if (GliphCollection.Count > UInt16.MaxValue)
                throw new InvalidOperationException("Слишком много глифов для формата VLW.");

            gFont.gCount = (UInt16)GliphCollection.Count;
            //Очищаем текщий массив
            resCollection[selectedIndex].img_byte_arr = null;

            //создаем новый массив
            List<byte> massiv = new List<byte>();
            massiv.AddRange(writeInt32((UInt32)gFont.gCount));
            massiv.AddRange(writeInt32(0));
            massiv.AddRange(writeInt32((UInt32)gFont.yAdvance));
            massiv.AddRange(writeInt32(0));
            massiv.AddRange(writeInt32((UInt32)gFont.ascent));
            massiv.AddRange(writeInt32((UInt32)gFont.descent));

            //Заполняем описалово для каждого глифа 28 байт * gFont.gCount
            for (int i = 0; i < gFont.gCount; i++)
            {
                massiv.AddRange(writeInt32((UInt32)(GliphCollection[i].gUnicode)));
                massiv.AddRange(writeInt32((UInt32)(GliphCollection[i].gHeight)));
                massiv.AddRange(writeInt32((UInt32)(GliphCollection[i].gWidth)));
                massiv.AddRange(writeInt32((UInt32)(GliphCollection[i].gxAdvance)));
                massiv.AddRange(writeInt32((UInt32)(GliphCollection[i].gdY)));
                massiv.AddRange(writeInt32((UInt32)(GliphCollection[i].gdX)));
                massiv.AddRange(writeInt32(0));
            }

            //Заполняем данных для каждого глифа
            for (int i = 0; i < gFont.gCount; i++)
            {
                massiv.AddRange(GliphCollection[i].data);
            }

            resCollection[selectedIndex].img_byte_arr = massiv.ToArray();

            listBox2.Items.Clear();
            listBox2.Items.Add("Размер: " + resCollection[selectedIndex].img_byte_arr.Length.ToString());
            listBox2.Items.Add("Модуль 4: " + (resCollection[selectedIndex].img_byte_arr.Length % 4).ToString());
        }

        /// <summary>
        /// Поиск того что Index присусвует в массиве massiv
        /// </summary>
        /// <param name="index"></param>
        /// <param name="massiv"></param>
        /// <returns></returns>
        private bool seachUnicodeInArray(ushort index, ushort[] massiv)
        {
            foreach (ushort i in massiv)
                if (index == i) return true;
            return false;
        }



        /// <summary>
        /// Преобразование Image ARGB в массив байтов
        /// </summary>
        /// <param name="value">Исходна картинка</param>
        /// <returns>Массив байтов</returns>
         byte[] ImageToBytes(Image value)
        {
            ImageConverter converter = new ImageConverter();
            byte[] arr = (byte[])converter.ConvertTo(value, typeof(byte[]));
            return arr;
        }

        /// <summary>
        /// Преобразование массив байтов в Image ARGB 
        /// </summary>
        /// <param name="value">Массив байтов</param>
        /// <returns>Image</returns>
        Image BytesToImage(byte[] value)
        {
            using (var ms = new MemoryStream(value))
            using (var source = Image.FromStream(ms))
            {
                return new Bitmap(source);
            }
        }


        /// <summary>
        /// Преобразование Image ARGB массив байтов с учетом битности преобразования 
        /// </summary>
        /// <param name="value">Image</param>
        /// <param name="bit">Битность 16 24 32 бит</param>
        /// <returns>Массив байтов</returns>
        byte[] ImageToBytesBit(Image value, int bit)
        { 
            if (bit != 16 && bit != 24 && bit != 32)
                throw new ArgumentOutOfRangeException("bit", "Поддерживаются только 16, 24 и 32 bit.");

            List<byte> arr = new List<byte>();
            using (Bitmap bmp = new Bitmap(value))
            {
                for (int y = 0; y < bmp.Height; y++)
                {
                    for (int x = 0; x < bmp.Width; x++)
                    {
                        Color Pixel = bmp.GetPixel(x, y);
                        byte A = Pixel.A;
                        byte R = Pixel.R;
                        byte G = Pixel.G;
                        byte B = Pixel.B;

                        if (bit == 32)
                        {
                            arr.Add(A);arr.Add(R);arr.Add(G);arr.Add(B);
                        }

                        if (bit == 24)
                        {
                            arr.Add(A);
                            arr.AddRange(RGB565(R, G, B));
                        }


                        if (bit == 16)
                        {
                            arr.AddRange(RGB565(R, G, B));
                        }
                    }
                }
            }

            return arr.ToArray();
        }

        /// <summary>
        /// Преобразование RGB в мссив из двух байт
        /// </summary>
        /// <param name="R">Красный</param>
        /// <returns>Image</returns>
        byte[] RGB565(byte R, byte G, byte B)
        {
            UInt16 r;
            r = (UInt16)(((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3));
            byte[] ret = { 0, 0 };
            ret[1] = (byte)(r >> 8);
            ret[0] = (byte)(r & 0xFF);
            return ret;
        }



        /// <summary>
        /// Логирование, вывод в listbox log
        /// </summary>
        /// <param name="str">Выводимая строка</param>
        private void LOG(string str)
        {
            // #this 
            // sdfsdfsdfsdf sdfsdfsd dsfsdfsdf

            log.SelectionMode = SelectionMode.One;
            log.Items.Add(str);
            log.SelectedIndex = log.Items.Count - 1;
        }

        private void SetPreviewImage(Image image)
        {
            Image previous = pBox.Image;
            pBox.Image = image;
            pBox.BackColor = image == null ? Color.Magenta : SystemColors.Control;
            if (previous != null)
                previous.Dispose();
        }

        private void ClearGlyphCollection()
        {
            foreach (Gliph glyph in GliphCollection)
                if (glyph.bmp != null)
                    glyph.bmp.Dispose();

            GliphCollection.Clear();
            dataGridView1.Rows.Clear();
        }



        List<resItem> swap (List<resItem> list, int firstIndex, int secondIndex)
        {
            resItem temp = list[firstIndex];
            list[firstIndex] = list[secondIndex];
            list[secondIndex] = temp;

            return list;
        }













    }

    public struct fontMetrics
    {
        public UInt16 gCount;     // Total number of characters
        public UInt16 yAdvance;   // Line advance
        public UInt16 spaceWidth; // Width of a space character
        public Int16  ascent;     // Height of top of 'd' above baseline, other characters may be taller
        public Int16  descent;    // Offset to bottom of 'p', other characters may have a larger descent
        public UInt16 maxAscent;  // Maximum ascent found in font
        public UInt16 maxDescent; // Maximum descent found in font
    };
    class resItem
    {
        public int    id { get; set; }              //id ресурса
        public int    start_adress { get; set; }    //Стартовый адресс  //Не используем
        public string name { get; set; }         //Название
        public string type { get; set; }         //тип ресурса BMP FONT 
        public int    H              { get; set; }               //высота
        public int    W              { get; set; }               //ширина
        public int    bit            { get; set; }             //тип ресурс
        public byte[] img_byte_arr { get; set; }
        
        byte[] binData  { get; set;}

        public void setBinData(byte [] data)
        {
            binData = data;
        }
        public byte[] getBinData()
        {
             return binData;
        }

        public UInt32 getBinLength()
        {
            return (UInt32)binData.Length;
        }

    }
    class Gliph
    {
        public UInt16 gUnicode { get; set; } //Unicode 16 bit Basic Multilingual Plane(0-FFFF)
        public UInt16 gHeight { get; set; } // Height of glyph
        public UInt16 gWidth { get; set; } // Width of glyph
        public UInt16 gxAdvance { get; set; } // xAdvance - to move x cursor
        public Int16 gdY { get; set; } // offset from bitmap top edge from lowest point in any character
        public Int16 gdX { get; set; } // offset for bitmap left edge relative to cursor X
        public UInt32 gBitmap { get; set; } // В данных от начала глифа
        public byte[] data { get; set; } // В данные глифа
        public Bitmap bmp { get; set; }
    }

}

