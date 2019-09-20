// Copyright (c) 2019, Kelvin Chan. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include <fltKernel.h>

#include "DevMon.h"
#include "Usb.h"
#include "log.h"

#include "include\capstone\capstone.h"
#include "Fuzzer.h"

#define	FUZZ_ENABLE		1

FUZZ NicFuzzer = { 0 };

EXTERN_C
{
	///////////////////////////////////////////////
	// Implementation
	//
	ULONG64 IntelNicHandleBarCallback(
		ULONG64 LowerBAR,
		ULONG64 UpperBAR
	)
	{
		return LowerBAR;
	}


	bool IntelNicHandleMmioAccessCallback(
		GpRegisters*  Context,
		ULONG_PTR InstPointer,
		ULONG_PTR MmioAddress,
		ULONG		  InstLen,
		ULONG		   Access
	)
{
	if (Access == 0x1 &&
		KeGetCurrentIrql() <= DISPATCH_LEVEL)
	{
		HYPERPLATFORM_LOG_DEBUG_SAFE("[Nic] context= %p Address= %p is being %d access by %p Len= %d Sign= %x\r\n",
			Context, MmioAddress, Access, InstPointer, InstLen, *(UCHAR*)InstPointer);
#ifdef FUZZ_ENABLE
		//Read from MMIO space, fuzz it.
		//return StartFuzz(Context, InstPointer, MmioAddress, InstLen, &NicFuzzer);
#endif 
	}
	 

	return false;
}
}