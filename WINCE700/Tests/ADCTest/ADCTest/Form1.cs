using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using BEAGLEBONE.ADC;

namespace ADCTest
{
    public partial class Form1 : Form
    {
        ADC adc;
        public Form1()
        {
            InitializeComponent();
            this.Text += System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();
            adc = new ADC();
            if (adc.IsCh0_3Available)
                pnl0_3.Enabled = true;
            else
                pnl0_3.Enabled = false;
            timer1.Enabled = true;
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (pnl0_3.Enabled)
            {
                adc.Scan = ADC.ADCChannel.AN0;
                tbAN0.Value = (int)adc.ReadChannel(ADC.ADCChannel.AN0);
                lblAN0VALUE.Text = tbAN0.Value.ToString();
                adc.Scan = ADC.ADCChannel.AN1;
                tbAN1.Value = (int)adc.ReadChannel(ADC.ADCChannel.AN1);
                lblAN1VALUE.Text = tbAN1.Value.ToString();
                adc.Scan = ADC.ADCChannel.AN2;
                tbAN2.Value = (int)adc.ReadChannel(ADC.ADCChannel.AN2);
                lblAN2VALUE.Text = tbAN2.Value.ToString();
                adc.Scan = ADC.ADCChannel.AN3;
                tbAN3.Value = (int)adc.ReadChannel(ADC.ADCChannel.AN3);
                lblAN3VALUE.Text = tbAN3.Value.ToString();
            }
            adc.Scan = ADC.ADCChannel.AN4;
            tbAN4.Value = (int)adc.ReadChannel(ADC.ADCChannel.AN4);
            lblAN4VALUE.Text = tbAN4.Value.ToString();
            adc.Scan = ADC.ADCChannel.AN5;
            tbAN5.Value = (int)adc.ReadChannel(ADC.ADCChannel.AN5);
            lblAN5VALUE.Text = tbAN5.Value.ToString();
            adc.Scan = ADC.ADCChannel.AN6;
            tbAN6.Value = (int)adc.ReadChannel(ADC.ADCChannel.AN6);
            lblAN6VALUE.Text = tbAN6.Value.ToString();
            adc.Scan = ADC.ADCChannel.AN7;
            tbAN7.Value = (int)adc.ReadChannel(ADC.ADCChannel.AN7);
            lblAN7VALUE.Text = tbAN7.Value.ToString();
        }
    }
}