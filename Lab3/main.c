/* Lab 3 Alarm Clock */

/**
The clock must be able to perform five functions. 

1) 	It will display hours and minutes in both graphical 
		and numeric forms on the LCD. The graphical output will 
		include the 12 numbers around a circle, the hour hand, 
		and the minute hand. The numerical output will be easy 
		to read. 
		
-2) 	It will allow the operator to set the current time 
		using switches or a keypad. 
		
-3) 	It will allow the operator to set the alarm time
		including enabling/disabling alarms. 
		
-4) 	It will make a sound at the alarm time. 

-5) 	It will allow the operator to stop the sound. 
		An LED heartbeat will show when the system is running.
**/

/* Hardware Connections */
// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected 
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) unconnected
// Data/Command (pin 4) connected to PA6 (GPIO)
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

#include <stdio.h>
#include <stdint.h>
#include "string.h"
#include "ST7735.h"
#include "../inc/tm4c123gh6pm.h"
#include "PLL.h"

#include "driver-lcd.h"
#include "driver-sound.h"
#include "driver-switch.h"
#include "driver-systick.h"
#include "stdbool.h"

void PortF_Init(void);
#define PF2   (*((volatile uint32_t *)0x40025010))
void EnableInterrupts(void);
void DisableInterrupts(void);
void editClock(char keypressed);
void editTime(void);
void editAlarm(void);
bool updateEditedTime(int8_t* sec, int8_t * min, int8_t * hr, bool* AM, int8_t counters[], int8_t place);

void DelayWait10ms(uint32_t n){uint32_t volatile time;
  while(n){
    time = 727240*2/91;  // 10msec
    while(time){
      time--;
    }
    n--;
  }
}

char lastkey = 0;
		int digital_color = ST7735_YELLOW;


void SystemInit(void){
}

int main(void){ 
	PLL_Init(Bus80MHz); 
	PortF_Init();
	Output_Init();
	ClockTimerInit();
  initKeypad();
  PWM0A_Init(40000, 30000);         // initialize PWM0, 1000 Hz, 75% duty	EnableInterrupts();
	TIMER0_CTL_R &= ~0x00000001;

	draw_digital_time(seconds_counter, minutes_counter, hours_counter, ante_meridiem, digital_color);
	
	// Loop infinitely for sake of updating the time on the LCD
	while(true){
		
		// If alarm should be rung, set the digital time color to RED
		if(ring_alarm == true)
		{
			digital_color = ST7735_RED;
		}
		else {
			digital_color = ST7735_YELLOW;
		}
		
		// Every 100 ms update the time on the LCD
		if(heartbeat_counter % 1000 == 0)
		{
				//draw_clock_face();
				draw_minute_hand( seconds_counter == 0 ?59 : seconds_counter -1, ST7735_BLACK, 60, minutes_counter);
				draw_minute_hand( minutes_counter == 0 ?59 : minutes_counter -1, ST7735_BLACK, 60, minutes_counter);
				draw_minute_hand( hours_counter - 1, ST7735_BLACK, 12, minutes_counter);
				//draw_clock_face();
				draw_minute_hand(seconds_counter, ST7735_WHITE, 60, minutes_counter);
				draw_minute_hand(minutes_counter, ST7735_WHITE, 60, minutes_counter);
				draw_minute_hand(hours_counter, ST7735_WHITE, 12, minutes_counter);
				draw_clock_markers(ST7735_WHITE );
				draw_digital_time(seconds_counter, minutes_counter, hours_counter, ante_meridiem, digital_color);
		}

		// retrieve a keypress from the key fifo
		// will receive a 0 if there are no keypresses
		char keypressed = getKey();
		if( keypressed != 0 && lastkey != keypressed)
		{
			lastkey = keypressed;
			
			// For debug purposes:
			//ST7735_DrawChar(0,0,keypressed,ST7735_GREEN,ST7735_BLACK, 2); 
			//ST7735_DrawChar(12,0,lastkey,ST7735_GREEN,ST7735_BLACK, 2);
			
			// Check if user wants to edit the current time or alarm
			if(keypressed == '*' || keypressed == '#')
			{
				editClock(keypressed);
				Output_Clear();
			}
			
			// If the alarm is ringing, pressing 0
			// results in the alarm to be turned off
			else if(keypressed == '0')
			{
				DisableInterrupts();
				ring_alarm = false;
				EnableInterrupts();
			}
		}
	}
}

// edit either the alarm or curren time
void editClock(char keypressed){
	
	if(keypressed == '*')
		editAlarm();
	if(keypressed == '#')
		editTime();
	
}

void editAlarm(void) {
	int8_t seconds = alarm_seconds_counter;
	int8_t minutes = alarm_minutes_counter;
	int8_t hours = alarm_hours_counter;
	bool AM = alarm_ante_meridiem;
	uint32_t counter = 0;
	int8_t counters[7] = {0};
	
	char keypressed = 0;
	char lastkey = '*';
	int8_t place = 0;
	DelayWait10ms(1);
	bool flip = false;
	bool clock_value_entered = false;
	
	// Spin infinitely, for sake of
	// blinking the time value the user is
	// editing, and reading any keypresses
	// as the user moves from time value to
	// time value
	while(true){
		
		if(counter++ % 500000 == 0)
		{
			if(flip)
				cover_digital_time(place);
			else
			{
				if(clock_value_entered == false)
				draw_digital_time_edit(seconds, minutes, hours,AM, ST7735_RED, place, 'A');
				else
				draw_digital_time(seconds, minutes, hours, AM, ST7735_RED);
			}
			flip = !flip;
		}
		

		keypressed = getKey();
		
		// Disable the alarm if the user presses 99
		if(lastkey == '9' && keypressed == '9')
		{
			
							DisableInterrupts();
				alarm_set = false;
				EnableInterrupts();
				return;
						/*
			else if(keypressed == '*')
			{ 
				DisableInterrupts();
				alarm_set = false;
				EnableInterrupts();
				return;
	*/
		}
		

		if( keypressed != 0 && lastkey != keypressed)
		{
		// Move on to the next key to be edited
		// if user presses *
			if(keypressed == '*')
			{
				clock_value_entered = false;
				place++;		

			}
			

			// Exit the editing alarm mode
			// if the user presses #, signifying
			// that the user is finished editing
			else if( keypressed == '#')
			{
				DisableInterrupts();
				alarm_set = true;
				alarm_seconds_counter = seconds;
				alarm_minutes_counter = minutes;
				alarm_hours_counter = hours;
				alarm_ante_meridiem = AM;
				EnableInterrupts();
				return;
			}
			
			else
			{
				counters[place] = keypressed - 48;
				bool success = updateEditedTime(&seconds, &minutes, &hours, &AM, counters, place);
				clock_value_entered = success == true ? true : false;
			}
			lastkey = keypressed;
		}
	}
}

void editTime(void){
	int8_t seconds = seconds_counter;
	int8_t minutes = minutes_counter;
	int8_t hours = hours_counter;
	bool AM = ante_meridiem;
	uint32_t counter = 0;
	int8_t counters[7] = {0};
	
	char keypressed = 0;
	char lastkey = '#';
	int8_t place = 0;
	DelayWait10ms(1);
	bool flip = false;
	bool clock_value_entered = false;
	
		// Spin infinitely, for sake of
	// blinking the time value the user is
	// editing, and reading any keypresses
	// as the user moves from time value to
	// time value
	while(true){
		
		if(counter++ % 500000 == 0)
		{
			if(flip)
				cover_digital_time(place);
			else
			{
				if(clock_value_entered == false)
				draw_digital_time_edit(seconds, minutes, hours,AM, digital_color, place, 'T');
				else
				draw_digital_time(seconds, minutes, hours, AM, digital_color);
			}
			flip = !flip;
		}
		

		keypressed = getKey();
		if( keypressed != 0 && lastkey != keypressed)
		{
		// Move on to the next key to be edited
		// if user presses *
			if(keypressed == '*')
			{
				place++;		
				clock_value_entered = false;
			}
			// Exit the editing alarm mode
			// if the user presses #, signifying
			// that the user is finished editing
			else if( keypressed == '#')
			{
				DisableInterrupts();
				seconds_counter = seconds;
				minutes_counter = minutes;
				hours_counter = hours;
				ante_meridiem = AM;
				EnableInterrupts();
				return;
			}
			
			// The user pressed a digit
			// to edit the time with
			// there for update the time
			// accordingly
			else
			{
				counters[place] = keypressed - 48;
				bool success = updateEditedTime(&seconds, &minutes, &hours, &AM, counters, place);
				clock_value_entered = success == true ? true : false;
			}
			lastkey = keypressed;
		}
	}
}

// Inputs: sec, min, hr, AM, to store the updated time in
// Inputs: counters[] holds the keypress char
//         int8_t  place specifies in which place in the counters[] array
//         that there is a character value
//         int8_t place also determines the time value
//         that is to be edited

//         Ensures that user keypress input is valid
//         e.g. you can't put 3 for the tens place when editing
//         the hour

//         place_value: 0  |1|2|3|4|5|6|
//         time  value: PM |1|2:3|2|5|9|
//         for example:
bool updateEditedTime(int8_t* sec, int8_t * min, int8_t * hr, bool* AM, int8_t counters[], int8_t place)
{
			uint8_t temp = 0;
	bool success = false;
	switch(place)
	{

		case 0:
			*AM = counters[0];
			success = true;
			break;
		case 1:
		{
			 temp = *hr;
			*hr = (*hr) % 10;
			*hr = (*hr) + counters[1]*10;
			
			if(counters[1] > 1 || counters[1] < 0)
			{
				*hr = temp;
				return false;
			}
			success = true;
			break;
		}
		case 2:
		{
			temp = *hr;
			*hr = ((*hr) / 10) * 10;
			*hr = (*hr) + counters[2];
			if(counters[2] > 9 ||  counters[2] < 0 )
			{
				*hr = temp;
				return false;
			}
			if(( (temp/10) < 1 && counters[2] < 1))
				{
					(*hr)++;
					return false;
				}
				
		  if(( (temp/10) > 0 && counters[2] > 2))
			{
				*hr = temp;
				return false;
			}
			success = true;
			break;

		}
		case 3:
		{
			temp = *min;
			*min = (*min) % 10;
			*min = (*min) + counters[3]*10;
			if(counters[3] > 5 || counters[3] < 0)
			{
				*min = temp;
				return false;
			}
			success = true;
						break;

		}
		case 4:
		{
			temp = *min;
			*min = ((*min) / 10) * 10;
			*min = (*min) + counters[4];
			if(counters[4] > 9 || counters[4] < 0)
			{
				*min = temp;
				return false;
			}
			success = true;
						break;

		}
		case 5:
		{
			temp = *sec;
			*sec = (*sec) % 10;
			*sec = (*sec) + counters[5]*10;
			if(counters[5] > 5 || counters[5] < 0)
			{
				*min = temp;
				return false;
			}
			success = true;
						break;

		}
		case 6:
		{
			temp = *sec;
			*sec = ((*sec) / 10) * 10;
			*sec = (*sec) + counters[6];
			if(counters[6] > 9 || counters[6] < 0)
			{
				*min = temp;
				return false;
			}
			success = true;
						break;
		}
	} // end switch()
	return success;
}

// PF4 is input
// Make PF2 an output, enable digital I/O, ensure alt. functions off
void PortF_Init(void){ 
  SYSCTL_RCGCGPIO_R |= 0x20;        // 1) activate clock for Port F
  while((SYSCTL_PRGPIO_R&0x20)==0){}; // allow time for clock to start
                                    // 2) no need to unlock PF2, PF4
  GPIO_PORTF_PCTL_R &= ~0x000F0F00; // 3) regular GPIO
  GPIO_PORTF_AMSEL_R &= ~0x14;      // 4) disable analog function on PF2, PF4
  GPIO_PORTF_PUR_R |= 0x10;         // 5) pullup for PF4
  GPIO_PORTF_DIR_R |= 0x04;         // 5) set direction to output
  GPIO_PORTF_AFSEL_R &= ~0x14;      // 6) regular port function
  GPIO_PORTF_DEN_R |= 0x14;         // 7) enable digital port
}
