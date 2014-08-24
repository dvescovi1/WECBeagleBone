using System;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using OpenNETCF.IO;

namespace Embedded101.I2C
{
    public enum I2CPort : uint
    {
        I2C0 = 1,
        I2C1,
        I2C2
    }

    
    public class I2C : StreamInterfaceDriver
    {

        public enum SubAddressMode
        {
            MODE_0,
            MODE_8,
            MODE_16,
            MODE_24,
            MODE_32
        }

        public enum Speed
        {
            SPEED100KHZ,
            SPEED400KHZ,
            SPEED1P6MHZ,
            SPEED2P4MHZ,
            SPEED3P2MHZ
        }

 
        #region I2C device IOCTL codes

        private const Int32 CODE_IOCTL_I2C_SET_SLAVE_ADDRESS =      0x0200;
        private const Int32 CODE_IOCTL_I2C_SET_SUBADDRESS_MODE =    0x0201;
        private const Int32 CODE_IOCTL_I2C_SET_BAUD_INDEX =         0x0202;


        private const Int32 FILE_DEVICE_UNKNOWN = 0x00000022;
        private const Int32 FILE_ANY_ACCESS = 0x0;
        private const Int32 METHOD_BUFFERED = 0x0;


        private const Int32 IOCTL_I2C_SET_SLAVE_ADDRESS =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_I2C_SET_SLAVE_ADDRESS) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_I2C_SET_SUBADDRESS_MODE =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_I2C_SET_SUBADDRESS_MODE) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_I2C_SET_BAUD_INDEX =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_I2C_SET_BAUD_INDEX) << 2) | (METHOD_BUFFERED);


        #endregion
        
        #region ctor / dtor
        /// <summary>
        /// Provides access to the I2C bus on the OMAP.
        /// </summary>
        public I2C(I2CPort port) : base("I2C" + Convert.ToString((uint)port) + ":")
        {
            // open the driver
            Open(FileAccess.ReadWrite, FileShare.ReadWrite);
        }

        ~I2C()
        {
            // close the driver
            Close();
        }
        #endregion

        public void SetSlaveAddress(UInt16 slaveAddress)
        {
            try
            {
                UInt32 SA = (UInt32)slaveAddress;
                this.DeviceIoControl(IOCTL_I2C_SET_SLAVE_ADDRESS, SerializeToByteArray(SA));
            }
            catch (Exception)
            {
                throw new Exception("Unable to complete SetSlaveAddress DeviceIoControl:" + Marshal.GetLastWin32Error());
            }
        }

        public void SetSubAddressMode(SubAddressMode subAddressMode)
        {
            try
            {
                UInt32 SAM = (UInt32)subAddressMode;
                this.DeviceIoControl(IOCTL_I2C_SET_SUBADDRESS_MODE, SerializeToByteArray(SAM));
            }
            catch (Exception)
            {
                throw new Exception("Unable to complete SetSubAddressMode DeviceIoControl:" + Marshal.GetLastWin32Error());
            }
        }

        public void SetBaudRate(Speed speed)
        {
            try
            {
                UInt32 SPD = (UInt32)speed;
                this.DeviceIoControl(IOCTL_I2C_SET_BAUD_INDEX, SerializeToByteArray(SPD));
            }
            catch (Exception)
            {
                throw new Exception("Unable to complete SetBaudRate DeviceIoControl:" + Marshal.GetLastWin32Error());
            }
        }

        public int Write(byte register, byte value)
        {
            base.Seek((int)register, SeekOrigin.Current);
            return base.Write(SerializeToByteArray(value));
        }

        public int Write(byte register, UInt16 value)
        {
            base.Seek((int)register, SeekOrigin.Current);
            return base.Write(SerializeToByteArray(value));
        }

        public int Write(byte register, UInt32 value)
        {
            base.Seek((int)register, SeekOrigin.Current);
            return base.Write(SerializeToByteArray(value));
        }

        public int Write(byte register, byte[] data)
        {
            base.Seek((int)register, SeekOrigin.Current);
            return base.Write(data);
        }
        
        public byte ReadByte(byte register)
        {
            base.Seek((int)register, SeekOrigin.Current);
            return (byte)DeserializeFromByteArray(base.Read(sizeof(byte)),typeof(byte));
        }

        public Int32 ReadInt24(byte register)
        {
            base.Seek((int)register, SeekOrigin.Current);
            return (Int32)DeserializeFromByteArray24(base.Read(3), typeof(Int32));
        }

        public Int16 ReadInt16(byte register)
        {
            base.Seek((int)register, SeekOrigin.Current);
            return (Int16)DeserializeFromByteArray(base.Read(sizeof(Int16)), typeof(Int16));
        }

        public UInt32 ReadUInt32(byte register)
        {
            base.Seek((int)register, SeekOrigin.Current);
            return (UInt32)DeserializeFromByteArray(base.Read(sizeof(UInt32)), typeof(UInt32));
        }

        public void Read(byte register, ref byte[] data)
        {
            base.Seek((int)register, SeekOrigin.Current);
            data = base.Read(data.Length);
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

        private static object DeserializeFromByteArray24(byte[] rawdatas, Type anytype)
        {
            int rawsize = Marshal.SizeOf(anytype);
            IntPtr buffer = Marshal.AllocHGlobal(rawsize);
            Marshal.Copy(rawdatas, 0, buffer, rawdatas.Length);
            object retobj = Marshal.PtrToStructure(buffer, anytype);
            Marshal.FreeHGlobal(buffer);
            return retobj;
        }

        
        #endregion
    
    }
}
