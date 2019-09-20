#pragma once
 #include "Util.h"

typedef struct _FUZZ
{
	UCHAR   dummy1;
	USHORT  dummy2;
	ULONG   dummy3;
	ULONG64 dummy4;
}FUZZ;

bool StartFuzz(
	GpRegisters*  Context,
	ULONG_PTR InstPointer,
	ULONG_PTR MmioAddress,
	ULONG		  InstLen,
	FUZZ*			 Fuzz
);