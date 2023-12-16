/* Linker script for STM32FEBK6T6, 32K flash, 10K RAM). */

/* Define memory regions. */
MEMORY
{
	rom (rx) : ORIGIN = 0x08000000, LENGTH = 32K
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 10K
}

/* Include the common ld script. */
INCLUDE libopencm3_stm32f1.ld

