using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using OpenNETCF.WindowsCE.Notification;
using System.Diagnostics;
using System.Threading;

namespace M2MQTT_test
{
    public partial class Form1 : Form
    {

        Stopwatch stopwatch = new Stopwatch();

        int ledCount;
        Led led;

        const int LEDA = 0;
        const int LEDB = 1;
        const int LEDC = 2;
        const int LEDD = 3;

        Communication com;

        public Form1()
        {
            InitializeComponent();
            Program.appSettings = ConfigurationManager.AppSettings;
            com = Communication.Instance;
            com.Connect("BeagleBone");
            led = new Led();
            ledCount = led.Count;

            com.Led1Event += HandleLed1Event;
            com.Led2Event += HandleLed2Event;
            com.Led3Event += HandleLed3Event;
            com.ResetElapsedTimeEvent += HandleResetElapsedTimeEvent;
            stopwatch.Start();
            tmrUpdate.Enabled = true;
        }


        delegate void ResetElapsedTimeHandler(object sender, EventArgs e);

        public void HandleResetElapsedTimeEvent(object sender, EventArgs e)
        {
            ResetElapsedTimeHandler cb = new ResetElapsedTimeHandler(btnRESET_Click);
            Invoke(cb, new object[] { this, e });            
        }
        
        public void HandleLed1Event(object sender, Communication.LedEventArgs e)
        {
            led.SetLedStatus(LEDA, (Led.LedState)e.Led);
        }        
        
        private void btnLED1ON_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDA, Led.LedState.On);
        }

        private void btnLED1OFF_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDA, Led.LedState.Off);
        }

        private void btnLED1FLASH_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDA, Led.LedState.Blink);
        }

        
        public void HandleLed2Event(object sender, Communication.LedEventArgs e)
        {
            led.SetLedStatus(LEDB, (Led.LedState)e.Led);
        }
        
        private void btnLED2ON_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDB, Led.LedState.On);
        }

        private void btnLED2OFF_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDB, Led.LedState.Off);
        }

        private void btnLED2FLASH_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDB, Led.LedState.Blink);
        }


        public void HandleLed3Event(object sender, Communication.LedEventArgs e)
        {
            led.SetLedStatus(LEDC, (Led.LedState)e.Led);
        }

        private void btnLED3ON_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDC, Led.LedState.On);
        }

        private void btnLED3OFF_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDC, Led.LedState.Off);
        }

        private void btnLED3FLASH_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDC, Led.LedState.Blink);
        }

        private void btnRESET_Click(object sender, EventArgs e)
        {
            stopwatch.Reset();
            stopwatch.Start();
        }

        private void tmrUpdate_Tick(object sender, EventArgs e)
        {
            com.publish_elapsedTime(stopwatch.Elapsed);
            lblElapsedTimeValue.Text = stopwatch.Elapsed.ToString();
        }
    }
}
