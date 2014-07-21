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


namespace uPLibrary.Networking.M2Mqtt.Messages
{
    /// <summary>
    /// Class for DISCONNECT message from client to broker
    /// </summary>
    public class MqttMsgDisconnect : MqttMsgBase
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MqttMsgDisconnect()
        {
            this.type = MQTT_MSG_DISCONNECT_TYPE;
        }

        /// <summary>
        /// Parse bytes for a DISCONNECT message
        /// </summary>
        /// <param name="fixedHeaderFirstByte">First fixed header byte</param>
        /// <param name="channel">Channel connected to the broker</param>
        /// <returns>DISCONNECT message instance</returns>
        public static MqttMsgDisconnect Parse(byte fixedHeaderFirstByte, IMqttNetworkChannel channel)
        {
            MqttMsgDisconnect msg = new MqttMsgDisconnect();

            // get remaining length and allocate buffer
            int remainingLength = MqttMsgBase.decodeRemainingLength(channel);
            // NOTE : remainingLength must be 0

            return msg;
        }

        public override byte[] GetBytes()
        {
            byte[] buffer = new byte[2];
            int index = 0;

            // first fixed header byte
            buffer[index++] = (MQTT_MSG_DISCONNECT_TYPE << MSG_TYPE_OFFSET);
            buffer[index++] = 0x00;

            return buffer;
        }

        public override string ToString()
        {
#if TRACE
            return this.GetTraceString(
                "DISCONNECT",
                null,
                null);
#else
            return base.ToString();
#endif
        }
    }
}
