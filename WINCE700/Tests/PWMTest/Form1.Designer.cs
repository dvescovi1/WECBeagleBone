namespace PWMTest
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
            this.tbDUTYCYCLEA = new System.Windows.Forms.TrackBar();
            this.lblDUTYCYCLEA = new System.Windows.Forms.Label();
            this.tbFREQUENCY = new System.Windows.Forms.TrackBar();
            this.lblFREQUENCY = new System.Windows.Forms.Label();
            this.btnSAVE = new System.Windows.Forms.Button();
            this.lblDUTYCYCLEB = new System.Windows.Forms.Label();
            this.tbDUTYCYCLEB = new System.Windows.Forms.TrackBar();
            this.pnlSETTINGS = new System.Windows.Forms.Panel();
            this.btnSTOPB = new System.Windows.Forms.Button();
            this.btnSTARTB = new System.Windows.Forms.Button();
            this.btnSTOPA = new System.Windows.Forms.Button();
            this.btnSTARTA = new System.Windows.Forms.Button();
            this.btnOPEN = new System.Windows.Forms.Button();
            this.pnlPWM = new System.Windows.Forms.Panel();
            this.rbPWM2 = new System.Windows.Forms.RadioButton();
            this.rbPWM1 = new System.Windows.Forms.RadioButton();
            this.rbPWM0 = new System.Windows.Forms.RadioButton();
            this.pnlSETTINGS.SuspendLayout();
            this.pnlPWM.SuspendLayout();
            this.SuspendLayout();
            // 
            // tbDUTYCYCLEA
            // 
            this.tbDUTYCYCLEA.Enabled = false;
            this.tbDUTYCYCLEA.LargeChange = 10;
            this.tbDUTYCYCLEA.Location = new System.Drawing.Point(226, 32);
            this.tbDUTYCYCLEA.Maximum = 100;
            this.tbDUTYCYCLEA.Name = "tbDUTYCYCLEA";
            this.tbDUTYCYCLEA.Orientation = System.Windows.Forms.Orientation.Vertical;
            this.tbDUTYCYCLEA.Size = new System.Drawing.Size(45, 164);
            this.tbDUTYCYCLEA.TabIndex = 0;
            this.tbDUTYCYCLEA.TickFrequency = 10;
            this.tbDUTYCYCLEA.Value = 90;
            this.tbDUTYCYCLEA.ValueChanged += new System.EventHandler(this.tbDUTYCYCLEA_ValueChanged);
            // 
            // lblDUTYCYCLEA
            // 
            this.lblDUTYCYCLEA.Location = new System.Drawing.Point(161, 199);
            this.lblDUTYCYCLEA.Name = "lblDUTYCYCLEA";
            this.lblDUTYCYCLEA.Size = new System.Drawing.Size(110, 20);
            this.lblDUTYCYCLEA.Text = "Duty Cycle A:";
            // 
            // tbFREQUENCY
            // 
            this.tbFREQUENCY.Enabled = false;
            this.tbFREQUENCY.LargeChange = 100;
            this.tbFREQUENCY.Location = new System.Drawing.Point(89, 32);
            this.tbFREQUENCY.Maximum = 10000000;
            this.tbFREQUENCY.Minimum = 50;
            this.tbFREQUENCY.Name = "tbFREQUENCY";
            this.tbFREQUENCY.Orientation = System.Windows.Forms.Orientation.Vertical;
            this.tbFREQUENCY.Size = new System.Drawing.Size(45, 164);
            this.tbFREQUENCY.SmallChange = 10;
            this.tbFREQUENCY.TabIndex = 2;
            this.tbFREQUENCY.TickFrequency = 100000;
            this.tbFREQUENCY.Value = 10000000;
            this.tbFREQUENCY.ValueChanged += new System.EventHandler(this.tbFREQUENCY_ValueChanged);
            // 
            // lblFREQUENCY
            // 
            this.lblFREQUENCY.Location = new System.Drawing.Point(24, 199);
            this.lblFREQUENCY.Name = "lblFREQUENCY";
            this.lblFREQUENCY.Size = new System.Drawing.Size(131, 20);
            this.lblFREQUENCY.Text = "Frequency:";
            // 
            // btnSAVE
            // 
            this.btnSAVE.Location = new System.Drawing.Point(24, 318);
            this.btnSAVE.Name = "btnSAVE";
            this.btnSAVE.Size = new System.Drawing.Size(406, 43);
            this.btnSAVE.TabIndex = 4;
            this.btnSAVE.Text = "Save Settings";
            this.btnSAVE.Click += new System.EventHandler(this.btnSAVE_Click);
            // 
            // lblDUTYCYCLEB
            // 
            this.lblDUTYCYCLEB.Location = new System.Drawing.Point(282, 199);
            this.lblDUTYCYCLEB.Name = "lblDUTYCYCLEB";
            this.lblDUTYCYCLEB.Size = new System.Drawing.Size(110, 20);
            this.lblDUTYCYCLEB.Text = "Duty Cycle B:";
            // 
            // tbDUTYCYCLEB
            // 
            this.tbDUTYCYCLEB.Enabled = false;
            this.tbDUTYCYCLEB.LargeChange = 10;
            this.tbDUTYCYCLEB.Location = new System.Drawing.Point(347, 32);
            this.tbDUTYCYCLEB.Maximum = 100;
            this.tbDUTYCYCLEB.Name = "tbDUTYCYCLEB";
            this.tbDUTYCYCLEB.Orientation = System.Windows.Forms.Orientation.Vertical;
            this.tbDUTYCYCLEB.Size = new System.Drawing.Size(45, 164);
            this.tbDUTYCYCLEB.TabIndex = 8;
            this.tbDUTYCYCLEB.TickFrequency = 10;
            this.tbDUTYCYCLEB.Value = 90;
            this.tbDUTYCYCLEB.ValueChanged += new System.EventHandler(this.tbDUTYCYCLEB_ValueChanged);
            // 
            // pnlSETTINGS
            // 
            this.pnlSETTINGS.Controls.Add(this.btnSTOPB);
            this.pnlSETTINGS.Controls.Add(this.btnSTARTB);
            this.pnlSETTINGS.Controls.Add(this.btnSTOPA);
            this.pnlSETTINGS.Controls.Add(this.btnSTARTA);
            this.pnlSETTINGS.Controls.Add(this.lblDUTYCYCLEB);
            this.pnlSETTINGS.Controls.Add(this.tbDUTYCYCLEA);
            this.pnlSETTINGS.Controls.Add(this.tbDUTYCYCLEB);
            this.pnlSETTINGS.Controls.Add(this.lblDUTYCYCLEA);
            this.pnlSETTINGS.Controls.Add(this.btnSAVE);
            this.pnlSETTINGS.Controls.Add(this.tbFREQUENCY);
            this.pnlSETTINGS.Controls.Add(this.lblFREQUENCY);
            this.pnlSETTINGS.Enabled = false;
            this.pnlSETTINGS.Location = new System.Drawing.Point(16, 92);
            this.pnlSETTINGS.Name = "pnlSETTINGS";
            this.pnlSETTINGS.Size = new System.Drawing.Size(457, 398);
            // 
            // btnSTOPB
            // 
            this.btnSTOPB.Location = new System.Drawing.Point(282, 273);
            this.btnSTOPB.Name = "btnSTOPB";
            this.btnSTOPB.Size = new System.Drawing.Size(91, 39);
            this.btnSTOPB.TabIndex = 17;
            this.btnSTOPB.Text = "Stop B";
            this.btnSTOPB.Click += new System.EventHandler(this.btnSTOPB_Click);
            // 
            // btnSTARTB
            // 
            this.btnSTARTB.Location = new System.Drawing.Point(282, 222);
            this.btnSTARTB.Name = "btnSTARTB";
            this.btnSTARTB.Size = new System.Drawing.Size(91, 39);
            this.btnSTARTB.TabIndex = 16;
            this.btnSTARTB.Text = "Start B";
            this.btnSTARTB.Click += new System.EventHandler(this.btnSTARTB_Click);
            // 
            // btnSTOPA
            // 
            this.btnSTOPA.Location = new System.Drawing.Point(161, 273);
            this.btnSTOPA.Name = "btnSTOPA";
            this.btnSTOPA.Size = new System.Drawing.Size(91, 39);
            this.btnSTOPA.TabIndex = 12;
            this.btnSTOPA.Text = "Stop A";
            this.btnSTOPA.Click += new System.EventHandler(this.btnSTOPA_Click);
            // 
            // btnSTARTA
            // 
            this.btnSTARTA.Location = new System.Drawing.Point(161, 222);
            this.btnSTARTA.Name = "btnSTARTA";
            this.btnSTARTA.Size = new System.Drawing.Size(91, 39);
            this.btnSTARTA.TabIndex = 11;
            this.btnSTARTA.Text = "Start A";
            this.btnSTARTA.Click += new System.EventHandler(this.btnSTARTA_Click);
            // 
            // btnOPEN
            // 
            this.btnOPEN.Location = new System.Drawing.Point(317, 31);
            this.btnOPEN.Name = "btnOPEN";
            this.btnOPEN.Size = new System.Drawing.Size(156, 39);
            this.btnOPEN.TabIndex = 12;
            this.btnOPEN.Text = "Open";
            this.btnOPEN.Click += new System.EventHandler(this.btnOPEN_Click);
            // 
            // pnlPWM
            // 
            this.pnlPWM.Controls.Add(this.rbPWM2);
            this.pnlPWM.Controls.Add(this.rbPWM1);
            this.pnlPWM.Controls.Add(this.rbPWM0);
            this.pnlPWM.Location = new System.Drawing.Point(16, 31);
            this.pnlPWM.Name = "pnlPWM";
            this.pnlPWM.Size = new System.Drawing.Size(286, 39);
            // 
            // rbPWM2
            // 
            this.rbPWM2.Location = new System.Drawing.Point(186, 16);
            this.rbPWM2.Name = "rbPWM2";
            this.rbPWM2.Size = new System.Drawing.Size(75, 20);
            this.rbPWM2.TabIndex = 2;
            this.rbPWM2.TabStop = false;
            this.rbPWM2.Text = "PWM 2";
            this.rbPWM2.CheckedChanged += new System.EventHandler(this.rbPWM2_CheckedChanged);
            // 
            // rbPWM1
            // 
            this.rbPWM1.Checked = true;
            this.rbPWM1.Location = new System.Drawing.Point(105, 16);
            this.rbPWM1.Name = "rbPWM1";
            this.rbPWM1.Size = new System.Drawing.Size(75, 20);
            this.rbPWM1.TabIndex = 1;
            this.rbPWM1.Text = "PWM 1";
            this.rbPWM1.CheckedChanged += new System.EventHandler(this.rbPWM1_CheckedChanged);
            // 
            // rbPWM0
            // 
            this.rbPWM0.Location = new System.Drawing.Point(24, 16);
            this.rbPWM0.Name = "rbPWM0";
            this.rbPWM0.Size = new System.Drawing.Size(75, 20);
            this.rbPWM0.TabIndex = 0;
            this.rbPWM0.TabStop = false;
            this.rbPWM0.Text = "PWM 0";
            this.rbPWM0.CheckedChanged += new System.EventHandler(this.rbPWM0_CheckedChanged);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(492, 562);
            this.Controls.Add(this.pnlPWM);
            this.Controls.Add(this.btnOPEN);
            this.Controls.Add(this.pnlSETTINGS);
            this.Name = "Form1";
            this.Text = "PWM Test ";
            this.pnlSETTINGS.ResumeLayout(false);
            this.pnlPWM.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TrackBar tbDUTYCYCLEA;
        private System.Windows.Forms.Label lblDUTYCYCLEA;
        private System.Windows.Forms.TrackBar tbFREQUENCY;
        private System.Windows.Forms.Label lblFREQUENCY;
        private System.Windows.Forms.Button btnSAVE;
        private System.Windows.Forms.Label lblDUTYCYCLEB;
        private System.Windows.Forms.TrackBar tbDUTYCYCLEB;
        private System.Windows.Forms.Panel pnlSETTINGS;
        private System.Windows.Forms.Button btnOPEN;
        private System.Windows.Forms.Panel pnlPWM;
        private System.Windows.Forms.RadioButton rbPWM2;
        private System.Windows.Forms.RadioButton rbPWM1;
        private System.Windows.Forms.RadioButton rbPWM0;
        private System.Windows.Forms.Button btnSTOPA;
        private System.Windows.Forms.Button btnSTARTA;
        private System.Windows.Forms.Button btnSTOPB;
        private System.Windows.Forms.Button btnSTARTB;
    }
}

