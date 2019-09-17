// Copyright (c) 2019, Kelvin Chan. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include <fltKernel.h>

#include "DevMon.h"
#include "Usb.h"
#include "log.h"


EXTERN_C
{
	///////////////////////////////////////////////
	// Implementation
	//
	bool IntelUsb3HandleMmioAccessCallback(
		GpRegisters*  Context,
		ULONG_PTR InstPointer,
		ULONG_PTR MmioAddress,
		ULONG		  InstLen,
		ULONG		   Access
	)
	{
	HYPERPLATFORM_LOG_DEBUG_SAFE("[USB3] context= %p Address= %p is being %d access by %p Len= %d\r\n",
			Context, MmioAddress, Access, InstPointer, InstLen);

		return true;
	}
}