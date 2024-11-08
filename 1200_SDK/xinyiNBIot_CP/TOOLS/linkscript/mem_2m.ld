/*
 * Memory Spaces Definitions.
 *
 * Need modifying for a specific board. 
 *   FLASH.ORIGIN: starting address of flash
 *   FLASH.LENGTH: length of flash
 *   RAM.ORIGIN: starting address of RAM bank 0
 *   RAM.LENGTH: length of RAM bank 0
 *
 * The values below can be addressed in further linker scripts
 * using functions like 'ORIGIN(RAM)' or 'LENGTH(RAM)'.
 */

MEMORY
{
	FLASH (xrw) : ORIGIN = 0x00017000, LENGTH = 0x0015d000    /* 1M-3K, text in flash, and data to move data section */
	RAM1  (xrw) : ORIGIN = 0x10000000, LENGTH = 0x00028000    /* 128+32K, text/data/bss/heap/stack */
}

/*
 * For external ram use something like:

  RAM (xrw) : ORIGIN = 0x68000000, LENGTH = 64K

 */
