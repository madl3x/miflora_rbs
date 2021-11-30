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

#include <Arduino.h>
#include <algorithm>

#include "hass.h"
#include "mqtt.h"
#include "version.h"
#include "config.h"
#include "led_strip.h"

#define LOG_TAG LOG_TAG_HASS
#include "log.h"

#define ENDL "\n"

HomeAssistant hass;

HomeAssistant::HomeAssistant() 
    : taskDiscover(120 * TASK_SECOND, TASK_ONCE, s_taskDiscoverCbk, &scheduler, false)
    , bufferFormat(NULL)
    , mqttBufferSize(0) {

}

bool HomeAssistant::begin() {

    // run discovery
    taskDiscover.restartDelayed(10 * TASK_SECOND);
    return true;
}

bool HomeAssistant::prepareBuffer() {

    if (bufferFormat != NULL) {
        return true;
    }

    // allocate buffer for formatting data
    bufferFormat = (char*) malloc(1024);
    bufferSize   = 1024;

    // enlarge MQTT buffer so we can publish
    mqttBufferSize = mqtt.getBufferSize();

    LOG_F("Setting MQTT buffer from %u to %u bytes", mqttBufferSize, bufferSize);

    // reconfigure MQTT buffer size
    if (mqtt.setBufferSize(bufferSize) == false) {

        LOG_LN("Failed setting MQTT buffer");

        // restore MQTT buffer
        freeBuffer();
        return false;
    }

    return bufferFormat != NULL;
}

void HomeAssistant::freeBuffer() {

    // free allocated buffers
    if (bufferFormat) {
        free(bufferFormat);
        bufferFormat = NULL;
    }

    // restore MQTT buffer size
    if (mqttBufferSize) {
        if (mqtt.getBufferSize() != mqttBufferSize) {
            LOG_F("Restoring MQTT buffer from %u to %u bytes", bufferSize, mqttBufferSize);
            mqtt.setBufferSize(mqttBufferSize);
        }
        mqttBufferSize = 0;
    }
}

std::string HomeAssistant::getDeviceID(DeviceIDType type) {

    std::string device_id;

    // central device id is common for all plants
    if (type == DEVICE_ID_CENTRAL) {

        // get from config or set default
        if (config.hass_central_device_id != NULL) {
            device_id.assign(config.hass_central_device_id);
        } else {
            device_id.assign("miflora_rbs_central");
        }
        
    } else
    // station device id is specific for each station
    if (type == DEVICE_ID_STATION) {

        // get from config or compose
        if (config.hass_station_device_id != NULL) {
            device_id.assign(config.hass_station_device_id);
        } else {
            device_id.assign("miflora_rbs_");
            device_id.append(config.station_name);
        }
    }

    return device_id;
}

std::string HomeAssistant::jsonGetDevice(DeviceIDType device) {

    std::string json;
    std::string device_id;
    std::string suggested_area;

    // setup device id
    device_id   = getDeviceID(device);

    if (device == DEVICE_ID_STATION) {
        if (config.hass_station_suggested_area) {
            suggested_area.assign(" \"suggested_area\": \"");
            suggested_area.append(config.hass_station_suggested_area);
            suggested_area.append("\"," ENDL);
        }
    }

    snprintf(
        bufferFormat, bufferSize, 
        "\"device\": {" ENDL
        " \"identifiers\": \"%s\"," ENDL
        " \"name\": \"%s\"," ENDL
        " \"sw_version\": \"miflora-rbs v" VERSION_STR " " __DATE__ ", " __TIME__ "\"," ENDL
        " \"model\": \"%s\"," ENDL
        "%s"
        " \"manufacturer\": \"Alex M.\"" ENDL
        "}"
        ,
        device_id.c_str(),
        device_id.c_str(),
        ARDUINO_BOARD,
        suggested_area.c_str()
    );

    // assign result
    json.assign(bufferFormat);
    return json;
}

std::string HomeAssistant::baseStationEntityName() {
    std::string entity_name;

    entity_name.assign(getDeviceID(DEVICE_ID_STATION).c_str());
    return entity_name;
}

std::string HomeAssistant::baseMiFloraEntityName(MiFloraDevice * device) {
    std::string entity_name;
    char flora_id[6];
    sprintf(flora_id, "%d", device->getID());

    entity_name.assign(getDeviceID(DEVICE_ID_CENTRAL).c_str());
    entity_name.append(" plant ");
    entity_name.append(flora_id);
    return entity_name;
}

std::string HomeAssistant::getDiscoveryTopic(DeviceIDType device, const char * componentType, const char * _entityName) {

    std::string topic;
    std::string entityName(_entityName);

    // topic prefix
    topic.assign(config.hass_discovery_topic_prefix);
    topic.append("/");

    // component
    topic.append(componentType);
    topic.append("/");

    // node id
    topic.append(getDeviceID(device));
    topic.append("/");

    // device name (object_id)
    std::replace(entityName.begin(), entityName.end(), ' ', '_');
    topic.append(entityName);

    topic.append("/config");
    std::replace(topic.begin(),topic.end(),'-','_');
    return topic;
}

std::string HomeAssistant::jsonDHTSensor(bool temperature, std::string & entity_name) {

    std::string json;
    std::string availability_topic;
    std::string state_topic;
    std::string unique_id;

    const char * unit = NULL;
    const char * icon = NULL;

    // prepare entity name
    entity_name = baseStationEntityName();

    // prepare availability topic
    config.formatTopic(availability_topic, ConfigMain::MQTT_TOPIC_AVAILABILITY);

    if (temperature) {
        unit = "ºC";
        icon = "mdi:thermometer-lines";
        entity_name.append(" temperature");
        config.formatTopic(state_topic, ConfigMain::MQTT_TOPIC_DHT_SENSOR, "temperature");
    } else {
        unit = "%";
        icon = "mdi:water";
        entity_name.append(" humidity");
        config.formatTopic(state_topic, ConfigMain::MQTT_TOPIC_DHT_SENSOR, "humidity");
    }

   // unique id is the same as entity name but with underscores
    unique_id   = entity_name;
    std::replace(unique_id.begin(), unique_id.end(), ' ', '_');

    // format discovery string
    snprintf(
        bufferFormat, bufferSize,
        "\"unit_of_measurement\": \"%s\"," ENDL
        "\"icon\": \"%s\"," ENDL
        "\"name\": \"%s\"," ENDL
        "\"state_topic\": \"%s\"," ENDL
        "\"availability_topic\": \"%s\"," ENDL
        "\"unique_id\": \"%s\"" ENDL
        ,
        unit, 
        icon,
        entity_name.c_str(),
        state_topic.c_str(),
        availability_topic.c_str(),
        unique_id.c_str()
    );

    json.assign(bufferFormat);
    return json;
}

std::string HomeAssistant::jsonMiFloraAttribute(MiFloraDevice * flora_device, AttributeID attr_id, std::string & entity_name) {

    std::string json;

    std::string availability_topic;
    std::string state_topic;
    std::string unique_id;
    
    const char * unit = NULL;
    const char * icon = NULL;
    const char * flora_address = flora_device->getAddress().c_str();

    // prepare the base for this entity name
    entity_name = baseMiFloraEntityName(flora_device);

    // prepare the base for unique id
    unique_id.assign("miflorarbs_");
    unique_id.append(flora_device->getAddressCompressed().c_str());
    unique_id.append("_");

    // prepare availability topic
    config.formatTopic(availability_topic, ConfigMain::MQTT_TOPIC_AVAILABILITY);

    // prepare attributes
    switch (attr_id) {
        case ATTR_ID_MOISTURE: {
            icon = "mdi:water";
            unit = "%";
            entity_name.append(" moisture");
            unique_id.append("moisture");
            config.formatTopic(state_topic, ConfigMain::MQTT_TOPIC_FLORA, flora_address, "moisture");
        } break;
        case ATTR_ID_TEMPERATURE: {
            icon = "mdi:thermometer";
            unit = "ºC";
            entity_name.append(" temperature");
            unique_id.append("temperature");
            config.formatTopic(state_topic, ConfigMain::MQTT_TOPIC_FLORA, flora_address, "temp");
        } break;
        case ATTR_ID_CONDUCTIVITY: {
            icon = "mdi:sprout";
            unit = "uS/cm";
            entity_name.append(" conductivity");
            unique_id.append("conductivity");
            config.formatTopic(state_topic, ConfigMain::MQTT_TOPIC_FLORA, flora_address, "conductivity");
        } break;
        case ATTR_ID_ILLUMINANCE: {
            icon = "mdi:sun-wireless";
            unit = "lux";
            entity_name.append(" light");
            unique_id.append("light");
            config.formatTopic(state_topic, ConfigMain::MQTT_TOPIC_FLORA, flora_address, "light");
        } break;
        case ATTR_ID_RSSI: {
            icon = "mdi:signal";
            unit = "dB";
            entity_name.append(" rssi");
            unique_id.append("rssi");
            config.formatTopic(state_topic, ConfigMain::MQTT_TOPIC_FLORA, flora_address, "moisture");
        } break;
        default: 
            // should not happen
            return json;
    }

    // format discovery string
    snprintf(
        bufferFormat, bufferSize,
        "\"unit_of_measurement\": \"%s\"," ENDL
        "\"icon\": \"%s\"," ENDL
        "\"name\": \"%s\"," ENDL
        "\"state_topic\": \"%s\"," ENDL
        "\"availability_topic\": \"%s\"," ENDL
        "\"unique_id\": \"%s\"" ENDL
        ,
        unit, 
        icon,
        entity_name.c_str(),
        state_topic.c_str(),
        availability_topic.c_str(),
        unique_id.c_str()
    );

    json.assign(bufferFormat);
    return json;
}

std::string HomeAssistant::jsonStatus(std::string & entity_name) {
    
    std::string json;    
    std::string unique_id;
    std::string availability_topic;

    // prepare status entity name
    entity_name = baseStationEntityName();
    entity_name.append(" status");

    // unique id for the status entity
    unique_id.assign(entity_name);
    std::replace(unique_id.begin(), unique_id.end(), ' ', '_');

    // prepare availability topic
    config.formatTopic(availability_topic, ConfigMain::MQTT_TOPIC_AVAILABILITY);

    // format discovery string
    snprintf(
        bufferFormat, bufferSize,
        "\"device_class\": \"connectivity\"," ENDL
        "\"payload_on\" : \"%s\"," ENDL
        "\"payload_off\" : \"%s\"," ENDL
        "\"icon\": \"%s\"," ENDL
        "\"name\": \"%s\"," ENDL
        "\"state_topic\": \"%s\"," ENDL
        "\"availability_topic\": \"%s\"," ENDL
        "\"unique_id\": \"%s\""
        ,
        config.station_payload_online, 
        config.station_payload_offline,
        "mdi:lan-connect",
        entity_name.c_str(),
        availability_topic.c_str(),
        availability_topic.c_str(),
        unique_id.c_str()
    );

    json.assign(bufferFormat);
    return json;
}

std::string HomeAssistant::jsonWiFiSignal(std::string & entity_name) {
    
    std::string json;    
    std::string unique_id;
    std::string state_topic;
    std::string availability_topic;

    // prepare status entity name
    entity_name = baseStationEntityName();
    entity_name.append(" wifi rssi");

    // unique id for the status entity
    unique_id.assign(entity_name);
    std::replace(unique_id.begin(), unique_id.end(), ' ', '_');

    // prepare availability topic
    config.formatTopic(state_topic, ConfigMain::MQTT_TOPIC_WIFI, "signal");
    config.formatTopic(availability_topic, ConfigMain::MQTT_TOPIC_AVAILABILITY);

    // format discovery string
    snprintf(
        bufferFormat, bufferSize,
        "\"icon\": \"%s\"," ENDL
        "\"unit\": \"%s\"," ENDL
        "\"name\": \"%s\"," ENDL
        "\"state_topic\": \"%s\"," ENDL
        "\"availability_topic\": \"%s\"," ENDL
        "\"unique_id\": \"%s\""
        ,
        "mdi:wifi",
        "dB",
        entity_name.c_str(),
        state_topic.c_str(),
        availability_topic.c_str(),
        unique_id.c_str()
    );

    json.assign(bufferFormat);
    return json;
}

std::string HomeAssistant::jsonNeopixel(std::string & entity_name) {

    std::string json;    
    std::string unique_id;
    std::string brightness_cmd_topic, brightness_state_topic;
    std::string rgb_cmd_topic, rgb_state_topic;
    std::string command_topic, state_topic;
    std::string availability_topic;

    // prepare status entity name
    entity_name = baseStationEntityName();
    entity_name.append(" neopixel");

    // unique id for the status entity
    unique_id.assign(entity_name);
    std::replace(unique_id.begin(), unique_id.end(), ' ', '_');

    // prepare topics
    config.formatTopic(availability_topic    , ConfigMain::MQTT_TOPIC_AVAILABILITY);
    config.formatTopic(brightness_cmd_topic  , ConfigMain::MQTT_TOPIC_LIGHT, Ledstrip::MQTT_SUBTOPIC_BRIGHTNESS_COMMAND);
    config.formatTopic(brightness_state_topic, ConfigMain::MQTT_TOPIC_LIGHT, Ledstrip::MQTT_SUBTOPIC_BRIGHTNESS_STATE);
    config.formatTopic(rgb_cmd_topic         , ConfigMain::MQTT_TOPIC_LIGHT, Ledstrip::MQTT_SUBTOPIC_RGB_COMMAND);
    config.formatTopic(rgb_state_topic       , ConfigMain::MQTT_TOPIC_LIGHT, Ledstrip::MQTT_SUBTOPIC_RGB_STATE);
    config.formatTopic(command_topic         , ConfigMain::MQTT_TOPIC_LIGHT, Ledstrip::MQTT_SUBTOPIC_COMMAND);
    config.formatTopic(state_topic           , ConfigMain::MQTT_TOPIC_LIGHT, Ledstrip::MQTT_SUBTOPIC_STATE);

    // format discovery string
    snprintf(
        bufferFormat, bufferSize,
        "\"icon\": \"%s\"," ENDL
        "\"name\": \"%s\"," ENDL
        "\"brightness\": true," ENDL
        "\"rgb\": true," ENDL
        "\"command_topic\": \"%s\"," ENDL
        "\"state_topic\": \"%s\"," ENDL
        "\"brightness_command_topic\": \"%s\"," ENDL
        //"\"brightness_state_topic\": \"%s\"," ENDL
        "\"rgb_command_topic\": \"%s\"," ENDL
       // "\"rgb_state_topic\": \"%s\"," ENDL
        "\"availability_topic\": \"%s\"," ENDL
        "\"retain\": true," ENDL
        "\"unique_id\": \"%s\""
        ,
        "mdi:alarm-light",
        entity_name.c_str(),
        command_topic.c_str(),
        state_topic.c_str(),
        brightness_cmd_topic.c_str(), 
        //brightness_state_topic.c_str(),
        rgb_cmd_topic.c_str(),
        //rgb_state_topic.c_str(),
        availability_topic.c_str(),
        unique_id.c_str()
    );

    json.assign(bufferFormat);
    return json;
}

bool HomeAssistant::publishMQTTComponent(DeviceIDType device, const char * componentType, const char * entityName, const char * entityJSON) {
    
    std::string deviceJSON     = jsonGetDevice(device);
    std::string discoveryTopic = getDiscoveryTopic(device, componentType, entityName);

    // prepare payload
    snprintf(
        bufferFormat, bufferSize,
        "{" ENDL
        "\"platform\": \"mqtt\", " ENDL
        "%s," ENDL
        "%s" ENDL
        "}" ENDL 
        ,
        entityJSON,
        deviceJSON.c_str()
    );


    // do publish
    return publishJSONPayload(discoveryTopic.c_str());
}

bool HomeAssistant::publishJSONPayload(const char * discoveryTopic) {
    
    bool publish_ok;

    // do publish
    publish_ok = mqtt.publish(
        discoveryTopic, 
        (uint8_t *) bufferFormat, 
        (unsigned int) strlen(bufferFormat), 
        config.hass_mqtt_retain);

    // debug
    LOG_F("%s%s %s",
        config.hass_mqtt_retain ? CONSOLE_GREEN "[R] " CONSOLE_RESET : "",
        publish_ok ? CONSOLE_GREEN " OK" CONSOLE_RESET : CONSOLE_RED "NOK" CONSOLE_RESET,
        discoveryTopic
    );

    return publish_ok;
}

bool HomeAssistant::mqttPublishDiscovery_MiFlora(MiFloraDevice * device) {

    bool publish_ok = true;

    AttributeID attr_ids [] = {
        ATTR_ID_MOISTURE,
        ATTR_ID_TEMPERATURE,
        ATTR_ID_CONDUCTIVITY,
        ATTR_ID_ILLUMINANCE,
        ATTR_ID_RSSI
    };

    LOG_F("Publishing Flora #%d %s (%s)", 
        device->getID(), device->getName().c_str(), device->getAddress().c_str());

    // for each attribute
    for (int attr_idx = 0 ; attr_idx < sizeof(attr_ids)/sizeof(AttributeID); ++ attr_idx ) {

        std::string entityName;
        std::string entityJson = jsonMiFloraAttribute(device, attr_ids[attr_idx], entityName);

        publish_ok = publishMQTTComponent(DEVICE_ID_CENTRAL, "sensor", entityName.c_str(), entityJson.c_str());
        if (publish_ok == false)
            break;
        
        // give it some time
        delay(100);
    }
    
    return publish_ok;
}

bool HomeAssistant::mqttPublishDiscovery_DHT() {

    bool publish_ok = true;

    LOG_LN("Publishing DHT sensor..");

    // publish both temperature and humidity
    for (int i = 0 ; i <= 1 ; ++ i) {

        std::string entityName;
        std::string entityJson = jsonDHTSensor(i==0, entityName);

        publish_ok = publishMQTTComponent(DEVICE_ID_STATION, "sensor", entityName.c_str(), entityJson.c_str());
        if (publish_ok == false)
            break;
    }

    if(publish_ok) delay(100);
    return publish_ok;
}

bool HomeAssistant::mqttPublishDiscovery_Status() {

    bool publish_ok = true;
    LOG_LN("Publishing status..");

    std::string entityName;
    std::string entityJson = jsonStatus(entityName);

    publish_ok = publishMQTTComponent(DEVICE_ID_STATION, "binary_sensor", entityName.c_str(), entityJson.c_str());
    if(publish_ok) delay(100);
    return publish_ok;
}

bool HomeAssistant::mqttPublishDiscovery_WiFiSignal() {

    bool publish_ok = true;
    LOG_LN("Publishing WiFi signal..");

    std::string entityName;
    std::string entityJson = jsonWiFiSignal(entityName);

    publish_ok = publishMQTTComponent(DEVICE_ID_STATION, "sensor", entityName.c_str(), entityJson.c_str());
    if(publish_ok) delay(100);
    return publish_ok;
}

bool HomeAssistant::mqttPublishDiscovery_Neopixel() {
    bool publish_ok = true;
    LOG_LN("Publishing Neopixel light..");

    std::string entityName;
    std::string entityJson = jsonNeopixel(entityName);

    publish_ok = publishMQTTComponent(DEVICE_ID_STATION, "light", entityName.c_str(), entityJson.c_str());
    if(publish_ok) delay(100);
    return publish_ok;
}

void HomeAssistant::taskDiscoverCbk() {
    bool discovery_done = true;

    if (!mqtt.connected()) {
        LOG_LN("Mqtt not connected, waiting..");
        taskDiscover.restartDelayed(10 * TASK_SECOND);
        return; // pass
    }

    LOG_F("Starting discovery..");

    do {

        // prepare internal buffer
        if (prepareBuffer() == false) {
            // OOM, try again later
            LOG_LN("Out of memory");
            discovery_done = false;
            return;
        }

        // plant devices are assigned to the central device
        if (config.hass_publish_central_device) {

            // mi flora devices
            for (auto flora_device : fleet.devices()) {

                if (mqttPublishDiscovery_MiFlora(flora_device) == false) {
                    discovery_done = false;
                    break;
                }
            }

            if (discovery_done == false)
                break;
        }

        // entities specific to station device

        // DHT sensor
        discovery_done = mqttPublishDiscovery_DHT();
        if (discovery_done == false)
            break;

        // Status (online/offline)
        discovery_done = mqttPublishDiscovery_Status();
        if (discovery_done == false)
            break;

        // WiFi signal
        discovery_done = mqttPublishDiscovery_WiFiSignal();
        if (discovery_done == false)
            break;

        // publish light
        discovery_done = mqttPublishDiscovery_Neopixel();
        if (discovery_done == false)
            break;

    } while (0);

    // free buffer
    freeBuffer();

    // check if everything went ok
    if (discovery_done == false) {
        LOG_LN("Discovery failed, trying again soon!");
        taskDiscover.restartDelayed(10 * TASK_SECOND);
        return;
    }

    LOG_LN("Discovery successful, stopping task!");
    taskDiscover.disable();
}