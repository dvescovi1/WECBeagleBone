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
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace uPLibrary.Networking.M2Mqtt.Managers
{
    /// <summary>
    /// Manager for client session
    /// </summary>
    public class MqttSessionManager
    {
        // subscription info for each client
        private Dictionary<string, List<MqttSubscription>> sessions;

        /// <summary>
        /// Constructor
        /// </summary>
        public MqttSessionManager()
        {
            this.sessions = new Dictionary<string, List<MqttSubscription>>();
        }

        /// <summary>
        /// Save session for a client (all related subscriptions)
        /// </summary>
        /// <param name="clientId">Client Id to save subscriptions</param>
        /// <param name="subscriptions">Subscriptions to save</param>
        public void SaveSession(string clientId, List<MqttSubscription> subscriptions)
        {
            // create session with all related subscriptions
            List<MqttSubscription> session = new List<MqttSubscription>(subscriptions.Count);
            foreach (MqttSubscription subscription in subscriptions)
            {
                session.Add(new MqttSubscription(subscription.ClientId, subscription.Topic, subscription.QosLevel, null));
            }

            if (!this.sessions.ContainsKey(clientId))
            {
                this.sessions.Add(clientId, session);
            }
            else
            {
                this.sessions[clientId] = session;
            }
        }

        /// <summary>
        /// Get session for a client (all related subscriptions)
        /// </summary>
        /// <param name="clientId">Client Id to get subscriptions</param>
        /// <returns>Subscriptions for the client</returns>
        public List<MqttSubscription> GetSession(string clientId)
        {
            if (!this.sessions.ContainsKey(clientId))
                return null;
            else
                return this.sessions[clientId];
        }

        /// <summary>
        /// Clear session for a client (all related subscriptions)
        /// </summary>
        /// <param name="clientId">Client Id to clear session</param>
        public void ClearSession(string clientId)
        {
            if (this.sessions.ContainsKey(clientId))
            {
                // dispose every subscription
                foreach (MqttSubscription subscription in this.sessions[clientId])
                    subscription.Dispose();

                this.sessions.Remove(clientId);
            }
        }
    }
}
