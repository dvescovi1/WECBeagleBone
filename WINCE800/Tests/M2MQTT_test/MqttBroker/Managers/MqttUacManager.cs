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
    /// Delegate for executing user authentication
    /// </summary>
    /// <param name="username">Username</param>
    /// <param name="password">Password</param>
    /// <returns></returns>
    public delegate bool MqttUserAuthenticationDelegate(string username, string password);

    /// <summary>
    /// Manager for User Access Control
    /// </summary>
    public class MqttUacManager
    {
        // user authentication delegate
        private MqttUserAuthenticationDelegate userAuth;

        /// <summary>
        /// User authentication method
        /// </summary>
        public MqttUserAuthenticationDelegate UserAuth
        {
            get { return this.userAuth; }
            set { this.userAuth = value; }
        }

        /// <summary>
        /// Execute user authentication
        /// </summary>
        /// <param name="username">Username</param>
        /// <param name="password">Password</param>
        /// <returns>Access granted or not</returns>
        public bool UserAuthentication(string username, string password)
        {
            if (this.userAuth == null)
                return true;
            else
                return this.userAuth(username, password);
        }
    }
}
