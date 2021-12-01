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

#ifndef _BUTTONS_H_
#define _BUTTONS_H_

#include <stdint.h>
#include "scheduler.h"


class Buttons {
    public:
        enum Events {
            EVENT_RELEASE = 0,
            EVENT_PRESS,
            EVENT_LONG_PRESS,
            EVENT_MAX_EVENT
        };
        enum ButtonsIndex {
            BTN_PREV,
            BTN_NEXT,
            BTN_MODEA,
            BTN_MODEB
        };

        typedef bool (* HandlerCallback_t) (Events event, uint8_t btnIdx);

    public:
        Buttons();

        void              begin();
        void              end();

        bool              isPressed(uint8_t btnIdx);
        bool              wasPressed(uint8_t btnIdx);
        HandlerCallback_t setHandler(HandlerCallback_t handler);

    protected:
        void taskUpdateCbk();
        void taskLongPressCbk();

        static void s_taskUpdateCbk();
        static void s_taskLongPressCbk();

        HandlerCallback_t cbkHandler;
        uint8_t stateCurrent;
        uint8_t statePrev;
        Task    taskUpdate;
        Task    taskLongPress;
};

extern Buttons buttons;

/* inlines for Buttons */
inline void Buttons::s_taskUpdateCbk() {
    buttons.taskUpdateCbk();
}

inline void Buttons::s_taskLongPressCbk() {
    buttons.taskLongPressCbk();
}

#endif//_BUTTONS_H_