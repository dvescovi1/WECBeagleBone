using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.Runtime.InteropServices;
using System.Configuration;
using uPLibrary.Networking.M2Mqtt;
using uPLibrary.Networking.M2Mqtt.Messages;

namespace M2MQTT_Test_Desktop
{
    public class Communication
    {

        private MqttClient mqttClient;

        private static volatile Communication instance;
        private static object syncRoot = new Object();

        /// <summary>
        /// Readings
        /// </summary>
        private const string MQTT_ELAPSEDTIME_TOPIC =   "ElapsedTime";

        /// <summary>
        /// Commands
        /// </summary>
        private const string MQTT_LED1_TOPIC =  "LED1";
        private const string MQTT_LED2_TOPIC =  "LED2";
        private const string MQTT_LED3_TOPIC =  "LED3";
        private const string MQTT_RESET_TIME_TOPIC = "RESET_TIME";


        public static Communication Instance
        {
            get
            {
                if (instance == null)
                {
                    lock (syncRoot)
                    {
                        if (instance == null)
                            instance = new Communication();
                    }
                }
                return instance;
            }
        }

        private Communication()
        {
//            mqttClient = new MqttClient(ConfigurationManager.AppSettings.Get("Broker HostName"));
            mqttClient = new MqttClient(IPAddress.Parse(ConfigurationManager.AppSettings.Get("Broker Address")));
            mqttClient.MqttMsgPublished += new MqttClient.MqttMsgPublishedEventHandler(mqttClient_MqttMsgPublished);
            mqttClient.MqttMsgPublishReceived += new MqttClient.MqttMsgPublishEventHandler(mqttClient_MqttMsgPublishReceived);
        }

        public void Connect(string client)
        {   // connect with broker
            mqttClient.Connect(client);
            // set up subscribers
            mqttClient.Subscribe(new string[] { MQTT_ELAPSEDTIME_TOPIC }, new byte[] { MqttMsgBase.QOS_LEVEL_AT_LEAST_ONCE });
        }

        void mqttClient_MqttMsgPublishReceived(object sender, MqttMsgPublishEventArgs e)
        {
            switch (e.Topic)
            {
                case MQTT_ELAPSEDTIME_TOPIC:
                    ElapsedTimeEvent(this, new ElapsedTimeEventArgs((TimeSpan)DeserializeFromByteArray(e.Message, typeof(TimeSpan))));
                    break;
                default:
                    break;
            }
        }

        void mqttClient_MqttMsgPublished(object sender, MqttMsgPublishedEventArgs e)
        {
//            Console.WriteLine(true, "Message published [" + e.MessageId + "]");
        }

        #region Subscribers

        public class ElapsedTimeEventArgs : EventArgs
        {
            public ElapsedTimeEventArgs(TimeSpan time)
            {
                this.Time = time;
            }

            public TimeSpan Time
            {   get; private set; }
        }


        public delegate void ElapsedTimeEventHandler(object sender, ElapsedTimeEventArgs a);

        public event EventHandler<ElapsedTimeEventArgs> ElapsedTimeEvent;

        #endregion

        #region Publishers

        public void publish_led1(LedState ledParameter)
        {
            // SerializeToByteArray can't marshal enum types so cast to a know layout type
            mqttClient.Publish(MQTT_LED1_TOPIC, SerializeToByteArray((int)ledParameter), MqttMsgBase.QOS_LEVEL_AT_LEAST_ONCE, true);
        }

        public void publish_led2(LedState ledParameter)
        {
            // SerializeToByteArray can't marshal enum types so cast to a know layout type
            mqttClient.Publish(MQTT_LED2_TOPIC, SerializeToByteArray((int)ledParameter), MqttMsgBase.QOS_LEVEL_AT_LEAST_ONCE, true);
        }

        public void publish_led3(LedState ledParameter)
        {
            // SerializeToByteArray can't marshal enum types so cast to a know layout type
            mqttClient.Publish(MQTT_LED3_TOPIC, SerializeToByteArray((int)ledParameter), MqttMsgBase.QOS_LEVEL_AT_LEAST_ONCE, true);
        }

        public void publish_resetElapsedTime()
        {
            mqttClient.Publish(MQTT_RESET_TIME_TOPIC, null, MqttMsgBase.QOS_LEVEL_AT_LEAST_ONCE, true);
        }

        #endregion

        #region helpers

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
