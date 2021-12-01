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
#include <ArduinoOTA.h>
#include "ota.h"
#include "ui.h"

#define LOG_TAG LOG_TAG_OTA
#include "log.h"

OTA ota;

OTA::OTA() : 
  taskHandle(500, TASK_FOREVER, s_TaskOTACbk, &scheduler, false), 
  running(false) {

}

/* OTA init */
void OTA::begin() {
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("flora");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      LOG_LN("Start updating " + type);
      ota.running = true;
      delay(100);

      // stop scheduler and watchdog
      scheduler.pause();
      disableLoopWDT();

      // prepare display
      display.fillScreen(Display_Background_Color);
      display.setCursor(0,0);
      display.setTextSize(1);
      display.setTextColor(Display_Text_Color);
      display.print("OTA update (");
      display.print(type);
      display.println("):");
    })
    .onEnd([]() {
      LOG_LN("Done!");
      ota.running = false;

      // re-enable scheduler and watchdog
      scheduler.resume();
      enableLoopWDT();
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      LOG_PART_START_F("Progress: %u%%\r", (progress / (total / 100)));

      // print progress on display
      display.fillRect(0,20, (long)display.width() * progress / total, 10, Display_Color_Green);
      display.fillRect(0,35,20,10, Display_Background_Color);
      display.setCursor(0,35);
      display.print((int)(progress / (total / 100)));
      display.print("%");
    })
    .onError([](ota_error_t error) {

      LOG_F("Error[%u]: ", error);
      
      if (error == OTA_AUTH_ERROR) { LOG_LN("Auth Failed"); }
      else if (error == OTA_BEGIN_ERROR) { LOG_LN("Begin Failed"); }
      else if (error == OTA_CONNECT_ERROR) { LOG_LN("Connect Failed"); }
      else if (error == OTA_RECEIVE_ERROR) { LOG_LN("Receive Failed"); }
      else if (error == OTA_END_ERROR) { LOG_LN("End Failed"); }

      display.fillRect(0,35,20,10, Display_Background_Color);
      display.setTextColor(Display_Color_Red);
      display.setCursor(0,35);
      display.print("OTA failed, code:");
      display.print(error);
      display.println("!");
      
      ota.running = false;

      // resume scheduler and watchdog
      scheduler.resume();
      enableLoopWDT();
    });

  // initialize OTA
  ArduinoOTA.begin();
  running = false;

  // start task
  taskHandle.enable();
}

void OTA::s_TaskOTACbk() {
  ArduinoOTA.handle();
}