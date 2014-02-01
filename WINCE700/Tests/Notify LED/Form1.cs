// Copyright © David Vescovi.  All rights reserved.
// Copyright © S-I Solutions 2010

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using OpenNETCF.WindowsCE.Notification;

namespace VUEII_test
{
    public partial class Form1 : Form
    {
        int ledCount;
        Led led;

        const int LEDA = 0;
        const int LEDB = 1;
        const int LEDC = 2;
        const int LEDD = 3;

        public Form1()
        {
            InitializeComponent();
            led = new Led();
            ledCount = led.Count;
        }

        private void btnNotificationLEDAOFF_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDA, Led.LedState.Off);
        }

        private void btnNotificationLEDAON_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDA, Led.LedState.On);
        }

        private void btnNotificationLEDAFLASH_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDA, Led.LedState.Blink);
        }

        private void btnNotificationLEDBOFF_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDB, Led.LedState.Off);
        }

        private void btnNotificationLEDBON_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDB, Led.LedState.On);
        }

        private void btnNotificationLEDBFLASH_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDB, Led.LedState.Blink);
        }

        private void btnNotificationLEDCOFF_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDC, Led.LedState.Off);
        }

        private void btnNotificationLEDCON_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDC, Led.LedState.On);
        }

        private void btnNotificationLEDCFLASH_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDC, Led.LedState.Blink);
        }

        private void btnNotificationLEDDOFF_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDD, Led.LedState.Off);
        }

        private void btnNotificationLEDDON_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDD, Led.LedState.On);
        }

        private void btnNotificationLEDDFLASH_Click(object sender, EventArgs e)
        {
            led.SetLedStatus(LEDD, Led.LedState.Blink);
        }
    }
}