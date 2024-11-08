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
