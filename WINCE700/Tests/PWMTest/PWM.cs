using System;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using OpenNETCF.IO;

namespace BEAGLEBONE.PWM
{
    public class PWM : StreamInterfaceDriver
    {

        #region PWM device IOCTL codes

        private const Int32 CODE_IOCTL_PWM_GET_FREQUENCY = 0x0200;
        private const Int32 CODE_IOCTL_PWM_GET_DUTYCYCLE = 0x0201;
        private const Int32 CODE_IOCTL_PWM_GET_STARTSTOP = 0x0202;
        private const Int32 CODE_IOCTL_PWM_SET_FREQUENCY = 0x0203;
        private const Int32 CODE_IOCTL_PWM_SET_DUTYCYCLE = 0x0204;
        private const Int32 CODE_IOCTL_PWM_SET_STARTSTOP = 0x0205;

        private const Int32 FILE_DEVICE_UNKNOWN = 0x00000022;
        private const Int32 FILE_ANY_ACCESS = 0x0;
        private const Int32 METHOD_BUFFERED = 0x0;


        private const Int32 IOCTL_PWM_GET_FREQUENCY =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_PWM_GET_FREQUENCY) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_PWM_GET_DUTYCYCLE =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_PWM_GET_DUTYCYCLE) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_PWM_GET_STARTSTOP =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_PWM_GET_STARTSTOP) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_PWM_SET_FREQUENCY =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_PWM_SET_FREQUENCY) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_PWM_SET_DUTYCYCLE =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_PWM_SET_DUTYCYCLE) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_PWM_SET_STARTSTOP =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_PWM_SET_STARTSTOP) << 2) | (METHOD_BUFFERED);

        #endregion

        #region ctor / dtor
        /// <summary>
        /// Provides access to the PWM pins.
        /// </summary>
        public PWM() : base("PWM1:")
        {
            // open the driver
            Open(FileAccess.ReadWrite, FileShare.ReadWrite);
        }

        public PWM(string port) : base(port)
        {
            // open the driver
            Open(FileAccess.ReadWrite, FileShare.ReadWrite);
        }

        ~PWM()
        {
            // close the driver
            Close();
        }
        #endregion

        
        /// <summary>
        /// PWM frequency
        /// </summary>
        public UInt32 Frequency
        {
            set
            {
                UInt32 frequency = value;
                if (frequency > 10000000 | frequency < 50)
                    throw new ApplicationException("Frequency out of range");
                try
                {
                    this.DeviceIoControl(IOCTL_PWM_SET_FREQUENCY, SerializeToByteArray(frequency));

                }
                catch (Exception)
                {
                    throw new Exception("Unable to complete SetDirection DeviceIoControl:" + Marshal.GetLastWin32Error());
                }
            }
            get
            {
                try
                {
                    byte [] data = new byte[4];
                    this.DeviceIoControl(IOCTL_PWM_GET_FREQUENCY, null, data);
                    return (UInt32)DeserializeFromByteArray(data, typeof(UInt32));
                }
                catch (Exception)
                {
                    throw new Exception("Unable to complete GetBit DeviceIoControl:" + Marshal.GetLastWin32Error());
                }
            }
        }

        /// <summary>
        /// PWM duty cycle
        /// </summary>
        public UInt32 DutyCycle
        {
            set
            {
                if (value > 100 | value < 0)
                    throw new ApplicationException("Duty cycle out of range");
                try
                {
                    this.DeviceIoControl(IOCTL_PWM_SET_DUTYCYCLE, SerializeToByteArray(value));

                }
                catch (Exception)
                {
                    throw new Exception("Unable to complete SetDirection DeviceIoControl:" + Marshal.GetLastWin32Error());
                }
            }
            get
            {
                try
                {
                    byte [] data = new byte[4];
                    this.DeviceIoControl(IOCTL_PWM_GET_DUTYCYCLE, null, data);
                    return (UInt32)DeserializeFromByteArray(data, typeof(UInt32));
                }
                catch (Exception)
                {
                    throw new Exception("Unable to complete GetBit DeviceIoControl:" + Marshal.GetLastWin32Error());
                }
            }
        }

        /// <summary>
        /// PWM start stop
        /// </summary>
        public bool RunPWM
        {
            set
            {
                UInt32 go = 0;
                if (value)
                    go = 1;

                try
                {
                    this.DeviceIoControl(IOCTL_PWM_SET_STARTSTOP, SerializeToByteArray(go));

                }
                catch (Exception)
                {
                    throw new Exception("Unable to complete SetDirection DeviceIoControl:" + Marshal.GetLastWin32Error());
                }
            }
            get
            {
                try
                {
                    byte [] data = new byte[4];
                    this.DeviceIoControl(IOCTL_PWM_GET_STARTSTOP, null, data);
                    if (1 == data[0])
                        return true;
                    else
                        return false;
                }
                catch (Exception)
                {
                    throw new Exception("Unable to complete GetBit DeviceIoControl:" + Marshal.GetLastWin32Error());
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
