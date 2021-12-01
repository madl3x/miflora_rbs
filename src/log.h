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

#ifndef _LOG_H_
#define _LOG_H_

#include <firmware_config.h>

/*
 * Console colors
 */
#ifndef CONSOLE_NO_COLORS
  #define CONSOLE_BLACK    "\033[30m"
  #define CONSOLE_RED      "\033[31m"
  #define CONSOLE_GREEN    "\033[32m"
  #define CONSOLE_YELLOW   "\033[33m"
  #define CONSOLE_BLUE     "\033[34m"
  #define CONSOLE_MAGENTA  "\033[35m"
  #define CONSOLE_CYAN     "\033[36m"
  #define CONSOLE_WHITE    "\033[37m"
  #define CONSOLE_RESET    "\033[0m"
#else 
  #define CONSOLE_BLACK
  #define CONSOLE_RED
  #define CONSOLE_GREEN
  #define CONSOLE_YELLOW
  #define CONSOLE_BLUE  
  #define CONSOLE_MAGENTA
  #define CONSOLE_CYAN
  #define CONSOLE_WHITE
  #define CONSOLE_RESET
#endif

/*
 * Predefined logging} tags
 */
#define LOG_TAG_MAIN     "[" CONSOLE_GREEN   "  MAIN" CONSOLE_RESET "] "
#define LOG_TAG_MQTT     "[" CONSOLE_CYAN    "  MQTT" CONSOLE_RESET "] "
#define LOG_TAG_BLE      "[" CONSOLE_BLUE    "   BLE" CONSOLE_RESET "] "
#define LOG_TAG_FLORA    "[" CONSOLE_YELLOW  " FLORA" CONSOLE_RESET "] "
#define LOG_TAG_FLEET    "[" CONSOLE_MAGENTA " FLEET" CONSOLE_RESET "] "
#define LOG_TAG_HASS     "[" CONSOLE_MAGENTA "  HASS" CONSOLE_RESET "] "
#define LOG_TAG_OTA      "[" CONSOLE_MAGENTA "   OTA" CONSOLE_RESET "] "
#define LOG_TAG_NETWORK  "[" CONSOLE_MAGENTA "   NET" CONSOLE_RESET "] "
#define LOG_TAG_DHT      "[" CONSOLE_GREEN   "   DHT" CONSOLE_RESET "] "
#define LOG_TAG_LEDSTRIP "[" CONSOLE_GREEN   "   NEO" CONSOLE_RESET "] "
#define LOG_TAG_DISPLAY  "["                 "  DISP"               "] "
#define LOG_TAG_CONFIG   "["                 "   CFG"               "] "

/*
 * Boot-time printing
 */
#define BOOT_PRINT_S(success, fmt, args...) \
  Serial.print(CONSOLE_RESET "["); \
  if (success) { Serial.print(CONSOLE_GREEN "   OK "); } \
  else         { Serial.print(CONSOLE_RED   " FAIL "); } \
  Serial.print(CONSOLE_RESET "] "); \
  Serial.printf(fmt "\r\n", ##args);

#define BOOT_PRINT_D(success, fmt, args...) \
  if (success) { display.setTextColor(Display_Color_Green); display.print("OK  "); } \
  else         { display.setTextColor(Display_Color_Red  ); display.print("NOK "); } \
  display.setTextColor(Display_Text_Color); \
  display.printf(fmt "\n", ##args); \
  display.setCursor(0, display.getCursorY()+2);

#define BOOT_PRINT(success, fmt, args...) \
  BOOT_PRINT_D(success, fmt, ##args) \
  BOOT_PRINT_S(success, fmt, ##args)

/* default stream used for logging */
#define LOG_STREAM Serial

/* Support for logging with tags */
#define LOG_(TAG, message)            { LOG_STREAM.print(TAG); LOG_STREAM.print(message); }
#define LOG_F_(TAG, message, args...) { LOG_STREAM.printf(TAG message, ##args); }
#define LOG_LN_(TAG, message)         { LOG_STREAM.print(TAG); LOG_STREAM.println(message); }

/* Default logging macros */
#ifndef LOG_TAG
    #error Define a log tag!
#endif 

#define LOG_PART_START(message) LOG_(LOG_TAG, message)
#define LOG_PART(message) LOG_STREAM.print(message)
#define LOG_PART_HEX(message) LOG_STREAM.print(message, HEX)
#define LOG_PART_END(message) LOG_STREAM.println(message)

#define LOG_PART_START_F(message, args...) LOG_F_(LOG_TAG, message, ##args)

#define LOG_F(message, args...) LOG_F_(LOG_TAG, message "\r\n", ##args)
#define LOG_LN(message) LOG_LN_(LOG_TAG, message)

#endif//_LOG_H_