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

#include <Adafruit_NeoPixel.h>
#include "mqtt.h"
/*
 * Class for managing neopixels
 */
class Ledstrip : public Adafruit_NeoPixel {

    protected:
        enum MQTTParam {
            MQTT_SUB_BRIGHTNESS,
            MQTT_SUB_RGB_SET,
            MQTT_SUB_COMMAND
        };

    public:
        // subtopics that this class is using
        static constexpr const char * MQTT_SUBTOPIC_RGB_COMMAND        = "rgb";
        static constexpr const char * MQTT_SUBTOPIC_BRIGHTNESS_COMMAND = "brightness";
        static constexpr const char * MQTT_SUBTOPIC_RGB_STATE          = "rgb/state";
        static constexpr const char * MQTT_SUBTOPIC_BRIGHTNESS_STATE   = "brightness/state";
        static constexpr const char * MQTT_SUBTOPIC_COMMAND            = "command";
        static constexpr const char * MQTT_SUBTOPIC_STATE              = "state";
        static constexpr const char * MQTT_COMMAND_PAYLOAD_ON          = "ON";
        static constexpr const char * MQTT_COMMAND_PAYLOAD_OFF         = "OFF";

    public:
        Ledstrip();
        void begin();

    protected:
        uint32_t lastColor;
        
        void onMQTT_Brightness (uint8_t *, unsigned int);
        void onMQTT_RGBSet     (uint8_t *, unsigned int);
        void onMQTT_Command    (uint8_t *, unsigned int);

        static void s_onMQTTMessage(const char *, uint8_t *, unsigned int, void * param);
};

extern Ledstrip ledstrip;

/* inlines for Ledstrip */
inline void Ledstrip::s_onMQTTMessage(const char * topic, uint8_t * payload, unsigned int len, void * param) {

    switch ((int) param) {
        case MQTT_SUB_BRIGHTNESS: {
            ledstrip.onMQTT_Brightness(payload, len);
            return;
        }
        case MQTT_SUB_RGB_SET : {
            ledstrip.onMQTT_RGBSet(payload, len);
            return;
        }
        case MQTT_SUB_COMMAND : {
            ledstrip.onMQTT_Command(payload, len);
            return;
        }
    }
}
