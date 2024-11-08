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
	FLASH (xrw)   : ORIGIN = 0x30217000, LENGTH = 0x1BC000	/* 852K,text in flash */
	RAM (xrw)     : ORIGIN = 0x01000000, LENGTH = 0x1FF80	/* 104K,text/data/bss/heap/stack in RAM */
    RETMEM (xrw)  : ORIGIN = 0x60010000, LENGTH = 0x100     /* 512, boot text/data*/
}
