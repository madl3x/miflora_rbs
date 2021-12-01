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

#include "mqtt.h"
#include "config.h"

#define LOG_TAG LOG_TAG_MQTT
#include "log.h"

MQTT mqtt;

/*
 * Class handling MQTT connection and subscriptions
 */ 
MQTT::MQTT() :
    taskHandle(MQTT_UPDATE_INTERVAL, TASK_FOREVER, s_taskHandleCbk, &scheduler, false) {    

    setClient(wifiClient);
    setCallback(s_subscribeCbk);
}

/* connect to MQTT server according to configuration */
bool MQTT::begin() {

    std::string clientid;

    // configure server
    setServer(config.mqtt_host, config.mqtt_port);

    // configure client-id
    if (config.mqtt_clientid != NULL) {

        // given by configuration
        clientid.assign(config.mqtt_clientid);
    } else {
        // generate one based on station name and mac address
        uint8_t mac_addr[6];
        char    mac_addr_str[7];

        // get mac address
        esp_read_mac(mac_addr, ESP_MAC_WIFI_STA);

        // construct client id: <station_name>-<mac_suffix>
        clientid.assign(config.station_name);
        clientid.append("-");
        sprintf(mac_addr_str, "%2x%2x%2x", mac_addr[3], mac_addr[4], mac_addr[5]);
        clientid.append(mac_addr_str);

        LOG_F("Client id generated: %s", clientid.c_str());
    }

    // do connect and set will topic as the availability topic
    if (!connect(
        // client id
        clientid.c_str(), 
        // credentials
        config.mqtt_username, 
        config.mqtt_password,
        // will topic, qos, retain and payload
        config.station_availability_topic, 0, true,  config.station_payload_offline )) {

        LOG_F("Connection failed: %d", state());
        return false;
    }

    // MQTT connected
    LOG_F("Connected (%s:%d)!", config.mqtt_host, config.mqtt_port);

    // publish online state
    std::string topic;
    config.formatTopic(topic, ConfigMain::MQTT_TOPIC_AVAILABILITY);
    publish(topic.c_str(), config.station_payload_online, true);

    // redo subscriptions
    resendSubscriptions();

    // start task for handling MQTT receiving packets
    taskHandle.restartDelayed();
    return true;
}

void MQTT::end() {
    disconnect();
    taskHandle.disable();
}

/* resubscribe to all known subscriptions, useful when reconnecting */
void MQTT::resendSubscriptions() {

    for (auto sub : subscriptions) {
        subscribe(sub.topic.c_str());
    }
}

/* check if a subscription exists, for a given topic */
bool MQTT::hasSubscription(const char * topic) {
    return getSubscriptionHandler(topic) != NULL;
}

/* return subscription callback handler */
MQTT::Callback_t MQTT::getSubscriptionHandler(const char * topic, void **param) {

    for (auto sub : subscriptions) {
        if (strcasecmp(topic, sub.topic.c_str()) == 0) {
            if (param != NULL)
                * param = sub.param;
            return sub.handler;
        }
    }
    return NULL;
}

/* subscribe to a given topic with a handler */
bool MQTT::subscribeTo(const char * topic, Callback_t callback, void * param) {
    
    std::string strTopic(topic);

    // subscribing for the same topic is now allowed
    if (hasSubscription(topic))
        return false;

    // add to subscriptions
    Subscription_t entry = {.topic = topic, .handler = callback, .param = param };
    subscriptions.push_back(entry);

    LOG_F("Subscription [%p] -> %s (param=%p)", callback, topic, param);

    // if MQTT is not connected, this subscription will get active
    // when it does get connected, via redoSubscriptions()
    if (connected()) subscribe(topic);
    return true;
}

/* callback called by PubSubClient when a message is received from the MQTT server */
void MQTT::subscribeCbk(char * topic, uint8_t * payload, unsigned int len) {

    LOG_F(CONSOLE_YELLOW "RX %3uB" CONSOLE_RESET" T:%s", len, topic);

    // invoke subscribers
    void * param = NULL;
    auto handlerCbk = getSubscriptionHandler(topic, &param);
    if (handlerCbk != NULL) {
        handlerCbk(topic, payload, len, param);
    } else {
        LOG_F("Failed finding subscription for %s!", topic);
    }
}

/* publish */
boolean MQTT::publish(const char* topic, const char* payload) {
    LOG_F(CONSOLE_GREEN "TX %3uB" CONSOLE_RESET " T:%s", strlen(payload), topic);
    return PubSubClient::publish(topic, payload);
}

boolean MQTT::publish(const char* topic, const char* payload, boolean retained) {
    LOG_F(CONSOLE_GREEN "TX %3uB" CONSOLE_RESET " T:%s %s", strlen(payload), topic,  retained ? "(retain)" : "");
    return PubSubClient::publish(topic, payload, retained);
}

boolean MQTT::publish(const char* topic, const uint8_t * payload, unsigned int plength) {
    LOG_F(CONSOLE_GREEN "TX %3uB" CONSOLE_RESET " T:%s", plength, topic);
    return PubSubClient::publish(topic, payload, plength);
}

boolean MQTT::publish(const char* topic, const uint8_t * payload, unsigned int plength, boolean retained) {
    LOG_F(CONSOLE_GREEN "TX %3uB" CONSOLE_RESET " T:%s %s", plength, topic, retained ? "(retain)" : "");
    return PubSubClient::publish(topic, payload, plength, retained);
}
