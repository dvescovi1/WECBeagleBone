using System;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using OpenNETCF.IO;

namespace BEAGLEBONE.ADC
{
    public class ADC : StreamInterfaceDriver
    {
        public enum ADCChannel
        {
            AN0,
            AN1,
            AN2,
            AN3,
            AN4,
            AN5,
            AN6,
            AN7
        }
        
        #region ADC device IOCTL codes

        private const Int32 CODE_IOCTL_ADC_GETCHANNEL = 0x0300;
        private const Int32 CODE_IOCTL_ADC_SCANCHANNEL = 0x0301;
        private const Int32 CODE_IOCTL_ADC_AVAILABLECHANNELS = 0x0302;

        private const Int32 FILE_DEVICE_UNKNOWN = 0x00000022;
        private const Int32 FILE_ANY_ACCESS = 0x0;
        private const Int32 METHOD_BUFFERED = 0x0;


        private const Int32 IOCTL_ADC_GETCHANNEL =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_ADC_GETCHANNEL) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_ADC_SCANCHANNEL =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_ADC_SCANCHANNEL) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_ADC_AVAILABLECHANNELS =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_ADC_AVAILABLECHANNELS) << 2) | (METHOD_BUFFERED);


        #endregion

        #region ctor / dtor
        /// <summary>
        /// Provides access to the ADC.
        /// </summary>
        public ADC() : base("ADC1:")
        {
            // open the driver
            Open(FileAccess.ReadWrite, FileShare.ReadWrite);
        }

        ~ADC()
        {
            // close the driver
            Close();
        }
        #endregion

        
        /// <summary>
        /// Scan and convert an ADC channel
        /// </summary>
        public ADCChannel Scan
        {
            set
            {
                try
                {
                    this.DeviceIoControl(IOCTL_ADC_SCANCHANNEL,SerializeToByteArray((UInt32)value));
                }
                catch (Exception)
                {
                    throw new Exception("Unable to complete DeviceIoControl:" + Marshal.GetLastWin32Error());
                }
            }
        }


        /// <summary>
        /// Reads an ADC channel result
        /// </summary>
        /// <param name="channel">channel to read</param>
        /// <returns>convert results</returns>
        public UInt32 ReadChannel(ADCChannel channel)
        {
            try
            {
                byte [] data = new byte[4];
                this.DeviceIoControl(IOCTL_ADC_GETCHANNEL,SerializeToByteArray((UInt32)channel),data);
                return (UInt32)DeserializeFromByteArray(data, typeof(UInt32));
            }
            catch (Exception)
            {
                throw new Exception("Unable to complete DeviceIoControl:" + Marshal.GetLastWin32Error());
            }
        }


        /// <summary>
        /// Test if ADC channel 0-3 are available
        /// </summary>
        public bool IsCh0_3Available
        {
            get
            {
                try
                {
                    byte[] data = new byte[4];
                    this.DeviceIoControl(IOCTL_ADC_AVAILABLECHANNELS, null, data);
                    if (data[0] == 0xff)
                        return true;
                    else
                        return false;
                }
                catch (Exception)
                {
                    throw new Exception("Unable to complete DeviceIoControl:" + Marshal.GetLastWin32Error());
                }
            }
        }


        #region P/Invoke helpers

        /// <summary>
        /// Byte array serializer
        /// </summary>
        /// <param name="anything"></param>
        /// <returns></returns>
        private static byte[] SerializeToByteArray(object anything)
        {
            int rawsize = Marshal.SizeOf(anything);
            IntPtr buffer = Marshal.AllocHGlobal(rawsize);
            Marshal.StructureToPtr(anything, buffer, false);
            byte[] rawdatas = new byte[rawsize];
            Marshal.Copy(buffer, rawdatas, 0, rawsize);
            Marshal.FreeHGlobal(buffer);
            return rawdatas;
        }

        /// <summary>
        /// De-serializer from byte array
        /// </summary>
        /// <param name="rawdatas"></param>
        /// <param name="anytype"></param>
        /// <returns></returns>
        private static object DeserializeFromByteArray(byte[] rawdatas, Type anytype)
        {
            int rawsize = Marshal.SizeOf(anytype);
            if (rawsize > rawdatas.Length)
                return null;
            IntPtr buffer = Marshal.AllocHGlobal(rawsize);
            Marshal.Copy(rawdatas, 0, buffer, rawsize);
            object retobj = Marshal.PtrToStructure(buffer, anytype);
            Marshal.FreeHGlobal(buffer);
            return retobj;
        }
        #endregion

    }
}
