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
#include "ble_tracker.h"
#include "config.h"

#define LOG_TAG LOG_TAG_BLE
#include "log.h"

#define BLE_TASK_STACK_SIZE 2048
BLE ble;

BLEUUID MiFlora_Data_UUID("0000fe95-0000-1000-8000-00805f9b34fb");

BLE::BLE() : 
  rtosTaskScan(NULL),
  rtosMutex(NULL),
  taskProcessQueue(50, TASK_FOREVER, BLE::s_taskProcessQueueCbk, &scheduler, false),
  scanEnabled(false), 
  scanTaskRunning(false),
  scanningNow(false),
  mifloraHandlerCbk(NULL) {

  // create semaphore
  rtosMutex = xSemaphoreCreateMutexStatic(&rtosMutexBuffer);
  configASSERT(rtosMutex);
}

/* noityf about new mi-flora result */
void BLE::queueServiceData(BLEAdvertisedDevice & device, std::string _data) {

  MiFloraScanData_t scanData;
  scanData.serviceData.assign(_data.begin(), _data.end());
  scanData.deviceAddress.assign(device.getAddress().toString());
  scanData.deviceRSSI = device.haveRSSI() ? device.getRSSI() : BLE_NO_RSSI;

  // print hex dump on verbose
  if (config.ble_verbose) {
    LOG_PART_START("Hex: ");
    for (int i = 0; i < scanData.serviceData.size(); i++) {
      LOG_PART_HEX((int)scanData.serviceData[i]);
      LOG_PART(" ");
    }
    LOG_PART_END("");
  }

  if (config.ble_verbose) {
    LOG_F("queuing scan data (%u bytes) for device %s", 
      scanData.serviceData.size(), scanData.deviceAddress.c_str());
  }

  // add the new scan data to queue
  if (xSemaphoreTake(rtosMutex, portMAX_DELAY) == pdTRUE) {

    // if scheduler is hanged try not to deplete memory
    if (mifloraQueue.size() > BLE_QUEUE_MAX_SIZE) {
      LOG_F("WARNING: max queue size reached for scanned data! (is scheduler hanged?)");
      mifloraQueue.pop();
    }
    
    mifloraQueue.push(scanData);

    // release acess to queue
    xSemaphoreGive(rtosMutex);
  }
}

bool BLE::isMiFloraDevice(BLEAdvertisedDevice & advertisedDevice) {

  // filter devices to miflora devices
  if (advertisedDevice.getServiceDataUUIDCount() != 1) 
    return false; // miflora has only one data advertised

  // compare data UUID with known
  /*
  BLEUUID & data_uuid = advertisedDevice.getServiceDataUUID(0);
  data_uuid.equals
  auto esp_uuid = data_uuid.to128().getNative();
  if ( esp_uuid->uuid.uuid128[12] != UUID_MiFlora_16[0] ||
        esp_uuid->uuid.uuid128[13] != UUID_MiFlora_16[1] )
    return; // not a MiFlora device */

  if (advertisedDevice.getServiceDataUUID(0).equals(MiFlora_Data_UUID) == false)
    return false; // not a MiFlora device
  
  return true;
}

/* called when a new device is detected upon scan */
void BLE::onResult(BLEAdvertisedDevice advertisedDevice) {

  // on verbose, print everything that BLE sees on serial
  if (config.ble_verbose) {

    LOG_PART_START("Device: ");
    LOG_PART(advertisedDevice.getAddress().toString().c_str());
    LOG_PART(" Name:");
    LOG_PART(advertisedDevice.haveName() ? advertisedDevice.getName().c_str() : "?");
    LOG_PART(" RSSI:");
    LOG_PART(advertisedDevice.getRSSI());
    LOG_PART_END("");

    if (advertisedDevice.getServiceUUIDCount()) {

        LOG_F("Service UUIDs: %d", advertisedDevice.getServiceUUIDCount());
        for (int i = 0 ; i < advertisedDevice.getServiceUUIDCount(); ++ i) {
          LOG_F(" -- UUID: %s", advertisedDevice.getServiceUUID(i).toString().c_str());
        }
    }
    
    if (advertisedDevice.getServiceDataUUIDCount()) {
        
        LOG_F("DATA UUIDs: %d", advertisedDevice.getServiceDataUUIDCount());
        for (int i = 0 ; i < advertisedDevice.getServiceDataUUIDCount(); ++ i) {
          LOG_F(" -- UUID: %s", advertisedDevice.getServiceDataUUID(i).toString().c_str());
        }
    }

    if (advertisedDevice.getServiceDataCount()) {

        LOG_F("Data: %d", advertisedDevice.getServiceDataCount());
        for (int i = 0 ; i < advertisedDevice.getServiceUUIDCount(); ++ i) {
          LOG_F(" -- Data: %d bytes", advertisedDevice.getServiceData(i).length());
        }
    }
  }

  // for Mi-Flora devices, queue service data
  if (isMiFloraDevice(advertisedDevice)) {
    queueServiceData(advertisedDevice, advertisedDevice.getServiceData(0));
    return;
  }
}
  
/* BLE scan task, works continuously to track for new devices */
void BLE::rtosBLETaskRoutine() {
  BLEScan * bleScan = BLEDevice::getScan();

  // mark this task as being started
  scanTaskRunning = true;
  LOG_LN("RTOS task started!");

  // register for advertised callback
  bleScan->setAdvertisedDeviceCallbacks((BLEAdvertisedDeviceCallbacks*)this, false, true);

  // configure scanning
  bleScan->setActiveScan (config.ble_active_scan       ); 
  bleScan->setInterval   (config.ble_scan_interval_ms  );
  bleScan->setWindow     (config.ble_window_interval_ms);  // must be less or equal to scan_interval

  // scan loop
  while(scanEnabled) {

    // starting BLE scan
    LOG_F("Scanning for %d seconds (%s)...", 
      config.ble_scan_duration_sec, config.ble_active_scan ? "active" : "passive" );

    // mark scan as being active
    scanningNow = true;

    // do start BLE scan
    bleScan->start(config.ble_scan_duration_sec, false);
  
    // scan complete
    LOG_LN("Scan complete!");
    bleScan->clearResults();   // delete results fromBLEScan buffer to release memory
    bleScan->stop();

    // scanning stopped
    scanningNow = false;

    // wait between scans
    unsigned long wait_times = config.ble_scan_wait_sec * 10;
    while (scanEnabled && wait_times--) {
      delay(100);
    }
  };

  // deregister from scan callbacks
  bleScan->setAdvertisedDeviceCallbacks(nullptr);

  // stopped scanning
  LOG_LN("[BLE] Scanning stopped, leaning data queue");

  // clean all entries in data queue
  if (xSemaphoreTake(rtosMutex, portMAX_DELAY) == pdTRUE) {
    MiFloraScanQueue_t empty_queue;
    mifloraQueue.swap(empty_queue);
    xSemaphoreGive(rtosMutex);
  }

  // mark task as being stopped
  LOG_LN("RTOS task stopped!");
  scanTaskRunning = false;

  // delete self task
  vTaskDelete(NULL);
}

void BLE::taskProcessQueueCbk() {

  MiFloraScanQueue_t scanQueue;

  // consume the scan queue for miflora devices
  if (xSemaphoreTake(rtosMutex, 10 / portTICK_PERIOD_MS) == pdFALSE)
    return;

    // swap the empty local queue with the scan queue
    mifloraQueue.swap(scanQueue);

  xSemaphoreGive(rtosMutex);

  // check if we have nothing to do
  if (mifloraHandlerCbk == NULL)
    return;

  // process the local queue
  while (!scanQueue.empty()) {
    const MiFloraScanData_t scanData = scanQueue.front();
    scanQueue.pop();

    if (config.ble_verbose) {
      LOG_F("processing scan data (%u bytes) for device %s", 
        scanData.serviceData.size(), scanData.deviceAddress.c_str());
    }
      
    // invoke handler
    mifloraHandlerCbk(scanData);
  }
}

/* initialize BLE */
bool BLE::begin(bool start) {

  BLEDevice::init("miflora");
//BLEDevice::setPower(ESP_PWR_LVL_P7);

  // make sure scan window is less than interval
  if (config.ble_window_interval_ms > config.ble_scan_interval_ms) {
    config.ble_window_interval_ms = config.ble_scan_interval_ms -1;
  }

  if (start) {
    return startScan();
  }
  
  return true;
}

/* stop BLE component */
void BLE::end() {
  stopScan();
}

/* start the BLE scan task */
bool BLE::startScan() {
   BaseType_t ret;

  // already started
  if (scanEnabled) 
    return false;

  // still running, wait for task to finish
  if (scanTaskRunning)
    return false; 
   
   // create RTOS task
   ret = xTaskCreate(
      BLE::s_rtosBLETaskRoutine, "ble", 
      BLE_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY, 
      &rtosTaskScan);

  if (ret != pdPASS) {
    return false;
  }

  // create scheduler task
  taskProcessQueue.restartDelayed();
  scanEnabled = true;
  return true;
}

/* stop the BLE scan task */
bool BLE::stopScan() {

  // nothing to stop
  if (scanEnabled == false)
    return false;

  scanEnabled = false;

  // stop scheduler task
  taskProcessQueue.disable();

  // stop scanning, task should exit now
  BLEDevice::getScan()->stop();
  return true;
}
