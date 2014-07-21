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
using System.Net;
using System.Net.Sockets;
using System.Threading;

namespace uPLibrary.Networking.M2Mqtt.Communication
{
    /// <summary>
    /// MQTT communication layer
    /// </summary>
    public class MqttTcpCommunicationLayer : IMqttCommunicationLayer
    {
        #region Constants ...

        // name for listener thread
        private const string LISTENER_THREAD_NAME = "MqttListenerThread";

        #endregion

        #region Properties ...

        /// <summary>
        /// TCP listening port
        /// </summary>
        public int Port { get; private set; }

        #endregion

        // TCP listener for incoming connection requests
        private TcpListener listener;

        // TCP listener thread
        private Thread thread;
        private bool isRunning;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="port">TCP listening port</param>
        public MqttTcpCommunicationLayer(int port)
        {
            this.Port = port;
        }

        #region IMqttCommunicationLayer ...

        // client connected event
        public event MqttClientConnectedEventHandler ClientConnected;

        /// <summary>
        /// Start communication layer listening
        /// </summary>
        public void Start()
        {
            this.isRunning = true;

            // create and start listener thread
            this.thread = new Thread(this.ListenerThread);
            this.thread.Name = LISTENER_THREAD_NAME;
            this.thread.Start();
        }

        /// <summary>
        /// Stop communication layer listening
        /// </summary>
        public void Stop()
        {
            this.isRunning = false;

            this.listener.Stop();

            // wait for thread
            this.thread.Join();
        }

        #endregion

        /// <summary>
        /// Listener thread for incoming connection requests
        /// </summary>
        private void ListenerThread()
        {
            // create listener...
//            this.listener = new TcpListener(IPAddress.Any, this.Port);
            this.listener = new TcpListener(IPAddress.IPv6Any, this.Port); // IPV6 binding
            this.listener.Server.SetSocketOption(SocketOptionLevel.IPv6, (SocketOptionName)27, false); // allow IPV4 and IPV6 connection
            // ...and start it
            this.listener.Start();

            while (this.isRunning)
            {
                try
                {
                    // blocking call to wait for client connection
                    Socket socketClient = this.listener.AcceptSocket();

                    // manage socket client connected
                    if (socketClient.Connected)
                    {
                        MqttClient client = new MqttClient(socketClient);
                        // raise client raw connection event
                        this.OnClientConnected(client);
                    }
                }
                catch (Exception)
                {
                    if (!this.isRunning)
                        return;
                }
            }
        }

        /// <summary>
        /// Raise client connected event
        /// </summary>
        /// <param name="e">Event args</param>
        private void OnClientConnected(MqttClient client)
        {
            if (this.ClientConnected != null)
                this.ClientConnected(this, new MqttClientConnectedEventArgs(client));
        }
    }
}
