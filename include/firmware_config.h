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

#ifndef _FIRMWARE_CONFIG_H_
#define _FIRMWARE_CONFIG_H_

/*
 * The options in this file are not configurable via SPIFFS's "config.cfg" file.
 * They are stored hardcoded at compile-time into the firmware binary.
 */

/*
 * Uncomment this to disable serial console colors
 */
//#define CONSOLE_NO_COLORS

/*
 * Define when the UI will display a miflora characteristic as being new (green)
 * warned to be old (yellow) or not updated for too long, stalled (red).
 */
#define UI_UPDATE_TIME_RECENT_MS     20000 // show items updated before this time in green (20 s)
#define UI_UPDATE_TIME_WARNING_MS   120000 // show items updated before this time in yellow (2 min)
#define UI_UPDATE_TIME_STALL_MS    3600000 // show items updated before this time in red (1h)


#endif//_FIRMWARE_CONFIG_H_