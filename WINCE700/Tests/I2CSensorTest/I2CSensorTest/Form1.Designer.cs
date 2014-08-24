namespace I2CSensorTest
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
            this.lblXDUCER1 = new System.Windows.Forms.Label();
            this.lblXDUCER2 = new System.Windows.Forms.Label();
            this.lblXDUCER1READING = new System.Windows.Forms.Label();
            this.lblXDUCER2READING = new System.Windows.Forms.Label();
            this.tmrUPDATE = new System.Windows.Forms.Timer();
            this.SuspendLayout();
            // 
            // lblXDUCER1
            // 
            this.lblXDUCER1.Location = new System.Drawing.Point(23, 47);
            this.lblXDUCER1.Name = "lblXDUCER1";
            this.lblXDUCER1.Size = new System.Drawing.Size(119, 20);
            this.lblXDUCER1.Text = "Xducer 1 Reading:";
            // 
            // lblXDUCER2
            // 
            this.lblXDUCER2.Location = new System.Drawing.Point(23, 83);
            this.lblXDUCER2.Name = "lblXDUCER2";
            this.lblXDUCER2.Size = new System.Drawing.Size(119, 20);
            this.lblXDUCER2.Text = "Xducer 2 Reading:";
            // 
            // lblXDUCER1READING
            // 
            this.lblXDUCER1READING.BackColor = System.Drawing.SystemColors.ControlLight;
            this.lblXDUCER1READING.Location = new System.Drawing.Point(148, 47);
            this.lblXDUCER1READING.Name = "lblXDUCER1READING";
            this.lblXDUCER1READING.Size = new System.Drawing.Size(119, 20);
            this.lblXDUCER1READING.Text = "128 mmHg";
            // 
            // lblXDUCER2READING
            // 
            this.lblXDUCER2READING.BackColor = System.Drawing.SystemColors.ControlLight;
            this.lblXDUCER2READING.Location = new System.Drawing.Point(148, 83);
            this.lblXDUCER2READING.Name = "lblXDUCER2READING";
            this.lblXDUCER2READING.Size = new System.Drawing.Size(119, 20);
            this.lblXDUCER2READING.Text = "56 mmHg";
            // 
            // tmrUPDATE
            // 
            this.tmrUPDATE.Interval = 500;
            this.tmrUPDATE.Tick += new System.EventHandler(this.tmrUPDATE_Tick);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(315, 200);
            this.Controls.Add(this.lblXDUCER2READING);
            this.Controls.Add(this.lblXDUCER1READING);
            this.Controls.Add(this.lblXDUCER2);
            this.Controls.Add(this.lblXDUCER1);
            this.Name = "Form1";
            this.Text = "I2C Sensor Test App";
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label lblXDUCER1;
        private System.Windows.Forms.Label lblXDUCER2;
        private System.Windows.Forms.Label lblXDUCER1READING;
        private System.Windows.Forms.Label lblXDUCER2READING;
        private System.Windows.Forms.Timer tmrUPDATE;
    }
}

