namespace M2MQTT_test
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
            this.panel1 = new System.Windows.Forms.Panel();
            this.btnLED1FLASH = new System.Windows.Forms.Button();
            this.btnLED1OFF = new System.Windows.Forms.Button();
            this.btnLED1ON = new System.Windows.Forms.Button();
            this.lblLED1 = new System.Windows.Forms.Label();
            this.panel2 = new System.Windows.Forms.Panel();
            this.btnLED2FLASH = new System.Windows.Forms.Button();
            this.btnLED2OFF = new System.Windows.Forms.Button();
            this.btnLED2ON = new System.Windows.Forms.Button();
            this.lblLED2 = new System.Windows.Forms.Label();
            this.panel3 = new System.Windows.Forms.Panel();
            this.btnLED3FLASH = new System.Windows.Forms.Button();
            this.btnLED3OFF = new System.Windows.Forms.Button();
            this.btnLED3ON = new System.Windows.Forms.Button();
            this.lblLED3 = new System.Windows.Forms.Label();
            this.btnRESET = new System.Windows.Forms.Button();
            this.lblElapsedTimeValue = new System.Windows.Forms.Label();
            this.lblElapsedTime = new System.Windows.Forms.Label();
            this.tmrUpdate = new System.Windows.Forms.Timer();
            this.panel1.SuspendLayout();
            this.panel2.SuspendLayout();
            this.panel3.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.btnLED1FLASH);
            this.panel1.Controls.Add(this.btnLED1OFF);
            this.panel1.Controls.Add(this.btnLED1ON);
            this.panel1.Controls.Add(this.lblLED1);
            this.panel1.Location = new System.Drawing.Point(3, 17);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(79, 124);
            // 
            // btnLED1FLASH
            // 
            this.btnLED1FLASH.Location = new System.Drawing.Point(3, 91);
            this.btnLED1FLASH.Name = "btnLED1FLASH";
            this.btnLED1FLASH.Size = new System.Drawing.Size(72, 20);
            this.btnLED1FLASH.TabIndex = 3;
            this.btnLED1FLASH.Text = "FLASH";
            this.btnLED1FLASH.Click += new System.EventHandler(this.btnLED1FLASH_Click);
            // 
            // btnLED1OFF
            // 
            this.btnLED1OFF.Location = new System.Drawing.Point(3, 65);
            this.btnLED1OFF.Name = "btnLED1OFF";
            this.btnLED1OFF.Size = new System.Drawing.Size(72, 20);
            this.btnLED1OFF.TabIndex = 2;
            this.btnLED1OFF.Text = "OFF";
            this.btnLED1OFF.Click += new System.EventHandler(this.btnLED1OFF_Click);
            // 
            // btnLED1ON
            // 
            this.btnLED1ON.Location = new System.Drawing.Point(3, 39);
            this.btnLED1ON.Name = "btnLED1ON";
            this.btnLED1ON.Size = new System.Drawing.Size(72, 20);
            this.btnLED1ON.TabIndex = 1;
            this.btnLED1ON.Text = "ON";
            this.btnLED1ON.Click += new System.EventHandler(this.btnLED1ON_Click);
            // 
            // lblLED1
            // 
            this.lblLED1.Location = new System.Drawing.Point(3, 16);
            this.lblLED1.Name = "lblLED1";
            this.lblLED1.Size = new System.Drawing.Size(72, 20);
            this.lblLED1.Text = "LED 1";
            this.lblLED1.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // panel2
            // 
            this.panel2.Controls.Add(this.btnLED2FLASH);
            this.panel2.Controls.Add(this.btnLED2OFF);
            this.panel2.Controls.Add(this.btnLED2ON);
            this.panel2.Controls.Add(this.lblLED2);
            this.panel2.Location = new System.Drawing.Point(88, 17);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(79, 124);
            // 
            // btnLED2FLASH
            // 
            this.btnLED2FLASH.Location = new System.Drawing.Point(3, 91);
            this.btnLED2FLASH.Name = "btnLED2FLASH";
            this.btnLED2FLASH.Size = new System.Drawing.Size(72, 20);
            this.btnLED2FLASH.TabIndex = 3;
            this.btnLED2FLASH.Text = "FLASH";
            this.btnLED2FLASH.Click += new System.EventHandler(this.btnLED2FLASH_Click);
            // 
            // btnLED2OFF
            // 
            this.btnLED2OFF.Location = new System.Drawing.Point(3, 65);
            this.btnLED2OFF.Name = "btnLED2OFF";
            this.btnLED2OFF.Size = new System.Drawing.Size(72, 20);
            this.btnLED2OFF.TabIndex = 2;
            this.btnLED2OFF.Text = "OFF";
            this.btnLED2OFF.Click += new System.EventHandler(this.btnLED2OFF_Click);
            // 
            // btnLED2ON
            // 
            this.btnLED2ON.Location = new System.Drawing.Point(3, 39);
            this.btnLED2ON.Name = "btnLED2ON";
            this.btnLED2ON.Size = new System.Drawing.Size(72, 20);
            this.btnLED2ON.TabIndex = 1;
            this.btnLED2ON.Text = "ON";
            this.btnLED2ON.Click += new System.EventHandler(this.btnLED2ON_Click);
            // 
            // lblLED2
            // 
            this.lblLED2.Location = new System.Drawing.Point(3, 16);
            this.lblLED2.Name = "lblLED2";
            this.lblLED2.Size = new System.Drawing.Size(72, 20);
            this.lblLED2.Text = "LED 2";
            this.lblLED2.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // panel3
            // 
            this.panel3.Controls.Add(this.btnLED3FLASH);
            this.panel3.Controls.Add(this.btnLED3OFF);
            this.panel3.Controls.Add(this.btnLED3ON);
            this.panel3.Controls.Add(this.lblLED3);
            this.panel3.Location = new System.Drawing.Point(173, 17);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(79, 124);
            // 
            // btnLED3FLASH
            // 
            this.btnLED3FLASH.Location = new System.Drawing.Point(3, 91);
            this.btnLED3FLASH.Name = "btnLED3FLASH";
            this.btnLED3FLASH.Size = new System.Drawing.Size(72, 20);
            this.btnLED3FLASH.TabIndex = 3;
            this.btnLED3FLASH.Text = "FLASH";
            this.btnLED3FLASH.Click += new System.EventHandler(this.btnLED3FLASH_Click);
            // 
            // btnLED3OFF
            // 
            this.btnLED3OFF.Location = new System.Drawing.Point(3, 65);
            this.btnLED3OFF.Name = "btnLED3OFF";
            this.btnLED3OFF.Size = new System.Drawing.Size(72, 20);
            this.btnLED3OFF.TabIndex = 2;
            this.btnLED3OFF.Text = "OFF";
            this.btnLED3OFF.Click += new System.EventHandler(this.btnLED3OFF_Click);
            // 
            // btnLED3ON
            // 
            this.btnLED3ON.Location = new System.Drawing.Point(3, 39);
            this.btnLED3ON.Name = "btnLED3ON";
            this.btnLED3ON.Size = new System.Drawing.Size(72, 20);
            this.btnLED3ON.TabIndex = 1;
            this.btnLED3ON.Text = "ON";
            this.btnLED3ON.Click += new System.EventHandler(this.btnLED3ON_Click);
            // 
            // lblLED3
            // 
            this.lblLED3.Location = new System.Drawing.Point(3, 16);
            this.lblLED3.Name = "lblLED3";
            this.lblLED3.Size = new System.Drawing.Size(72, 20);
            this.lblLED3.Text = "LED 3";
            this.lblLED3.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // btnRESET
            // 
            this.btnRESET.Location = new System.Drawing.Point(173, 162);
            this.btnRESET.Name = "btnRESET";
            this.btnRESET.Size = new System.Drawing.Size(75, 20);
            this.btnRESET.TabIndex = 11;
            this.btnRESET.Text = "RESET";
            this.btnRESET.Click += new System.EventHandler(this.btnRESET_Click);
            // 
            // lblElapsedTimeValue
            // 
            this.lblElapsedTimeValue.Location = new System.Drawing.Point(107, 162);
            this.lblElapsedTimeValue.Name = "lblElapsedTimeValue";
            this.lblElapsedTimeValue.Size = new System.Drawing.Size(56, 20);
            this.lblElapsedTimeValue.Text = "0";
            // 
            // lblElapsedTime
            // 
            this.lblElapsedTime.Location = new System.Drawing.Point(3, 162);
            this.lblElapsedTime.Name = "lblElapsedTime";
            this.lblElapsedTime.Size = new System.Drawing.Size(100, 20);
            this.lblElapsedTime.Text = "Elapsed Time:";
            this.lblElapsedTime.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // tmrUpdate
            // 
            this.tmrUpdate.Interval = 1000;
            this.tmrUpdate.Tick += new System.EventHandler(this.tmrUpdate_Tick);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(638, 455);
            this.Controls.Add(this.lblElapsedTime);
            this.Controls.Add(this.btnRESET);
            this.Controls.Add(this.lblElapsedTimeValue);
            this.Controls.Add(this.panel3);
            this.Controls.Add(this.panel2);
            this.Controls.Add(this.panel1);
            this.Name = "Form1";
            this.Text = "M2MQTT Simple test Device ";
            this.panel1.ResumeLayout(false);
            this.panel2.ResumeLayout(false);
            this.panel3.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Button btnLED1FLASH;
        private System.Windows.Forms.Button btnLED1OFF;
        private System.Windows.Forms.Button btnLED1ON;
        private System.Windows.Forms.Label lblLED1;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.Button btnLED2FLASH;
        private System.Windows.Forms.Button btnLED2OFF;
        private System.Windows.Forms.Button btnLED2ON;
        private System.Windows.Forms.Label lblLED2;
        private System.Windows.Forms.Panel panel3;
        private System.Windows.Forms.Button btnLED3FLASH;
        private System.Windows.Forms.Button btnLED3OFF;
        private System.Windows.Forms.Button btnLED3ON;
        private System.Windows.Forms.Label lblLED3;
        private System.Windows.Forms.Button btnRESET;
        private System.Windows.Forms.Label lblElapsedTimeValue;
        private System.Windows.Forms.Label lblElapsedTime;
        private System.Windows.Forms.Timer tmrUpdate;

    }
}

