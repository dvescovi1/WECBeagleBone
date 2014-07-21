/*
M2Mqtt Project - MQTT Client Library for .Net and GnatMQ MQTT Broker for .NET
Copyright (c) 2014, Paolo Patierno, All rights reserved.

Licensed under the Apache License, Version 2.0 (the ""License""); you may not use this 
file except in compliance with the License. You may obtain a copy of the License at 
http://www.apache.org/licenses/LICENSE-2.0

THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, 
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR 
CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR 
NON-INFRINGEMENT.

See the Apache Version 2.0 License for specific language governing permissions and 
limitations under the License.
*/

namespace uPLibrary.Networking.M2Mqtt
{
    /// <summary>
    /// Settings class for the MQTT broker
    /// </summary>
    public class MqttSettings
    {
        // default port for MQTT protocol
        public const int MQTT_BROKER_DEFAULT_PORT = 1883;
        public const int MQTT_BROKER_DEFAULT_SSL_PORT = 8883;
        // default timeout on receiving from client
        public const int MQTT_DEFAULT_TIMEOUT = 5000;
        // max publish, subscribe and unsubscribe retry for QoS Level 1 or 2
        public const int MQTT_ATTEMPTS_RETRY = 3;
        // delay for retry publish, subscribe and unsubscribe for QoS Level 1 or 2
        public const int MQTT_DELAY_RETRY = 10000;
        // broker need to receive the first message (CONNECT)
        // within a reasonable amount of time after TCP/IP connection 
        public const int MQTT_CONNECT_TIMEOUT = 5000;

        /// <summary>
        /// Listening connection port
        /// </summary>
        public int Port { get; internal set; }

        /// <summary>
        /// Listening connection SSL port
        /// </summary>
        public int SslPort { get; internal set; }

        /// <summary>
        /// Timeout on client connection (before receiving CONNECT message)
        /// </summary>
        public int TimeoutOnConnection { get; internal set; }

        /// <summary>
        /// Timeout on receiving
        /// </summary>
        public int TimeoutOnReceiving { get; internal set; }

        /// <summary>
        /// Attempts on retry
        /// </summary>
        public int AttemptsOnRetry { get; internal set; }

        /// <summary>
        /// Delay on retry
        /// </summary>
        public int DelayOnRetry { get; internal set; }
        
        /// <summary>
        /// Singleton instance of settings
        /// </summary>
        public static MqttSettings Instance
        {
            get
            {
                if (instance == null)
                    instance = new MqttSettings();
                return instance;
            }
        }

        // singleton instance
        private static MqttSettings instance;

        /// <summary>
        /// Constructor
        /// </summary>
        private MqttSettings()
        {
            this.Port = MQTT_BROKER_DEFAULT_PORT;
            this.SslPort = MQTT_BROKER_DEFAULT_SSL_PORT;
            this.TimeoutOnReceiving = MQTT_DEFAULT_TIMEOUT;
            this.AttemptsOnRetry = MQTT_ATTEMPTS_RETRY;
            this.DelayOnRetry = MQTT_DELAY_RETRY;
            this.TimeoutOnConnection = MQTT_CONNECT_TIMEOUT;
        }
    }
}
