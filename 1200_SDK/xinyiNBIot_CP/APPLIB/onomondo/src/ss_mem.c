/** 
* @file     ss_mem.c
* @date     2024-06-25
* @author   Onomondo
* @brief    memory management wrapper of the xy platform in accordance to onomondo-uicc expectations
*/

#include "xy_system.h"
#include "onomondo/softsim/mem.h"

void *port_malloc(size_t size) 
{ 
	return xy_malloc(size);
}

void port_free(void *p) 
{ 
	if (p != NULL)
		xy_free(p);
}
