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
using System.Threading;
using uPLibrary.Networking.M2Mqtt.Messages;

namespace uPLibrary.Networking.M2Mqtt.Managers
{
    /// <summary>
    /// Manager for publishing messages
    /// </summary>
    public class MqttPublisherManager
    {
        #region Constants ...

        // topic wildcards '+' and '#'
        private const string PLUS_WILDCARD = "+";
        private const string SHARP_WILDCARD = "#";

        // replace for wildcards '+' and '#' for using regular expression on topic match
        private const string PLUS_WILDCARD_REPLACE = @"[^/]+";
        private const string SHARP_WILDCARD_REPLACE = @".*";

        // name for listener thread
        private const string PUBLISH_THREAD_NAME = "MqttPublishThread";

        #endregion

        // queue messages to publish
        private Queue<MqttMsgBase> publishQueue;

        // thread for publishing
        private Thread publishThread;
        // event for starting publish
        private AutoResetEvent publishQueueWaitHandle;
        private bool isRunning;

        // reference to subscriber manager
        private MqttSubscriberManager subscriberManager;

        // retained messages
        private Dictionary<string, MqttMsgPublish> retainedMessages;

        // subscriptions to send retained messages (new subscriber or reconnected client)
        private Queue<MqttSubscription> subscribersForRetained;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="subscriberManager">Reference to subscriber manager</param>
        public MqttPublisherManager(MqttSubscriberManager subscriberManager)
        {
            // save reference to subscriber manager
            this.subscriberManager = subscriberManager;

            // create empty list for retained messages
            this.retainedMessages = new Dictionary<string, MqttMsgPublish>();

            // create empty list for destination subscribers for retained message
            this.subscribersForRetained = new Queue<MqttSubscription>();

            // create publish messages queue
            this.publishQueue = new Queue<MqttMsgBase>();
            this.publishQueueWaitHandle = new AutoResetEvent(false);
        }
        
        /// <summary>
        /// Start publish handling
        /// </summary>
        public void Start()
        {
            this.isRunning = true;
            // create and start thread for publishing messages
            this.publishThread = new Thread(this.PublishThread);
            this.publishThread.Name = PUBLISH_THREAD_NAME;
            this.publishThread.Start();
        }

        /// <summary>
        /// Stop publish handling
        /// </summary>
        public void Stop()
        {
            this.isRunning = false;
            this.publishQueueWaitHandle.Set();

            // wait for thread
            this.publishThread.Join();
        }

        /// <summary>
        /// Publish message
        /// </summary>
        /// <param name="publish">Message to publish</param>
        public void Publish(MqttMsgPublish publish)
        {
            if (publish.Retain)
            {
                lock (this.retainedMessages)
                {
                    // retained message already exists for the topic
                    if (retainedMessages.ContainsKey(publish.Topic))
                    {
                        // if empty message, remove current retained message
                        if (publish.Message.Length == 0)
                            retainedMessages.Remove(publish.Topic);
                        // set new retained message for the topic
                        else
                            retainedMessages[publish.Topic] = publish;
                    }
                    else
                    {
                        // add new topic with related retained message
                        retainedMessages.Add(publish.Topic, publish);
                    }
                }
            }

            // enqueue
            lock (this.publishQueue)
            {
                this.publishQueue.Enqueue(publish);
            }

            // unlock thread for sending messages to the subscribers
            this.publishQueueWaitHandle.Set();
        }

        /// <summary>
        /// Publish retained messaged for a topic to a client
        /// </summary>
        /// <param name="topic">Topic to search for a retained message</param>
        /// <param name="clientId">Client Id to send retained message</param>
        public void PublishRetaind(string topic, string clientId)
        {
            lock (this.subscribersForRetained)
            {
                MqttSubscription subscription = this.subscriberManager.GetSubscription(topic, clientId);

                // add subscription to list of subscribers for receiving retained messages
                if (subscription != null)
                {
                    this.subscribersForRetained.Enqueue(subscription);
                }
            }

            // unlock thread for sending messages to the subscribers
            this.publishQueueWaitHandle.Set();
        }

        /// <summary>
        /// Process the message queue to publish
        /// </summary>
        public void PublishThread()
        {
            int count;
            byte qosLevel;
            MqttMsgPublish publish;

            while (this.isRunning)
            {
                // wait on message queueud to publish
                this.publishQueueWaitHandle.WaitOne();

                // first check new subscribers to send retained messages ...
                lock (this.subscribersForRetained)
                {
                    count = this.subscribersForRetained.Count;

                    // publish retained messages to subscribers (new or reconnected)
                    while (count > 0)
                    {
                        count--;
                        MqttSubscription subscription = this.subscribersForRetained.Dequeue();

                        var query = from p in this.retainedMessages
                                    where (new Regex(subscription.Topic)).IsMatch(p.Key)     // check for topics based also on wildcard with regex
                                    select p.Value;

                        if (query.Count() > 0)
                        {
                            foreach (MqttMsgPublish retained in query)
                            {
                                qosLevel = (subscription.QosLevel < retained.QosLevel) ? subscription.QosLevel : retained.QosLevel;

                                // send PUBLISH message to the current subscriber
                                subscription.Client.Publish(retained.Topic, retained.Message, qosLevel, retained.Retain);
                            }
                        }
                    }
                }
                
                // ... then pass to process publish queue
                lock (this.publishQueue)
                {
                    publish = null;

                    count = this.publishQueue.Count;
                    // publish all queued messages
                    while (count > 0)
                    {
                        count--;
                        publish = (MqttMsgPublish)this.publishQueue.Dequeue();

                        if (publish != null)
                        {
                            // get all subscriptions for a topic
                            List<MqttSubscription> subscriptions = this.subscriberManager.GetSubscriptionsByTopic(publish.Topic);

                            if ((subscriptions != null) && (subscriptions.Count > 0))
                            {
                                foreach (MqttSubscription subscription in subscriptions)
                                {
                                    qosLevel = (subscription.QosLevel < publish.QosLevel) ? subscription.QosLevel : publish.QosLevel;

                                    // send PUBLISH message to the current subscriber
                                    subscription.Client.Publish(publish.Topic, publish.Message, qosLevel, publish.Retain);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
