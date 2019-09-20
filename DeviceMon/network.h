#pragma once
// Copyright (c) 2019, Kelvin Chan. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#pragma once 
#include "Util.h"

#define MARVELL_NIC_BUS_NUMBER			  1
#define MARVELL_NIC_DEVICE_NUMBER		  0 
#define MARVELL_NIC_FUNC_NUMBER			  0

#define MARVELL_NIC_BAR_LOWER_OFFSET   0x10 
#define MARVELL_NIC_BAR_HIGH_OFFSET    0x14

#define MARVELL_NIC_BAR1_LOWER_OFFSET  0x18
#define MARVELL_NIC_BAR1_HIGHT_OFFSET  0x1C

extern "C"
{
	ULONG64 IntelNicHandleBarCallback(
		ULONG64 LowerBAR,
		ULONG64 UpperBAR
	);

	bool IntelNicHandleMmioAccessCallback(
		GpRegisters*  Context,
		ULONG_PTR InstPointer,
		ULONG_PTR MmioAddress,
		ULONG		  InstLen,
		ULONG		   Access
	);
}