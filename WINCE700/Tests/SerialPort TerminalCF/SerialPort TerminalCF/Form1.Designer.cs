namespace SerialPort_TerminalCF
{
    partial class frmTerminal
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
            this.rtfTerminal = new System.Windows.Forms.TextBox();
            this.lblSend = new System.Windows.Forms.Label();
            this.txtSendData = new System.Windows.Forms.TextBox();
            this.lblComPort = new System.Windows.Forms.Label();
            this.cmbPortName = new System.Windows.Forms.ComboBox();
            this.lblStopBits = new System.Windows.Forms.Label();
            this.cmbBaudRate = new System.Windows.Forms.ComboBox();
            this.cmbStopBits = new System.Windows.Forms.ComboBox();
            this.lblBaudRate = new System.Windows.Forms.Label();
            this.lblDataBits = new System.Windows.Forms.Label();
            this.cmbParity = new System.Windows.Forms.ComboBox();
            this.cmbDataBits = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.btnOpenPort = new System.Windows.Forms.Button();
            this.btnSend = new System.Windows.Forms.Button();
            this.pnlPortSettings = new System.Windows.Forms.Panel();
            this.lblHandshake = new System.Windows.Forms.Label();
            this.cmbHandshake = new System.Windows.Forms.ComboBox();
            this.btnDTR = new System.Windows.Forms.Button();
            this.btnRTS = new System.Windows.Forms.Button();
            this.tmrStatus = new System.Windows.Forms.Timer();
            this.rbDSR = new System.Windows.Forms.RadioButton();
            this.rbCD = new System.Windows.Forms.RadioButton();
            this.pnlRTS = new System.Windows.Forms.Panel();
            this.rbCTS = new System.Windows.Forms.RadioButton();
            this.pnlDSR = new System.Windows.Forms.Panel();
            this.pnlCD = new System.Windows.Forms.Panel();
            this.pnlStatus = new System.Windows.Forms.Panel();
            this.pnlPortSettings.SuspendLayout();
            this.pnlRTS.SuspendLayout();
            this.pnlDSR.SuspendLayout();
            this.pnlCD.SuspendLayout();
            this.pnlStatus.SuspendLayout();
            this.SuspendLayout();
            // 
            // rtfTerminal
            // 
            this.rtfTerminal.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.rtfTerminal.Location = new System.Drawing.Point(4, 3);
            this.rtfTerminal.Multiline = true;
            this.rtfTerminal.Name = "rtfTerminal";
            this.rtfTerminal.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.rtfTerminal.Size = new System.Drawing.Size(343, 75);
            this.rtfTerminal.TabIndex = 1;
            // 
            // lblSend
            // 
            this.lblSend.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.lblSend.Location = new System.Drawing.Point(3, 13);
            this.lblSend.Name = "lblSend";
            this.lblSend.Size = new System.Drawing.Size(62, 20);
            this.lblSend.Text = "Send &Data:";
            // 
            // txtSendData
            // 
            this.txtSendData.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.txtSendData.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.txtSendData.Location = new System.Drawing.Point(60, 11);
            this.txtSendData.Name = "txtSendData";
            this.txtSendData.Size = new System.Drawing.Size(149, 19);
            this.txtSendData.TabIndex = 3;
            // 
            // lblComPort
            // 
            this.lblComPort.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.lblComPort.Location = new System.Drawing.Point(9, 7);
            this.lblComPort.Name = "lblComPort";
            this.lblComPort.Size = new System.Drawing.Size(56, 13);
            this.lblComPort.Text = "COM Port:";
            // 
            // cmbPortName
            // 
            this.cmbPortName.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDown;
            this.cmbPortName.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.cmbPortName.Location = new System.Drawing.Point(10, 23);
            this.cmbPortName.Name = "cmbPortName";
            this.cmbPortName.Size = new System.Drawing.Size(68, 19);
            this.cmbPortName.TabIndex = 13;
            // 
            // lblStopBits
            // 
            this.lblStopBits.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.lblStopBits.Location = new System.Drawing.Point(278, 7);
            this.lblStopBits.Name = "lblStopBits";
            this.lblStopBits.Size = new System.Drawing.Size(52, 13);
            this.lblStopBits.Text = "Stop Bits:";
            // 
            // cmbBaudRate
            // 
            this.cmbBaudRate.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.cmbBaudRate.Items.Add("300");
            this.cmbBaudRate.Items.Add("600");
            this.cmbBaudRate.Items.Add("1200");
            this.cmbBaudRate.Items.Add("2400");
            this.cmbBaudRate.Items.Add("4800");
            this.cmbBaudRate.Items.Add("9600");
            this.cmbBaudRate.Items.Add("14400");
            this.cmbBaudRate.Items.Add("28800");
            this.cmbBaudRate.Items.Add("38400");
            this.cmbBaudRate.Items.Add("115200");
            this.cmbBaudRate.Location = new System.Drawing.Point(84, 23);
            this.cmbBaudRate.Name = "cmbBaudRate";
            this.cmbBaudRate.Size = new System.Drawing.Size(69, 19);
            this.cmbBaudRate.TabIndex = 14;
            // 
            // cmbStopBits
            // 
            this.cmbStopBits.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.cmbStopBits.Items.Add("0");
            this.cmbStopBits.Items.Add("1");
            this.cmbStopBits.Items.Add("2");
            this.cmbStopBits.Items.Add("1.5");
            this.cmbStopBits.Location = new System.Drawing.Point(276, 23);
            this.cmbStopBits.Name = "cmbStopBits";
            this.cmbStopBits.Size = new System.Drawing.Size(45, 19);
            this.cmbStopBits.TabIndex = 19;
            // 
            // lblBaudRate
            // 
            this.lblBaudRate.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.lblBaudRate.Location = new System.Drawing.Point(83, 7);
            this.lblBaudRate.Name = "lblBaudRate";
            this.lblBaudRate.Size = new System.Drawing.Size(61, 13);
            this.lblBaudRate.Text = "Baud Rate:";
            // 
            // lblDataBits
            // 
            this.lblDataBits.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.lblDataBits.Location = new System.Drawing.Point(225, 7);
            this.lblDataBits.Name = "lblDataBits";
            this.lblDataBits.Size = new System.Drawing.Size(53, 13);
            this.lblDataBits.Text = "Data Bits:";
            // 
            // cmbParity
            // 
            this.cmbParity.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.cmbParity.Items.Add("None");
            this.cmbParity.Items.Add("Odd");
            this.cmbParity.Items.Add("Even");
            this.cmbParity.Items.Add("Mark");
            this.cmbParity.Items.Add("Space");
            this.cmbParity.Location = new System.Drawing.Point(159, 23);
            this.cmbParity.Name = "cmbParity";
            this.cmbParity.Size = new System.Drawing.Size(60, 19);
            this.cmbParity.TabIndex = 16;
            // 
            // cmbDataBits
            // 
            this.cmbDataBits.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.cmbDataBits.Items.Add("7");
            this.cmbDataBits.Items.Add("8");
            this.cmbDataBits.Items.Add("9");
            this.cmbDataBits.Location = new System.Drawing.Point(225, 23);
            this.cmbDataBits.Name = "cmbDataBits";
            this.cmbDataBits.Size = new System.Drawing.Size(45, 19);
            this.cmbDataBits.TabIndex = 18;
            // 
            // label1
            // 
            this.label1.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.label1.Location = new System.Drawing.Point(161, 7);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(36, 13);
            this.label1.Text = "Parity:";
            // 
            // btnOpenPort
            // 
            this.btnOpenPort.Location = new System.Drawing.Point(353, 3);
            this.btnOpenPort.Name = "btnOpenPort";
            this.btnOpenPort.Size = new System.Drawing.Size(75, 23);
            this.btnOpenPort.TabIndex = 23;
            this.btnOpenPort.Text = "&Open Port";
            this.btnOpenPort.Click += new System.EventHandler(this.btnOpenPort_Click);
            // 
            // btnSend
            // 
            this.btnSend.Location = new System.Drawing.Point(216, 8);
            this.btnSend.Name = "btnSend";
            this.btnSend.Size = new System.Drawing.Size(75, 25);
            this.btnSend.TabIndex = 24;
            this.btnSend.Text = "Send";
            this.btnSend.Click += new System.EventHandler(this.btnSend_Click);
            // 
            // pnlPortSettings
            // 
            this.pnlPortSettings.Controls.Add(this.lblHandshake);
            this.pnlPortSettings.Controls.Add(this.cmbHandshake);
            this.pnlPortSettings.Controls.Add(this.cmbBaudRate);
            this.pnlPortSettings.Controls.Add(this.label1);
            this.pnlPortSettings.Controls.Add(this.cmbDataBits);
            this.pnlPortSettings.Controls.Add(this.lblComPort);
            this.pnlPortSettings.Controls.Add(this.cmbParity);
            this.pnlPortSettings.Controls.Add(this.cmbPortName);
            this.pnlPortSettings.Controls.Add(this.lblDataBits);
            this.pnlPortSettings.Controls.Add(this.lblStopBits);
            this.pnlPortSettings.Controls.Add(this.lblBaudRate);
            this.pnlPortSettings.Controls.Add(this.cmbStopBits);
            this.pnlPortSettings.Location = new System.Drawing.Point(4, 84);
            this.pnlPortSettings.Name = "pnlPortSettings";
            this.pnlPortSettings.Size = new System.Drawing.Size(471, 51);
            // 
            // lblHandshake
            // 
            this.lblHandshake.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.lblHandshake.Location = new System.Drawing.Point(338, 7);
            this.lblHandshake.Name = "lblHandshake";
            this.lblHandshake.Size = new System.Drawing.Size(86, 13);
            this.lblHandshake.Text = "Handshake:";
            // 
            // cmbHandshake
            // 
            this.cmbHandshake.Font = new System.Drawing.Font("Tahoma", 8F, System.Drawing.FontStyle.Regular);
            this.cmbHandshake.Items.Add("None");
            this.cmbHandshake.Items.Add("RTS");
            this.cmbHandshake.Items.Add("RTSXONXOFF");
            this.cmbHandshake.Items.Add("XONXOFF");
            this.cmbHandshake.Location = new System.Drawing.Point(338, 23);
            this.cmbHandshake.Name = "cmbHandshake";
            this.cmbHandshake.Size = new System.Drawing.Size(86, 19);
            this.cmbHandshake.TabIndex = 23;
            // 
            // btnDTR
            // 
            this.btnDTR.Location = new System.Drawing.Point(297, 9);
            this.btnDTR.Name = "btnDTR";
            this.btnDTR.Size = new System.Drawing.Size(75, 24);
            this.btnDTR.TabIndex = 26;
            this.btnDTR.Text = "DTR";
            this.btnDTR.Click += new System.EventHandler(this.btnDTR_Click);
            // 
            // btnRTS
            // 
            this.btnRTS.Location = new System.Drawing.Point(378, 9);
            this.btnRTS.Name = "btnRTS";
            this.btnRTS.Size = new System.Drawing.Size(75, 24);
            this.btnRTS.TabIndex = 27;
            this.btnRTS.Text = "RTS";
            this.btnRTS.Click += new System.EventHandler(this.btnRTS_Click);
            // 
            // tmrStatus
            // 
            this.tmrStatus.Interval = 300;
            this.tmrStatus.Tick += new System.EventHandler(this.tmrStatus_Tick);
            // 
            // rbDSR
            // 
            this.rbDSR.Location = new System.Drawing.Point(3, 3);
            this.rbDSR.Name = "rbDSR";
            this.rbDSR.Size = new System.Drawing.Size(49, 20);
            this.rbDSR.TabIndex = 29;
            this.rbDSR.Text = "DSR";
            // 
            // rbCD
            // 
            this.rbCD.Location = new System.Drawing.Point(4, 3);
            this.rbCD.Name = "rbCD";
            this.rbCD.Size = new System.Drawing.Size(49, 20);
            this.rbCD.TabIndex = 32;
            this.rbCD.Text = "CD";
            // 
            // pnlRTS
            // 
            this.pnlRTS.Controls.Add(this.rbCTS);
            this.pnlRTS.Location = new System.Drawing.Point(3, 36);
            this.pnlRTS.Name = "pnlRTS";
            this.pnlRTS.Size = new System.Drawing.Size(75, 26);
            // 
            // rbCTS
            // 
            this.rbCTS.Location = new System.Drawing.Point(3, 4);
            this.rbCTS.Name = "rbCTS";
            this.rbCTS.Size = new System.Drawing.Size(62, 19);
            this.rbCTS.TabIndex = 0;
            this.rbCTS.Text = "CTS";
            // 
            // pnlDSR
            // 
            this.pnlDSR.Controls.Add(this.rbDSR);
            this.pnlDSR.Location = new System.Drawing.Point(82, 36);
            this.pnlDSR.Name = "pnlDSR";
            this.pnlDSR.Size = new System.Drawing.Size(75, 26);
            // 
            // pnlCD
            // 
            this.pnlCD.Controls.Add(this.rbCD);
            this.pnlCD.Location = new System.Drawing.Point(165, 36);
            this.pnlCD.Name = "pnlCD";
            this.pnlCD.Size = new System.Drawing.Size(71, 26);
            // 
            // pnlStatus
            // 
            this.pnlStatus.Controls.Add(this.btnDTR);
            this.pnlStatus.Controls.Add(this.btnSend);
            this.pnlStatus.Controls.Add(this.txtSendData);
            this.pnlStatus.Controls.Add(this.pnlCD);
            this.pnlStatus.Controls.Add(this.lblSend);
            this.pnlStatus.Controls.Add(this.btnRTS);
            this.pnlStatus.Controls.Add(this.pnlDSR);
            this.pnlStatus.Controls.Add(this.pnlRTS);
            this.pnlStatus.Enabled = false;
            this.pnlStatus.Location = new System.Drawing.Point(4, 141);
            this.pnlStatus.Name = "pnlStatus";
            this.pnlStatus.Size = new System.Drawing.Size(471, 68);
            // 
            // frmTerminal
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(478, 217);
            this.Controls.Add(this.pnlStatus);
            this.Controls.Add(this.pnlPortSettings);
            this.Controls.Add(this.btnOpenPort);
            this.Controls.Add(this.rtfTerminal);
            this.Font = new System.Drawing.Font("Arial", 8F, System.Drawing.FontStyle.Regular);
            this.Name = "frmTerminal";
            this.Text = "SerialTerminalCF ";
            this.pnlPortSettings.ResumeLayout(false);
            this.pnlRTS.ResumeLayout(false);
            this.pnlDSR.ResumeLayout(false);
            this.pnlCD.ResumeLayout(false);
            this.pnlStatus.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TextBox rtfTerminal;
        private System.Windows.Forms.Label lblSend;
        private System.Windows.Forms.TextBox txtSendData;
        private System.Windows.Forms.Label lblComPort;
        private System.Windows.Forms.ComboBox cmbPortName;
        private System.Windows.Forms.Label lblStopBits;
        private System.Windows.Forms.ComboBox cmbBaudRate;
        private System.Windows.Forms.ComboBox cmbStopBits;
        private System.Windows.Forms.Label lblBaudRate;
        private System.Windows.Forms.Label lblDataBits;
        private System.Windows.Forms.ComboBox cmbParity;
        private System.Windows.Forms.ComboBox cmbDataBits;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnOpenPort;
        private System.Windows.Forms.Button btnSend;
        private System.Windows.Forms.Panel pnlPortSettings;
        private System.Windows.Forms.Button btnDTR;
        private System.Windows.Forms.Button btnRTS;
        private System.Windows.Forms.Timer tmrStatus;
        private System.Windows.Forms.RadioButton rbDSR;
        private System.Windows.Forms.RadioButton rbCD;
        private System.Windows.Forms.Panel pnlRTS;
        private System.Windows.Forms.RadioButton rbCTS;
        private System.Windows.Forms.Panel pnlDSR;
        private System.Windows.Forms.Panel pnlCD;
        private System.Windows.Forms.Panel pnlStatus;
        private System.Windows.Forms.Label lblHandshake;
        private System.Windows.Forms.ComboBox cmbHandshake;


    }
}

