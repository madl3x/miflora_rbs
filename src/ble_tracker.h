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

#ifndef _BLE_TRACKER_H_
#define _BLE_TRACKER_H_

#include <BLEDevice.h>
#include <queue>
#include "xiaomi.h"
#include "scheduler.h"

#define BLE_NO_RSSI 0
#define BLE_QUEUE_MAX_SIZE (20)

/*
 * Class for handling BLE functionality
 */
class BLE : public BLEAdvertisedDeviceCallbacks {

    public:
        typedef struct {
            std::vector<uint8_t> serviceData;
            std::string deviceAddress;
            int deviceRSSI;
        } MiFloraScanData_t;

        typedef std::queue<MiFloraScanData_t> MiFloraScanQueue_t;
        typedef void (* MifloraScanCallback_t) (const MiFloraScanData_t & );

    public:
        BLE();

        bool begin(bool startScan = true);
        void end();

        bool stopScan();
        bool startScan();

        bool isScanTaskStarted();
        bool isScanningNow();

        void setMifloraHandler(MifloraScanCallback_t callback);

    protected:
        void queueServiceData(BLEAdvertisedDevice & device, std::string _data);
        bool isMiFloraDevice(BLEAdvertisedDevice & advertisedDevice);

        /* from BLEAdvertisedDeviceCallbacks */
        void onResult(BLEAdvertisedDevice advertisedDevice);

    protected:
        /* FreeRTOS handles */
        TaskHandle_t          rtosTaskScan;
        StaticSemaphore_t     rtosMutexBuffer;
        SemaphoreHandle_t     rtosMutex;

        /* Scheduler task */
        Task                  taskProcessQueue;

        bool                  scanEnabled;
        bool                  scanTaskRunning;
        bool                  scanningNow;
        MifloraScanCallback_t mifloraHandlerCbk;
        MiFloraScanQueue_t    mifloraQueue;

        /* RTOS and scheduler task functions */
        void rtosBLETaskRoutine();
        void taskProcessQueueCbk();

        static void s_rtosBLETaskRoutine(void *);
        static void s_taskProcessQueueCbk();
};

extern BLE ble;

/* inlines */
inline bool BLE::isScanTaskStarted() {
    return scanTaskRunning;
}
inline bool BLE::isScanningNow() {
    return scanningNow;
}
inline void BLE::setMifloraHandler(MifloraScanCallback_t handler) {
    mifloraHandlerCbk = handler;
}
inline void BLE::s_rtosBLETaskRoutine(void *parameter) {
    ble.rtosBLETaskRoutine();
}
inline void BLE::s_taskProcessQueueCbk() {
    ble.taskProcessQueueCbk();
}

#endif//_BLE_TRACKER_H_
