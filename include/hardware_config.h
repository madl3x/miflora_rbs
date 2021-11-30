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

#ifndef _HARDWARE_CONFIG_H_
#define _HARDWARE_CONFIG_H_
/*
 * The options in this file define the hardare configuration for your station.
 * Modify this file if you use other GPIO pins to connect your peripherals.
 */

/* 
 * Display connection 
 */
#define PIN_TFT_CS          13
#define PIN_TFT_RST          5
#define PIN_TFT_DC          21
#define PIN_TFT_BACKLIGHT   15

/* 
 * GPIO buttons
 */
#define PIN_BTN_PREV         0 
#define PIN_BTN_NEXT        27
#define PIN_BTN_MODEA       26
#define PIN_BTN_MODEB       25

/* 
 * DHT humidity and temperature sensor
 */
#define PIN_DHT              4
#define DHT_TYPE         DHT22

/* 
 * Neopixel
 */
#define PIN_NEOPIXEL        22
#define NEOPIXEL_CNT         2
#define NEOPIXEL_TYPE  (NEO_GRB + NEO_KHZ800)

/*
 * ADC sensors
 */
#define PIN_ADC_BAT         A0

#endif//_HARDWARE_CONFIG_H_