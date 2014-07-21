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
using uPLibrary.Networking.M2Mqtt.Messages;
using uPLibrary.Networking.M2Mqtt.Exceptions;
using uPLibrary.Networking.M2Mqtt.Managers;
using uPLibrary.Networking.M2Mqtt.Communication;

namespace uPLibrary.Networking.M2Mqtt
{
    /// <summary>
    /// MQTT broker business logic
    /// </summary>
    public class MqttBroker
    {
        // MQTT broker settings
        private MqttSettings settings;

        // clients connected list
        private MqttClientCollection clients;

        // reference to publisher manager
        private MqttPublisherManager publisherManager;
        
        // reference to subscriber manager
        private MqttSubscriberManager subscriberManager;

        // reference to session manager
        private MqttSessionManager sessionManager;

        // reference to User Access Control manager
        private MqttUacManager uacManager;

        // MQTT communication layer
        private IMqttCommunicationLayer commLayer;

        /// <summary>
        /// User authentication method
        /// </summary>
        public MqttUserAuthenticationDelegate UserAuth
        {
            get { return this.uacManager.UserAuth; }
            set { this.uacManager.UserAuth = value; }
        }

        /// <summary>
        /// Constructor (TCP/IP communication layer and default settings)
        /// </summary>
        public MqttBroker()
            : this(new MqttTcpCommunicationLayer(MqttSettings.Instance.Port), MqttSettings.Instance)
        {
        }
        
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="commLayer">Communication layer to use (TCP)</param>
        /// <param name="settings">Broker settings</param>
        public MqttBroker(IMqttCommunicationLayer commLayer, MqttSettings settings)
        {
            // MQTT broker settings
            this.settings = settings;

            // MQTT communication layer
            this.commLayer = commLayer;
            this.commLayer.ClientConnected += commLayer_ClientConnected;           

            // create managers (publisher, subscriber, session and UAC)
            this.subscriberManager = new MqttSubscriberManager();
            this.publisherManager = new MqttPublisherManager(this.subscriberManager);
            this.sessionManager = new MqttSessionManager();
            this.uacManager = new MqttUacManager();

            this.clients = new MqttClientCollection();
        }

        /// <summary>
        /// Start broker
        /// </summary>
        public void Start()
        {
            this.commLayer.Start();
            this.publisherManager.Start();
        }

        /// <summary>
        /// Stop broker
        /// </summary>
        public void Stop()
        {
            this.commLayer.Stop();
            this.publisherManager.Stop();

            // close connection with all clients
            foreach (MqttClient client in this.clients)
            {
                client.Close();
            }
        }

        /// <summary>
        /// Close a client
        /// </summary>
        /// <param name="client">Client to close</param>
        private void CloseClient(MqttClient client)
        {
            // if client is connected and it has a will message
            if (client.IsConnected && client.WillFlag)
            {
                // create the will PUBLISH message
                MqttMsgPublish publish =
                    new MqttMsgPublish(client.WillTopic, Encoding.UTF8.GetBytes(client.WillMessage), false, client.WillQosLevel, false);

                // publish message through publisher manager
                this.publisherManager.Publish(publish);
            }

            // if not clean session
            if (!client.CleanSession)
            {
                List<MqttSubscription> subscriptions = this.subscriberManager.GetSubscriptionsByClient(client.ClientId);

                if ((subscriptions != null) && (subscriptions.Count > 0))
                {
                    this.sessionManager.SaveSession(client.ClientId, subscriptions);

                    // TODO : persist client session if broker close
                }
            }

            // delete client from runtime subscription
            this.subscriberManager.Unsubscribe(client);

            // close the client
            client.Close();

            // remove client from the collection
            this.clients.Remove(client);
        }

        void commLayer_ClientConnected(object sender, MqttClientConnectedEventArgs e)
        {
            // register event handlers from client
            e.Client.MqttMsgDisconnected += Client_MqttMsgDisconnected;
            e.Client.MqttMsgPublishReceived += Client_MqttMsgPublishReceived;
            e.Client.MqttMsgConnected += Client_MqttMsgConnected;
            e.Client.MqttMsgSubscribeReceived += Client_MqttMsgSubscribeReceived;
            e.Client.MqttMsgUnsubscribeReceived += Client_MqttMsgUnsubscribeReceived;

            // add client to the collection
            this.clients.Add(e.Client);

            // start client threads
            e.Client.Open();
        }

        void Client_MqttMsgPublishReceived(object sender, MqttMsgPublishEventArgs e)
        {
            MqttClient client = (MqttClient)sender;

            // create PUBLISH message to publish
            MqttMsgPublish publish = new MqttMsgPublish(e.Topic, e.Message, e.DupFlag, e.QosLevel, e.Retain);
            
            // publish message through publisher manager
            this.publisherManager.Publish(publish);
        }

        void Client_MqttMsgUnsubscribeReceived(object sender, MqttMsgUnsubscribeEventArgs e)
        {
            MqttClient client = (MqttClient)sender;

            for (int i = 0; i < e.Topics.Length; i++)
            {
                // unsubscribe client for each topic requested
                this.subscriberManager.Unsubscribe(e.Topics[i], client);
            }

            try
            {
                // send UNSUBACK message to the client
                client.Unsuback(e.MessageId);
            }
            catch (MqttCommunicationException)
            {
                this.CloseClient(client);
            }
        }

        void Client_MqttMsgSubscribeReceived(object sender, MqttMsgSubscribeEventArgs e)
        {
            MqttClient client = (MqttClient)sender;

            for (int i = 0; i < e.Topics.Length; i++)
            {
                // TODO : business logic to grant QoS levels based on some conditions ?
                //        now the broker granted the QoS levels requested by client

                // subscribe client for each topic and QoS level requested
                this.subscriberManager.Subscribe(e.Topics[i], e.QoSLevels[i], client);
            }

            try
            {
                // send SUBACK message to the client
                client.Suback(e.MessageId, e.QoSLevels);

                for (int i = 0; i < e.Topics.Length; i++)
                {
                    // publish retained message on the current subscription
                    this.publisherManager.PublishRetaind(e.Topics[i], client.ClientId);
                }
            }
            catch (MqttCommunicationException)
            {
                this.CloseClient(client);
            }
        }

        void Client_MqttMsgConnected(object sender, MqttMsgConnectEventArgs e)
        {
            MqttClient client = (MqttClient)sender;

            // verify message to determine CONNACK message return code to the client
            byte returnCode = this.MqttConnectVerify(e.Message);

            // connection "could" be accepted
            if (returnCode == MqttMsgConnack.CONN_ACCEPTED)
            {
                // check if there is a client already connected with same client Id
                MqttClient clientConnected = this.GetClient(e.Message.ClientId);

                // force connection close to the existing client (MQTT protocol)
                if (clientConnected != null)
                {
                    this.CloseClient(clientConnected);
                }
            }

            try
            {
                // send CONNACK message to the client
                client.Connack(returnCode, e.Message);

                // connection accepted, load (if exists) client session
                if (returnCode == MqttMsgConnack.CONN_ACCEPTED)
                {
                    // get all subscriptions for the connected client
                    List<MqttSubscription> subscriptions = this.sessionManager.GetSession(client.ClientId);

                    if (subscriptions != null)
                    {
                        // register all subscriptions for the connected client
                        foreach (MqttSubscription subscription in subscriptions)
                        {
                            this.subscriberManager.Subscribe(subscription.Topic, subscription.QosLevel, client);

                            // publish retained message on the current subscription
                            this.publisherManager.PublishRetaind(subscription.Topic, client.ClientId);
                        }
                        this.sessionManager.ClearSession(client.ClientId);
                    }
                }
            }
            catch (MqttCommunicationException)
            {
                this.CloseClient(client);
            }
        }

        void Client_MqttMsgDisconnected(object sender, EventArgs e)
        {
            MqttClient client = (MqttClient)sender;

            // close the client
            this.CloseClient(client);
        }

        /// <summary>
        /// Check CONNECT message to accept or not the connection request 
        /// </summary>
        /// <param name="connect">CONNECT message received from client</param>
        /// <returns>Return code for CONNACK message</returns>
        private byte MqttConnectVerify(MqttMsgConnect connect)
        {
            byte returnCode = MqttMsgConnack.CONN_ACCEPTED;

            // unacceptable protocol version
            if (connect.ProtocolVersion != MqttMsgConnect.PROTOCOL_VERSION)
                returnCode = MqttMsgConnack.CONN_REFUSED_PROT_VERS;
            else
            {
                // client id length exceeded
                if (connect.ClientId.Length > MqttMsgConnect.CLIENT_ID_MAX_LENGTH)
                    returnCode = MqttMsgConnack.CONN_REFUSED_IDENT_REJECTED;
                else
                {
                    // check user authentication
                    if (!this.uacManager.UserAuthentication(connect.Username, connect.Password))
                        returnCode = MqttMsgConnack.CONN_REFUSED_USERNAME_PASSWORD;
                    // server unavailable and not authorized ?
                    else
                    {
                        // TODO : other checks on CONNECT message
                    }
                }
            }

            return returnCode;
        }

        /// <summary>
        /// Return reference to a client with a specified Id is already connected
        /// </summary>
        /// <param name="clientId">Client Id to verify</param>
        /// <returns>Reference to client</returns>
        private MqttClient GetClient(string clientId)
        {
            var query = from c in this.clients
                        where c.ClientId == clientId
                        select c;

            return query.FirstOrDefault();
        }
    }
}
