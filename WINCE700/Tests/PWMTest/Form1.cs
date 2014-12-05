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
        string PWMChannelText;
        RegistryKey pwmKey;

        public Form1()
        {
            InitializeComponent();
            this.Text += System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();
            PWMChannelText = "PWM1";
            pwmKey = Registry.LocalMachine.CreateSubKey("\\Drivers\\BuiltIn\\" + PWMChannelText);
        }

        private void tbFREQUENCY_ValueChanged(object sender, EventArgs e)
        {
            pwm.Frequency = (UInt32)tbFREQUENCY.Value;
            lblFREQUENCY.Text = "Frequency: " + tbFREQUENCY.Value.ToString() + "Hz";
        }

        private void btnSAVE_Click(object sender, EventArgs e)
        {
            pwmKey.SetValue("Frequency", tbFREQUENCY.Value);
            pwmKey.SetValue("DutyCycleA", tbDUTYCYCLEA.Value);
            pwmKey.SetValue("DutyCycleB", tbDUTYCYCLEB.Value);
        }

        private void tbDUTYCYCLEA_ValueChanged(object sender, EventArgs e)
        {
            pwm.DutyCycleA = (UInt32)tbDUTYCYCLEA.Value;
            lblDUTYCYCLEA.Text = "Duty Cycle A: " + tbDUTYCYCLEA.Value.ToString() + "%";
        }

        private void tbDUTYCYCLEB_ValueChanged(object sender, EventArgs e)
        {
            pwm.DutyCycleB = (UInt32)tbDUTYCYCLEB.Value;
            lblDUTYCYCLEB.Text = "Duty Cycle B: " + tbDUTYCYCLEB.Value.ToString() + "%";
        }

        private void rbPWM0_CheckedChanged(object sender, EventArgs e)
        {
            if (rbPWM0.Checked)
            {
                PWMChannelText = "PWM0";
                pwmKey = Registry.LocalMachine.CreateSubKey("\\Drivers\\BuiltIn\\" + PWMChannelText);
            }

        }

        private void rbPWM1_CheckedChanged(object sender, EventArgs e)
        {
            if (rbPWM1.Checked)
            {
                PWMChannelText = "PWM1";
                pwmKey = Registry.LocalMachine.CreateSubKey("\\Drivers\\BuiltIn\\" + PWMChannelText);
            }

        }

        private void rbPWM2_CheckedChanged(object sender, EventArgs e)
        {
            if (rbPWM1.Checked)
            {
                PWMChannelText = "PWM2";
                pwmKey = Registry.LocalMachine.CreateSubKey("\\Drivers\\BuiltIn\\" + PWMChannelText);
            }

        }

        private void btnOPEN_Click(object sender, EventArgs e)
        {
            btnOPEN.Enabled = false;
            pnlPWM.Enabled = false;
            pnlSETTINGS.Enabled = true;
            pwm = new PWM(PWMChannelText + ":");
            if (1 == (int)pwmKey.GetValue("EPWMXA_Active"))
            {
                tbDUTYCYCLEA.Enabled = true;
                tbDUTYCYCLEA.Value = (int)pwm.DutyCycleA;
                lblDUTYCYCLEA.Text = "Duty Cycle A: " + tbDUTYCYCLEA.Value.ToString() + "%";
            }
            else
            {
                tbDUTYCYCLEA.Enabled = false;
                tbDUTYCYCLEA.Value = 0;
                lblDUTYCYCLEA.Text = "Duty Cycle A: ";
            }
            if (1 == (int)pwmKey.GetValue("EPWMXB_Active"))
            {
                tbDUTYCYCLEB.Enabled = true;
                tbDUTYCYCLEB.Value = (int)pwm.DutyCycleB;
                lblDUTYCYCLEB.Text = "Duty Cycle B: " + tbDUTYCYCLEB.Value.ToString() + "%";
            }
            else
            {
                tbDUTYCYCLEB.Enabled = false;
                tbDUTYCYCLEB.Value = 0;
                lblDUTYCYCLEB.Text = "Duty Cycle B: ";
            }
            if ((1 == (int)pwmKey.GetValue("EPWMXA_Active")) || (1 == (int)pwmKey.GetValue("EPWMXB_Active")))
            {
                tbFREQUENCY.Enabled = true;
                tbFREQUENCY.Value = (int)pwm.Frequency;
                lblFREQUENCY.Text = "Frequency: " + tbFREQUENCY.Value.ToString() + "Hz";
            }
        }

        private void btnSTARTA_Click(object sender, EventArgs e)
        {
            pwm.RunPWMA = true;
        }

        private void btnSTOPA_Click(object sender, EventArgs e)
        {
            pwm.RunPWMA = false;
        }

        private void btnSTARTB_Click(object sender, EventArgs e)
        {
            pwm.RunPWMB = true;
        }

        private void btnSTOPB_Click(object sender, EventArgs e)
        {
            pwm.RunPWMB = false;
        }
    }
}