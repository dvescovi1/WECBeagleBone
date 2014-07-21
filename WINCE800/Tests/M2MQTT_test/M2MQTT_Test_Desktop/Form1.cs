using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Configuration;

namespace M2MQTT_Test_Desktop
{
    /// <summary>
    /// Defines the possible states for an LED
    /// </summary>
    public enum LedState : int
    {
        /// <summary>
        /// LED is off
        /// </summary>
        Off = 0,
        /// <summary>
        /// LED is on
        /// </summary>
        On = 1,
        /// <summary>
        /// LED cycles between On and Off
        /// </summary>
        Blink = 2
    }

    public partial class Form1 : Form
    {
        Communication com;

        public Form1()
        {
            InitializeComponent();
            com = Communication.Instance;
            com.Connect("PC");
            com.ElapsedTimeEvent += HandleElapsedTimeEvent;
        }

        delegate void SetElapsedTimeHandler(object sender, Communication.ElapsedTimeEventArgs e);
        
        public void HandleElapsedTimeEvent(object sender, Communication.ElapsedTimeEventArgs e)
        {
            SetElapsedTimeHandler cb = new SetElapsedTimeHandler(ElapsedTime_Update);
            Invoke(cb, new object[] { this, e });
        }

        private void ElapsedTime_Update(object sender, Communication.ElapsedTimeEventArgs e)
        {
            this.lblElapsedTimeValue.Text = e.Time.ToString();
        }
        
        private void btnLED1ON_Click(object sender, EventArgs e)
        {
            com.publish_led1(LedState.On);
        }

        private void btnLED1OFF_Click(object sender, EventArgs e)
        {
            com.publish_led1(LedState.Off);
        }

        private void btnLED1FLASH_Click(object sender, EventArgs e)
        {
            com.publish_led1(LedState.Blink);
        }

        private void btnLED2ON_Click(object sender, EventArgs e)
        {
            com.publish_led2(LedState.On);
        }

        private void btnLED2OFF_Click(object sender, EventArgs e)
        {
            com.publish_led2(LedState.Off);
        }

        private void btnLED2FLASH_Click(object sender, EventArgs e)
        {
            com.publish_led2(LedState.Blink);
        }

        private void btnLED3ON_Click(object sender, EventArgs e)
        {
            com.publish_led3(LedState.On);
        }

        private void btnLED3OFF_Click(object sender, EventArgs e)
        {
            com.publish_led3(LedState.Off);
        }

        private void btnLED3FLASH_Click(object sender, EventArgs e)
        {
            com.publish_led3(LedState.Blink);
        }

        private void btnRESET_Click(object sender, EventArgs e)
        {
            com.publish_resetElapsedTime();
        }
    }
}
