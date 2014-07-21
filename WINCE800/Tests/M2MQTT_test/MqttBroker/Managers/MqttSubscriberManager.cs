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
using System.Text.RegularExpressions;

namespace uPLibrary.Networking.M2Mqtt.Managers
{
    /// <summary>
    /// Manager for topics and subscribers
    /// </summary>
    public class MqttSubscriberManager
    {
        #region Constants ...

        // topic wildcards '+' and '#'
        private const string PLUS_WILDCARD = "+";
        private const string SHARP_WILDCARD = "#";

        // replace for wildcards '+' and '#' for using regular expression on topic match
        private const string PLUS_WILDCARD_REPLACE = @"[^/]+";
        private const string SHARP_WILDCARD_REPLACE = @".*";

        #endregion

        // subscribers list for each topic
        private Dictionary<string, List<MqttSubscription>> subscribers;

        /// <summary>
        /// Constructor
        /// </summary>
        public MqttSubscriberManager()
        {
            this.subscribers = new Dictionary<string, List<MqttSubscription>>();
        }

        /// <summary>
        /// Add a subscriber for a topic
        /// </summary>
        /// <param name="topic">Topic for subscription</param>
        /// <param name="qosLevel">QoS level for the topic subscription</param>
        /// <param name="client">Client to subscribe</param>
        public void Subscribe(string topic, byte qosLevel, MqttClient client)
        {
            string topicReplaced = topic.Replace(PLUS_WILDCARD, PLUS_WILDCARD_REPLACE).Replace(SHARP_WILDCARD, SHARP_WILDCARD_REPLACE);

            lock (this.subscribers)
            {
                // if the topic doesn't exist
                if (!this.subscribers.ContainsKey(topicReplaced))
                {
                    // create a new empty subscription list for the topic
                    List<MqttSubscription> list = new List<MqttSubscription>();
                    this.subscribers.Add(topicReplaced, list);
                }

                // query for check client already subscribed
                var query = from s in this.subscribers[topicReplaced]
                            where s.ClientId == client.ClientId
                            select s;

                // if the client isn't already subscribed to the topic 
                if (query.Count() == 0)
                {
                    MqttSubscription subscription = new MqttSubscription()
                    {
                        ClientId = client.ClientId,
                        Topic = topicReplaced,
                        QosLevel = qosLevel,
                        Client = client
                    };
                    // add subscription to the list for the topic
                    this.subscribers[topicReplaced].Add(subscription);
                }
            }
        }

        /// <summary>
        /// Remove a subscriber for a topic
        /// </summary>
        /// <param name="topic">Topic for unsubscription</param>
        /// <param name="client">Client to unsubscribe</param>
        public void Unsubscribe(string topic, MqttClient client)
        {
            string topicReplaced = topic.Replace(PLUS_WILDCARD, PLUS_WILDCARD_REPLACE).Replace(SHARP_WILDCARD, SHARP_WILDCARD_REPLACE);

            lock (this.subscribers)
            {
                // if the topic exists
                if (this.subscribers.ContainsKey(topicReplaced))
                {
                    // query for check client subscribed
                    var query = from s in this.subscribers[topicReplaced]
                                where s.ClientId == client.ClientId
                                select s;

                    // if the client is subscribed for the topic
                    if (query.Count() > 0)
                    {
                        MqttSubscription subscription = query.First();
                        
                        // remove subscription from the list for the topic
                        this.subscribers[topicReplaced].Remove(subscription);
                        // dispose subscription
                        subscription.Dispose();

                        // remove topic if there aren't subscribers
                        if (this.subscribers[topicReplaced].Count == 0)
                            this.subscribers.Remove(topicReplaced);
                    }
                }
            }
        }

        /// <summary>
        /// Remove a subscriber for all topics
        /// </summary>
        /// <param name="client">Client to unsubscribe</param>
        public void Unsubscribe(MqttClient client)
        {
            lock (this.subscribers)
            {
                List<string> topicToRemove = new List<string>();

                foreach (string topic in this.subscribers.Keys)
                {
                    // query for check client subscribed
                    var query = from s in this.subscribers[topic]
                                where s.ClientId == client.ClientId
                                select s;

                    // if the client is subscribed for the topic
                    if (query.Count() > 0)
                    {
                        MqttSubscription subscription = query.First();

                        // remove subscription from the list for the topic
                        this.subscribers[topic].Remove(subscription);
                        // dispose subscription
                        subscription.Dispose();

                        // add topic to remove list if there aren't subscribers
                        if (this.subscribers[topic].Count == 0)
                            topicToRemove.Add(topic);
                    }
                }

                // remove topic without subscribers
                // loop needed to avoid exception on modify collection inside previous loop
                foreach (string topic in topicToRemove)
                    this.subscribers.Remove(topic);
            }
        }

        /// <summary>
        /// Get subscription list for a specified topic and QoS Level
        /// </summary>
        /// <param name="topic">Topic to get subscription list</param>
        /// <param name="qosLevel">QoS level requested</param>
        /// <returns>Subscription list</returns>
        public List<MqttSubscription> GetSubscriptions(string topic, byte qosLevel)
        {
            var query = from ss in this.subscribers
                        where (new Regex(ss.Key)).IsMatch(topic)    // check for topics based also on wildcard with regex
                        from s in this.subscribers[ss.Key]
                        where s.QosLevel == qosLevel                // check for subscriber only with a specified QoS level granted
                        select s;

            // use comparer for multiple subscriptions that overlap (e.g. /test/# and  /test/+/foo)
            // If a client is subscribed to multiple subscriptions with topics that overlap
            // it has more entries into subscriptions list but broker sends only one message
            return query.Distinct(new MqttSubscriptionComparer()).ToList();
        }

        /// <summary>
        /// Get a subscription for a specified topic and client
        /// </summary>
        /// <param name="topic">Topic to get subscription</param>
        /// <param name="clientId">Client Id to get subscription</param>
        /// <returns>Subscription list</returns>
        public MqttSubscription GetSubscription(string topic, string clientId)
        {
            var query = from ss in this.subscribers
                        where (new Regex(ss.Key)).IsMatch(topic)    // check for topics based also on wildcard with regex
                        from s in this.subscribers[ss.Key]
                        where s.ClientId == clientId                // check for subscriber only with a specified Client Id
                        select s;

            // use comparer for multiple subscriptions that overlap (e.g. /test/# and  /test/+/foo)
            // If a client is subscribed to multiple subscriptions with topics that overlap
            // it has more entries into subscriptions list but broker sends only one message
            return query.Distinct(new MqttSubscriptionComparer()).First();
        }

        /// <summary>
        /// Get subscription list for a specified topic
        /// </summary>
        /// <param name="topic">Topic to get subscription list</param>
        /// <returns>Subscription list</returns>
        public List<MqttSubscription> GetSubscriptionsByTopic(string topic)
        {
            var query = from ss in this.subscribers
                        where (new Regex(ss.Key)).IsMatch(topic)    // check for topics based also on wildcard with regex
                        from s in this.subscribers[ss.Key]
                        select s;

            // use comparer for multiple subscriptions that overlap (e.g. /test/# and  /test/+/foo)
            // If a client is subscribed to multiple subscriptions with topics that overlap
            // it has more entries into subscriptions list but broker sends only one message
            return query.Distinct(new MqttSubscriptionComparer()).ToList();
        }

        /// <summary>
        /// Get subscription list for a specified client
        /// </summary>
        /// <param name="clientId">Client Id to get subscription list</param>
        /// <returns>Subscription lis</returns>
        public List<MqttSubscription> GetSubscriptionsByClient(string clientId)
        {
            var query = from ss in this.subscribers
                        from s in this.subscribers[ss.Key]
                        where s.ClientId == clientId
                        select s;

            // use comparer for multiple subscriptions that overlap (e.g. /test/# and  /test/+/foo)
            // If a client is subscribed to multiple subscriptions with topics that overlap
            // it has more entries into subscriptions list but broker sends only one message
            return query.Distinct(new MqttSubscriptionComparer()).ToList();
        }
    }

    /// <summary>
    /// MQTT subscription comparer
    /// </summary>
    public class MqttSubscriptionComparer : IEqualityComparer<MqttSubscription>
    {
        public bool Equals(MqttSubscription x, MqttSubscription y)
        {
            return x.ClientId.Equals(y.ClientId);
        }

        public int GetHashCode(MqttSubscription obj)
        {
            return obj.ClientId.GetHashCode();
        }
    }

    /// <summary>
    /// MQTT subscription
    /// </summary>
    public class MqttSubscription
    {
        /// <summary>
        /// Client Id
        /// </summary>
        public string ClientId { get; set; }

        /// <summary>
        /// Topic of subscription
        /// </summary>
        public string Topic { get; set; }

        /// <summary>
        /// QoS level granted for the subscription
        /// </summary>
        public byte QosLevel { get; set; }

        /// <summary>
        /// Client related to the subscription
        /// </summary>
        public MqttClient Client { get; set; }

        /// <summary>
        /// Constructor
        /// </summary>
        public MqttSubscription()
        {
            this.ClientId = null;
            this.Topic = null;
            this.QosLevel = 0;
            this.Client = null;
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="clientId">Client Id of the subscription</param>
        /// <param name="topic">Topic of subscription</param>
        /// <param name="qosLevel">QoS level of subscription</param>
        /// <param name="client">Client related to the subscription</param>
        public MqttSubscription(string clientId, string topic, byte qosLevel, MqttClient client = null)
        {
            this.ClientId = clientId;
            this.Topic = topic;
            this.QosLevel = qosLevel;
            this.Client = client;
        }

        /// <summary>
        /// Dispose subscription
        /// </summary>
        public void Dispose()
        {
            this.ClientId = null;
            this.Topic = null;
            this.QosLevel = 0;
            this.Client = null;
        }
    }
}
