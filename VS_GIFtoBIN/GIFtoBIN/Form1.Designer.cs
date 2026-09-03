
namespace GIFtoBIN
{
    partial class Form1
    {
        /// <summary>
        /// Обязательная переменная конструктора.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Освободить все используемые ресурсы.
        /// </summary>
        /// <param name="disposing">истинно, если управляемый ресурс должен быть удален; иначе ложно.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Код, автоматически созданный конструктором форм Windows

        /// <summary>
        /// Требуемый метод для поддержки конструктора — не изменяйте 
        /// содержимое этого метода с помощью редактора кода.
        /// </summary>
        private void InitializeComponent()
        {
            this.button1 = new System.Windows.Forms.Button();
            this.listBox1 = new System.Windows.Forms.ListBox();
            this.pBox = new System.Windows.Forms.PictureBox();
            this.lbFrames = new System.Windows.Forms.ListBox();
            this.buttonSaveBIN = new System.Windows.Forms.Button();
            this.comboBox1 = new System.Windows.Forms.ComboBox();
            this.buttonOpenFolder = new System.Windows.Forms.Button();
            this.buttonSaveFrame = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.pBox)).BeginInit();
            this.SuspendLayout();
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(570, 12);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(218, 54);
            this.button1.TabIndex = 0;
            this.button1.Text = "Открыть Gif";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // listBox1
            // 
            this.listBox1.FormattingEnabled = true;
            this.listBox1.ItemHeight = 16;
            this.listBox1.Location = new System.Drawing.Point(570, 332);
            this.listBox1.Name = "listBox1";
            this.listBox1.Size = new System.Drawing.Size(218, 84);
            this.listBox1.TabIndex = 1;
            // 
            // pBox
            // 
            this.pBox.BackColor = System.Drawing.SystemColors.ActiveCaption;
            this.pBox.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
            this.pBox.Location = new System.Drawing.Point(12, 12);
            this.pBox.Name = "pBox";
            this.pBox.Size = new System.Drawing.Size(400, 400);
            this.pBox.TabIndex = 2;
            this.pBox.TabStop = false;
            // 
            // lbFrames
            // 
            this.lbFrames.FormattingEnabled = true;
            this.lbFrames.ItemHeight = 16;
            this.lbFrames.Location = new System.Drawing.Point(432, 12);
            this.lbFrames.Name = "lbFrames";
            this.lbFrames.Size = new System.Drawing.Size(120, 404);
            this.lbFrames.TabIndex = 3;
            this.lbFrames.SelectedIndexChanged += new System.EventHandler(this.lbFrames_SelectedIndexChanged);
            // 
            // buttonSaveBIN
            // 
            this.buttonSaveBIN.Location = new System.Drawing.Point(570, 72);
            this.buttonSaveBIN.Name = "buttonSaveBIN";
            this.buttonSaveBIN.Size = new System.Drawing.Size(218, 54);
            this.buttonSaveBIN.TabIndex = 4;
            this.buttonSaveBIN.Text = "Сохранить BIN";
            this.buttonSaveBIN.UseVisualStyleBackColor = true;
            this.buttonSaveBIN.Click += new System.EventHandler(this.buttonSaveBIN_Click);
            // 
            // comboBox1
            // 
            this.comboBox1.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBox1.FormattingEnabled = true;
            this.comboBox1.Items.AddRange(new object[] {
            "32 ARGB",
            "24 A565",
            "16 565"});
            this.comboBox1.Location = new System.Drawing.Point(570, 132);
            this.comboBox1.Name = "comboBox1";
            this.comboBox1.Size = new System.Drawing.Size(218, 24);
            this.comboBox1.TabIndex = 5;
            this.comboBox1.SelectedIndexChanged += new System.EventHandler(this.comboBox1_SelectedIndexChanged);
            // 
            // buttonOpenFolder
            // 
            this.buttonOpenFolder.Location = new System.Drawing.Point(570, 162);
            this.buttonOpenFolder.Name = "buttonOpenFolder";
            this.buttonOpenFolder.Size = new System.Drawing.Size(218, 54);
            this.buttonOpenFolder.TabIndex = 6;
            this.buttonOpenFolder.Text = "Открыть папку";
            this.buttonOpenFolder.UseVisualStyleBackColor = true;
            this.buttonOpenFolder.Click += new System.EventHandler(this.button2_Click);
            // 
            // buttonSaveFrame
            // 
            this.buttonSaveFrame.Location = new System.Drawing.Point(570, 222);
            this.buttonSaveFrame.Name = "buttonSaveFrame";
            this.buttonSaveFrame.Size = new System.Drawing.Size(218, 54);
            this.buttonSaveFrame.TabIndex = 7;
            this.buttonSaveFrame.Text = "Сохранить кадр B.png";
            this.buttonSaveFrame.UseVisualStyleBackColor = true;
            this.buttonSaveFrame.Click += new System.EventHandler(this.button2_Click_1);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 430);
            this.Controls.Add(this.buttonSaveFrame);
            this.Controls.Add(this.buttonOpenFolder);
            this.Controls.Add(this.comboBox1);
            this.Controls.Add(this.buttonSaveBIN);
            this.Controls.Add(this.lbFrames);
            this.Controls.Add(this.pBox);
            this.Controls.Add(this.listBox1);
            this.Controls.Add(this.button1);
            this.Name = "Form1";
            this.Text = "GIFtoBIN V7 2023.10.07 ";
            this.Load += new System.EventHandler(this.Form1_Load);
            ((System.ComponentModel.ISupportInitialize)(this.pBox)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.ListBox listBox1;
        private System.Windows.Forms.PictureBox pBox;
        private System.Windows.Forms.ListBox lbFrames;
        private System.Windows.Forms.Button buttonSaveBIN;
        private System.Windows.Forms.ComboBox comboBox1;
        private System.Windows.Forms.Button buttonOpenFolder;
        private System.Windows.Forms.Button buttonSaveFrame;
    }
}

