using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using OpenNETCF.IO;

namespace SIS.VEUII.OMAP.PROXY
{
    public class PROXY : StreamInterfaceDriver
    {

        #region PROXY device IOCTL codes

        private const Int32 FILE_DEVICE_STREAMS = 0x0000001e;
        private const Int32 FILE_DEVICE_UNKNOWN = 0x00000022;
        private const Int32 FILE_ANY_ACCESS = 0x0;
        private const Int32 METHOD_BUFFERED = 0x0;

        private const Int32 CODE_IOCTL_PROXY_DVFS_FORCE = 103;

        private static byte[] inbuffer = new byte[4];
        private static byte[] outbuffer = new byte[4];

        private const Int32 IOCTL_PROXY_DVFS_FORCE =
            ((FILE_DEVICE_STREAMS) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_PROXY_DVFS_FORCE) << 2) | (METHOD_BUFFERED);


        #endregion

        #region ctor / dtor
        /// <summary>
        /// Provides access to the Accelerometer.
        /// </summary>
        public PROXY() : base("PXY1:")
        {
            // open the driver
            Open(FileAccess.ReadWrite, FileShare.ReadWrite);
        }

        ~PROXY()
        {
            // close the driver
            Close();
        }
        #endregion

        public void setOpm(int opm)
        {
            inbuffer = SerializeToByteArray(opm);
 
            try
            {
                this.DeviceIoControl(IOCTL_PROXY_DVFS_FORCE, inbuffer);
            }
            catch (Exception)
            {
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
