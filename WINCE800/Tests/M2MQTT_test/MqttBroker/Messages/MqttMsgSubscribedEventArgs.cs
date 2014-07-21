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

#if (!MF_FRAMEWORK_VERSION_V4_2 && !MF_FRAMEWORK_VERSION_V4_3)
using System;
#else
using Microsoft.SPOT;
#endif

namespace uPLibrary.Networking.M2Mqtt.Messages
{
    /// <summary>
    /// Event Args class for subscribed topics
    /// </summary>
    public class MqttMsgSubscribedEventArgs : EventArgs
    {
        #region Properties...

        /// <summary>
        /// Message identifier
        /// </summary>
        public ushort MessageId
        {
            get { return this.messageId; }
            internal set { this.messageId = value; }
        }

        /// <summary>
        /// List of granted QOS Levels
        /// </summary>
        public byte[] GrantedQoSLevels
        {
            get { return this.grantedQosLevels; }
            internal set { this.grantedQosLevels = value; }
        }

        #endregion

        // message identifier
        ushort messageId;
        // granted QOS levels
        byte[] grantedQosLevels;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="messageId">Message identifier for subscribed topics</param>
        /// <param name="grantedQosLevels">List of granted QOS Levels</param>
        public MqttMsgSubscribedEventArgs(ushort messageId, byte[] grantedQosLevels)
        {
            this.messageId = messageId;
            this.grantedQosLevels = grantedQosLevels;
        }
    }
}
