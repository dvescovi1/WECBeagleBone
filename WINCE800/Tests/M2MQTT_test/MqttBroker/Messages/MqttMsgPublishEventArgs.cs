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
    /// Event Args class for PUBLISH message received from broker
    /// </summary>
    public class MqttMsgPublishEventArgs : EventArgs
    {
        #region Properties...

        /// <summary>
        /// Message topic
        /// </summary>
        public string Topic
        {
            get { return this.topic; }
            internal set { this.topic = value; }
        }

        /// <summary>
        /// Message data
        /// </summary>
        public byte[] Message
        {
            get { return this.message; }
            internal set { this.message = value; }
        }

        /// <summary>
        /// Duplicate message flag
        /// </summary>
        public bool DupFlag
        {
            get { return this.dupFlag; }
            set { this.dupFlag = value; }
        }

        /// <summary>
        /// Quality of Service level
        /// </summary>
        public byte QosLevel
        {
            get { return this.qosLevel; }
            internal set { this.qosLevel = value; }
        }

        /// <summary>
        /// Retain message flag
        /// </summary>
        public bool Retain
        {
            get { return this.retain; }
            internal set { this.retain = value; }
        }

        #endregion

        // message topic
        private string topic;
        // message data
        private byte[] message;
        // duplicate delivery
        private bool dupFlag;
        // quality of service level
        private byte qosLevel;
        // retain flag
        private bool retain;       

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="topic">Message topic</param>
        /// <param name="message">Message data</param>
        /// <param name="dupFlag">Duplicate delivery flag</param>
        /// <param name="qosLevel">Quality of Service level</param>
        /// <param name="retain">Retain flag</param>
        public MqttMsgPublishEventArgs(string topic,
            byte[] message,
            bool dupFlag,
            byte qosLevel,
            bool retain)
        {
            this.topic = topic;
            this.message = message;
            this.dupFlag = dupFlag;
            this.qosLevel = qosLevel;
            this.retain = retain;
        }
    }
}
