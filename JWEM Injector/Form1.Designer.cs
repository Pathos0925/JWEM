namespace JWEM_Injector
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.statusLabel = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.enabledCheckBox = new System.Windows.Forms.CheckBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.btnSave = new System.Windows.Forms.Button();
            this.panel1 = new System.Windows.Forms.Panel();
            this.resDropdown = new System.Windows.Forms.ComboBox();
            this.btnRemoveTexture = new System.Windows.Forms.Button();
            this.listBox1 = new System.Windows.Forms.ListBox();
            this.btnAddTexture = new System.Windows.Forms.Button();
            this.textureTypeDropdown = new System.Windows.Forms.ComboBox();
            this.speciesDropdown = new System.Windows.Forms.ComboBox();
            this.findApplicationTimer = new System.Windows.Forms.Timer(this.components);
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.saveFileDialog1 = new System.Windows.Forms.SaveFileDialog();
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.useGPUChk = new System.Windows.Forms.CheckBox();
            this.buildStatusLbl = new System.Windows.Forms.Label();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.panel1.SuspendLayout();
            this.tabControl1.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.tabPage2.SuspendLayout();
            this.SuspendLayout();
            // 
            // statusLabel
            // 
            this.statusLabel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.statusLabel.Location = new System.Drawing.Point(6, 39);
            this.statusLabel.Name = "statusLabel";
            this.statusLabel.Size = new System.Drawing.Size(290, 23);
            this.statusLabel.TabIndex = 0;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.enabledCheckBox);
            this.groupBox1.Controls.Add(this.statusLabel);
            this.groupBox1.Location = new System.Drawing.Point(6, 10);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(302, 74);
            this.groupBox1.TabIndex = 3;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Injector";
            // 
            // enabledCheckBox
            // 
            this.enabledCheckBox.AutoSize = true;
            this.enabledCheckBox.Checked = true;
            this.enabledCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.enabledCheckBox.Location = new System.Drawing.Point(6, 19);
            this.enabledCheckBox.Name = "enabledCheckBox";
            this.enabledCheckBox.Size = new System.Drawing.Size(65, 17);
            this.enabledCheckBox.TabIndex = 1;
            this.enabledCheckBox.Text = "Enabled";
            this.enabledCheckBox.UseVisualStyleBackColor = true;
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.buildStatusLbl);
            this.groupBox2.Controls.Add(this.useGPUChk);
            this.groupBox2.Controls.Add(this.pictureBox1);
            this.groupBox2.Controls.Add(this.btnSave);
            this.groupBox2.Controls.Add(this.panel1);
            this.groupBox2.Controls.Add(this.speciesDropdown);
            this.groupBox2.Location = new System.Drawing.Point(3, 6);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(308, 320);
            this.groupBox2.TabIndex = 4;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Build JWEskin";
            // 
            // pictureBox1
            // 
            this.pictureBox1.Location = new System.Drawing.Point(143, 129);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(156, 162);
            this.pictureBox1.TabIndex = 6;
            this.pictureBox1.TabStop = false;
            // 
            // btnSave
            // 
            this.btnSave.Location = new System.Drawing.Point(143, 69);
            this.btnSave.Name = "btnSave";
            this.btnSave.Size = new System.Drawing.Size(156, 23);
            this.btnSave.TabIndex = 5;
            this.btnSave.Text = "Save";
            this.btnSave.UseVisualStyleBackColor = true;
            this.btnSave.Click += new System.EventHandler(this.btnSave_Click);
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.resDropdown);
            this.panel1.Controls.Add(this.btnRemoveTexture);
            this.panel1.Controls.Add(this.listBox1);
            this.panel1.Controls.Add(this.btnAddTexture);
            this.panel1.Controls.Add(this.textureTypeDropdown);
            this.panel1.Location = new System.Drawing.Point(6, 13);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(131, 278);
            this.panel1.TabIndex = 4;
            // 
            // resDropdown
            // 
            this.resDropdown.FormattingEnabled = true;
            this.resDropdown.Location = new System.Drawing.Point(3, 222);
            this.resDropdown.Name = "resDropdown";
            this.resDropdown.Size = new System.Drawing.Size(121, 21);
            this.resDropdown.TabIndex = 5;
            // 
            // btnRemoveTexture
            // 
            this.btnRemoveTexture.Location = new System.Drawing.Point(66, 249);
            this.btnRemoveTexture.Name = "btnRemoveTexture";
            this.btnRemoveTexture.Size = new System.Drawing.Size(58, 23);
            this.btnRemoveTexture.TabIndex = 4;
            this.btnRemoveTexture.Text = "Remove";
            this.btnRemoveTexture.UseVisualStyleBackColor = true;
            this.btnRemoveTexture.Click += new System.EventHandler(this.btnRemoveTexture_Click);
            // 
            // listBox1
            // 
            this.listBox1.FormattingEnabled = true;
            this.listBox1.Location = new System.Drawing.Point(3, 3);
            this.listBox1.Name = "listBox1";
            this.listBox1.Size = new System.Drawing.Size(122, 186);
            this.listBox1.TabIndex = 3;
            // 
            // btnAddTexture
            // 
            this.btnAddTexture.Location = new System.Drawing.Point(3, 249);
            this.btnAddTexture.Name = "btnAddTexture";
            this.btnAddTexture.Size = new System.Drawing.Size(58, 23);
            this.btnAddTexture.TabIndex = 2;
            this.btnAddTexture.Text = "Add";
            this.btnAddTexture.UseVisualStyleBackColor = true;
            this.btnAddTexture.Click += new System.EventHandler(this.btnAddTexture_Click);
            // 
            // textureTypeDropdown
            // 
            this.textureTypeDropdown.FormattingEnabled = true;
            this.textureTypeDropdown.Location = new System.Drawing.Point(3, 195);
            this.textureTypeDropdown.Name = "textureTypeDropdown";
            this.textureTypeDropdown.Size = new System.Drawing.Size(122, 21);
            this.textureTypeDropdown.TabIndex = 1;
            // 
            // speciesDropdown
            // 
            this.speciesDropdown.FormattingEnabled = true;
            this.speciesDropdown.Location = new System.Drawing.Point(143, 19);
            this.speciesDropdown.Name = "speciesDropdown";
            this.speciesDropdown.Size = new System.Drawing.Size(156, 21);
            this.speciesDropdown.TabIndex = 0;
            // 
            // findApplicationTimer
            // 
            this.findApplicationTimer.Interval = 1;
            this.findApplicationTimer.Tick += new System.EventHandler(this.findApplicationTimer_Tick);
            // 
            // tabControl1
            // 
            this.tabControl1.Controls.Add(this.tabPage1);
            this.tabControl1.Controls.Add(this.tabPage2);
            this.tabControl1.Location = new System.Drawing.Point(12, 5);
            this.tabControl1.Margin = new System.Windows.Forms.Padding(0);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.Padding = new System.Drawing.Point(0, 0);
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(325, 358);
            this.tabControl1.TabIndex = 5;
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.groupBox1);
            this.tabPage1.Location = new System.Drawing.Point(4, 22);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(317, 332);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "Injector";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // tabPage2
            // 
            this.tabPage2.Controls.Add(this.groupBox2);
            this.tabPage2.Location = new System.Drawing.Point(4, 22);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(317, 332);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "Build Skin";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // openFileDialog1
            // 
            this.openFileDialog1.FileName = "openFileDialog1";
            // 
            // useGPUChk
            // 
            this.useGPUChk.AutoSize = true;
            this.useGPUChk.Checked = true;
            this.useGPUChk.CheckState = System.Windows.Forms.CheckState.Checked;
            this.useGPUChk.Location = new System.Drawing.Point(143, 46);
            this.useGPUChk.Name = "useGPUChk";
            this.useGPUChk.Size = new System.Drawing.Size(71, 17);
            this.useGPUChk.TabIndex = 7;
            this.useGPUChk.Text = "Use GPU";
            this.useGPUChk.UseVisualStyleBackColor = true;
            this.useGPUChk.CheckedChanged += new System.EventHandler(this.useGPUChk_CheckedChanged);
            // 
            // buildStatusLbl
            // 
            this.buildStatusLbl.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.buildStatusLbl.Location = new System.Drawing.Point(3, 294);
            this.buildStatusLbl.Name = "buildStatusLbl";
            this.buildStatusLbl.Size = new System.Drawing.Size(299, 23);
            this.buildStatusLbl.TabIndex = 8;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(346, 372);
            this.Controls.Add(this.tabControl1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Name = "Form1";
            this.Text = "JWEM Injector";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.panel1.ResumeLayout(false);
            this.tabControl1.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            this.tabPage2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label statusLabel;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Timer findApplicationTimer;
        private System.Windows.Forms.CheckBox enabledCheckBox;
        private System.Windows.Forms.ComboBox speciesDropdown;
        private System.Windows.Forms.ComboBox textureTypeDropdown;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Button btnRemoveTexture;
        private System.Windows.Forms.ListBox listBox1;
        private System.Windows.Forms.Button btnAddTexture;
        private System.Windows.Forms.Button btnSave;
        private System.Windows.Forms.TabControl tabControl1;
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.TabPage tabPage2;
        private System.Windows.Forms.SaveFileDialog saveFileDialog1;
        private System.Windows.Forms.OpenFileDialog openFileDialog1;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.ComboBox resDropdown;
        private System.Windows.Forms.CheckBox useGPUChk;
        private System.Windows.Forms.Label buildStatusLbl;
    }
}
