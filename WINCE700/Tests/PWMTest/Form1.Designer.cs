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
            this.tbDUTYCYCLE = new System.Windows.Forms.TrackBar();
            this.lblDUTYCYCLE = new System.Windows.Forms.Label();
            this.tbFREQUENCY = new System.Windows.Forms.TrackBar();
            this.lblFREQUENCY = new System.Windows.Forms.Label();
            this.btnSAVE = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // tbDUTYCYCLE
            // 
            this.tbDUTYCYCLE.LargeChange = 10;
            this.tbDUTYCYCLE.Location = new System.Drawing.Point(268, 22);
            this.tbDUTYCYCLE.Maximum = 100;
            this.tbDUTYCYCLE.Name = "tbDUTYCYCLE";
            this.tbDUTYCYCLE.Orientation = System.Windows.Forms.Orientation.Vertical;
            this.tbDUTYCYCLE.Size = new System.Drawing.Size(45, 164);
            this.tbDUTYCYCLE.TabIndex = 0;
            this.tbDUTYCYCLE.TickFrequency = 10;
            this.tbDUTYCYCLE.Value = 90;
            this.tbDUTYCYCLE.ValueChanged += new System.EventHandler(this.tbDUTYCYCLE_ValueChanged);
            // 
            // lblDUTYCYCLE
            // 
            this.lblDUTYCYCLE.Location = new System.Drawing.Point(203, 193);
            this.lblDUTYCYCLE.Name = "lblDUTYCYCLE";
            this.lblDUTYCYCLE.Size = new System.Drawing.Size(110, 20);
            this.lblDUTYCYCLE.Text = "Duty Cycle:";
            // 
            // tbFREQUENCY
            // 
            this.tbFREQUENCY.LargeChange = 100;
            this.tbFREQUENCY.Location = new System.Drawing.Point(85, 22);
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
            this.lblFREQUENCY.Location = new System.Drawing.Point(20, 189);
            this.lblFREQUENCY.Name = "lblFREQUENCY";
            this.lblFREQUENCY.Size = new System.Drawing.Size(131, 20);
            this.lblFREQUENCY.Text = "Frequency:";
            // 
            // btnSAVE
            // 
            this.btnSAVE.Location = new System.Drawing.Point(20, 239);
            this.btnSAVE.Name = "btnSAVE";
            this.btnSAVE.Size = new System.Drawing.Size(293, 43);
            this.btnSAVE.TabIndex = 4;
            this.btnSAVE.Text = "Save Settings";
            this.btnSAVE.Click += new System.EventHandler(this.btnSAVE_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(387, 296);
            this.Controls.Add(this.btnSAVE);
            this.Controls.Add(this.lblFREQUENCY);
            this.Controls.Add(this.tbFREQUENCY);
            this.Controls.Add(this.lblDUTYCYCLE);
            this.Controls.Add(this.tbDUTYCYCLE);
            this.Name = "Form1";
            this.Text = "PWM Test ";
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TrackBar tbDUTYCYCLE;
        private System.Windows.Forms.Label lblDUTYCYCLE;
        private System.Windows.Forms.TrackBar tbFREQUENCY;
        private System.Windows.Forms.Label lblFREQUENCY;
        private System.Windows.Forms.Button btnSAVE;
    }
}

