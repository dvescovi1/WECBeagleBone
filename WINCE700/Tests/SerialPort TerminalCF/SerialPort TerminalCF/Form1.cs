using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.IO.Ports;
using System.Windows.Forms;

namespace SerialPort_TerminalCF
{
    #region Public Enumerations
    public enum LogMsgType { Incoming, Outgoing, Normal, Warning, Error };
    #endregion

    public partial class frmTerminal : Form
    {
        #region Local Variables

        // The main control for communicating through the RS-232 port
        private SerialPort comport = new SerialPort();

        // Various colors for logging info
        private Color[] LogMsgTypeColor = { Color.Blue, Color.Green, Color.Black, Color.Orange, Color.Red };

        // Temp holder for whether a key was pressed
        private bool KeyHandled = false;

        #endregion

        #region Constructor
        public frmTerminal()
        {
            InitializeComponent();

            this.Text = this.Text + System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();

            // Restore the users settings
            InitializeControlValues();

            // Enable/disable controls based on the current state
            EnableControls();

            // When data is recieved through the port, call this method
            comport.DataReceived += new SerialDataReceivedEventHandler(port_DataReceived);
        }
        #endregion

        #region Local Methods

        /// <summary> Populate the form's controls with default settings. </summary>
        private void InitializeControlValues()
        {

            // defaults
            cmbParity.SelectedIndex     = 0;    // None
            cmbDataBits.SelectedIndex   = 1;    // 8
            cmbStopBits.SelectedIndex   = 1;    // one
            cmbBaudRate.SelectedIndex   = 9;    // 115200
            cmbHandshake.SelectedIndex  = 0;    // None

            cmbPortName.Items.Clear();
            foreach (string s in SerialPort.GetPortNames())
                cmbPortName.Items.Add(s);

            if (cmbPortName.Items.Count > 0) 
                cmbPortName.SelectedIndex = 0;
            else
            {
                MessageBox.Show("There are no COM Ports detected on this computer.\nPlease install a COM Port and restart this app.", "No COM Ports Installed", MessageBoxButtons.OK, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1);
                this.Close();
            }
            //if (cmbPortName.Items.Contains(Settings.Default.PortName)) 
            //    cmbPortName.Text = Settings.Default.PortName;
            //else if (cmbPortName.Items.Count > 0) 
            //    cmbPortName.SelectedIndex = 0;
            //else
            //{
            //    MessageBox.Show(this, "There are no COM Ports detected on this computer.\nPlease install a COM Port and restart this app.", "No COM Ports Installed", MessageBoxButtons.OK, MessageBoxIcon.Error);
            //    this.Close();
            //}
        }

        /// <summary> Enable/disable controls based on the app's current state. </summary>
        private void EnableControls()
        {
            // Enable/disable controls based on whether the port is open or not
            pnlPortSettings.Enabled = !comport.IsOpen;
            txtSendData.Enabled = btnSend.Enabled = comport.IsOpen;
            btnDTR.Enabled = true;
            btnRTS.Enabled = true;

            if (comport.IsOpen)
            {
                btnOpenPort.Text = "&Close Port";
                pnlStatus.Enabled = true;
                tmrStatus.Enabled = true;
                rtfTerminal.Text = string.Empty;
            }
            else
            {
                btnOpenPort.Text = "&Open Port";
                pnlStatus.Enabled = false;
                tmrStatus.Enabled = false;
            }
        }

        /// <summary> Send the user's data currently entered in the 'send' box.</summary>
        private void SendData()
        {
            // Send the user's text straight out the port
//            comport.Write(txtSendData.Text);
            comport.Write(txtSendData.Text + "\r\n");

            // Show in the terminal window the user's text
//            Log(LogMsgType.Outgoing, txtSendData.Text + "\n");
//            txtSendData.SelectAll();
        }

        /// <summary> Log data to the terminal window. </summary>
        /// <param name="msgtype"> The type of message to be written. </param>
        /// <param name="msg"> The string containing the message to be shown. </param>
        private void Log(LogMsgType msgtype, string msg)
        {
            rtfTerminal.Invoke(new EventHandler(delegate
            {
                rtfTerminal.SelectedText = string.Empty;
//                rtfTerminal.AppendText(msg);
                rtfTerminal.Text += msg;
                rtfTerminal.SelectionStart = rtfTerminal.Text.Length;
                rtfTerminal.ScrollToCaret();
                rtfTerminal.Refresh();
            }));
        }

        #endregion


        private void port_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            // This method will be called when there is data waiting in the port's buffer

            // Read all the data waiting in the buffer
            string data = comport.ReadExisting();

            // Display the text to the user in the terminal
            Log(LogMsgType.Incoming, data);
        }

        private void txtSendData_KeyDown(object sender, KeyEventArgs e)
        {
            // If the user presses [ENTER], send the data now
            if (KeyHandled = e.KeyCode == Keys.Enter) { e.Handled = true; SendData(); }
        }
 
        private void txtSendData_KeyPress(object sender, KeyPressEventArgs e)
        { e.Handled = KeyHandled; }
       
        
        private void btnOpenPort_Click(object sender, EventArgs e)
        {
            // If the port is open, close it.
            if (comport.IsOpen)
            {
                comport.Close();
            }
            else
            {
                // Set the port's settings
                comport.BaudRate = int.Parse(cmbBaudRate.Text);
                comport.DataBits = int.Parse(cmbDataBits.Text);
                comport.StopBits = (StopBits)cmbStopBits.SelectedIndex;
                comport.Parity = (Parity)cmbParity.SelectedIndex;
                comport.PortName = cmbPortName.Text;
                comport.Handshake = (Handshake)cmbHandshake.SelectedIndex;

                //comport.ReadTimeout = 500;
                comport.WriteTimeout = 500;

                // Open the port
                try
                {
                    comport.Open();
                }
                catch (Exception)
                {
                    Log(LogMsgType.Error, "Fail to open port!");
                }
                   
            }

            // Change the state of the form's controls
            EnableControls();

            // If the port is open, send focus to the send data box
            if (comport.IsOpen) txtSendData.Focus();
        }

        private void btnSend_Click(object sender, EventArgs e)
        {
            SendData(); 
        }

        private void btnDTR_Click(object sender, EventArgs e)
        {
            if (comport.DtrEnable == true)
            {
                comport.DtrEnable = false;
            }
            else
            {
                comport.DtrEnable = true;
            }
        }

        private void btnRTS_Click(object sender, EventArgs e)
        {
            if (comport.RtsEnable == true)
            {
                comport.RtsEnable = false;
            }
            else
            {
                comport.RtsEnable = true;
            }
        }

        private void tmrStatus_Tick(object sender, EventArgs e)
        {
            if (comport.DtrEnable == true)
            {
                btnDTR.Text = "DTR OFF";
            }
            else
            {
                btnDTR.Text = "DTR ON";
            }
            if (comport.Handshake == Handshake.None)
            {
                if (comport.RtsEnable == true)
                {
                    btnRTS.Text = "RTS OFF";
                }
                else
                {
                    btnRTS.Text = "RTS ON";
                }
            }
            else
            {
                btnRTS.Enabled = false;
                btnRTS.Text = "RTS HS";
            }
            if (comport.CtsHolding == true)
            {
                rbCTS.Checked = true;
            }
            else
            {
                rbCTS.Checked = false;
            }
            if (comport.DsrHolding == true)
            {
                rbDSR.Checked = true;
            }
            else
            {
                rbDSR.Checked = false;
            }
            if (comport.CDHolding == true)
            {
                rbCD.Checked = true;
            }
            else
            {
                rbCD.Checked = false;
            }
        }

    }
}