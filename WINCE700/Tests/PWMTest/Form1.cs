using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Microsoft.Win32;
using BEAGLEBONE.PWM;

namespace PWMTest
{
    public partial class Form1 : Form
    {
        PWM pwm;
        RegistryKey pwmKey = Registry.LocalMachine.CreateSubKey("\\Drivers\\BuiltIn\\PWM1");

        public Form1()
        {
            InitializeComponent();
            this.Text += System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();
            pwm = new PWM();
            tbFREQUENCY.Value = (int)pwm.Frequency;
            tbDUTYCYCLE.Value = (int)pwm.DutyCycle;
            lblFREQUENCY.Text = "frequency: " + tbFREQUENCY.Value.ToString() + "Hz";
            lblDUTYCYCLE.Text = "Duty Cycle: " + tbDUTYCYCLE.Value.ToString() + "%";

            if ((int)pwm.Frequency != (int)(pwmKey.GetValue("Frequency")))
                throw new Exception("Frequency registry value mismatch");
            if ((int)pwm.DutyCycle != (int)pwmKey.GetValue("DutyCycle"))
                throw new Exception("DutyCycle registry value mismatch");
        }

        private void tbFREQUENCY_ValueChanged(object sender, EventArgs e)
        {
            pwm.Frequency = (UInt32)tbFREQUENCY.Value;
            lblFREQUENCY.Text = "frequency: " + tbFREQUENCY.Value.ToString() + "Hz";
        }

        private void tbDUTYCYCLE_ValueChanged(object sender, EventArgs e)
        {
            pwm.DutyCycle = (UInt32)tbDUTYCYCLE.Value;
            lblDUTYCYCLE.Text = "Duty Cycle: " + tbDUTYCYCLE.Value.ToString() + "%";
        }

        private void btnSAVE_Click(object sender, EventArgs e)
        {
            pwmKey.SetValue("Frequency", tbFREQUENCY.Value);
            pwmKey.SetValue("DutyCycle", tbDUTYCYCLE.Value);
        }
    }
}