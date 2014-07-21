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

namespace uPLibrary.Networking.M2Mqtt.Communication
{
    /// <summary>
    /// MQTT client connected event args
    /// </summary>
    public class MqttClientConnectedEventArgs : EventArgs
    {
        /// <summary>
        /// Connected client
        /// </summary>
        public MqttClient Client { get; private set; }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="client">Connected client</param>
        public MqttClientConnectedEventArgs(MqttClient client)
        {
            this.Client = client;
        }
    }
}
