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
#include <stddef.h>
#include "mbed.h"
#include "max17201.h"
#include "Thread.h"

using namespace sixtron;

namespace {
#define PERIOD_MS 2000
#define MAX17201_ENABLE_ALERT
#define MAX_VOLTAGE_ALERT        4.2 // V
#define MIN_VOLTAGE_ALERT        3.1 // V
#define MAX_CURRENT_ALERT        500 // mA
#define MIN_CURRENT_ALERT        1 	 // mA
#define MAX_TEMPERATURE_ALERT    50  // °C
#define MIN_TEMPERATURE_ALERT    5   // °C
}

#ifdef MAX17201_ENABLE_ALERT
// these functions/parameters are used for max17201 management alert
void myCallack_alert(void);
void management_alrt(void);
Thread thread_alrt;
EventQueue queue;
uint16_t max17201_alrtStatus = 0;
#endif

I2C i2c(I2C_SDA, I2C_SCL);
MAX17201 gauge(&i2c, DIO1);
static DigitalOut led1(LED1);


// main() runs in its own thread in the OSvv
// (note the calls to Thread::wait below for delays)
int main()
{
    i2c.frequency(400000);
    Thread::wait(2000);

    /* When configured, the gauge looses all the learning done between last
    configuration. So if gauge remained powered since last configuration,
    dont use the configuration function to keep the learning in order to have more
    accurate values.
    if the last parameter = true (enable_alert), it's necessary to configure the alert threshold value
    in gauge.configure function */
    if (gauge.configure(1, 800, 3.3, false, false)) {
        printf("Gauge configured !\n\r");
#ifdef MAX17201_ENABLE_ALERT
            // here, set the alert threshold function
            gauge.set_temperature_alerts(MAX_TEMPERATURE_ALERT, MIN_TEMPERATURE_ALERT);
            gauge.set_voltage_alerts(MAX_VOLTAGE_ALERT, MIN_VOLTAGE_ALERT);
            gauge.enable_alerts(); // max17201 alert enable
            gauge.enable_temperature_alerts();
            wait_ms(250); // let time to software to compute new values
            gauge.alert_callback(&myCallack_alert); //attach callback alert interrupt function
            // to use alert management with EventQueue interrupt system (attached an other thread), you need to use this mechanism :
            thread_alrt.start(callback(&queue, &EventQueue::dispatch_forever));
#endif
    }
    else {
        printf("Error with gauge ! \n\r");
    }

    while (true) {
        printf("Alive!\n\r");
        printf("Capacity : %.3f mAh\n\r", gauge.reported_capacity());
        printf("Full Capacity : %.3f mAh\n\r", gauge.full_capacity());
        printf("SOC: %.3f percent\n\r", gauge.state_of_charge());
        printf("Voltage : %.3f Volts\n\r",gauge.cell_voltage()/1000);
        printf("Current : %.3f mA\n\r",gauge.current());
        printf("Temperature : %.3f\n\r", gauge.temperature());
        led1 = !led1;
        Thread::wait(PERIOD_MS);
    }
}

#ifdef MAX17201_ENABLE_ALERT
/** myCallback_alert
 *
 * attach your function on Queue when max17201 alert was detected on interrupt pin
 *
 */
void myCallack_alert()
{
    // here, implement your EventQueue attached function
    queue.call(management_alrt);
}

/** management_alrt
 *
 * this function manage the type of alert detected
 *
 */
void management_alrt()
{
    // printf for debug : "\r\n" allow to separate this content with main thread contents
    printf("\r\n/!\\ alert detected /!\\\n\r");
    max17201_alrtStatus = gauge.status();
    /* treatment status : for each bit of i2c register status (0x00)
	 * cf page 65 of max17201 datasheet */
    for (int i=((sizeof(max17201_alrtStatus)*8)-1); i>-1; i--){
        // if detected bit = 1
        if ((max17201_alrtStatus & (1 << i))){
            switch(static_cast<MAX17201::StatusAlert>(i))
            {
                // Power On Reset Indicator
                case MAX17201::StatusAlert::ALERT_POR_RST:
                    printf("info : Power On Reset Indicator\n\r");
                    break;

                // Minimum Current Alert Threshold Exceeded
                case MAX17201::StatusAlert::ALERT_CURRENT_L:
                    printf("Alert : Minimum Current Threshold Exceeded\n\r");
                    break;

                // Battery presence indicator
                case MAX17201::StatusAlert::BATTERY_IS_PRESENT:
                    printf("Alert : Battery presence indicator\n\r");
                    break;

                // Maximum Current Alert Threshold Exceeded
                case MAX17201::StatusAlert::ALERT_CURRENT_H :
                    printf("Alert : Maximum Current Threshold Exceeded\n\r");
                    break;

                // 1% SOC change alert
                case MAX17201::StatusAlert::ALERT_dSOCI_ :
                    printf("Warning : 1%% SOC change\n\r");
                    break;

                // Minimum Voltage Alert Threshold Exceeded
                case MAX17201::StatusAlert::ALERT_VOLTAGE_L :
                    printf("Alert : Minimum Voltage Alert Threshold Exceeded\n\r");
                    break;

                // Minimum Temperature Alert Threshold Exceeded
                case MAX17201::StatusAlert::ALERT_TEMP_L :
                    printf("Alert : Minimum Temperature Alert Threshold Exceeded\n\r");
                    break;

                // Minimum SOC Alert Threshold Exceeded
                case MAX17201::StatusAlert::ALERT_SOC_L :
                    printf("Alert : Minimum State of Charge Alert Threshold Exceeded\n\r");
                    break;

                // Battery Insertion
                case MAX17201::StatusAlert::ALERT_BATTERY_INSERT :
                    printf("Alert : Battery Insertion\n\r");
                    break;

                // Maximum Voltage Alert Threshold Exceeded
                case MAX17201::StatusAlert::ALERT_VOLTAGE_H :
                    printf("Alert : Maximum Voltage Alert Threshold Exceeded\n\r");
                    break;

                // Maximum Temperature Alert Threshold Exceeded
                case MAX17201::StatusAlert::ALERT_TEMP_H :
                    printf("Alert : Maximum Temperature Alert Threshold Exceeded\n\r");
                    break;

                // Maximum SOC Alert Threshold Exceeded
                case MAX17201::StatusAlert::ALERT_SOC_H :
                    printf("Alert : Maximum SOC Alert Threshold Exceeded\n\r");
                    break;

                // Battery Removal
                case MAX17201::StatusAlert::ALERT_BATTERY_REMOVE :
                    printf("Alert : Battery Removal\n\r");
                    break;

                default:
                    // error unsupported
                    printf("unsupported Alert\n\r");
                    break;
            }
        }
    }
    // clear alertStatus register for a new acquisition...
    gauge.clear_alertStatus_register();
    printf("\r\n");
}
#endif
