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

#include "ui.h"
#include "config.h"
#include <Arduino.h>

/*
 * Progress bar}
 */
ProgressBar::ProgressBar() {
    const int x_right_margin = 5;
    const int x_left_margin = 35;
    
    percentage = 0;
    x_off = x_left_margin;
    y_off = 22;
    y_bar_height = 14;
    x_bar_width  = display.width() - x_left_margin - x_right_margin;
    color = Display_Color_Cyan;
    color_label = Display_Color_White;
    color_value_light = Display_Color_White;
    color_value_dark  = Display_Color_Black;
    value = NULL;
    label = NULL;
}

void ProgressBar::draw() {
  
  int bar_width;
  const int char_width = 6;
  const int char_height = 6;
  const int x_text_margin = 3;

  display.setTextSize(1);

  // text upper offset (aligns text to the middle of the bar)
  const int y_text_off = (y_bar_height - char_height) / 2;

  // cap values
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;

  // draw bar
  bar_width = map(percentage, 0, 100, 0, x_bar_width);
  display.fillRect(x_off, y_off, bar_width, y_bar_height, color);

  // draw label
  if (label) {
    display.setTextColor(color_label);
    display.setCursor(x_off - x_text_margin - strlen(label) * char_width, y_off + y_text_off);
    display.print(label);
  }

  // draw value
  if (value) {
    int x_pos = x_off + bar_width - x_text_margin - char_width * strlen(value);
    
    if (x_pos < x_off + x_text_margin) {
      x_pos = x_off + bar_width + x_text_margin;
      display.setTextColor(color_value_light);
    } else {
      display.setTextColor(color_value_dark);
    }
    
    display.setCursor(x_pos, y_off + y_text_off);
    display.print(value);
  }
}

void ProgressBar::drawAttribute(MiFloraDevice * device, DeviceAttribute * attr, const char * given_label) {

    char value_str[32];
    int val;
    unsigned long lastUpdated = millis() - attr->lastUpdated();
    
    // green  = recently updated (20 seconds)
    // yellow = not updated in a while (2 minutes)
    // white  = updated
    // red    = no value is present or not updated for one hour
    
    color_label = 
      attr->hasValue() == false               ? Display_Color_Red    :
      lastUpdated < UI_UPDATE_TIME_RECENT_MS  ? (attr->getSource() == SOURCE_BLE ? Display_Color_Green : Display_Color_Cyan) : 
      lastUpdated < UI_UPDATE_TIME_WARNING_MS ? Display_Color_White  : 
      lastUpdated < UI_UPDATE_TIME_STALL_MS   ? Display_Color_Yellow :  Display_Color_Red;

    value = value_str;
    value_str[0] = '\0';

    // set label
    if (given_label == NULL) {
      label = attr->getLabel();
    } else {
      label = given_label;
    }

    // set value
    if (attr->hasValue()) {
        
      // moisture
      if (attr == &device->moisture) {
        val = device->moisture.get();
        percentage = map(val, 0, 80, 0, 100);
        sprintf(value_str, "%u%%", device->moisture.getUInt());
      } else
  
      // temperature
      if (attr == &device->temperature) {
        val = device->temperature.get();
        percentage = map(val, 0, 50, 0, 100);
        sprintf(value_str, "%.1fC", device->temperature.get());
      } else
  
      // light
      if (attr == &device->illuminance) {
        val = device->illuminance.get();
        percentage = map(val, 0, 5000, 0, 100);
        sprintf(value_str, "%ulu", device->illuminance.getUInt());
      } else
  
      // conductivity
      if (attr == &device->conductivity) {
        val = device->conductivity.get();
        percentage = map(val, 0, 2000, 0, 100);
        sprintf(value_str, "%uus", (int) device->conductivity.getUInt());
      } else
  
      // RSSI
      if (attr == &device->RSSI) {
        val = device->RSSI.get();
        percentage = map(val, -100, -50, 0, 100);
        sprintf(value_str, "%ddb", device->RSSI.getInt());
      }

      int restore_color = color;
      int restore_color_value_light = color_value_light;
      
      // if attribute exceeds recommended limits show background color in orange
      if (attr->inLimits() == false) {
        color = Display_Color_Orange;
        color_value_light = Display_Color_Orange;
      }

      // draw progress bar
      draw();

      // restore colors
      color = restore_color;
      color_value_light = restore_color_value_light;

    } else {
      // attribute never updated 
      percentage = 0;
      strcpy(value_str, "n/a");

      // draw progress bar
      draw();
    }
    
    // reset value and label
    value = NULL;
    label = NULL;
}

/*
 * Screen for showing MiFlora attributes
 */
DisplayScreen_MiFloraFleet::DisplayScreen_MiFloraFleet() 
  : index (0) {

}

/* from DisplayScreen interfce */
void DisplayScreen_MiFloraFleet::update() {

  MiFloraDevice * device = fleet.atIndex(index);
  if (device == NULL) {

    // this should not happen
    Serial.printf("Failed: no device at index %u", index);
    
    // try again
    if (index != 0) {
      index = 0;
      update();
    }
    return;
  }

  updateFor(device);
}

void DisplayScreen_MiFloraFleet::updateFor(MiFloraDevice * device) {
  
  ProgressBar bar;
  int w = display.width();

  display.fillScreen(Display_Background_Color);
  display.setCursor(0,0);
  display.setTextSize(1);

  // print device address
  display.setTextColor(Display_Text_Color);
  display.print("[");
  display.print(device->getID());
  display.print("] ");

  display.setTextSize(2);
  display.setTextWrap(false);
  display.println(device->getName().c_str());
  display.setTextSize(1);

  // active circle
  unsigned long lastUpdated = millis() - device->lastUpdated();
  int color =
      lastUpdated < UI_UPDATE_TIME_RECENT_MS  ? Display_Color_Green  : 
      lastUpdated < UI_UPDATE_TIME_WARNING_MS ? Display_Color_White  : 
      lastUpdated < UI_UPDATE_TIME_STALL_MS   ? Display_Color_Yellow :  Display_Color_Red;

  display.fillCircle(w-5, 3, 2, color);

  // draw progress bars
  bar.drawAttribute(device, &device->moisture);
  bar.moveDown();
  bar.drawAttribute(device, &device->temperature);
  bar.moveDown();
  bar.drawAttribute(device, &device->conductivity);
  bar.moveDown();
  bar.drawAttribute(device, &device->illuminance);
  bar.moveDown();
  bar.drawAttribute(device, &device->RSSI);
  bar.moveDown();

  // print address  
  display.setCursor(5, display.height()-10);
  display.setTextColor(Display_Color_Gray);
  display.print(device->getAddress().c_str());

  // print last seen
  char seen[12];
  if (lastUpdated < UI_UPDATE_TIME_RECENT_MS) {
    strcpy(seen, "now");
  } else
  if (lastUpdated < 60000) {
    sprintf(seen, "%u sec", (unsigned int) ( lastUpdated/1000));
  } else
  if (lastUpdated < 3600000) {
    sprintf(seen, "%u min", (unsigned int) ( lastUpdated/(60*1000)));
  } else 
  if (lastUpdated < 3600000 * 24 * 2) {
    sprintf(seen, "%u h", (unsigned int) ( lastUpdated/(60*60*1000) ));
  } else {
    sprintf(seen, "%u days", (unsigned int) ( lastUpdated/(24*60*60*1000) ));
  }

  display.setCursor(display.width() - (strlen(seen) * 6 + 5), display.height()-10);
  display.setTextColor(Display_Color_Gray);
  display.print(seen);
}

/*
 * Screen for showing all attributes
 */
DisplayScreen_MiFloraAttributes::DisplayScreen_MiFloraAttributes(AttributeID ID)
  : attributeID(ID)
  , pageIdx(0)
  {}


void DisplayScreen_MiFloraAttributes::update() {

  ProgressBar     bar;
  uint16_t        firstIndex  = pageIdx * MAX_DEVICES_PER_PAGE;
  MiFloraDevice * firstDevice = fleet.atIndex(firstIndex);

  // something gone wrong
  if (firstDevice == NULL) {
    return;
  }

  bar.x_off = 12 * 6;
  bar.x_bar_width = display.width() - bar.x_off - 5;

  // TODO: implement what to show when the fleet is empty
  if (fleet.count() == 0) {
    display.fillScreen(Display_Background_Color);
    display.setCursor(0,0);
    display.setTextSize(2);
    display.print("N/A");
    return;
  }

  display.fillScreen(Display_Background_Color);
  display.setCursor(0,0);
  display.setTextSize(1);

  // print device address
  display.setTextColor(Display_Text_Color);
  display.print("[");
  display.print((int) attributeID);
  display.print("] ");

  // print label
  display.setTextSize(2);
  display.setTextWrap(false);
  display.print(firstDevice->attributeByID(attributeID)->getLabel());
  //display.println(AttributeLabel[attribute]);
  display.setTextSize(1);
  
  // draw progress bars (no more than 5)
  for (int i = firstIndex; i < fleet.count() && i < firstIndex + MAX_DEVICES_PER_PAGE; ++ i) {
      auto device = fleet.devices()[i];
      DeviceAttribute * device_attr;

      // get device attribute
      device_attr = device->attributeByID(attributeID);

      // something gone wrong
      if (device_attr == NULL )
        return;

      bar.drawAttribute(device, device_attr, device->getName().c_str());
      bar.moveDown();
  }
}

/*
 * Screen for showing DHT sensor data
 */ 
#include <WiFi.h>
#include "network.h"
#include "ble_tracker.h"
#include "dht_sensor.h"

void DisplayScreen_Station::update() {
  
  ProgressBar bar;
  char value[32];

  display.fillScreen(Display_Background_Color);
  display.setCursor(0,0);

  display.setTextSize(1);
  display.setTextColor(Display_Text_Color);
  display.print("    ");

  display.setTextSize(2);
  display.print("Station");

  // DHT temperature
  bar.label = "Temp";
  sprintf(value, "%.1fC", dht.temperature());
  bar.value = value;
  bar.percentage = map((long) dht.temperature(), 0, 50, 0, 100);
  bar.draw();
  bar.moveDown();

  // DHT humidity
  bar.label = "Hum";
  sprintf(value, "%u%%", dht.humidity());
  bar.percentage = (long) dht.humidity();
  bar.draw();
  bar.moveDown();

  // battery level
  bar.label = "Bat";
  auto mV = analogReadMilliVolts(PIN_ADC_BAT) * 2;
  sprintf(value, "%.2fV", (float) mV / 1000);
  bar.percentage = map(mV, 2500, 4200, 0, 100);
  bar.draw();
  bar.moveDown();
  
  // Wifi signal
  bar.label = "WiFi";
  if (WiFi.isConnected()) {
    sprintf(value, "%ddb", WiFi.RSSI());
    bar.percentage = map(WiFi.RSSI(), -100, -30, 0, 100);
  } else {
    strcpy(value, "disconnected");
    bar.color_value_light = Display_Color_Red;
    bar.percentage = 0;
  }
  bar.percentage = map(WiFi.isConnected() ? WiFi.RSSI() : -100, -100, -30, 0, 100);
  bar.draw();
  bar.moveDown();

  // MQTT
  display.setCursor(10, bar.y_off+3);
  display.setTextColor(Display_Text_Color);
  display.print("MQTT");
  display.setCursor(display.getCursorX() + 5, display.getCursorY());
  display.fillCircle(display.getCursorX(), display.getCursorY() + 3, 3, 
    mqtt.connected() ? Display_Color_Green : Display_Color_Red );

  // BLE
  display.setCursor(display.getCursorX() + 10, display.getCursorY());
  display.print("BLE");
  display.setCursor(display.getCursorX() + 5, display.getCursorY());
  display.fillCircle(display.getCursorX(), display.getCursorY() + 3, 3, 
    ble.isScanTaskStarted() == false ? Display_Color_Red : 
    ble.isScanningNow()     == false ? Display_Color_Green : Display_Color_Cyan );

  display.setCursor(display.getCursorX() + 8, display.getCursorY()-1);
  if (ble.isScanningNow()) {
    display.setTextColor(Display_Color_Cyan);
    display.print("scanning");
  }
 
  // print current IP address
  display.setTextColor(Display_Color_Gray);
  if (WiFi.isConnected()) {
    String ip = WiFi.localIP().toString();

    display.setCursor(5, display.height()-10);  
    display.print(WiFi.localIP().toString().c_str());
  }

  // print firmware version
  sprintf(value, "vs%s", VERSION_STR);
  display.setCursor(display.width() - (strlen(value) * 6 + 5), display.height()-10);  
  display.print(value);
}

/*
 * Screen for showing  configuration data
 */ 
DisplayScreen_Config::DisplayScreen_Config()
 : startIndex(0) {
}

bool DisplayScreen_Config::onButtonEvent(Buttons::Events event, uint8_t button) {

  // not interested in other events
  if (event != Buttons::EVENT_PRESS)
    return false;
  
  // scroll up
  if (button == Buttons::BTN_MODEA) {
    if (startIndex) -- startIndex;
    display.refresh();
    return true;
  }

  // scroll down
  if (button == Buttons::BTN_MODEB) {
    ++ startIndex;
    display.refresh();
    return true;
  }
  
  return false;
}

void DisplayScreen_Config::update() {

  // prepare
  display.setCursor(0,0);
  display.setTextSize(1);
  display.fillScreen(Display_Background_Color);

  int maxLines = display.height() / 8;
  char section[32], current_section[32];

  section[0] = '\0';
  current_section[0] = '\0';

  // print config
  for (int i = startIndex; i < config.keys().size() && maxLines > 0; ++ i, maxLines -= 2) {

    auto key = config.keys()[i].c_str();
    auto value = config.get(key);

    // extract section
    const char * delim = strchr(key,':');

    // should not happen
    if (delim == NULL) 
      continue;
    if (delim - key > sizeof(section) - 1)
      continue;

    // extract section
    strncpy(section, key, delim-key);
    section[delim-key] = '\0';

    // strip section from key name
    key = delim + 1;

    // if section has changed, print it out
    if (strcmp(current_section, section) != 0) {
      display.setTextColor(Display_Color_Green);
      display.printf("[%s]\n", section);

      // reduce the number of maximum line printed
      -- maxLines;

      // store current section
      strcpy(current_section, section);
    }

    // obfuscate passwords
    if (strcasecmp(key,"password") == 0 and strlen(value) > 0)
      value = "****";

    // print key and value
    display.setTextColor(Display_Color_Cyan);
    display.printf("%s\n", key);
    display.print(" = ");
    display.setTextColor(Display_Text_Color);
    display.printf("%s\n", value);
  }
}