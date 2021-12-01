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

#include <algorithm>
#include "display.h"
#include "config.h"

#define LOG_TAG LOG_TAG_DISPLAY
#include "log.h"

Display display(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

/*
 * Display
 */
Display::Display(int8_t cs, int8_t dc, int8_t rst)
    : Adafruit_ST7735(cs, dc, rst)
    //: GFXcanvas8 (180, 128)
    , tft(cs, dc, rst)
    , taskUpdate    (0, TASK_FOREVER, Display::s_taskUpdateCbk   , &scheduler, false)
    , taskBacklight (0, TASK_ONCE,    Display::s_taskBacklightCbk, &scheduler, false) {
    currentScreen = screens.end();
    
}

void Display::begin() {
  LOG_LN("Initialize display..");

  // initialize TFT display
  initR(INITR_BLACKTAB);
  setRotation(3);
  enableDisplay(true);

  // prepare drawing
  setFont();
  fillScreen(Display_Background_Color);
  setTextColor(Display_Text_Color);
  setTextSize(1);
  setCursor(0,0);  

  // turn on display
  pinMode(PIN_TFT_BACKLIGHT, OUTPUT);
  enableBacklight();

  // start update
  taskUpdate.setInterval(config.display_refresh_sec * TASK_SECOND);
  taskUpdate.restart();

  // automatically turn off display
  taskBacklight.setInterval(config.display_backlight_off_sec * TASK_SECOND);
  taskBacklight.restartDelayed();
}

void Display::enableBacklight(bool on, bool auto_turn_off) {

  digitalWrite(PIN_TFT_BACKLIGHT, on ? HIGH : LOW);

  // turn off automatically
  if (on && auto_turn_off) {
      taskBacklight.restartDelayed();
  }
}

bool Display::isBacklightOn() {
  return digitalRead(PIN_TFT_BACKLIGHT);
}

void Display::screenAdd(DisplayScreen * screen) {

    // check input
    if (screen == NULL) return;

    // add screen
    screens.push_back(screen);

    // if we don't have a current screen select, select this one
    if (getCurrentScreen() == NULL) {
        screenSelect(screen, true);
    }
}

void Display::screenRemove(DisplayScreen * screen) {

    // check input
    if (screen == NULL) return;

    // if we are removing the current screen, move to next
    if (getCurrentScreen() == screen)
        screenNext(true, false);

    // if screenNext() didn't switch to anything, notify screen about leaving
    if (getCurrentScreen() == screen)
        screen->onLeave();

    // do remove
    screens.remove(screen);
    if (screens.size() == 0) {
        currentScreen = screens.end();
    }
}

void Display::screenNext(bool selectFirstPage, bool update) {
    // no screens available
    if (screens.size() == 0) return;

    ScreensList_t::iterator itScreen = currentScreen;
    DisplayScreen * screenNew = NULL;

    // move next until we find a screen that is enabled
    // note: protected against infinite loop
    size_t times = screens.size();
    while (times --) {
        // move to the next screen
        ++ itScreen;
        if (itScreen == screens.end()) {
            itScreen = screens.begin();
        }
        // stop when an enabled screen is found
        if ((*itScreen)->isEnabled()) {
            screenNew = *itScreen;
            break;
        }
    }

    // no screen found
    if (screenNew == NULL)
        return;

    // select screen
    screenSelect(screenNew, update, &itScreen);
    
    // when moving backward, select the last page of the prev screen
    if (selectFirstPage) 
        screenNew->pageSelect(0);
}

void Display::screenPrev(bool selectLastPage, bool update) {
    // no screens available
    if (screens.size() == 0) return;

    ScreensList_t::iterator itScreen = currentScreen;
    DisplayScreen * screenNew = NULL;

    // move to previous screens until we find one that is enabled
    // note: protected against infinite loop
    size_t times = screens.size();
    while (times --) {
        // move to the prev screen
        if (itScreen == screens.begin()) {
            itScreen = screens.end();
        }
        -- itScreen;
        // stop when an enabled screen is found
        if ((*itScreen)->isEnabled()) {
            screenNew = *itScreen;
            break;
        }
    }

    // no screen found
    if (screenNew == NULL)
        return;

    // select screen
    screenSelect(screenNew, update, &itScreen);
    
    // when moving backward, select the last page of the prev screen
    if (selectLastPage) 
        screenNew->pageSelect((*currentScreen)->pageCount() - 1);
}

void Display::screenSelect(DisplayScreen * screenNew, bool update, ScreensList_t::iterator * itScreen) {

    // disabled screens can't be selected
    if (screenNew == NULL || screenNew->isEnabled() == false) 
        return;

    // get previous screen
    DisplayScreen * screenOld = getCurrentScreen();

    // we are switching to the same scren
    if (screenOld == screenNew) 
        return;
    
    // if the position is not given, search the screen by address
    if (itScreen == NULL) {

        auto it = std::find(screens.begin(), screens.end(), screenNew);
        if (it == screens.end())
            return; // not found (?!)

        // repair iterator
        itScreen = &it;
    }

    // select screen
    currentScreen = *itScreen;

    // notify screens about the switch
    if (screenOld) screenOld->onLeave();
    screenNew->onEnter();

    // update now
    if (update) refresh();
}

void Display::next(bool update) {

    if (currentScreen == screens.end()) 
        return;

    DisplayScreen * screen = * currentScreen;

    // attempt to shift pages first
    if (screen->pageIndex() < screen->pageCount() - 1) {
        screen->pageNext();
    } else {

        // no more pages, shift screen
        screenNext(true, false);

        // no more screens, reset pages
        if (screen == getCurrentScreen()) {
            screen->pageSelect(0);
        }
    }

    // update now
    if (update) refresh();
}

void Display::prev(bool update) {

   if (currentScreen == screens.end()) 
        return;

    DisplayScreen * screen = * currentScreen;

    // attempt to shift pages first
    if (screen->pageIndex() > 0) {
        screen->pagePrev();
    } else {
        // no more pages, shift screens
        screenPrev(true, false);

        // single screen => reset pages
        if (screen == getCurrentScreen()) {
            screen->pageSelect(screen->pageCount() - 1);
        }
    }

    // update now
    if (update) refresh();
}

void Display::screenUpdate() {
    
    auto screen = getCurrentScreen();
    if (screen) screen->update();
}

void Display::refresh() {
    taskUpdate.restart();
}

DisplayScreen * Display::getCurrentScreen() {
    if (currentScreen == screens.end()) 
        return NULL;
    
    return (*currentScreen);
}

uint16_t Display::screenCount() {
    return (uint16_t) screens.size();
}

void Display::taskUpdateCbk() {

    // screen will draw on canvas
    screenUpdate();
}

void Display::taskBacklightCbk() {
    enableBacklight(false);
}
