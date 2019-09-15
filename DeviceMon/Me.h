// Copyright (c) 2019, Kelvin Chan. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#pragma once 
#include "Util.h"

#define INTEL_ME_BUS_NUMBER		0
#define INTEL_ME_DEVICE_NUMBER 22
#define INTEL_ME_1_FUNC_NUMBER	0
#define INTEL_ME_2_FUNC_NUMBER	1
#define INTEL_ME_3_FUNC_NUMBER	4

#define INTEL_ME_BAR_UPPER_OFFSET  0x14
#define INTEL_ME_BAR_LOWER_OFFSET  0x10 

#define INTEL_ME_LOWER_MASK 0xFFFFFFFF0
#define INTEL_ME_UPPER_MASK 0xFFFFFFFFF
extern "C"
{
	ULONG64 IntelMeHandleBarCallback(
		ULONG64 LowerBAR,
		ULONG64 UpperBAR
	);

	bool IntelMeHandleMmioAccessCallback(
		GpRegisters*  Context,
		ULONG_PTR InstPointer,
		ULONG_PTR MmioAddress,
		ULONG		  InstLen,
		ULONG		   Access
	);
}