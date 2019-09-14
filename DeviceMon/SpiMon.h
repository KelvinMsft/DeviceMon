#pragma once
#include "ept.h"
#include "util.h"
extern "C"
{ 
void	 SpiSetMonitorTrapFlag(void* sh_data, bool enable);
NTSTATUS SpiEnableMonitor();
NTSTATUS SpiDisableMonitor();
EptCommonEntry* SpiEnableSpiMonitor(EptData* ept_data);
NTSTATUS SpiDisableSpiMonitor(EptData* ept_data);
ULONG64  SpiGetSpiBar0();
bool	 SpiHandleMmioAccess(GpRegisters* Context, ULONG_PTR Rip, ULONG_PTR Address, ULONG Width, ULONG Access);

}