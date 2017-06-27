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
#include "MAX17201.hpp"

namespace {
#define PERIOD_MS 2000
}

I2C i2c(I2C1_SDA, I2C1_SCL);
static DigitalOut led1(LED1);

MAX17201 gauge(&i2c, DIO1);

// main() runs in its own thread in the OS
// (note the calls to Thread::wait below for delays)
int main()
{
	i2c.frequency(400000);
	Thread::wait(2000);

	if (gauge.configure(1, 800, 3.3, false, false)){
		printf("Gauge configured !\n");
	}
	else
	{
		printf("Error with gauge ! \n");
	}

    while (true) {
        printf("Alive!\n");
        printf("Capacity : %.3f mAh\n", gauge.reported_capacity());
        printf("SOC: %.3f percent\n", gauge.state_of_charge());
        printf("Voltage : %.3f Volts\n",gauge.cell_voltage()/1000);
        printf("Current : %.3f mA\n",gauge.current());
        led1 = !led1;
        Thread::wait(PERIOD_MS);
    }
}


