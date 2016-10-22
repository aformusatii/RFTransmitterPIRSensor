/********************************************************************************
	Includes
********************************************************************************/
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <stdlib.h>

#include "../nrf24l01/RF24.h"
#include "../common/util.h"
#include "../nrf24l01/atmega328.h"
#include "../atmega328/mtimer.h"

extern "C" {
#include "../atmega328/usart.h"
}

/********************************************************************************
	Macros and Defines
********************************************************************************/

/********************************************************************************
	Function Prototypes
********************************************************************************/

/********************************************************************************
	Global Variables
********************************************************************************/
RF24 radio;
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
volatile uint64_t resendDetectionJobCicles = 0;

/********************************************************************************
	Interrupt Service
********************************************************************************/
ISR(USART_RX_vect)
{
	handle_usart_interrupt();
}

ISR(TIMER1_OVF_vect)
{
	incrementOvf();
}

/********************************************************************************
	Main
********************************************************************************/
int main(void) {
    _delay_ms(50);

    // INT0 input
    _in(DDD2, DDRD);

    // initialize usart module
	usart_init();

    initTimer();

    // enable interrupts
    sei();

	// Console friendly output
    printf(CONSOLE_PREFIX);

    radio.begin();
    radio.setRetries(15, 15);
    radio.setPayloadSize(8);
    radio.setPALevel(RF24_PA_MAX);
	radio.setChannel(120);

    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1, pipes[1]);

    radio.startListening();

    radio.printDetails();

    _delay_ms(10);

    radio.stopListening();

    _delay_ms(10);

	uint8_t data[] = {177, 102};
	bool ok;

	uint8_t retry = 5;
	while (retry--) {
		_delay_ms(10);
		debug_print("Try %d", retry);
		ok = radio.write(data, 2);
	}

	resendDetectionJobCicles = convertSecondsToCicles(5);

	// main loop
    while (1) {
    	uint64_t currentTimeCicles = getCurrentTimeCicles();

   	    // Job: Re-send PIR Detection
    	if ((resendDetectionJobCicles != 0) && (currentTimeCicles >= resendDetectionJobCicles)) {
    		resendDetectionJobCicles = convertSecondsToCicles(5);

    		radio.write(data, 2);
    		debug_print("R");
    	}

    	// ================= main usart loop for console ========================
    	usart_check_loop();
    }
}


void handle_usart_cmd(char *cmd, char *args) {
	if (strcmp(cmd, "test") == 0) {
		printf("\n TEST [%s]", args);
	}
}
