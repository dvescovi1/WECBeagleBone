using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using BEAGLEBONE.GPIO;

namespace GPIOTest
{
    public partial class Form1 : Form
    {
        GPIO gpio;

        const GPIOPin IN1 = GPIOPin.GPIO1_12;
        const GPIOPin IN2 = GPIOPin.GPIO1_13;
        const GPIOPin IN3 = GPIOPin.GPIO1_14;
        const GPIOPin IN4 = GPIOPin.GPIO1_15;

        const GPIOPin OUT1 = GPIOPin.GPIO1_16;
        const GPIOPin OUT2 = GPIOPin.GPIO1_17;
        const GPIOPin OUT3 = GPIOPin.GPIO3_19;
        const GPIOPin OUT4 = GPIOPin.GPIO3_21;

        
        public Form1()
        {
            InitializeComponent();
            this.Text += System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();
            gpio = new GPIO();
            gpio.SetDirection(IN1, Direction.Input);
            gpio.SetDirection(IN2, Direction.Input);
            gpio.SetDirection(IN3, Direction.Input);
            gpio.SetDirection(IN4, Direction.Input);
            gpio.SetDirection(OUT1, Direction.Output);
            gpio.SetDirection(OUT2, Direction.Output);
            gpio.SetDirection(OUT3, Direction.Output);
            gpio.SetDirection(OUT4, Direction.Output);
            tmrPOLL.Enabled = true;
        }

        private void btnOut1ON_Click(object sender, EventArgs e)
        {
            gpio.SetBit(OUT1);
        }

        private void btnOut1OFF_Click(object sender, EventArgs e)
        {
            gpio.ClearBit(OUT1);
        }

        private void btnOut2ON_Click(object sender, EventArgs e)
        {
            gpio.SetBit(OUT2);
        }

        private void btnOut2OFF_Click(object sender, EventArgs e)
        {
            gpio.ClearBit(OUT2);
        }

        private void btnOut3ON_Click(object sender, EventArgs e)
        {
            gpio.SetBit(OUT3);
        }

        private void btnOut3OFF_Click(object sender, EventArgs e)
        {
            gpio.ClearBit(OUT3);
        }

        private void btnOut4ON_Click(object sender, EventArgs e)
        {
            gpio.SetBit(OUT4);
        }

        private void btnOut4OFF_Click(object sender, EventArgs e)
        {
            gpio.ClearBit(OUT4);
        }

        private void tmrPOLL_Tick(object sender, EventArgs e)
        {
            if (gpio.GetBit(IN1))
            {
                rbIN1.Checked = true;
            }
            else
            {
                rbIN1.Checked = false;
            }
            if (gpio.GetBit(IN2))
            {
                rbIN2.Checked = true;
            }
            else
            {
                rbIN2.Checked = false;
            }
            if (gpio.GetBit(IN3))
            {
                rbIN3.Checked = true;
            }
            else
            {
                rbIN3.Checked = false;
            }
            if (gpio.GetBit(IN4))
            {
                rbIN4.Checked = true;
            }
            else
            {
                rbIN4.Checked = false;
            }
        }
    }
}