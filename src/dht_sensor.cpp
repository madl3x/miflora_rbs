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

#include "dht_sensor.h"
#include "config.h"
#include "mqtt.h"

#define LOG_TAG LOG_TAG_DHT
#include "log.h"

DHTSensor dht;

DHTSensor::DHTSensor() 
    : DHT(PIN_DHT, DHT_TYPE)
    , taskUpdate(5000, TASK_FOREVER, s_taskUpdateCbk, &scheduler, false)
    , lastTemp(.0f)
    , lastHum(0)
    , lastPublish(0) {
}

bool DHTSensor::begin() {

    // initialize DHT
    DHT::begin();

    // force a reading to make sure everything works
    if(read(true) == false) {

        // DHT reading failed
        LOG_F("failed init: check PIN (%d) and TYPE (%d)", PIN_DHT, DHT_TYPE);
        return false;
    }

    // update first temperature and humidity
    // this will be read from cache anyway
    lastTemp = readTemperature(false, false);
    lastHum  = readHumidity(false);
    lastPublish = millis()/1000;

    // start periodic updates
    taskUpdate.restartDelayed();

    return true;
}

void DHTSensor::taskUpdateCbk() {

    char val[12];
    auto now = millis()/1000;

    // attempt to read by force
    if (read(true) == false) {
        LOG_LN("failed reading!");
        return;
    }

    // update temperature and humidity
    auto temp = (float  ) readTemperature(false, false);
    auto  hum = (uint8_t) readHumidity(false);

    // publish to MQTT
    LOG_F("Temp: %.2fC Hum: %u%% Last publish: %lus Retain: %s",
         temp, hum, (now - lastPublish), config.dht_mqtt_retain ? "yes" : "no");

    if ( (now - lastPublish)  > config.dht_publish_min_interval_sec) {
        std::string topic;

        // temperature
        sprintf(val, "%.2f", temp);
        config.formatTopic(topic, ConfigMain::MQTT_TOPIC_DHT_SENSOR, "temperature");
        mqtt.publish(topic.c_str(), val, config.dht_mqtt_retain);

        // humidity
        sprintf(val, "%u", hum);
        config.formatTopic(topic, ConfigMain::MQTT_TOPIC_DHT_SENSOR, "humidity");
        mqtt.publish(topic.c_str(), val, config.dht_mqtt_retain);

        lastPublish = now;
    }

    // update
    lastTemp = temp;
    lastHum  = hum;
}
