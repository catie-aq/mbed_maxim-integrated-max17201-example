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
#include "swo.h"
#include "Thread.h"

using namespace sixtron;

namespace {
#define PERIOD_MS 2000
}


I2C i2c(I2C_SDA, I2C_SCL);
static DigitalOut led1(LED1);
MAX17201 gauge(&i2c, DIO1);

// this functions/parameters are used for max17201 management alert
void print_alrt(uint16_t reg);
void listener_alrt(void);
Thread thread_alrt;
EventQueue queue;
InterruptIn max17201alrt1(DIO1);
uint16_t max17201_alrtStatus = 0;

Serial serial(SERIAL_RX, SERIAL_TX);

// main() runs in its own thread in the OSvv
// (note the calls to Thread::wait below for delays)
int main()
{
    i2c.frequency(400000);
    Thread::wait(2000);

    led1 = 0; // led off

    /* When configured, the gauge looses all the learning done between last
    configuration. So if gauge remained powered since last configuration,
    dont use the configuration function to keep the learning in order to have more
    accurate values.
    if the last parameter = true (enable_alert), it's necessary to configure the alert threshold value
    in gauge.configure function */
    if (gauge.configure(1, 800, 3.3, false, false, true)){

        printf("Gauge configured !\n");

        // to use alert management with interrupt system you need to use this mechanism
        thread_alrt.start(callback(&queue, &EventQueue::dispatch_forever));
        max17201alrt1.fall(queue.event(&listener_alrt));

    }
    else {
    	printf("Error with gauge ! \n");
    }

    while (true) {


        printf("Alive!\n");
        printf("Capacity : %.3f mAh\n", gauge.reported_capacity());
        printf("Full Capacity : %.3f mAh\n", gauge.full_capacity());
        printf("SOC: %.3f percent\n", gauge.state_of_charge());
        printf("Voltage : %.3f Volts\n",gauge.cell_voltage()/1000);
        printf("Current : %.3f mA\n",gauge.current());
        printf("Temperature : %.3f\n", gauge.temperature());

        led1 = !led1;
        Thread::wait(PERIOD_MS);


    }
}



/******************************************************************
 *
 * function : void listener_alrt()
 * param : none
 *
 * this function is called when alert was detected on Interrupt pin
 *
 ******************************************************************/
void listener_alrt()
{
	// printf for debug : "\r\n" allow to separate this content with main thread contents
	printf("\r\n");
	printf("/!\\ alert detected /!\\\n");

	max17201_alrtStatus = gauge.status();
	print_alrt(max17201_alrtStatus);

	printf("\r\n");

}

/******************************************************************
 *
 * function : void print_alrt()
 * param : none
 *
 * this function print label of alerts
 *
 ******************************************************************/
void print_alrt(uint16_t res)
{
	uint16_t temp = 0;
	/* treatment status : for each bit of i2c register status (0x00)
	 * cf page 65 of max17201 datasheet */
	for (int i=((sizeof(res)*8)-1); i>-1; i--)
	{
		// if bit = 1 detected
		if ((res & (1 << i)))
		{
			switch(static_cast<MAX17201::StatusAlert>(i))
			{
				// Power On Reset Indicator
				case MAX17201::StatusAlert::ALERT_POR_RST:
					printf("info : Power On Reset Indicator\n");
					break;

				// Minimum Current Alert Threshold Exceeded
				case MAX17201::StatusAlert::ALERT_CURRENT_L:
					printf("Alert : Minimum Current Threshold Exceeded\n");
					break;

				// Battery presence indicator
				case MAX17201::StatusAlert::BATTERY_IS_PRESENT:
					printf("Alert : Battery presence indicator\n");
					break;

				// Maximum Current Alert Threshold Exceeded
				case MAX17201::StatusAlert::ALERT_CURRENT_H :
					printf("Alert : Maximum Current Threshold Exceeded\n");
					break;

				// 1% SOC change alert
				case MAX17201::StatusAlert::ALERT_dSOCI_ :
					printf("Warning : 1%% SOC change\n");
					break;

				// Minimum Voltage Alert Threshold Exceeded
				case MAX17201::StatusAlert::ALERT_VOLTAGE_L :
					printf("Alert : Minimum Voltage Alert Threshold Exceeded\n");
					break;

				// Minimum Temperature Alert Threshold Exceeded
				case MAX17201::StatusAlert::ALERT_TEMP_L :
					printf("Alert : Minimum Temperature Alert Threshold Exceeded\n");
					break;

				// Minimum SOC Alert Threshold Exceeded
				case MAX17201::StatusAlert::ALERT_SOC_L :
					printf("Alert : Minimum SOC Alert Threshold Exceeded\n");
					break;

				// Battery Insertion
				case MAX17201::StatusAlert::ALERT_BATTERY_INSERT :
					printf("Alert : Battery Insertion\n");
					break;

				// Maximum Voltage Alert Threshold Exceeded
				case MAX17201::StatusAlert::ALERT_VOLTAGE_H :
					printf("Alert : Maximum Voltage Alert Threshold Exceeded\n");
					break;

				// Maximum Temperature Alert Threshold Exceeded
				case MAX17201::StatusAlert::ALERT_TEMP_H :
					printf("Alert : Maximum Temperature Alert Threshold Exceeded\n");
					break;

				// Maximum SOC Alert Threshold Exceeded
				case MAX17201::StatusAlert::ALERT_SOC_H :
					printf("Alert : Maximum SOC Alert Threshold Exceeded\n");
					break;

				// Battery Removal
				case MAX17201::StatusAlert::ALERT_BATTERY_REMOVE :
					printf("Alert : Battery Removal\n");
					break;

				default:
					// error unsupported
					printf("unsupported Alert\n");
					break;
			 }
		 }

	}

}


