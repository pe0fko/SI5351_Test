/*
 * si5351_calibration.ino - Simple calibration routine for the Si5351
 *                          breakout board.
 *
 * Copyright 2015 - 2018 Paul Warren <pwarren@pwarren.id.au>
 *                       Jason Milldrum <milldrum@gmail.com>
 *
 * Uses code from https://github.com/darksidelemm/open_radio_miniconf_2015
 * and the old version of the calibration sketch
 *
 * This sketch  is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License.
 * If not, see <http://www.gnu.org/licenses/>.
*/

#include "si5351.h"
#include "Wire.h"

Si5351			si5351;
int32_t			cal_factor	= 0;
int32_t			old_cal		= 0;
si5351_drive	drive		= SI5351_DRIVE_8MA;
uint8_t			xtal_load_c	= SI5351_CRYSTAL_LOAD_8PF;	// 0pF, 6pF, 8pF, 10pF
uint64_t		rx_freq;

//uint64_t target_freq = 500000000ULL;		// 10 MHz, in hundredths of hertz
//uint64_t target_freq = 1000000000ULL;		// 10 MHz, in hundredths of hertz
uint64_t target_freq = 2000000000ULL; 		// 20 MHz, in hundredths of hertz
//uint64_t target_freq = 1405000000ULL;		// 14.050 MHz, in hundredths of hertz
//uint64_t target_freq = 10000000000ULL;	// 100.0 MHz, in hundredths of hertz
//uint64_t target_freq = 14550000000ULL;	// 145.5 MHz, in hundredths of hertz

static	void	vfo_interface(void);
static	void	set_cal_freq();
static	void	printHz();
static	void	prt_drive();
static	void	prt_load();
static	void	prt_status();


void setup()
{
	// Start serial and initialize the Si5351
	Serial.begin(115200);
	while (!Serial);
	delay(2000);

	// The crystal load value needs to match in order to have an accurate calibration
	si5351.init(xtal_load_c, 0, 0);
	set_cal_freq();
}

void loop()
{
	si5351.update_status();
	if (si5351.dev_status.SYS_INIT == 1)
	{
		Serial.println(F("Initialising Si5351, you shouldn't see many of these!"));
		delay(500);
	}
	else
	{
		Serial.println();
		Serial.println(F("Adjust until your frequency counter reads as close to 10 MHz as possible."));
		Serial.println(F("Press 'q' when complete."));
		vfo_interface();
	}
}

static void flush_input(void)
{
	while (Serial.available() > 0)
	Serial.read();
}

static void vfo_interface(void)
{
	rx_freq = target_freq;
	cal_factor = old_cal;
	Serial.println(F("   Up:   r   t   y   u   i   o  p   ]   }   1-4   6-9"));
	Serial.println(F(" Down:   f   g   h   j   k   l  ;   [   {   Drive Load-C"));
	Serial.println(F("   Hz: 0.01 0.1  1   10  100 1K 10K 100K 1M  2-8mA 0-10pF"));
	while (1)
	{
		if (Serial.available() > 0)
		{
			char c = Serial.read();
			switch (c)
			{
				case 'q':
					flush_input();
					Serial.print(F("Use: "));
					set_cal_freq();
					old_cal = cal_factor;
					return;
				case 'R':
					si5351.init(xtal_load_c, 0, 0);
					cal_factor	= 0;
					old_cal		= 0;
					drive		= SI5351_DRIVE_8MA;
					xtal_load_c	= SI5351_CRYSTAL_LOAD_8PF;
					rx_freq		= target_freq;
					break;
				case 'r': rx_freq += 1; break;			// 0.01 Hz
				case 'f': rx_freq -= 1; break;
				case 't': rx_freq += 10; break;			//  0.1 Hz
				case 'g': rx_freq -= 10; break;
				case 'y': rx_freq += 100; break;		//  1 Hz
				case 'h': rx_freq -= 100; break;
				case 'u': rx_freq += 1000; break;		//  10 Hz
				case 'j': rx_freq -= 1000; break;
				case 'i': rx_freq += 10000; break;		// 100 Hz
				case 'k': rx_freq -= 10000; break;
				case 'o': rx_freq += 100000; break;		//  1 KHz
				case 'l': rx_freq -= 100000; break;
				case 'p': rx_freq += 1000000; break;	// 10 KHz
				case ';': rx_freq -= 1000000; break;

				case ']': rx_freq = (target_freq += 100000000L); break;		// 1 MHz
				case '[': rx_freq = (target_freq -= 100000000L); break;
				case '}': rx_freq = (target_freq += 1000000000L); break;	// 10 MHz
				case '{': rx_freq = (target_freq -= 1000000000L); break;

				case '1': drive = SI5351_DRIVE_2MA; break;
				case '2': drive = SI5351_DRIVE_4MA; break;
				case '3': drive = SI5351_DRIVE_6MA; break;
				case '4': drive = SI5351_DRIVE_8MA; break;

				case '6': xtal_load_c = SI5351_CRYSTAL_LOAD_0PF; break;
				case '7': xtal_load_c = SI5351_CRYSTAL_LOAD_6PF; break;
				case '8': xtal_load_c = SI5351_CRYSTAL_LOAD_8PF; break;
				case '9': xtal_load_c = SI5351_CRYSTAL_LOAD_10PF; break;

				default:	continue;	// Do nothing
			}

			cal_factor = (int32_t)(target_freq - rx_freq) + old_cal;

			set_cal_freq();
		}
	}
}

static void 
set_cal_freq()
{
	si5351.si5351_write(SI5351_CRYSTAL_LOAD, (xtal_load_c & SI5351_CRYSTAL_LOAD_MASK) | 0b00010010);
	si5351.drive_strength(SI5351_CLK0, drive);
	si5351.set_correction(cal_factor, SI5351_PLL_INPUT_XO);
	si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
	si5351.pll_reset(SI5351_PLLA);

	if (si5351.set_freq(target_freq, SI5351_CLK0))
		Serial.print(F("ERROR: Frequency not set"));

//	Serial.println();
	Serial.print(F("Calibration: "));
	Serial.print(cal_factor, DEC);
	Serial.print(F(", 0x"));
	Serial.print(cal_factor, HEX);
	Serial.print(F(" "));
	Serial.print((uint16_t)(target_freq / 100000000ULL));
	Serial.print(F("MHz"));

//	Serial.print(F(" "));
//	printHz();

	prt_drive();
	prt_load();
	prt_status();

	Serial.println();
}

static void
printHz()
{
	uint64_t	freq = target_freq;
	char		c[14+1];
	int8_t		i;

	c[14] = '\0';
	for(i = 13; i >= 0; i--)
	{
		if (i == 11) c[i--] = ',';
		if (i ==  7) c[i--] = '.';
		if (i ==  3) c[i--] = '.';
		c[i] = '0' + (uint8_t)(do_div(freq, 10));
	}
	Serial.print(c);
	Serial.print(F("Hz"));
}

static void
prt_drive()
{
	switch(drive) {
		case SI5351_DRIVE_2MA:
			Serial.print(F(" 2mA"));
			break;
		case SI5351_DRIVE_4MA:
			Serial.print(F(" 4mA"));
			break;
		case SI5351_DRIVE_6MA:
			Serial.print(F(" 6mA"));
			break;
		case SI5351_DRIVE_8MA:
			Serial.print(F(" 8mA"));
			break;
	}
}

static void
prt_load()
{
	switch(xtal_load_c) {
		case SI5351_CRYSTAL_LOAD_0PF:
			Serial.print(F(" 0pF"));
			break;
		case SI5351_CRYSTAL_LOAD_6PF:
			Serial.print(F(" 6pF"));
			break;
		case SI5351_CRYSTAL_LOAD_8PF:
			Serial.print(F(" 8pF"));
			break;
		case SI5351_CRYSTAL_LOAD_10PF:
			Serial.print(F(" 10pF"));
			break;
	}
}

static void
prt_status()
{
	uint8_t reg_val = 0;

	reg_val = si5351.si5351_read(SI5351_DEVICE_STATUS);

	if ((reg_val & 0xF8) != 0)
	{
		/* 7
		 * SYS_INIT System Initialization Status.
		 * During power up the device copies the content of the NVM into RAM and performs a system initialization. 
		 * The device is not operational until initialization is complete. It is not recommended to read or write 
		 * registers in RAM through the I2C interface until initialization is
		 * complete. An interrupt will be triggered (INTR pin = 0, Si5351C only) during the system
		 * initialization period.
		 * 0: System initialization is complete. Device is ready.
		 * 1: Device is in system initialization mode.
		 */
		if (reg_val & 0b10000000) Serial.print(F(" SYS_INIT"));

		/* 6
		 * LOL_A PLL A Loss Of Lock Status.
		 * PLL A will operate in a locked state when it has a valid reference from CLKIN or XTAL. A
		 * loss of lock will occur if the frequency of the reference clock forces the PLL to operate 
		 * outside of its lock range as specified in the data sheet, or if the reference clock fails to meet
		 * the minimum requirements of a valid input signal as specified in the Si5351 data sheet. An
		 * interrupt will be triggered (INTR pin = 0, Si5351C only) during a LOL condition.
		 * 0: PLL A is operating normally.
		 * 1: PLL A is unlocked. When the device is in this state it will trigger an interrupt causing the
		 * INTR pin to go low (Si5351C only).
		 * 
		 */
		if (reg_val & 0b01000000) Serial.print(F(" LOL_A"));

		/* 5
		 */
		if (reg_val & 0b00100000) Serial.print(F(" LOL_B"));

		/* 4
		 * LOS_CLKIN CLKIN Loss Of Signal (Si5351C Only).
		 * A loss of signal status indicates if the reference clock fails to meet the minimum requirements 
		 * of a valid input signal as specified in the Si5351 data sheet. An interrupt will be triggered 
		 * (INTR pin = 0, Si5351C only) during a LOS condition.
		 * 0: Valid clock signal at the CLKIN pin.
		 * 1: Loss of signal detected at the CLKIN pin.
		 */
		if (reg_val & 0b00010000) Serial.print(F(" LOS_CLKIN"));

		/* 3
		 * LOS_XTAL Crystal Loss of Signal
		 * A loss of signal status indicates that the crystal failed to meet the minimum requirements
		 * of a valid input signal as specified in the Si5351 datasheet. This bit going high will trigger
		 * an interrupt (INTR pin = 0) on a Si5351C device.
		 * 0: Valid crystal signal at the XA and XB Pins
		 * 1: Loss of crystal signal detected
		 */
		if (reg_val & 0b00001000) Serial.print(F(" LOS_XTAL"));

		/* 2
		 * Reserved.
		 */
	}
		/* 0-1
		 * REVID[1:0] Revision number of the device.
		 */
		if (reg_val & 0b00000011) { 
			Serial.print(F(" REVID:")); 
			Serial.print(reg_val & 0b00000011); 
		}
//	}
}
