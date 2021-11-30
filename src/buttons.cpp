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
#include "buttons.h"
#include "config.h"

Buttons buttons;

Buttons::Buttons() : 
    stateCurrent(0), statePrev(0),
    taskUpdate   (100 , TASK_FOREVER, Buttons::s_taskUpdateCbk   ), 
    taskLongPress(2000, TASK_ONCE   , Buttons::s_taskLongPressCbk) {
}

void Buttons::begin() {
    // setup buttons
    pinMode(PIN_BTN_PREV, INPUT_PULLUP);
    pinMode(PIN_BTN_NEXT, INPUT_PULLUP);
    pinMode(PIN_BTN_MODEA, INPUT_PULLUP);
    pinMode(PIN_BTN_MODEB, INPUT_PULLUP);

    // add  button tasks
    scheduler.addTask(taskUpdate);
    scheduler.addTask(taskLongPress);

    taskUpdate.enable();
    taskLongPress.disable();
}

void Buttons::end() {
    scheduler.deleteTask(taskUpdate);
    scheduler.deleteTask(taskLongPress);
}

/* task callback run at 10Hz */
void Buttons::taskUpdateCbk() {

    // backup state
    statePrev = stateCurrent;

    // read states
    stateCurrent = 0;
    if (digitalRead(PIN_BTN_PREV) == LOW) stateCurrent |= 0x1;
    if (digitalRead(PIN_BTN_NEXT) == LOW) stateCurrent |= 0x2;
    if (digitalRead(PIN_BTN_MODEA) == LOW) stateCurrent |= 0x4;
    if (digitalRead(PIN_BTN_MODEB) == LOW) stateCurrent |= 0x8;
    
    // state changed => invoke handler
    if (statePrev != stateCurrent) {

        if (cbkHandler == NULL) 
            return; // nothing to do

        // loop buttons
        for (int i = 0 ; i < 4 ; ++ i) {

            // callback on pressed
            if (isPressed(i) && wasPressed(i) == false) {
                cbkHandler(EVENT_PRESS, i);
            }

            // callback on released
            if (isPressed(i) == false && wasPressed(i)) {
                cbkHandler(EVENT_RELEASE, i);
            }
        }

        // long press detection
        if (stateCurrent == 0) {
            // all buttons released, disable long press task
            taskLongPress.disable();
        } else {
            // one or more buttons pressed, restart long press task
            taskLongPress.restartDelayed();
        }
    }
}

/* task callback run for detecting long presses */
void Buttons::taskLongPressCbk() {
    
    if (cbkHandler == NULL) 
        return; // nothing to do

    // loop buttons
    for (int i = 0 ; i < 4 ; ++ i) {
        if (isPressed(i)) {
            cbkHandler(EVENT_LONG_PRESS, i);
        }
    }
}

/* returns true if a button is pressed, false otherwise */
bool Buttons::isPressed(uint8_t btnIdx) {

    if (stateCurrent & (1<<btnIdx)) 
        return true;
    return false;
}

/* returns true if a button was previously pressed, false otherwise */
bool Buttons::wasPressed(uint8_t btnIdx) {

    if (statePrev & (1<<btnIdx)) 
        return true;
    return false;
}

/* installs a handler and returns the previous one */
Buttons::HandlerCallback_t Buttons::setHandler(HandlerCallback_t handler) {
    
    HandlerCallback_t old_handler = cbkHandler;
    cbkHandler = handler;
    return old_handler;
}