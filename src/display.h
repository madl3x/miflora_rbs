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

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <Adafruit_ST7735.h>
#include <list>
#include <stdint.h>

#include "scheduler.h"
#include "buttons.h"

/* standard color definitions */
const uint16_t  Display_Color_Black        = 0x0000;
const uint16_t  Display_Color_Blue         = 0x001F;
const uint16_t  Display_Color_Red          = 0xF800;
const uint16_t  Display_Color_Orange       = 0xFA60;
const uint16_t  Display_Color_Green        = 0x07E0;
const uint16_t  Display_Color_Lime         = 0x07FF;
const uint16_t  Display_Color_Cyan         = 0x07FF;
const uint16_t  Display_Color_Aqua         = 0x04FF; 
const uint16_t  Display_Color_Magenta      = 0xF81F;
const uint16_t  Display_Color_Yellow       = 0xFFE0;
const uint16_t  Display_Color_White        = 0xFFFF;
const uint16_t  Display_Color_Gray         = 0x8410; 


// The colors we actually want to use
const uint16_t  Display_Text_Color         = Display_Color_White;
const uint16_t  Display_Background_Color   = Display_Color_Black;


/*
 * Abstract class defining a screen interface
 */
class DisplayScreen {
  public:
  
    DisplayScreen();

    /* Draw routine for the current page */
    virtual void     update() = 0;

    /* Each screen can have one ore multiple pages to show */
    virtual uint16_t pageCount() = 0;
    virtual uint16_t pageNext() = 0;
    virtual uint16_t pagePrev() = 0;
    virtual bool     pageSelect(uint16_t page) = 0;
    virtual uint16_t pageIndex() = 0;

    /* Disabled screens will not be visible when switching screens */
    virtual void     enable(bool e);
    bool             isEnabled();

    /* Screen change handler, invoked when display switches screens */
    virtual void     onEnter();
    virtual void     onLeave();

    /* Event handlers, return false if event was not handled */
    virtual bool     onButtonEvent(Buttons::Events event, uint8_t button);

  protected:
    bool enabled;
};

/*
 * Simplifying interface for screens with only one page
 */
class DisplayScreenSinglePage : public DisplayScreen {
  public:
    uint16_t pageCount()   { return 1; }
    uint16_t currentPage() { return 0; }
    uint16_t pageNext()    { return 0; }
    uint16_t pagePrev()    { return 0; }
    bool     pageSelect(uint16_t page) { return page == 0;}
    uint16_t pageIndex()   { return 0; }
};

/*
 * Display class 
 */
class Display : public Adafruit_ST7735 {
//#include <Adafruit_GFX.h>
//class Display : public GFXcanvas8 {
  public:
    typedef std::list<DisplayScreen *> ScreensList_t;

  public:
    Display(int8_t cs, int8_t dc, int8_t rst);

    void     begin();
    void     enableBacklight(bool on = true, bool auto_turn_off = true);
    bool     isBacklightOn();

    void     screenAdd(DisplayScreen *);
    void     screenRemove(DisplayScreen *);
    uint16_t screenCount();

    void     screenSelect(DisplayScreen * screen, bool update = true, ScreensList_t::iterator * position = NULL);
    void     screenNext(bool selectFirstPage = true, bool update = true);
    void     screenPrev(bool selectLastPage = true, bool update = true);
    void     screenUpdate();
    void     next(bool update = true);
    void     prev(bool update = true);
    void     refresh();

    DisplayScreen * getCurrentScreen();
    Task &          getUpdateTask();
    Task &          getBacklightTask();

  protected:
    Adafruit_ST7735 tft;
    Task taskUpdate;
    Task taskBacklight;
    ScreensList_t screens;
    ScreensList_t::iterator currentScreen;

    void taskUpdateCbk();
    void taskBacklightCbk();

    static void s_taskUpdateCbk();
    static void s_taskBacklightCbk();
};

extern Display display;

/* inlines for DisplayScreen */
inline DisplayScreen::DisplayScreen() : enabled (true) {
}

inline void DisplayScreen::enable(bool e){
    enabled = e;
}

inline bool DisplayScreen::isEnabled() {
    return enabled;
}

inline void DisplayScreen::onEnter() {
}

inline void DisplayScreen::onLeave() {
}

inline bool DisplayScreen::onButtonEvent(Buttons::Events event, uint8_t button) {
  return false; // event not interpreted
}

/* inlines for Display */
inline Task & Display::getUpdateTask() {
  return taskUpdate;
}

inline Task & Display::getBacklightTask() {
  return taskBacklight;
}

inline void Display::s_taskBacklightCbk() {
    display.taskBacklightCbk();
}

inline void Display::s_taskUpdateCbk() {
  display.taskUpdateCbk();
}

#endif//_DISPLAY_H_