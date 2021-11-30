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

#ifndef _LED_STRIP_H_
#define _LED_STRIP_H_

#include "led_strip.h"
#include "config.h"

#define LOG_TAG LOG_TAG_LEDSTRIP
#include "log.h"

Ledstrip ledstrip;

/*
 * Class for managing neopixels
 */
Ledstrip::Ledstrip() 
: Adafruit_NeoPixel(NEOPIXEL_CNT, PIN_NEOPIXEL, NEOPIXEL_TYPE)
, lastColor(0) {

}

void Ledstrip::begin() {

    // clear any previous values
    lastColor = Color(255,255,255);
    clear();
    show();

    // subscribe to mqtt topics
    std::string topic;

    // subscribe for brightness commands
    config.formatTopic(topic, ConfigMain::MQTT_TOPIC_LIGHT, MQTT_SUBTOPIC_BRIGHTNESS_COMMAND);
    mqtt.subscribeTo(topic.c_str(), s_onMQTTMessage, (void*) MQTT_SUB_BRIGHTNESS);

    // subscribe for state commands
    config.formatTopic(topic, ConfigMain::MQTT_TOPIC_LIGHT, MQTT_SUBTOPIC_RGB_COMMAND);
    mqtt.subscribeTo(topic.c_str(), s_onMQTTMessage, (void*) MQTT_SUB_RGB_SET);

     // subscribe for command
    config.formatTopic(topic, ConfigMain::MQTT_TOPIC_LIGHT, MQTT_SUBTOPIC_COMMAND);
    mqtt.subscribeTo(topic.c_str(), s_onMQTTMessage, (void*) MQTT_SUB_COMMAND);
}

void Ledstrip::onMQTT_Command(uint8_t * payload, unsigned int len) {

    char str[4];
    std::string state_topic;

    // convert payload to string
    if (!len || len > (sizeof(str) - 1)) return;
    strncpy(str, (char*) payload, len);
    str[len] = '\0';

    LOG_F("Command: %s", str);

    // state topic
    config.formatTopic(state_topic, ConfigMain::MQTT_TOPIC_LIGHT, MQTT_SUBTOPIC_STATE);

    // ON
    if (strcasecmp(str, MQTT_COMMAND_PAYLOAD_ON) == 0) {
        fill(lastColor);
        show();

        // reply with state
        mqtt.publish(state_topic.c_str(), str);
    }

    // OFF
    if (strcasecmp(str, MQTT_COMMAND_PAYLOAD_OFF) == 0) {
        clear();
        show();

        // reply with state
        mqtt.publish(state_topic.c_str(), str);
    }
}

void Ledstrip::onMQTT_Brightness(uint8_t * payload, unsigned int len) {

    char str[12];
    int brightness;

    if (!len || len > (sizeof(str) - 1)) return;
    strncpy(str, (char*) payload, len);
    str[len] = '\0';

    LOG_F("Brightness: %s", str);

    brightness = atoi(str);
    setBrightness(brightness);
}

void Ledstrip::onMQTT_RGBSet(uint8_t * payload, unsigned int len) {

    char str[32];
    char * end;
    int r,g,b;

    // convert payload to string
    if (!len || len > (sizeof(str) -1)) return;
    strncpy(str, (char*) payload, sizeof(str));
    str[len] = '\0';

    LOG_F("RGB: %s", str);
    
    // get rgb
    r = (int) strtol(str, &end, 10); ++end; if (end > &str[len]) return;
    g = (int) strtol(end, &end, 10); ++end; if (end > &str[len]) return;
    b = (int) strtol(end, &end, 10); ++end; 

    lastColor = Color(r,g,b);
    fill(lastColor);
}

#endif//_LED_STRIP_H_