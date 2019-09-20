// Copyright (c) 2019, Kelvin Chan. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include <fltKernel.h>

#include "DevMon.h"
#include "Me.h"
#include "log.h"

///////////////////////////////////////////////
// Implementation
//

ULONG64 IntelMeHandleBarCallback(
	ULONG64 LowerBAR,
	ULONG64 UpperBAR
)
{
	PHYSICAL_ADDRESS IntelMeBarPa = {0};

	if (((LowerBAR & 0xFFFFFFFF) == 0xFFFFFFFF) ||
		((UpperBAR & 0xFFFFFFFF) == 0xFFFFFFFF))
	{
		return 0;
	}

		HYPERPLATFORM_LOG_DEBUG("[IntelMe] Made Address= %I64X_%I64X  \r\n", UpperBAR, LowerBAR);

	IntelMeBarPa.HighPart = (ULONG)UpperBAR;
	IntelMeBarPa.LowPart  = (ULONG)(LowerBAR & INTEL_ME_LOWER_MASK);
	
	HYPERPLATFORM_LOG_DEBUG("[IntelMe] Made Address= %p ", IntelMeBarPa.QuadPart);

	return IntelMeBarPa.QuadPart;

}

bool IntelMeHandleMmioAccessCallback(
	GpRegisters*  Context,
	ULONG_PTR InstPointer,
	ULONG_PTR MmioAddress,
	ULONG		  InstLen,
	ULONG		   Access
)
{
	HYPERPLATFORM_LOG_DEBUG("[IntelMe] context= %p Address= %p is being %d access by %p Len= %d\r\n",
		Context, MmioAddress, Access, InstPointer,InstLen);

	return false;
}