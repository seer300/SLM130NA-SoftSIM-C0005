#pragma once

#include <stdint.h>


typedef struct
{
	struct List_t	*next;
	void *bufaddr;
}ZeroCopy_Buf_t;


void Insert_ZeroCopy_Buf(void *buf_addr);

void Delet_ZeroCopy_Buf(void *buf_addr);

void Free_ZeroCopy_Buf();
