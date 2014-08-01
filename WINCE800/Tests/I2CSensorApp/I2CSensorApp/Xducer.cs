using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;
using System.Threading;

namespace Embedded101.I2C
{
    public static class Xducer
    {
        static I2C XducerBus;
        static Int32 Xducer1Offset = 0;
        static Int32 Xducer2Offset = 0;


        #region ctor /dtor
        /// <summary>
        /// Access to the pressure xducers on I2C bus
        /// </summary>
        public static void Init()
        {
            XducerBus = new I2C(I2CPort.I2C1);
            XducerBus.SetBaudRate(I2C.Speed.SPEED400KHZ);
            XducerBus.SetSlaveAddress(0x48);
            XducerBus.SetSubAddressMode(I2C.SubAddressMode.MODE_8);
            XducerBus.Write(0x02, unchecked((UInt16)0x00028000));      // lo thresh register
            XducerBus.Write(0x03, unchecked((UInt16)0x00037fff));      // hi thresh register
            XducerBus.Write(0x01, unchecked((UInt16)0x00010480));
            XducerBus.SetSubAddressMode(I2C.SubAddressMode.MODE_0);
            XducerBus.Write(0x48, 0);      // results register
        }

        #endregion


        #region Raw xducer readings

        public static Int32 Xducer1Raw
        {
            get
            {
                byte[] data = new byte[2];
                byte[] config = new byte[3];

                config[0] = 0x01;                   // point to configuration register
                config[1] = 0x34;                   // mux channel (AN2/AN3 = Xducer1)
                config[2] = 0x80;
                XducerBus.Write(0x48, config);      // config registers
                XducerBus.Write(0x48, 0);
                XducerBus.Read(0x48, ref data);
                Thread.Sleep(10);
                XducerBus.Read(0x48, ref data);
                data[0] ^= 0x80;
                return (256 * data[0] + data[1]);
             }
        }

        public static Int32 Xducer2Raw
        {
            get
            {
                byte[] data = new byte[2];
                byte[] config = new byte[3];

                config[0] = 0x01;                   // point to configuration register
                config[1] = 0x04;                   // mux channel (AN0/AN1 = Xducer2)
                config[2] = 0x80;
                XducerBus.Write(0x48, config);      // config registers
                XducerBus.Write(0x48, 0);
                XducerBus.Read(0x48, ref data);
                Thread.Sleep(10);
                XducerBus.Read(0x48, ref data);
                data[0] ^= 0x80;
                return (256 * data[0] + data[1]);
            }
        }

        #endregion

        /// <summary>
        /// Returns O2 pressure in mmHg x 10
        /// </summary>
        public static Int32 Xducer1Reading
        {
            get
            {
                double scale;
                scale = 0.128 * (double)(Xducer1Raw - 32768 + Xducer1Offset);
                return (Int32)scale;
            }
        }

        /// <summary>
        /// Returns scaled xducer reading
        /// </summary>
        public static Int32 Xducer2Reading
        {
            get
            {
                double scale;
                scale = 0.128 * (double)(Xducer2Raw - 32768 + Xducer2Offset);
                return (Int32)scale;
            }
        }

        public static void ZeroXducers()
        {
            Xducer1Offset = -(Xducer1Raw - 32768);
            Xducer2Offset = -(Xducer2Raw - 32768);
        }
    }
}
