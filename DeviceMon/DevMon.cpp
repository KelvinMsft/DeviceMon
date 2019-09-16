// Copyright (c) 2019, Kelvin Chan. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include <fltKernel.h>
#include "ept.h"
#include "log.h"
#include "DevMon.h"
#include "common.h"
#include "util.h"
#include "Spi.h"
#include "Me.h"
extern "C"
{
	//////////////////////////////////////////////
	//	Types
	//
	#define PCI_BAR_64BIT 0x1

	#define MAX_BAR_INDEX 0x6
	
	typedef ULONG64 (*OFFSETMAKECALLBACK)(
		ULONG64 UpperBAR,
		ULONG64 LowerBAR
	);

	typedef bool(*MMIOCALLBACK)(
		GpRegisters*  Context,
		ULONG_PTR InstPointer,
		ULONG_PTR MmioAddress,
		ULONG		  InstLen,
		ULONG		   Access
	);



	typedef struct _PCI_MONITOR_CFG
	{
		UINT8				BusNumber;						// Device Bus
		UINT8				DeviceNum;						// Device Number
		UINT8				FuncNum;						// Device Function

		UINT8				BarOffset[MAX_BAR_INDEX];		// BAR offset in PCI Config , check your chipset datasheet
															// By default is 6 32bit BAR, if your device is 64bit BAR maxium is 3
															// for 64bit, please adding BarAddressWidth
															// LOWER and UPPER offset have to be in order as IntelMeDeviceInfo

		UINT8				BarCount;						// Number of BAR in PCI Config , check your chipset datasheet
		ULONG64				BarAddress[MAX_BAR_INDEX];		// Obtained BAR address, it will be filled out runtime
		MMIOCALLBACK		Callback;						// MMIO handler
		ULONG				BarAddressWidth[MAX_BAR_INDEX]; // 0 by default, PCI_64BIT_DEVICE affect BarOffset parsing n, n+1 => 64bit

		OFFSETMAKECALLBACK  Callback2;						// callback that indicate offset is 64bit BAR
															// offset combination is compatible for those 64bit combined BAR,
															// and it should take device-dependent bitwise operation. 
	}PCIMONITORCFG, *PPCIMONITORCFG;

	//////////////////////////////////////////////
	//	Variables
	//

	PCIMONITORCFG SpiDeviceInfo = {
			SPI_INTERFACE_BUS_NUMBER,
			SPI_INTERFACE_DEVICE_NUMBER,
			SPI_INTERFACE_FUNC_NUMBER ,
			{
				SPI_INTERFACE_SPIBAR_OFFSET,
				0,0,0,0,0
			},
			1,
			{ 0 , 0 , 0 , 0 , 0 , 0 },
			SpiHandleMmioAccessCallback,
			{0,0,0,0,0,0},
			nullptr,
	};

	PCIMONITORCFG IntelMeDeviceInfo = {
		INTEL_ME_BUS_NUMBER,
		INTEL_ME_DEVICE_NUMBER,
		INTEL_ME_1_FUNC_NUMBER ,
		{
			INTEL_ME_BAR_LOWER_OFFSET,
			INTEL_ME_BAR_UPPER_OFFSET,
			0,0,0,0,
		},
		1,
		{ 0 , 0 , 0 , 0 , 0 , 0 },
		IntelMeHandleMmioAccessCallback,
		{
			PCI_BAR_64BIT , 
			0 , 0 , 0 , 0 , 0 ,
		},
		IntelMeHandleBarCallback,
	};

	PCIMONITORCFG IntelMe2DeviceInfo = {
		INTEL_ME_BUS_NUMBER,
		INTEL_ME_DEVICE_NUMBER,
		INTEL_ME_2_FUNC_NUMBER ,
		{
			INTEL_ME_BAR_LOWER_OFFSET,
			INTEL_ME_BAR_UPPER_OFFSET,
			0,0,0,0,
		},
		1,
		{ 0 , 0 , 0 , 0 , 0 , 0 },
		IntelMeHandleMmioAccessCallback,
		{
			PCI_BAR_64BIT ,
			0 , 0 , 0 , 0 , 0 ,
		},
		IntelMeHandleBarCallback,
	};

	PCIMONITORCFG IntelMe3DeviceInfo = {
		INTEL_ME_BUS_NUMBER,
		INTEL_ME_DEVICE_NUMBER,
		INTEL_ME_3_FUNC_NUMBER ,
		{
			INTEL_ME_BAR_LOWER_OFFSET,
			INTEL_ME_BAR_UPPER_OFFSET,
			0,0,0,0,
		},
		1,		 
		{ 0 , 0 , 0 , 0 , 0 , 0 },
		IntelMeHandleMmioAccessCallback,
		{
			PCI_BAR_64BIT ,
			0 , 0 , 0 , 0 , 0 ,
		},
		IntelMeHandleBarCallback,
	};

	PCIMONITORCFG g_MonitorDeviceList[] =
	{
		SpiDeviceInfo,
		IntelMeDeviceInfo,
		IntelMe2DeviceInfo,
		IntelMe3DeviceInfo,
	};

	//////////////////////////////////////////////
	//	Implementation
	//

	ULONG64 DmGetDeviceBarAddress(
		_In_ UINT8 BusNumber, 
		_In_ UINT8 DeviceNumber,
		_In_ UINT8 FunctionNumber,
		_In_ UINT8 Offset
	)
	{
		PHYSICAL_ADDRESS PhysAddr = { 0 };

		ULONG PciBarPa = ReadPCICfg(
			BusNumber,
			DeviceNumber,		//0x1F
			FunctionNumber,     //PCI: 0x05 / LPC: 0
			Offset,				//PCI: 0x10 / LPC: F0
			sizeof(ULONG)
		);

		PhysAddr.QuadPart = ((ULONG64)PciBarPa & 0x00000000FFFFFFFF);

		HYPERPLATFORM_LOG_DEBUG("[IntelMe] PCI= %d %d %d %d Pa= %x %p ", 
			BusNumber, DeviceNumber, FunctionNumber, Offset, PciBarPa, PhysAddr.QuadPart);

		return PhysAddr.QuadPart;
	}

	void DmSetMonitorTrapFlag(
		_In_ bool enable)
	{
		VmxProcessorBasedControls vm_procctl = {
			static_cast<unsigned int>(UtilVmRead(VmcsField::kCpuBasedVmExecControl)) };
		vm_procctl.fields.monitor_trap_flag = enable;
		UtilVmWrite(VmcsField::kCpuBasedVmExecControl, vm_procctl.all);
	}

	NTSTATUS DmEnableMonitor()
	{
		return UtilForEachProcessor(
			[](void* context) {
			UNREFERENCED_PARAMETER(context);
			return UtilVmCall(HypercallNumber::kEnableVmmDeviceMonitor, nullptr);
		},
			nullptr);
	}

	NTSTATUS DmDisableMonitor()
	{
		return UtilForEachProcessor(
			[](void* context) {
			UNREFERENCED_PARAMETER(context);
			return UtilVmCall(HypercallNumber::kDisableVmmDeviceMonitor, nullptr);
		},
			nullptr);
	}

	EptCommonEntry*
	DmGetBarEptEntry(
		_In_ PCIMONITORCFG* Cfg,
		_In_ ULONG			BarIndex,
        _In_ ULONG			BarWidthIndex,
		_In_ EptData*		ept_data
	)
	{
		EptCommonEntry*  Entry = nullptr;
		ULONG64       LowerPa  = 0;
		ULONG64       PciBarPa = 0;

		if (BarIndex >= MAX_BAR_INDEX)
		{
			return Entry;
		}

		LowerPa = DmGetDeviceBarAddress(
			Cfg->BusNumber,
			Cfg->DeviceNum,
			Cfg->FuncNum,
			Cfg->BarOffset[BarIndex]
		);

		PciBarPa = LowerPa;
		if (Cfg->BarAddressWidth[BarWidthIndex] & PCI_BAR_64BIT && ((BarIndex + 1) < MAX_BAR_INDEX))
		{
			ULONG64 UpperPa = DmGetDeviceBarAddress(
				Cfg->BusNumber,
				Cfg->DeviceNum,
				Cfg->FuncNum,
				Cfg->BarOffset[BarIndex + 1]
			);

			HYPERPLATFORM_LOG_DEBUG("[IntelMe] Lower= %I64x_%I64x \r\n", UpperPa, LowerPa);

			PciBarPa = Cfg->Callback2(LowerPa, UpperPa);
		}
 		if (!PciBarPa || !ept_data)
		{
			HYPERPLATFORM_LOG_DEBUG(" - [PCI] Empty PCIBAR, check your datasheet \r\n");
			return Entry;
		}

		//TODO: Get BAR size by filling 0xFFFFFFFF, and monitor all pages.
		Entry = EptGetEptPtEntry(ept_data, PciBarPa);

		//Cannot find in current EPT means it is device memory.
		if (!Entry || !Entry->all)
		{
			HYPERPLATFORM_LOG_DEBUG(" - [PCI] PCIBAR0 never hasn't been memory-mapped, we map it now. \r\n");
			Entry = EptpMapGpaToHpa(PciBarPa, ept_data);
			UtilInveptGlobal();
		}

		if (!Entry || !Entry->all)
		{
			HYPERPLATFORM_LOG_DEBUG(" - [PCI] Create mapping from PCIBAR mmio is failed, terminate now \r\n");
			return Entry;
		}

		Cfg->BarAddress[BarIndex] = PciBarPa;

		HYPERPLATFORM_LOG_DEBUG(" + [PCI] PCIBAR Ept Entry found 0x%I64x \r\n", Entry->all);
		return Entry;
	}
	
	NTSTATUS DmEnableDeviceMonitor(
		_In_ EptData* ept_data)
	{
		EptCommonEntry*  Entry = nullptr;
		for (int i = 0; i < sizeof(g_MonitorDeviceList) / sizeof(PCIMONITORCFG); i++)
		{
			ULONG BarWidthIndex = 0;
			for (int j = 0; j < g_MonitorDeviceList[i].BarCount; j++, BarWidthIndex++)
			{
				Entry = DmGetBarEptEntry(&g_MonitorDeviceList[i], j, BarWidthIndex, ept_data);
				if (!Entry)
				{
					HYPERPLATFORM_LOG_DEBUG("- [PCI] PCIBAR ept Entry Not Found \r\n");
					continue;
				}
					
				HYPERPLATFORM_LOG_DEBUG("+ [PCI] Entry= %p BAR= %p \r\n", 
					Entry->all, g_MonitorDeviceList[i].BarAddress[j]);

				Entry->fields.read_access	 = false;
				Entry->fields.write_access	 = false;
				Entry->fields.execute_access = true;

				if (g_MonitorDeviceList[i].BarAddressWidth[BarWidthIndex] & PCI_BAR_64BIT)
				{
					//Lower - Upper by default.
					j++;
				}
			}
		}
		return STATUS_SUCCESS;
	}

	NTSTATUS DmDisableDeviceMonitor(
		_In_ EptData* ept_data)
	{       
        ULONG            BarWidthIndex = 0; 
		NTSTATUS		status = STATUS_SUCCESS;
		EptCommonEntry*          Entry = nullptr;
		
                for (int i = 0; i < sizeof(g_MonitorDeviceList) / sizeof(PCIMONITORCFG); i++)
		{
			for (int j = 0; j < g_MonitorDeviceList[i].BarCount; j++, BarWidthIndex++ )
			{
				Entry = DmGetBarEptEntry(&g_MonitorDeviceList[i], j, BarWidthIndex, ept_data);
				if (!Entry)
				{
					HYPERPLATFORM_LOG_DEBUG("- [PCI] PCIBAR ept Entry Not Found \r\n");
					continue;
				}

				HYPERPLATFORM_LOG_DEBUG("+ [PCI] Entry= %p BAR= %p \r\n",
					Entry->all, g_MonitorDeviceList[i].BarAddress[j]);

				Entry->fields.read_access = true;
				Entry->fields.write_access = true;
				Entry->fields.execute_access = true;

				if (g_MonitorDeviceList[i].BarAddressWidth[BarWidthIndex] & PCI_BAR_64BIT)
				{
					//Lower - Upper by default.
					j++;
				}
 
			}
		}
		return status;
	}


	bool DmIsMonitoredDevice(
		_In_ ULONG MmioAddress)
	{
		bool ret = false;
		ULONG i = 0;
		ULONG j = 0;
		for (i = 0; i < sizeof(g_MonitorDeviceList) / sizeof(PCIMONITORCFG); i++)
		{
			for (j = 0; j < g_MonitorDeviceList[i].BarCount; j++)
			{
				if (PAGE_ALIGN(MmioAddress) == (void*)g_MonitorDeviceList[i].BarAddress[j])
				{
					ret = true;
					break;
				}
			}
		}
		return ret;
	}

	bool DmExecuteCallback(
		GpRegisters*  Context,
		ULONG_PTR InstPointer,
		ULONG_PTR MmioAddress,
		ULONG		  InstLen,
		ULONG		   Access
	)
	{
		bool ret = false;
		ULONG i = 0;
		ULONG j = 0;
		for (i = 0; i < sizeof(g_MonitorDeviceList) / sizeof(PCIMONITORCFG); i++)
		{
			for (j = 0; j < g_MonitorDeviceList[i].BarCount; j++)
			{
				if (PAGE_ALIGN(MmioAddress) == (void*)g_MonitorDeviceList[i].BarAddress[j])
				{
					if (g_MonitorDeviceList[i].Callback)
					{
						g_MonitorDeviceList[i].Callback(Context, InstPointer, MmioAddress, InstLen, Access);
					}
					break;
				}
			}
		}
		return ret;
	}
}
