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

#include "network.h"
#include "config.h"
#include "led_strip.h"

#define LOG_TAG LOG_TAG_NETWORK
#include "log.h"

Network net;

Network::Network() 
    : taskCheckWiFi(5000, TASK_FOREVER, s_taskCheckWiFiCbk, &scheduler, false)
    , state(STATE_DISCONNECTED)
    , lastDisconnect(0) {

}

void Network::begin() {

    // connect WiFi
    LOG_F("Wifi connect SSID:%s", config.wifi_ssid);
    WiFi.begin(config.wifi_ssid, config.wifi_password);

    // remember last disconnect
    lastDisconnect = millis();

    // connect WiFi
    state = STATE_WIFI_CONNECTING;
    taskCheckWiFi.setInterval(1000);
    taskCheckWiFi.restart();

    ledstrip.setPixelColor(0,255,0,0);
    ledstrip.show();
}

void Network::end() {

    // shtudown wifi
    WiFi.disconnect(true);

    // disable reconnect task
    taskCheckWiFi.disable();

    // set new state
    state = STATE_DISCONNECTED;
    lastDisconnect = millis();
}

void Network::taskCheckWiFiCbk() {

    switch(state) {
        // disconnected state - should not happen
        case STATE_DISCONNECTED: {
            taskCheckWiFi.disable();
        } break;

        // wifi needs to be reconfigured 
        case STATE_WIFI_SETUP: {

            LOG_F("Wifi connect SSID:%s", config.wifi_ssid);
            WiFi.begin(config.wifi_ssid, config.wifi_password);

            // change state
            state = STATE_WIFI_CONNECTING;
            taskCheckWiFi.restartDelayed(500);
        } break;

        // wifi is connecting ...
        case STATE_WIFI_CONNECTING: {

            // wifi is connected
            if (WiFi.status() == WL_CONNECTED) {

                // wifi connected, now connect MQTT
                ledstrip.setPixelColor(0, 0, 255, 0);

                // change state to connected
                state = STATE_WIFI_CONNECTED;
                taskCheckWiFi.restart();
                return;
            }

            // restart WiFi is it doesn't connect for a given set of task iterations
            if (config.wifi_restart_core_sec) {
                uint16_t sec_passed = ((millis() - lastDisconnect)/1000);

                if (sec_passed > config.wifi_restart_core_sec) {
                    LOG_LN(" ---- ");
                    LOG_LN("Restarting core due to WiFi inactivity!");
                    LOG_LN(" ---- ");

                    ESP.restart();

                    // should not reach
                    lastDisconnect = millis();
                }
            }

            // still connecting => toggle led 0
            ledstrip.setPixelColor(0, ledstrip.getPixelColor(0) ? 0 : 255,0,0);
            ledstrip.show();

            // check every second
            taskCheckWiFi.setInterval(1000);
        } break;

        // Wifi connected, start MQTT automatically
        case STATE_WIFI_CONNECTED:
        case STATE_MQTT_CONNECTING: {

            // set pixel to read
            ledstrip.setPixelColor(1, 0, ledstrip.getPixelColor(1) ? 0 : 255, ledstrip.getPixelColor(1) ? 0 : 255);
            ledstrip.show();

            // connect to MQTT
            if (!mqtt.begin()) {

                // failed connecting, retry in 1 second
                taskCheckWiFi.setInterval(1000);
                return;
            } 

            // mqtt connected => ledstrip is now in control of MQTT commands
            ledstrip.clear();
            ledstrip.show();

            // change state to connected (keep lights on for a 2 more seconds)
            state = STATE_MQTT_CONNECTED;
            taskCheckWiFi.setInterval(100);
            taskCheckWiFi.restart();
        } break;

        // MQTT is connected
        case STATE_MQTT_CONNECTED: {

            // for the moment we have nothing to do in this state

            // all connected, move to verifying network periodically
            state = STATE_VERIFY_NETWORK;
            taskCheckWiFi.setInterval(5000);
            taskCheckWiFi.restartDelayed();
        } break;

        // verify network periodically
        case STATE_VERIFY_NETWORK: {

            // if WiFi disconnected meanwhile, move back to connecting state
            if (WiFi.isConnected() == false) {

                // get back to connecting state
                LOG_F("WiFi status is:%d, reconnecting..", WiFi.status());
                state = STATE_WIFI_CONNECTING;
                taskCheckWiFi.restart();
                lastDisconnect = millis();

                // reset ledstrip
                ledstrip.clear();
                ledstrip.show();
                return;
            }

            // if MQTT disconnected meanwhile, move back to connecting state
            if (mqtt.state() != MQTT_CONNECTED) {

                // get back to connecting state
                LOG_F("MQTT state is:%d, reconnecting..", mqtt.state());
                state = STATE_MQTT_CONNECTING;
                taskCheckWiFi.restart();

                // reset ledstrip
                ledstrip.clear();
                ledstrip.show();
                return;
            }

            // all good, publish signal
            static unsigned long lastPublished = 0;
            unsigned long now = millis();

            if ( (now - lastPublished) > 10000 ) {
                std::string topic;
                char val[6];

                config.formatTopic(topic, ConfigMain::MQTT_TOPIC_WIFI, "signal");
                sprintf(val, "%d", WiFi.RSSI());
                mqtt.publish(topic.c_str(), val);

                lastPublished = now;
            }

        } break;
    }
}