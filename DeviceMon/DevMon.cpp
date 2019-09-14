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
extern "C"
{
	//////////////////////////////////////////////
	//	Types
	//

	typedef bool(*MMIOCALLBACK)(GpRegisters*  Context,
		ULONG_PTR InstPointer,
		ULONG_PTR MmioAddress,
		ULONG		  InstLen,
		ULONG		   Access
	);

	typedef struct _PCI_MONITOR_CFG
	{
		UINT8	BusNumber;			//
		UINT8	DeviceNum;			//
		UINT8	FuncNum;			//
		UINT8	BarOffset[6];		// BAR offset in PCI Config , check your chipset datasheet
		UINT8	BarCount;			// Number of BAR in PCI Config , check your chipset datasheet
		ULONG64 BarAddress[6];		// Retrieved address 
		MMIOCALLBACK Callback;		// callback whenever access to this adress
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
			},
			1,
			{ 0 , 0 , 0 , 0 , 0, 0 },
			SpiHandleMmioAccessCallback,
	};

	PCIMONITORCFG g_MonitorDeviceList[] =
	{
		SpiDeviceInfo,
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
		_In_ ULONG	  BarIndex,
		_In_ EptData*  ept_data
	)
	{
		EptCommonEntry*  Entry = nullptr;
		ULONG64       PciBarPa = 0;

		PciBarPa = DmGetDeviceBarAddress(
			Cfg->BusNumber,
			Cfg->DeviceNum,
			Cfg->FuncNum,
			Cfg->BarOffset[BarIndex]
		);

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
			for (int j = 0; j < g_MonitorDeviceList[i].BarCount; j++)
			{
				Entry = DmGetBarEptEntry(&g_MonitorDeviceList[i], j, ept_data);
				if (!Entry)
				{
					HYPERPLATFORM_LOG_DEBUG("- [PCI] PCIBAR ept Entry Not Found \r\n");
					continue;
				}

				HYPERPLATFORM_LOG_DEBUG("+ [PCI] Entry= %p BAR= %p \r\n", 
					Entry->all, g_MonitorDeviceList[i].BarAddress[j]);

				Entry->fields.read_access		= false;
				Entry->fields.write_access		= false;
				Entry->fields.execute_access	= true;
			}
		}
		return STATUS_SUCCESS;
	}

	NTSTATUS DmDisableDeviceMonitor(
		_In_ EptData* ept_data)
	{
		NTSTATUS		status = STATUS_SUCCESS;
		EptCommonEntry*  Entry = nullptr;
		for (int i = 0; i < sizeof(g_MonitorDeviceList) / sizeof(PCIMONITORCFG); i++)
		{
			for (int j = 0; j < g_MonitorDeviceList[i].BarCount; j++)
			{
				Entry = DmGetBarEptEntry(&g_MonitorDeviceList[i], j, ept_data);
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