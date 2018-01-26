/*
 * Copyright (c) 2016, CATIE, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "max17201.h"

using namespace sixtron;

namespace {
#define PERIOD_MS 2000
#define MAX_VOLTAGE_ALERT 4.2 // V
#define MIN_VOLTAGE_ALERT 3.1 // V
#define MAX_CURRENT_ALERT 500 // mA
#define MIN_CURRENT_ALERT 1 // mA
#define MAX_TEMPERATURE_ALERT 50 // °C
#define MIN_TEMPERATURE_ALERT 5 // °C
}

// these functions/parameters are used for max17201 management alert
void on_alert(void);
void manage_alert(void);
Thread thread_alrt;
EventQueue queue;

I2C i2c(I2C_SDA, I2C_SCL);
MAX17201 gauge(&i2c, DIO1);
static DigitalOut led1(LED1);

int main()
{
    i2c.frequency(400000);
    Thread::wait(2000);

    /* When configured, the gauge looses all the learning done between last
    configuration. So if gauge remained powered since last configuration,
    dont use the configuration function to keep the learning in order to have more
    accurate values.
    */
    if (gauge.configure(1, 800, 3.3, false, false)) {
        printf("Gauge configured\n");
        // here, set the alert threshold function
        gauge.set_temperature_alerts(MAX_TEMPERATURE_ALERT, MIN_TEMPERATURE_ALERT);
        gauge.set_voltage_alerts(MAX_VOLTAGE_ALERT, MIN_VOLTAGE_ALERT);
        // Enable gauge alerts
        gauge.enable_alerts();
        gauge.enable_temperature_alerts();
        wait_ms(250);
        // attach callback alert interrupt function
        gauge.alert_callback(&on_alert);
        // The Event Queue run in its own thread
        thread_alrt.start(callback(&queue, &EventQueue::dispatch_forever));
    } else {
        printf("Error with gauge!\n");
    }

    while (true) {
        printf("Capacity: %.3f mAh\n", gauge.reported_capacity());
        printf("Full Capacity: %.3f mAh\n", gauge.full_capacity());
        printf("State of Charge: %.3f%%\n", gauge.state_of_charge());
        printf("Voltage: %.3f V\n", gauge.cell_voltage() / 1000);
        printf("Current: %.3f mA\n", gauge.current());
        printf("Temperature: %.3f °C\n", gauge.temperature());
        led1 = !led1;
        Thread::wait(PERIOD_MS);
    }
}


/*! Gauge alert callback
 */
void on_alert()
{
    // use an EventQueue to manage alert outside of interrupt context
    queue.call(manage_alert);
}

/*! Take action following gauge alert
 *
 * \note This function should not be called in interrupt context
 */
void manage_alert()
{
    printf("** Alert detected! **\n");
    uint16_t gauge_alert_status = gauge.status();

    if (gauge_alert_status & static_cast<uint16_t>(MAX17201::AlertFlags::POWER_RESET)) {
        printf("Info: Power On Reset Indicator\n");
    }
    if (gauge_alert_status & static_cast<uint16_t>(MAX17201::AlertFlags::CURRENT_MIN)) {
        printf("Alert: Minimum Current Threshold Exceeded\n");
    }
    if (gauge_alert_status & static_cast<uint16_t>(MAX17201::AlertFlags::BATTERY_PRESENT)) {
        printf("Alert: Battery presence indicator\n");
    }
    if (gauge_alert_status & static_cast<uint16_t>(MAX17201::AlertFlags::CURRENT_MAX)) {
        printf("Alert: Maximum Current Threshold Exceeded\n");
    }
    if (gauge_alert_status & static_cast<uint16_t>(MAX17201::AlertFlags::STATE_OF_CHARGE_CHANGE)) {
        printf("Warning: 1%% SOC change\n");
    }
    if (gauge_alert_status & static_cast<uint16_t>(MAX17201::AlertFlags::VOLTAGE_MIN)) {
        printf("Alert: Minimum Voltage Alert Threshold Exceeded\n");
    }
    if (gauge_alert_status & static_cast<uint16_t>(MAX17201::AlertFlags::TEMPERATURE_MIN)) {
        printf("Alert: Minimum Temperature Alert Threshold Exceeded\n");
    }
    if (gauge_alert_status & static_cast<uint16_t>(MAX17201::AlertFlags::STATE_OF_CHARGE_MIN)) {
        printf("Alert: Minimum State of Charge Alert Threshold Exceeded\n");
    }
    if (gauge_alert_status & static_cast<uint16_t>(MAX17201::AlertFlags::BATTERY_INSERT)) {
        printf("Alert: Battery Insertion\n");
    }
    if (gauge_alert_status & static_cast<uint16_t>(MAX17201::AlertFlags::VOLTAGE_MAX)) {
        printf("Alert: Maximum Voltage Alert Threshold Exceeded\n");
    }
    if (gauge_alert_status & static_cast<uint16_t>(MAX17201::AlertFlags::TEMPERATURE_MAX)) {
        printf("Alert: Maximum Temperature Alert Threshold Exceeded\n");
    }
    if (gauge_alert_status & static_cast<uint16_t>(MAX17201::AlertFlags::STATE_OF_CHARGE_MAX)) {
        printf("Alert: Maximum SOC Alert Threshold Exceeded\n");
    }
    if (gauge_alert_status & static_cast<uint16_t>(MAX17201::AlertFlags::BATTERY_REMOVE)) {
        printf("Alert: Battery Removal\n");
    }
}

