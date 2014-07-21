using System;
using System.Linq;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Collections.Specialized;

namespace M2MQTT_test
{
    static class Program
    {
        public static NameValueCollection appSettings;
        
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [MTAThread]
        static void Main()
        {
            Application.Run(new Form1());
        }
    }
}