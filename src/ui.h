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

#ifndef _UI_H_
#define _UI_H_

#include "device.h"
#include "config.h"
#include "scheduler.h"
#include "display.h"

/*
 * Horizontal progress bar widget
 */
class ProgressBar {

  public:
    ProgressBar();

    void draw();
    void drawAttribute(MiFloraDevice * device, DeviceAttribute * attr, const char * label = NULL);
    void moveDown(int space=5);

  public:
    int x_off, y_off;
    int y_bar_height, x_bar_width;
    int color, color_value_light, color_value_dark, color_label;
    int percentage;
    const char * label, * value;
};

/*
 * Screen for showing MiFlora attributes
 */
class DisplayScreen_MiFloraFleet : public DisplayScreen {
  public:

    DisplayScreen_MiFloraFleet();

    /* from DisplayScreen */
    void     update();
    uint16_t pageCount();
    uint16_t pageNext();
    uint16_t pagePrev();
    uint16_t pageIndex();
    bool     pageSelect(uint16_t page);

  protected:
    void updateFor(MiFloraDevice * device);

    uint8_t index;
};

/*
 * Screen for showing all attributes
 */
class DisplayScreen_MiFloraAttributes : public DisplayScreen {

  private:
    const uint8_t MAX_DEVICES_PER_PAGE = 5;

  public:

    DisplayScreen_MiFloraAttributes(AttributeID ID);

    /* from DisplayScreen */
    void     update();
    uint16_t pageCount();
    uint16_t pageNext();
    uint16_t pagePrev();
    uint16_t pageIndex();
    bool     pageSelect(uint16_t page);

  protected:
    AttributeID attributeID;
    uint16_t    pageIdx;
};

/*
 * Screen for showing station sensor data
 */ 
class DisplayScreen_Station : public DisplayScreenSinglePage {
  void update();
};
/*
 * Screen for showing  configuration data
 */ 
class DisplayScreen_Config : public DisplayScreenSinglePage {
  public:
    DisplayScreen_Config();
    void     update();
    bool     onButtonEvent(Buttons::Events event, uint8_t button);
  protected:
    int startIndex;
};

/* inlines for ProgressBar */
inline void ProgressBar::moveDown(int space) {
  y_off += y_bar_height + space;
}

/* inlines for DisplayScreen_MiFloraFleet */
inline uint16_t DisplayScreen_MiFloraFleet::pageCount() {
  return fleet.count();
}
inline uint16_t DisplayScreen_MiFloraFleet::pageNext() {
  index = (index + 1) % fleet.count();
  return index;
}
inline uint16_t DisplayScreen_MiFloraFleet::pagePrev() {
  if (index == 0) index = fleet.count() - 1;
  index = index - 1;
  return index;
}
inline uint16_t DisplayScreen_MiFloraFleet::pageIndex() {
  return index;
}
inline bool DisplayScreen_MiFloraFleet::pageSelect(uint16_t page) {
  if (page >= fleet.count())
    return false;

  index = page;
  return true;
}

/* inlines for DisplayScreen_MiFloraAttributes */
inline uint16_t DisplayScreen_MiFloraAttributes::pageCount() {
  uint16_t count  = fleet.count();
  uint16_t base   = count / MAX_DEVICES_PER_PAGE;
  uint16_t remain = count % MAX_DEVICES_PER_PAGE;
  return base + (remain ? 1 : 0);
}
inline uint16_t DisplayScreen_MiFloraAttributes::pageNext() {
  pageIdx = (pageIdx + 1) % pageCount();
  return pageIdx;
}
inline uint16_t DisplayScreen_MiFloraAttributes::pagePrev() {
  if (pageIdx == 0) pageIdx = pageCount() - 1;
  pageIdx = pageIdx - 1;
  return pageIdx;
}
inline uint16_t DisplayScreen_MiFloraAttributes::pageIndex() {
  return pageIdx;
}
inline bool DisplayScreen_MiFloraAttributes::pageSelect(uint16_t page) {
  if (page >= pageCount())
    return false;
  pageIdx = page;
  return true;
}

#endif//_UI_H_
