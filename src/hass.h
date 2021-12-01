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

#ifndef _HASS_H_
#define _HASS_H_

#include "scheduler.h"
#include "device.h"
#include <string>

/*
 * Class for managing home assistant automatic discovery
 */
class HomeAssistant {
    public:
        enum DeviceIDType {
            DEVICE_ID_CENTRAL,
            DEVICE_ID_STATION
        };
    public:
        HomeAssistant();
        
        bool begin();
        void restart();

    protected:
        Task     taskDiscover;
        char *   bufferFormat;
        size_t   bufferSize;
        uint16_t mqttBufferSize;

        bool        prepareBuffer();
        void        freeBuffer();

        std::string getDeviceID(DeviceIDType type);
        std::string getDiscoveryTopic(DeviceIDType device, const char * componentType, const char * entityName);

        std::string jsonGetDevice(DeviceIDType device);
        std::string baseStationEntityName();
        std::string baseMiFloraEntityName(MiFloraDevice * device);

        std::string jsonMiFloraAttribute(MiFloraDevice * device, AttributeID attribute, std::string & entityName);
        std::string jsonDHTSensor(bool temperature_or_humidity, std::string& entityName);
        std::string jsonStatus(std::string & entityName);
        std::string jsonWiFiSignal(std::string & entity_name);
        std::string jsonNeopixel(std::string &entity_name);

        bool        mqttPublishDiscovery_MiFlora(MiFloraDevice * device);
        bool        mqttPublishDiscovery_DHT();
        bool        mqttPublishDiscovery_Status();
        bool        mqttPublishDiscovery_WiFiSignal();
        bool        mqttPublishDiscovery_Neopixel();

        bool        publishMQTTComponent(DeviceIDType device, const char * componentType, const char * entityName, const char * entityJSON);
        bool        publishJSONPayload(const char * deviceTopic);

        void        taskDiscoverCbk();
        static void s_taskDiscoverCbk();
};

extern HomeAssistant hass;

/* inlines for HomeAssistant */
inline void HomeAssistant::s_taskDiscoverCbk() {
    hass.taskDiscoverCbk();
}
#endif//_HASS_H_