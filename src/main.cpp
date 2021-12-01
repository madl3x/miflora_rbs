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

#include "config.h"
#include "xiaomi.h"
#include "ble_tracker.h"
#include "ui.h"
#include "ota.h"
#include "scheduler.h"
#include "buttons.h"
#include "network.h"
#include "dht_sensor.h"
#include "hass.h"
#include "led_strip.h"

#define LOG_TAG LOG_TAG_MAIN
#include "log.h"

/* handler for button events */
bool onButtonsEvent(Buttons::Events event, uint8_t btn);

/* tasks handler */
void taskCyclePagesCbk();

Task taskCyclePages(20 * TASK_SECOND, TASK_FOREVER, taskCyclePagesCbk, &scheduler, true);

/* screens */
DisplayScreen_MiFloraFleet      screenFleet;
DisplayScreen_MiFloraAttributes screenAttrMoist (ATTR_ID_MOISTURE);
DisplayScreen_MiFloraAttributes screenAttrCond  (ATTR_ID_CONDUCTIVITY);
DisplayScreen_MiFloraAttributes screenAttrTemp  (ATTR_ID_TEMPERATURE);
DisplayScreen_MiFloraAttributes screenAttrLight (ATTR_ID_ILLUMINANCE);
DisplayScreen_MiFloraAttributes screenAttrRSSI  (ATTR_ID_RSSI);
DisplayScreen_Station           screenStation;

// for debug only
//DisplayScreen_Config            screenConfig;

int comparePayload(uint8_t * payload, const char * term, unsigned int payload_len) {
  
  auto term_len = (unsigned int) strlen(term);
  if (payload_len != term_len) 
    return -1;
  return strncasecmp((const char *) payload, term, (size_t) payload_len);
}

void onMQTT_ScreenCommand(const char * , uint8_t * payload, unsigned int len, void * param) {

  if (comparePayload(payload, "next", len) == 0) {
    display.next(); 
    return;
  }
  if (comparePayload(payload, "prev", len) == 0) {
    display.prev();
    return;
  }
  if (comparePayload(payload, "next_screen", len) == 0) {
    display.screenNext();
    return;
  }
  if (comparePayload(payload, "prev_screen", len) == 0) {
    display.screenPrev();
    return;
  }
  if (comparePayload(payload, "on", len) == 0 || comparePayload(payload, "off", len) == 0) {
    display.enableBacklight(payload[1] == 'n');
    return;
  }

  if (comparePayload(payload, "station", len) == 0) {
    display.screenSelect(&screenStation);
    return;
  }
}

void onMQTT_WifiCommand(const char *, uint8_t * payload, unsigned int len, void * param) {
  
  if (comparePayload(payload, "disconnect", len) == 0) {
    WiFi.disconnect(true);
    return;
  }
}

void onMQTT_MQTTCommand(const char *, uint8_t * payload, unsigned int len, void * param) {
  
  if (comparePayload(payload, "disconnect", len) == 0) {
    mqtt.end();
    return;
  }
}

void onMQTT_BLECommand(const char *, uint8_t * payload, unsigned int len, void * param) {

  bool ret;
  if (comparePayload(payload, "stopscan", len) == 0) {
    ret = ble.stopScan();
    LOG_F("Stopped BLE scan: %s", ret ? "success" : "failed");
    return;
  }
  if (comparePayload(payload, "startscan", len) == 0) {
    ret = ble.startScan();
    LOG_F("Start BLE scanning: %s", ret ? "success" : "failed");
    return;
  }
}

/*
 * Setting up the main loop task
 */
void setup() {
  bool success;
  
  // initialize serial
  Serial.begin(115200);

  // prepare config fs
  success = DataFS::mount();
  BOOT_PRINT_S(success, "SPIFFS filesystem");

  // load and print configuration
  success = config.load();
  BOOT_PRINT_S(success, "Loading config (#%d)", config.countValues());

    if (success == false) {
      LOG_LN("!!! Failed loading configuration, using compile-time defaults!!");
    }

  // print configuration to serial
  config.print(Serial, "config.cfg");

  // setup display
  display.begin();
  BOOT_PRINT(true, "TFT display");

  // setup buttons
  buttons.begin();
  buttons.setHandler(onButtonsEvent);
  BOOT_PRINT(true, "Buttons");

  // setup DHT sensor
  success = dht.begin();
  BOOT_PRINT(success, "DHT sensor");

  // setup ADC
  pinMode(PIN_ADC_BAT, INPUT);
  BOOT_PRINT(true, "ADC battery");

  // setup neopixels
  ledstrip.begin();
  BOOT_PRINT(true, "NEO ledstrip");

  // load mi flora devices from SPIFFS
  success = fleet.begin("/devices.cfg");
  BOOT_PRINT(success, "Loading devices (#%d)", fleet.count());

  /*
  for (int i = 0 ; i < 20; ++i) {
    auto device = new MiFloraDevice("00:00:00:01:02:03");
    char name[12];
    sprintf(name,"dev %d", i);
    device->setName(name);
    device->setID(10+i);
    device->moisture.set(i,SOURCE_BLE);
    device->RSSI.set(-i,SOURCE_BLE);
    device->temperature.set(i,SOURCE_BLE);
    device->conductivity.set(i*10,SOURCE_BLE);
    fleet.addDevice(device);
  }*/

  // prepare subscriptions for commands
  std::string topic;

  config.formatTopic(topic, ConfigMain::MQTT_TOPIC_COMMAND, "screen");
  mqtt.subscribeTo(topic.c_str()  , onMQTT_ScreenCommand);

  config.formatTopic(topic, ConfigMain::MQTT_TOPIC_COMMAND, "wifi");
  mqtt.subscribeTo(topic.c_str()  , onMQTT_WifiCommand  );

  config.formatTopic(topic, ConfigMain::MQTT_TOPIC_COMMAND, "mqtt");
  mqtt.subscribeTo(topic.c_str()  , onMQTT_MQTTCommand  );
  
  config.formatTopic(topic, ConfigMain::MQTT_TOPIC_COMMAND, "ble");
  mqtt.subscribeTo(topic.c_str()  , onMQTT_BLECommand   );

  //
  // load screens
  //

  // screen that shows a single plant per page
  display.screenAdd(&screenFleet);

  // screen that shows a single attribute for all plants in the fleet
  // when the number of plants exceeds the maximum devices shown per page
  // it creates and additional page for the rest of devices
  display.screenAdd(&screenAttrMoist);
  display.screenAdd(&screenAttrCond);
  display.screenAdd(&screenAttrTemp);
  display.screenAdd(&screenAttrLight);
  display.screenAdd(&screenAttrRSSI);

  // station screen, shows information about the station
  display.screenAdd(&screenStation);

  // for debug only
  //display.screenAdd(&screenConfig);

  // go to the default screen
  display.screenSelect(&screenFleet);
  BOOT_PRINT(true, "Setup display screens");

  // initialize network manager  
  net.begin();
  BOOT_PRINT(true, "WiFi Network");

  // setup OTA
  ota.begin();
  BOOT_PRINT(true, "OTA");

  // setup BLE tracker
  success = ble.begin();
  BOOT_PRINT(success, "BLE tracker");

  // setup HA discovery
  success = hass.begin();
  BOOT_PRINT(success, "HASS Discovery");

  // disable pixels
  ledstrip.clear();
  ledstrip.show();

  BOOT_PRINT(true, "Done!");

  taskCyclePages.setInterval(config.display_cycle_pages_sec * TASK_SECOND);
  taskCyclePages.restartDelayed(config.display_cycle_pages_start_sec * TASK_SECOND);
  display.getUpdateTask().restartDelayed();

  // DONE !

  // some useful information
  display.printf("Flash:%.0fMB SPIFFS:%.0fKB\n",
    (float) ESP.getFlashChipSize()/1024/1024,
    (float) DataFS::getTotalBytes()/1024);

  display.printf("Heap free: %.2fKB", 
    (float) xPortGetFreeHeapSize() /1024);

  // enable watchdog for this task
  enableLoopWDT();
}

void taskCyclePagesCbk() {
  display.next();
}

/*
 * Buttons handler
 */
bool onButtonsEvent(Buttons::Events event, uint8_t btn) {

  bool handled = false;
  DisplayScreen * screen = display.getCurrentScreen();

  // always turn on display (will automatically turn off)
  display.enableBacklight();

  // restart cycle pages delayed after interaction
  taskCyclePages.restartDelayed(config.display_cycle_pages_start_sec * TASK_SECOND);

  // forward the event to the current screen
  // if the event is not handled, continue processing
  if (screen && screen->onButtonEvent(event, btn)) {
    display.getUpdateTask().restart();
    return true;
  }

  // ON short press
  if (event == Buttons::EVENT_PRESS) {

    switch(btn) {

      // move to previous page
      case Buttons::BTN_PREV: {
        display.prev();
        handled = true;
      } break;

      // move to next page
      case Buttons::BTN_NEXT: {
        display.next();
        handled = true;
      } break;

      // show station screen
      case Buttons::BTN_MODEA: {
        display.screenSelect(&screenStation);
        handled = true;
      } break;

      // show moisture screen
      case Buttons::BTN_MODEB: {
        display.screenSelect(&screenAttrMoist);
      } break;
    }

  } else 

  // on release
  if (event == Buttons::EVENT_RELEASE) {
    
    // nothing to do here

  } else

  // on long press
  if (event == Buttons::EVENT_LONG_PRESS) {

    switch(btn) {
      // move to prev screen
      case Buttons::BTN_PREV: {
        display.screenPrev();
        handled = true;
      } break;

      // move to next screen
      case Buttons::BTN_NEXT: {
        display.screenNext();
        handled = true;
      } break;

      // redo HASS discovery
      case Buttons::BTN_MODEA: {
        LOG_LN("Restarting HASS discovery..");
        hass.restart();
      } break;

      // restart
      case Buttons::BTN_MODEB: {
        LOG_LN("Restarting core..");
        ESP.restart();
        handled = true;
      } break;
    }
  }

  return handled;
}

/*
 * Main loop: it simply runs the scheduler
 */
void loop() {
  scheduler.execute();
}
