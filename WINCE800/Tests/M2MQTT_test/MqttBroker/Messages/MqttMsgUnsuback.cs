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
    /// Class for UNSUBACK message from broker to client
    /// </summary>
    public class MqttMsgUnsuback : MqttMsgBase
    {
        #region Properties...

        /// <summary>
        /// Message identifier for the unsubscribe message
        /// that is acknowledged
        /// </summary>
        public ushort MessageId
        {
            get { return this.messageId; }
            set { this.messageId = value; }
        }

        #endregion

        // message identifier
        private ushort messageId;
        
        /// <summary>
        /// Constructor
        /// </summary>
        public MqttMsgUnsuback()
        {
            this.type = MQTT_MSG_UNSUBACK_TYPE;
        }

        /// <summary>
        /// Parse bytes for a UNSUBACK message
        /// </summary>
        /// <param name="fixedHeaderFirstByte">First fixed header byte</param>
        /// <param name="channel">Channel connected to the broker</param>
        /// <returns>UNSUBACK message instance</returns>
        public static MqttMsgUnsuback Parse(byte fixedHeaderFirstByte, IMqttNetworkChannel channel)
        {
            byte[] buffer;
            int index = 0;
            MqttMsgUnsuback msg = new MqttMsgUnsuback();

            // get remaining length and allocate buffer
            int remainingLength = MqttMsgBase.decodeRemainingLength(channel);
            buffer = new byte[remainingLength];

            // read bytes from socket...
            channel.Receive(buffer);

            // message id
            msg.messageId = (ushort)((buffer[index++] << 8) & 0xFF00);
            msg.messageId |= (buffer[index++]);

            return msg;
        }

        public override byte[] GetBytes()
        {
            int fixedHeaderSize = 0;
            int varHeaderSize = 0;
            int payloadSize = 0;
            int remainingLength = 0;
            byte[] buffer;
            int index = 0;

            // message identifier
            varHeaderSize += MESSAGE_ID_SIZE;

            remainingLength += (varHeaderSize + payloadSize);

            // first byte of fixed header
            fixedHeaderSize = 1;

            int temp = remainingLength;
            // increase fixed header size based on remaining length
            // (each remaining length byte can encode until 128)
            do
            {
                fixedHeaderSize++;
                temp = temp / 128;
            } while (temp > 0);

            // allocate buffer for message
            buffer = new byte[fixedHeaderSize + varHeaderSize + payloadSize];

            // first fixed header byte
            buffer[index] = (byte)(MQTT_MSG_UNSUBACK_TYPE << MSG_TYPE_OFFSET);
            index++;

            // encode remaining length
            index = this.encodeRemainingLength(remainingLength, buffer, index);

            // message id
            buffer[index++] = (byte)((this.messageId >> 8) & 0x00FF); // MSB
            buffer[index++] = (byte)(this.messageId & 0x00FF); // LSB

            return buffer;
        }

        public override string ToString()
        {
#if TRACE
            return this.GetTraceString(
                "UNSUBACK",
                new object[] { "messageId" },
                new object[] { this.messageId });
#else
            return base.ToString();
#endif
        }
    }
}
