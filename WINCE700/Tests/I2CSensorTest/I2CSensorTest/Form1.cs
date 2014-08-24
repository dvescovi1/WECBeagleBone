using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Embedded101.I2C;

namespace I2CSensorTest
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
            Xducer.Init();
            tmrUPDATE.Interval = 500;
            tmrUPDATE.Enabled = true;
        }

        private void tmrUPDATE_Tick(object sender, EventArgs e)
        {
            lblXDUCER1READING.Text = Xducer.Xducer1Reading.ToString() + " mmHg";
            lblXDUCER2READING.Text = Xducer.Xducer2Reading.ToString() + " mmHg";
        }
    }
}
