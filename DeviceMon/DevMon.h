// Copyright (c) 2019, Kelvin Chan. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#pragma once
#include "ept.h"
#include "util.h"
extern "C"
{ 

void	 
DmSetMonitorTrapFlag(
	bool enable
);

NTSTATUS 
DmEnableMonitor();

NTSTATUS
DmDisableMonitor();

NTSTATUS
DmEnableDeviceMonitor(EptData* ept_data);

NTSTATUS 
DmDisableDeviceMonitor(EptData* ept_data);

bool	 DmExecuteCallback(
	GpRegisters*  Context,
	ULONG_PTR InstPointer,
	ULONG_PTR MmioAddress,
	ULONG		  InstLen,
	ULONG		   Access
);

bool	 DmIsMonitoredDevice(ULONG MmioAddress);
}