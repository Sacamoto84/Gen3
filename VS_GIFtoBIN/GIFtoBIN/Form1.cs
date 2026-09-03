using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Media.Imaging;
using System.Windows.Interop;
using System.Drawing.Imaging;
using System.Windows;
using System.Diagnostics;
using Gif.Components;

namespace GIFtoBIN
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        Bitmap [] bmplist;

        List<byte> output = new List<byte>();

        int H, W;

        private void button1_Click(object sender, EventArgs e)
        {

            var ofd = new OpenFileDialog();
            ofd.RestoreDirectory = true;
            ofd.Filter = "gif files (*.gif)|*.gif";


            if (ofd.ShowDialog() == DialogResult.OK)
            {
                 

                GifDecoder gifDecoder = new GifDecoder();
                gifDecoder.Read( ofd.FileName );

                bmplist = new Bitmap[gifDecoder.GetFrameCount()];

                for (int i = 0, count = gifDecoder.GetFrameCount(); i < count; i++)
                {
                    Image frame = gifDecoder.GetFrame(i);  // frame i
                    pBox.Image = frame;
                    bmplist[i] = (Bitmap)frame;
                    lbFrames.Items.Add(i.ToString());
                }

                H = (int)bmplist[0].Height;
                W = (int)bmplist[0].Width;
         
                listBox1.Items.Add("H: "+H.ToString());
                listBox1.Items.Add("W: "+W.ToString());
                pBox.Width  = (int)W;
                pBox.Height = (int)H;
                listBox1.Items.Add("Кадров: "+gifDecoder.GetFrameCount().ToString()); 

                pBox.Image = bmplist[0];

            }
        }

        private System.Drawing.Bitmap BitmapFromSource(BitmapSource bitmapsource)
        {
            System.Drawing.Bitmap bitmap;
            using (MemoryStream outStream = new MemoryStream())
            {
                BitmapEncoder enc = new BmpBitmapEncoder();

                enc.Frames.Add(BitmapFrame.Create(bitmapsource));
                enc.Save(outStream);
                bitmap = new System.Drawing.Bitmap(outStream);
            }
            return bitmap;
        }

        private void lbFrames_SelectedIndexChanged(object sender, EventArgs e)
        {
            pBox.Image = bmplist[lbFrames.SelectedIndex];
        }

        private void buttonSaveBIN_Click(object sender, EventArgs e)
        {
            if (bmplist == null) return;

            output.Clear();

            foreach (Bitmap bmp in bmplist)
            {
                output.AddRange(ImageToBytesBit(bmp , comboBox1.Text));
            }

            byte[] fileout = output.ToArray();
            listBox1.Items.Add(fileout.Length.ToString());

            File.WriteAllBytes("res.bin", fileout);

            List<string> str = new List<string>();
            str.Add(H.ToString());
            str.Add(W.ToString());
            if (comboBox1.Text == "32 ARGB")  str.Add("32");
            if (comboBox1.Text == "24 A565")  str.Add("24");
            if (comboBox1.Text == "16 565")   str.Add("16");

            str.Add(bmplist.Length.ToString());

            using (TextWriter tw = new StreamWriter("i.txt"))
            {
                foreach (String s in str)
                    tw.WriteLine(s);
            }

        }

        byte[] ImageToBytesBit(Image value, string bit)
        {
            List<byte> arr = new List<byte>();
            Bitmap bmp = new Bitmap(value);
            byte A = 0;
            byte R = 0;
            byte G = 0;
            byte B = 0;

            for (int y = 0; y < bmp.Height; y++)
            {
                for (int x = 0; x < bmp.Width; x++)
                {
                    Color Pixel = bmp.GetPixel(x, y);
                    A = Pixel.A; R = Pixel.R; G = Pixel.G; B = Pixel.B;

                    if (bit == "32 ARGB")
                    {
                        arr.Add(A); arr.Add(R); arr.Add(G); arr.Add(B);
                    }

                    if (bit == "24 A565")
                    {
                        arr.Add(A);
                        arr.AddRange(RGB565(R, G, B));
                    }

                    if (bit == "16 565")
                    {
                        arr.AddRange(RGB565(R, G, B));
                    }

                }
            }

            return arr.ToArray();
        }

        byte[] RGB565(byte R, byte G, byte B)
        {
            UInt16 r;
            r = (UInt16)(((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3));
            byte[] ret = { 0, 0 };
            ret[1] = (byte)(r >> 8);
            ret[0] = (byte)(r & 0xFF);
            return ret;
        }

        private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
        {

        }

        private void button2_Click(object sender, EventArgs e)
        {
            Process.Start(Application.StartupPath);
        }

        private void pictureBox1_Click(object sender, EventArgs e)
        {

        }

        private void button2_Click_1(object sender, EventArgs e)
        {
            Bitmap bmp = bmplist[lbFrames.SelectedIndex];

            bmp.Save("B.png", ImageFormat.Png);

        }

        private void Form1_Load(object sender, EventArgs e)
        {
            comboBox1.SelectedIndex = 0;
        }
    }


}
