/*
 *   MIT License
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 * 
 *  Copyright (c) 2021 Alex Mircescu
 */

#ifndef _MQTT_H_
#define _MQTT_H_

#include <WiFi.h>
#include <PubSubClient.h>
#include <list>
#include <utility>

#include "scheduler.h"

#define MQTT_UPDATE_INTERVAL 100 /* ms */

/*
 * Class handling MQTT connection and subscriptions
 */ 
class MQTT : public PubSubClient {
    public:
    
        typedef void (*Callback_t)(const char *, uint8_t *, unsigned int, void * param);

        typedef struct {
            std::string topic;
            Callback_t  handler;
            void *      param;
        } Subscription_t;

        typedef std::list<Subscription_t> SubscriptionList_t;

    public:
        MQTT();

        bool       begin();
        void       end();

        bool       subscribeTo(const char * topic, Callback_t callback, void * param = NULL);
        bool       hasSubscription(const char * topic);
        Callback_t getSubscriptionHandler(const char * topic, void ** param = NULL);
        
        boolean    publish(const char* topic, const char* payload);
        boolean    publish(const char* topic, const char* payload, boolean retained);
        boolean    publish(const char* topic, const uint8_t * payload, unsigned int plength);
        boolean    publish(const char* topic, const uint8_t * payload, unsigned int plength, boolean retained);

    protected: 
        WiFiClient         wifiClient;
        Task               taskHandle;
        SubscriptionList_t subscriptions;

        void       resendSubscriptions();
        void       subscribeCbk(char * topic, uint8_t * payload, unsigned int len);

        static void s_subscribeCbk(char * topic, uint8_t * payload, unsigned int len);
        static void s_taskHandleCbk();
};

extern MQTT mqtt;

/* inlines for MQTT */
inline void MQTT::s_taskHandleCbk() {
    mqtt.loop();
}

inline void MQTT::s_subscribeCbk(char * topic, uint8_t * payload, unsigned int len) {
    mqtt.subscribeCbk(topic, payload, len);
}

#endif//_MQTT_H_