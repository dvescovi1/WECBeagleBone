using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using SIS.VEUII.OMAP.PROXY;

namespace DVFS
{
    public partial class Form1 : Form
    {
        PROXY proxy;

        public Form1()
        {
            InitializeComponent();
            this.Text = this.Text + System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();
            proxy = new PROXY();
        }

        private void btnSWITCH_Click(object sender, EventArgs e)
        {
            int opm = 3;

            if (rb1GHZ.Checked)
                opm = 4;
            else if (rb800MHZ.Checked)
                opm = 3;
            else if (rb600MHZ.Checked)
                opm = 2;
            else if (rb300MHZ.Checked)
                opm = 1;

            proxy.setOpm(opm);
        }
    }
}