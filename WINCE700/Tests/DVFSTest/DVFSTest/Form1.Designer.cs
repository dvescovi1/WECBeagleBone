namespace DVFS
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
            this.pnlSPEED = new System.Windows.Forms.Panel();
            this.btnSWITCH = new System.Windows.Forms.Button();
            this.rb300MHZ = new System.Windows.Forms.RadioButton();
            this.rb600MHZ = new System.Windows.Forms.RadioButton();
            this.rb800MHZ = new System.Windows.Forms.RadioButton();
            this.rb1GHZ = new System.Windows.Forms.RadioButton();
            this.pnlSPEED.SuspendLayout();
            this.SuspendLayout();
            // 
            // pnlSPEED
            // 
            this.pnlSPEED.Controls.Add(this.btnSWITCH);
            this.pnlSPEED.Controls.Add(this.rb300MHZ);
            this.pnlSPEED.Controls.Add(this.rb600MHZ);
            this.pnlSPEED.Controls.Add(this.rb800MHZ);
            this.pnlSPEED.Controls.Add(this.rb1GHZ);
            this.pnlSPEED.Location = new System.Drawing.Point(3, 41);
            this.pnlSPEED.Name = "pnlSPEED";
            this.pnlSPEED.Size = new System.Drawing.Size(417, 164);
            // 
            // btnSWITCH
            // 
            this.btnSWITCH.Location = new System.Drawing.Point(29, 81);
            this.btnSWITCH.Name = "btnSWITCH";
            this.btnSWITCH.Size = new System.Drawing.Size(350, 43);
            this.btnSWITCH.TabIndex = 4;
            this.btnSWITCH.Text = "Switch";
            this.btnSWITCH.Click += new System.EventHandler(this.btnSWITCH_Click);
            // 
            // rb300MHZ
            // 
            this.rb300MHZ.Location = new System.Drawing.Point(303, 29);
            this.rb300MHZ.Name = "rb300MHZ";
            this.rb300MHZ.Size = new System.Drawing.Size(76, 20);
            this.rb300MHZ.TabIndex = 3;
            this.rb300MHZ.Text = "300Mhz";
            // 
            // rb600MHZ
            // 
            this.rb600MHZ.Location = new System.Drawing.Point(225, 29);
            this.rb600MHZ.Name = "rb600MHZ";
            this.rb600MHZ.Size = new System.Drawing.Size(72, 20);
            this.rb600MHZ.TabIndex = 2;
            this.rb600MHZ.Text = "600Mhz";
            // 
            // rb800MHZ
            // 
            this.rb800MHZ.Checked = true;
            this.rb800MHZ.Location = new System.Drawing.Point(148, 29);
            this.rb800MHZ.Name = "rb800MHZ";
            this.rb800MHZ.Size = new System.Drawing.Size(71, 20);
            this.rb800MHZ.TabIndex = 1;
            this.rb800MHZ.Text = "800Mhz";
            // 
            // rb1GHZ
            // 
            this.rb1GHZ.Location = new System.Drawing.Point(29, 29);
            this.rb1GHZ.Name = "rb1GHZ";
            this.rb1GHZ.Size = new System.Drawing.Size(113, 20);
            this.rb1GHZ.TabIndex = 0;
            this.rb1GHZ.Text = "1Ghz (caution)";
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(423, 223);
            this.Controls.Add(this.pnlSPEED);
            this.Name = "Form1";
            this.Text = "DVFS Test ";
            this.pnlSPEED.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel pnlSPEED;
        private System.Windows.Forms.RadioButton rb300MHZ;
        private System.Windows.Forms.RadioButton rb600MHZ;
        private System.Windows.Forms.RadioButton rb800MHZ;
        private System.Windows.Forms.RadioButton rb1GHZ;
        private System.Windows.Forms.Button btnSWITCH;
    }
}

