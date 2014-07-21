using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.Runtime.InteropServices;
using uPLibrary.Networking.M2Mqtt;
using uPLibrary.Networking.M2Mqtt.Messages;
using OpenNETCF.WindowsCE.Notification;

namespace M2MQTT_test
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
            mqttClient = new MqttClient(Program.appSettings.Get("Broker HostName"));
//            mqttClient = new MqttClient(IPAddress.Parse(Program.appSettings.Get("Broker Address")));
            mqttClient.MqttMsgPublished += new MqttClient.MqttMsgPublishedEventHandler(mqttClient_MqttMsgPublished);
            mqttClient.MqttMsgPublishReceived += new MqttClient.MqttMsgPublishEventHandler(mqttClient_MqttMsgPublishReceived);
        }

        public void Connect(string client)
        {   // connect with broker
            mqttClient.Connect(client);
            // set up subscribers
            mqttClient.Subscribe(new string[] { MQTT_LED1_TOPIC }, new byte[] { MqttMsgBase.QOS_LEVEL_AT_LEAST_ONCE });
            mqttClient.Subscribe(new string[] { MQTT_LED2_TOPIC }, new byte[] { MqttMsgBase.QOS_LEVEL_AT_LEAST_ONCE });
            mqttClient.Subscribe(new string[] { MQTT_LED3_TOPIC }, new byte[] { MqttMsgBase.QOS_LEVEL_AT_LEAST_ONCE });
            mqttClient.Subscribe(new string[] { MQTT_RESET_TIME_TOPIC }, new byte[] { MqttMsgBase.QOS_LEVEL_AT_LEAST_ONCE });
        }

        void mqttClient_MqttMsgPublishReceived(object sender, MqttMsgPublishEventArgs e)
        {
            switch (e.Topic)
            {
                case MQTT_LED1_TOPIC:
                    Led1Event(this, new LedEventArgs((Led.LedState)DeserializeFromByteArray(e.Message, typeof(Led.LedState))));
                    break;
                case MQTT_LED2_TOPIC:
                    Led2Event(this, new LedEventArgs((Led.LedState)DeserializeFromByteArray(e.Message, typeof(Led.LedState))));
                    break;
                case MQTT_LED3_TOPIC:
                    Led3Event(this, new LedEventArgs((Led.LedState)DeserializeFromByteArray(e.Message, typeof(Led.LedState))));
                    break;
                case MQTT_RESET_TIME_TOPIC:
                    ResetElapsedTimeEvent(this, new EventArgs());
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

        public class LedEventArgs : EventArgs
        {
            public LedEventArgs(Led.LedState ledState)
            {
                Led = ledState;
            }

            public Led.LedState Led
            { get; private set; }
        }


        public delegate void Led1EventHandler(object sender, LedEventArgs a);
        public delegate void Led2EventHandler(object sender, LedEventArgs a);
        public delegate void Led3EventHandler(object sender, LedEventArgs a);
        public delegate void ResetElapsedTimeEventHandler(object sender, EventArgs a);

        public event EventHandler<LedEventArgs> Led1Event;
        public event EventHandler<LedEventArgs> Led2Event;
        public event EventHandler<LedEventArgs> Led3Event;
        public event EventHandler<EventArgs> ResetElapsedTimeEvent;

        #endregion

        #region Publishers

        public void publish_elapsedTime(TimeSpan elapsedTimeParameter)
        {
            mqttClient.Publish(MQTT_ELAPSEDTIME_TOPIC, SerializeToByteArray(elapsedTimeParameter), MqttMsgBase.QOS_LEVEL_AT_LEAST_ONCE, true);
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
