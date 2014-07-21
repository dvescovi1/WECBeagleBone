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
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace uPLibrary.Networking.M2Mqtt
{
    /// <summary>
    /// MQTT client collection
    /// </summary>
    public class MqttClientCollection : IList<MqttClient>, IEnumerable
    {
        // clients list
        private List<MqttClient> clients;

        public MqttClientCollection()
        {
            this.clients = new List<MqttClient>();
        }
                
        #region IEnumerable ...

        public IEnumerator GetEnumerator()
        {
            return this.clients.GetEnumerator();
        }

        #endregion

        #region IList<MqttClient> ...

        public int IndexOf(MqttClient item)
        {
            return this.clients.IndexOf(item);
        }

        public void Insert(int index, MqttClient item)
        {
            lock (this.clients)
            {
                this.clients.Insert(index, item);
            }
        }

        public void RemoveAt(int index)
        {
            lock (this.clients)
            {
                this.clients.RemoveAt(index);
            }
        }

        public MqttClient this[int index]
        {
            get { return this.clients[index]; }
            set { this.clients[index] = value; }
        }

        public void Add(MqttClient item)
        {
            lock (this.clients)
            {
                this.clients.Add(item);
            }
        }

        public void Clear()
        {
            this.clients.Clear();
        }

        public bool Contains(MqttClient item)
        {
            return this.clients.Contains(item);
        }

        public void CopyTo(MqttClient[] array, int arrayIndex)
        {
            this.clients.CopyTo(array, arrayIndex);
        }

        public int Count
        {
            get { return this.clients.Count; }
        }

        public bool IsReadOnly
        {
            get { return false; }
        }

        public bool Remove(MqttClient item)
        {
            lock (this.clients)
            {
                return this.clients.Remove(item);
            }
        }

        IEnumerator<MqttClient> IEnumerable<MqttClient>.GetEnumerator()
        {
            return this.clients.GetEnumerator();
        }

        #endregion
    }
}
