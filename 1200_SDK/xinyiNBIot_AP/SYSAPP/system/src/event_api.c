#include "xy_system.h"

volatile uint32_t g_event_bitmap = 0;

void set_event(uint32_t event)
{
    DisablePrimask();
    g_event_bitmap = g_event_bitmap | (1U << event);
    EnablePrimask();
}

void clear_event(uint32_t event)
{
    DisablePrimask();
    g_event_bitmap = g_event_bitmap & (~(1U << event));
    EnablePrimask();
}

bool is_event_set(uint32_t event)
{
	if(g_event_bitmap & (1U << event))
		return 1;
    else
		return 0;
}

void clear_all_event()
{
    g_event_bitmap = 0;
}

