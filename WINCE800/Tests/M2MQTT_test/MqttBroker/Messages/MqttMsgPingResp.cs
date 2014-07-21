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

using System;

namespace uPLibrary.Networking.M2Mqtt.Messages
{
    /// <summary>
    /// Class for PINGRESP message from client to broker
    /// </summary>
    public class MqttMsgPingResp : MqttMsgBase
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MqttMsgPingResp()
        {
            this.type = MQTT_MSG_PINGRESP_TYPE;
        }

        /// <summary>
        /// Parse bytes for a PINGRESP message
        /// </summary>
        /// <param name="fixedHeaderFirstByte">First fixed header byte</param>
        /// <param name="channel">Channel connected to the broker</param>
        /// <returns>PINGRESP message instance</returns>
        public static MqttMsgPingResp Parse(byte fixedHeaderFirstByte, IMqttNetworkChannel channel)
        {
            MqttMsgPingResp msg = new MqttMsgPingResp();

            // already know remaininglength is zero (MQTT specification),
            // so it isn't necessary to read other data from socket
            int remainingLength = MqttMsgBase.decodeRemainingLength(channel);
            
            return msg;
        }

        public override byte[] GetBytes()
        {
            byte[] buffer = new byte[2];
            int index = 0;

            // first fixed header byte
            buffer[index++] = (MQTT_MSG_PINGRESP_TYPE << MSG_TYPE_OFFSET);
            buffer[index++] = 0x00;

            return buffer;
        }

        public override string ToString()
        {
#if TRACE
            return this.GetTraceString(
                "PINGRESP",
                null,
                null);
#else
            return base.ToString();
#endif
        }
    }
}
