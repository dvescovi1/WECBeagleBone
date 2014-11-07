using System;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using OpenNETCF.IO;

namespace BEAGLEBONE.GPIO
{
    /// <summary>
    /// GPIO pin direction
    /// </summary>
    public enum Direction : uint
    {
        Output = 0,
        Input
    }
    
    
    /// <summary>
    /// GPIO pin ID's defined for OMAP
    /// </summary>
    public enum GPIOPin : uint
    {
        // CPU GPIOBank0
        GPIO0_0 = 0,
        GPIO0_1,
        GPIO0_2,
        GPIO0_3,
        GPIO0_4,
        GPIO0_5,
        GPIO0_6,
        GPIO0_7,
        GPIO0_8,
        GPIO0_9,
        GPIO0_10,
        GPIO0_11,
        GPIO0_12,
        GPIO0_13,
        GPIO0_14,
        GPIO0_15,
        GPIO0_16,
        GPIO0_17,
        GPIO0_18,
        GPIO0_19,
        GPIO0_20,
        GPIO0_21,
        GPIO0_22,
        GPIO0_23,
        GPIO0_24,
        GPIO0_25,
        GPIO0_26,
        GPIO0_27,
        GPIO0_28,
        GPIO0_29,
        GPIO0_30,
        GPIO0_31,

        // CPU GPIOBank1
        GPIO1_0,
        GPIO1_1,
        GPIO1_2,
        GPIO1_3,
        GPIO1_4,
        GPIO1_5,
        GPIO1_6,
        GPIO1_7,
        GPIO1_8,
        GPIO1_9,
        GPIO1_10,
        GPIO1_11,
        GPIO1_12,
        GPIO1_13,
        GPIO1_14,
        GPIO1_15,
        GPIO1_16,
        GPIO1_17,
        GPIO1_18,
        GPIO1_19,
        GPIO1_20,
        GPIO1_21,
        GPIO1_22,
        GPIO1_23,
        GPIO1_24,
        GPIO1_25,
        GPIO1_26,
        GPIO1_27,
        GPIO1_28,
        GPIO1_29,
        GPIO1_30,
        GPIO1_31,

        // CPU GPIOBank2
        GPIO2_0,
        GPIO2_1,
        GPIO2_2,
        GPIO2_3,
        GPIO2_4,
        GPIO2_5,
        GPIO2_6,
        GPIO2_7,
        GPIO2_8,
        GPIO2_9,
        GPIO2_10,
        GPIO2_11,
        GPIO2_12,
        GPIO2_13,
        GPIO2_14,
        GPIO2_15,
        GPIO2_16,
        GPIO2_17,
        GPIO2_18,
        GPIO2_19,
        GPIO2_20,
        GPIO2_21,
        GPIO2_22,
        GPIO2_23,
        GPIO2_24,
        GPIO2_25,
        GPIO2_26,
        GPIO2_27,
        GPIO2_28,
        GPIO2_29,
        GPIO2_30,
        GPIO2_31,

        // CPU GPIOBank3
        GPIO3_0,
        GPIO3_1,
        GPIO3_2,
        GPIO3_3,
        GPIO3_4,
        GPIO3_5,
        GPIO3_6,
        GPIO3_7,
        GPIO3_8,
        GPIO3_9,
        GPIO3_10,
        GPIO3_11,
        GPIO3_12,
        GPIO3_13,
        GPIO3_14,
        GPIO3_15,
        GPIO3_16,
        GPIO3_17,
        GPIO3_18,
        GPIO3_19,
        GPIO3_20,
        GPIO3_21,
        GPIO3_22,
        GPIO3_23,
        GPIO3_24,
        GPIO3_25,
        GPIO3_26,
        GPIO3_27,
        GPIO3_28,
        GPIO3_29,
        GPIO3_30,
        GPIO3_31,
        GPIO_MAX_NUM
    }

    

    public class GPIO : StreamInterfaceDriver 
    {

        #region GIO device IOCTL codes
        
        private const Int32 CODE_IOCTL_GPIO_SETBIT =                0x0300;
        private const Int32 CODE_IOCTL_GPIO_CLRBIT =                0x0301;
        private const Int32 CODE_IOCTL_GPIO_UPDATEBIT =             0x0302;
        private const Int32 CODE_IOCTL_GPIO_GETBIT =                0x0303;
        private const Int32 CODE_IOCTL_GPIO_SETMODE =               0x0304;
        private const Int32 CODE_IOCTL_GPIO_GETMODE =               0x0305;
        private const Int32 CODE_IOCTL_GPIO_SET_DEBOUNCE_TIME =     0x0306;
        private const Int32 CODE_IOCTL_GPIO_GET_DEBOUNCE_TIME =     0x0307;
        private const Int32 CODE_IOCTL_GPIO_INIT_INTERRUPT =        0x0308;
        private const Int32 CODE_IOCTL_GPIO_ACK_INTERRUPT =         0x0309;

        private const Int32 CODE_IOCTL_GPIO_DISABLE_INTERRUPT =     0x0310;
        private const Int32 CODE_IOCTL_GPIO_RELEASE_INTERRUPT =     0x0311;
        private const Int32 CODE_IOCTL_GPIO_ENABLE_WAKE =           0x0312;
        private const Int32 CODE_IOCTL_GPIO_GET_OMAP_HW_INTR =      0x0313;
        private const Int32 CODE_IOCTL_GPIO_MASK_INTERRUPT =        0x0314;
        private const Int32 CODE_IOCTL_GPIO_PULLUP_ENABLE =         0x0315;
        private const Int32 CODE_IOCTL_GPIO_PULLUP_DISABLE =        0x0316;
        private const Int32 CODE_IOCTL_GPIO_PULLDOWN_ENABLE =       0x0317;
        private const Int32 CODE_IOCTL_GPIO_PULLDOWN_DISABLE =      0x0318;
        private const Int32 CODE_IOCTL_GPIO_GETIRQ =                0x0319;
        

        
        private const Int32 FILE_DEVICE_UNKNOWN = 0x00000022;
        private const Int32 FILE_ANY_ACCESS = 0x0;
        private const Int32 METHOD_BUFFERED = 0x0;


        private const Int32 IOCTL_GPIO_SETBIT =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_SETBIT) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_CLRBIT =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_CLRBIT) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_UPDATEBIT =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_UPDATEBIT) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_GETBIT =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_GETBIT) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_SETMODE =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_SETMODE) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_GETMODE =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_GETMODE) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_SET_DEBOUNCE_TIME =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_SET_DEBOUNCE_TIME) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_GET_DEBOUNCE_TIME =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_GET_DEBOUNCE_TIME) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_INIT_INTERRUPT =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_INIT_INTERRUPT) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_ACK_INTERRUPT =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_ACK_INTERRUPT) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_DISABLE_INTERRUPT =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_DISABLE_INTERRUPT) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_RELEASE_INTERRUPT =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_RELEASE_INTERRUPT) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_ENABLE_WAKE =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_ENABLE_WAKE) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_GET_OMAP_HW_INTR =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_GET_OMAP_HW_INTR) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_MASK_INTERRUPT =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_MASK_INTERRUPT) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_PULLUP_ENABLE =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_PULLUP_ENABLE) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_PULLUP_DISABLE =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_PULLUP_DISABLE) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_PULLDOWN_ENABLE =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_PULLDOWN_ENABLE) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_PULLDOWN_DISABLE =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_PULLDOWN_DISABLE) << 2) | (METHOD_BUFFERED);

        private const Int32 IOCTL_GPIO_GETIRQ =
            ((FILE_DEVICE_UNKNOWN) << 16) | ((FILE_ANY_ACCESS) << 14)
            | ((CODE_IOCTL_GPIO_GETIRQ) << 2) | (METHOD_BUFFERED);


        #endregion

        #region Native interface structures


        [StructLayout(LayoutKind.Sequential)]
        private struct PinDirection
        {
            public UInt32 gpioId;
            public UInt32 direction;

            public PinDirection(GPIOPin gpioId, Direction dir)
            {
                this.gpioId = (UInt32)gpioId;
                if (dir == Direction.Input)
                    this.direction = 1;
                else
                    this.direction = 0;
            }
        }


        [StructLayout(LayoutKind.Sequential)]
        private struct DebounceTime
        {
            public UInt32 gpioId;
            public UInt32 debounceTime;

            public DebounceTime(GPIOPin gpioId, UInt32 debounceTime)
            {
                this.gpioId = (UInt32)gpioId;
                this.debounceTime = debounceTime;
            }
        }


        [StructLayout(LayoutKind.Sequential)]
        private struct InterruptMask
        {
            public UInt32 gpioId;
            public bool enable;

            public InterruptMask(GPIOPin gpioId, bool enable)
            {
                this.gpioId = (UInt32)gpioId;
                this.enable = enable;
            }
        }


        [StructLayout(LayoutKind.Sequential)]
        private struct EnableWakeIn
        {
            public UInt32 gpioId;
            public bool enable;

            public EnableWakeIn(GPIOPin gpioId, bool enable)
            {
                this.gpioId = (UInt32)gpioId;
                this.enable = enable;
            }
        }


        [StructLayout(LayoutKind.Sequential)]
        private struct InitInterruptInfo
        {
            public UInt32 gpioId;
            public UInt32 hEvent;

            public InitInterruptInfo(GPIOPin gpioId, UInt32 hEvent)
            {
                this.gpioId = (UInt32)gpioId;
                this.hEvent = hEvent;
            }
        }


        
        #endregion

        #region ctor / dtor
        /// <summary>
        /// Provides access to the GPIO pins on the OMAP.
        /// </summary>
        public GPIO() : base("GIO1:")
        {
            // open the driver
            Open(FileAccess.ReadWrite, FileShare.ReadWrite);
        }

        ~GPIO()
        {
            // close the driver
            Close();
        }
        #endregion


        /// <summary>
        /// Sets the GPIO direction
        /// </summary>
        /// <param name="pin">GPIO Pin</param>
        /// <param name="dir">Direction</param>
        public void SetDirection(GPIOPin pin, Direction dir)
        {
            try
            {
                PinDirection pd = new PinDirection(pin, dir);
                this.DeviceIoControl(IOCTL_GPIO_SETMODE, SerializeToByteArray(pd));
            }
            catch (Exception)
            {
                throw new Exception("Unable to complete SetDirection DeviceIoControl:" + Marshal.GetLastWin32Error());
            }
        }

        
        
        /// <summary>
        /// Get the status of a GPIO pin
        /// </summary>
        /// <param name="pin">GPIO pin ID</param>
        /// <returns></returns>
        public bool GetBit(GPIOPin pin)
        {
            try
            {
                byte [] data = new byte[4];
                this.DeviceIoControl(IOCTL_GPIO_GETBIT, SerializeToByteArray(pin),data);
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
        
        /// <summary>
        /// Sets the GPIO pin high (SET)
        /// </summary>
        /// <param name="pin">GPIO Pin ID</param>
        public void SetBit(GPIOPin pin)
        {
            try
            {
                this.DeviceIoControl(IOCTL_GPIO_SETBIT, SerializeToByteArray(pin));
            }
            catch (Exception)
            {
                throw new Exception("Unable to complete SetBit DeviceIoControl:" + Marshal.GetLastWin32Error());
            }
        }

        /// <summary>
        /// Sets the GPIO pin low (CLEAR)
        /// </summary>
        /// <param name="pin">GPIO Pin ID</param>
        public void ClearBit(GPIOPin pin)
        {
            try
            {
                this.DeviceIoControl(IOCTL_GPIO_CLRBIT, SerializeToByteArray(pin));
            }
            catch (Exception)
            {
                throw new Exception("Unable to complete ClearBit DeviceIoControl:" + Marshal.GetLastWin32Error());
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
