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

namespace uPLibrary.Networking.M2Mqtt.Utility
{
    /// <summary>
    /// Extension class for a Queue
    /// </summary>
    internal static class QueueExtension
    {
        /// <summary>
        /// Predicate for searching inside a queue
        /// </summary>
        /// <param name="item">Item of the queue</param>
        /// <returns>Result of predicate</returns>
        internal delegate bool QueuePredicate(object item);

        /// <summary>
        /// Get (without removing) an item from queue based on predicate
        /// </summary>
        /// <param name="queue">Queue in which to search</param>
        /// <param name="predicate">Predicate to verify to get item</param>
        /// <returns>Item matches the predicate</returns>
        internal static object Get(this Queue queue, QueuePredicate predicate)
        {
            foreach (var item in queue)
            {
                if (predicate(item))
                    return item;
            }
            return null;
        }
    }
}
