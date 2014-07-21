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
    /// Class for CONNACK message from broker to client
    /// </summary>
    public class MqttMsgConnack : MqttMsgBase
    {
        #region Constants...

        // return codes for CONNACK message
        public const byte CONN_ACCEPTED = 0x00;
        public const byte CONN_REFUSED_PROT_VERS = 0x01;
        public const byte CONN_REFUSED_IDENT_REJECTED = 0x02;
        public const byte CONN_REFUSED_SERVER_UNAVAILABLE = 0x03;
        public const byte CONN_REFUSED_USERNAME_PASSWORD = 0x04;
        public const byte CONN_REFUSED_NOT_AUTHORIZED = 0x05;

        private const byte TOPIC_NAME_COMP_RESP_BYTE_OFFSET = 0;
        private const byte TOPIC_NAME_COMP_RESP_BYTE_SIZE = 1;
        private const byte CONN_RETURN_CODE_BYTE_OFFSET = 1;
        private const byte CONN_RETURN_CODE_BYTE_SIZE = 1;

        #endregion

        #region Properties...

        /// <summary>
        /// Return Code
        /// </summary>
        public byte ReturnCode
        {
            get { return this.returnCode; }
            set { this.returnCode = value; }
        }

        #endregion

        // return code for CONNACK message
        private byte returnCode;

        /// <summary>
        /// Constructor
        /// </summary>
        public MqttMsgConnack()
        {
            this.type = MQTT_MSG_CONNACK_TYPE;
        }

        /// <summary>
        /// Parse bytes for a CONNACK message
        /// </summary>
        /// <param name="fixedHeaderFirstByte">First fixed header byte</param>
        /// <param name="channel">Channel connected to the broker</param>
        /// <returns>CONNACK message instance</returns>
        public static MqttMsgConnack Parse(byte fixedHeaderFirstByte, IMqttNetworkChannel channel)
        {
            byte[] buffer;
            MqttMsgConnack msg = new MqttMsgConnack();

            // get remaining length and allocate buffer
            int remainingLength = MqttMsgBase.decodeRemainingLength(channel);
            buffer = new byte[remainingLength];

            // read bytes from socket...
            channel.Receive(buffer);
            // ...and set return code from broker
            msg.returnCode = buffer[CONN_RETURN_CODE_BYTE_OFFSET];

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

            // topic name compression response and connect return code
            varHeaderSize += (TOPIC_NAME_COMP_RESP_BYTE_SIZE + CONN_RETURN_CODE_BYTE_SIZE);

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
            buffer[index] = (byte)(MQTT_MSG_CONNACK_TYPE << MSG_TYPE_OFFSET);
            index++;

            // encode remaining length
            index = this.encodeRemainingLength(remainingLength, buffer, index);

            // topic name compression response (reserved values. not used);
            buffer[index++] = 0x00;
            
            // connect return code
            buffer[index++] = this.returnCode;

            return buffer;
        }

        public override string ToString()
        {
#if TRACE
            return this.GetTraceString(
                "CONNACK",
                new object[] { "returnCode" },
                new object[] { this.returnCode });
#else
            return base.ToString();
#endif
        }
    }
}
