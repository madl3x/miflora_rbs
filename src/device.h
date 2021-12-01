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

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <arduino.h>
#include "xiaomi.h"
#include "config.h"
#include "ble_tracker.h"

enum UpdateSource {
  SOURCE_NONE,
  SOURCE_BLE,
  SOURCE_MQTT
};

enum AttributeID {
  ATTR_ID_MOISTURE,
  ATTR_ID_TEMPERATURE,
  ATTR_ID_CONDUCTIVITY,
  ATTR_ID_ILLUMINANCE,
  ATTR_ID_RSSI,
  ATTR_ID_MAX,
  ATTR_ID_NONE = ATTR_ID_MAX
};

class DeviceAttribute;

class Device {
  public:
    virtual unsigned int      attributeCount() = 0;
    virtual DeviceAttribute * attributeAt(unsigned int index) = 0;
    virtual int               attributeIndex(DeviceAttribute * attr) = 0;
    virtual DeviceAttribute * attributeByID(AttributeID ID) = 0;
    virtual void              onAttributeUpdate(DeviceAttribute * attr) = 0;
};

class DeviceAttribute {
 public:
    DeviceAttribute(Device * device, AttributeID id = ATTR_ID_NONE, const char * label = NULL) : 
      _device(device), 
      _source(SOURCE_NONE),
      _ID(id),
      _last_updated(0),
      _has_value(false),_has_min(false),_has_max(false),
      _value(0)        , _value_min(0) , _value_max(0),
      _label(label) {
    }

    void updated() {
      _last_updated = millis();
      if (_device) {
        _device->onAttributeUpdate(this);
      }
    }
    
    void reset() { 
      _has_value = false;
      _source = SOURCE_NONE;
      updated();
    }

    UpdateSource getSource() {
      return _source;
    }
    
    bool hasValue() { 
        return _has_value; 
    }

    unsigned long lastUpdated() {
      return _last_updated;
    }
    
    bool set(const float v, UpdateSource source){ 
        _value = v; 
        _has_value = true;
        _source = source;
        updated();
        return true;
    }
    
    float get(float default_value) { 
        if (_has_value == false) {
            return default_value;
        }
        return _value; 
    }

    float get() { 
        return _value; 
    }

    int getInt() {
      return (int) _value;
    }

    long getLong() {
      return (long) _value;
    }

    unsigned int getUInt() {
      return (unsigned int) _value;
    }

    unsigned long getULong() {
      return (unsigned long) _value;
    }

    void setMin(float min_val) {
      _value_min = min_val;
      _has_min = true;
    }

    void setMax(float max_val) {
      _value_max = max_val;
      _has_max = true;
    }

    void resetMin() {
      _has_min = false;
      _value_min = 0.0;
    }
    
    void resetMax() {
      _has_max = false;
      _value_max = 0.0;
    }

    void resetMinMax() {
      _has_min = false;
      _has_max = false;
      _value_min = 0.0;
      _value_max = 0.0;
    }

    bool inLimits() {
      if (_has_max && _value > _value_max) return false;
      if (_has_min && _value < _value_min) return false;
      return true;
    }

    void setLabel(char * label) {
      _label = label;
    }

    const char * getLabel() {
      return _label;
    }

  private:
    Device * _device;
    UpdateSource _source;
    AttributeID _ID;
    unsigned long _last_updated;
    bool _has_value, _has_min, _has_max;
    float _value, _value_min, _value_max;
    const char * _label;
};

/*
 * Device implementation for MiFlora
 */ 
class MiFloraDevice : public Device {

  public:
    static const int ATTRIBUTES_COUNT = 5;

  public:
    MiFloraDevice(const std::string & addr);
  
    void                updateFromBLEScan(XiaomiParseResult & result);
    void                updateRSSI(int rssi);

    const std::string & getAddress();
    std::string         getAddressCompressed();
    int                 getID();
    void                setID(int id);
    const std::string & getName();
    void                setName(const char * name);
    unsigned long       lastUpdated();

    /* from Device class */
    unsigned int        attributeCount();
    DeviceAttribute *   attributeAt(unsigned int index);
    int                 attributeIndex(DeviceAttribute * attr);
    DeviceAttribute *   attributeByID(AttributeID ID);
    void                onAttributeUpdate(DeviceAttribute * attr);

  public:
    DeviceAttribute conductivity;
    DeviceAttribute temperature;
    DeviceAttribute moisture;
    DeviceAttribute illuminance;
    DeviceAttribute RSSI;

  private:
    unsigned long _last_updated;
    int _id;
    std::string _address;
    std::string _name;

    void updateFromMQTT(const char * topic, uint8_t * payload, unsigned int len);
    static void s_onMQTTMessage(const char * topic, uint8_t * payload, unsigned int len, void * param);
};

/* inlines for MiFloraDevice */
inline const std::string & MiFloraDevice::getAddress() {
  return _address;
}
inline std::string MiFloraDevice::getAddressCompressed() {
  std::string addr_compressed;
  const char * ch = _address.c_str();
  while (*ch != '\0') {
    if (* ch == ':') { ++ ch; continue; }
    addr_compressed.push_back(*ch);
    ++ ch;
  }
  return addr_compressed;
}

inline unsigned long MiFloraDevice::lastUpdated() {
  return _last_updated;
}

inline int MiFloraDevice::getID() {
  return _id;
}

inline void MiFloraDevice::setID(int id) {
  _id = id;
}

inline const std::string & MiFloraDevice::getName() {
  return _name;
}

inline void MiFloraDevice::setName(const char * name) {
  _name = name;
}

inline unsigned int MiFloraDevice::attributeCount() {
  return 5;
}

inline DeviceAttribute * MiFloraDevice::attributeAt(unsigned int index) {

  switch(index) {
    case 0: return &moisture;
    case 1: return &temperature;
    case 2: return &conductivity;
    case 3: return &illuminance;
    case 4: return &RSSI;
  }
  return NULL;
}

inline int MiFloraDevice::attributeIndex(DeviceAttribute * attr) {

  if (attr == &moisture    ) return 0;
  if (attr == &temperature ) return 1;
  if (attr == &conductivity) return 2;
  if (attr == &illuminance ) return 3;
  if (attr == &RSSI        ) return 4;
  return -1;
}

inline DeviceAttribute * MiFloraDevice::attributeByID(AttributeID id) {
  if (id == ATTR_ID_MOISTURE    ) return &moisture;
  if (id == ATTR_ID_TEMPERATURE ) return &temperature;
  if (id == ATTR_ID_CONDUCTIVITY) return &conductivity;
  if (id == ATTR_ID_ILLUMINANCE ) return &illuminance;
  if (id == ATTR_ID_RSSI        ) return &RSSI;
  return NULL;
}

inline void MiFloraDevice::onAttributeUpdate(DeviceAttribute * attr) {
  _last_updated = millis();
}

inline void MiFloraDevice::s_onMQTTMessage(const char * topic, uint8_t * payload, unsigned int len, void * param) {
  ((MiFloraDevice*)param)->updateFromMQTT(topic, payload, len);
}

/*
 * Fleet class for managing MiFlora devices
 */
class MiFloraFleet {
  public:

    bool begin(const char * filename);

    void loadFromConfig(ConfigFile & configDevices);
    void addDevice(MiFloraDevice* device);

    MiFloraDevice * findByAddress(const char * address);
    MiFloraDevice * findByName(const char * name);
    MiFloraDevice * findByID(int id);
    MiFloraDevice * atIndex(int idx);

    const std::vector<MiFloraDevice *> & devices();
    const unsigned int count();

  private:
    std::vector<MiFloraDevice *> _devices;
    static void s_BLE_MiFloraHandler(const BLE::MiFloraScanData_t & scanData);
};

inline void MiFloraFleet::addDevice(MiFloraDevice* device) {
  _devices.push_back(device);
}

inline MiFloraDevice * MiFloraFleet::findByAddress(const char * address) {
  for (auto device : _devices)
    if (device->getAddress().compare(address) == 0)
      return device;
  return NULL;
}

inline MiFloraDevice * MiFloraFleet::findByName(const char * name) {
  for (auto device : _devices)
    if (device->getName().compare(name) == 0)
      return device;
  return NULL;
}

inline MiFloraDevice* MiFloraFleet::atIndex(int idx) {
  if (idx > _devices.size()) {
    return NULL;
  }
  return _devices.at(idx);
}

inline MiFloraDevice * MiFloraFleet::findByID(int id) {
  for (auto device : _devices)
    if (device->getID() == id)
      return device;
  return NULL;
}

inline const std::vector<MiFloraDevice *> & MiFloraFleet::devices() {
  return _devices;
}

inline const unsigned int MiFloraFleet::count() {
  return (unsigned int) _devices.size();
}

extern MiFloraFleet fleet;

#endif//_DEVICE_H_
