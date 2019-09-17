#pragma once
// Copyright (c) 2019, Kelvin Chan. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#pragma once 
#include "Util.h"

#define INTEL_USB3_BUS_NUMBER	  0
#define INTEL_USB3_DEVICE_NUMBER 20
#define INTEL_USB3_FUNC_NUMBER	  0

#define INTEL_USB3_BAR_LOWER_OFFSET  0x10 

extern "C"
{
	bool IntelUsb3HandleMmioAccessCallback(
		GpRegisters*  Context,
		ULONG_PTR InstPointer,
		ULONG_PTR MmioAddress,
		ULONG		  InstLen,
		ULONG		   Access
	);
}