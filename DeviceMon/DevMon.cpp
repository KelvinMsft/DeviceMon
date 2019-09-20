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
#include "Usb.h"
#include "network.h"
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
		ULONG64				BarLimit[MAX_BAR_INDEX];		// Obtained BAR size   , it will be filled out runtime

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
		{ 0 , 0 , 0 , 0 , 0 , 0 },
		IntelMeHandleMmioAccessCallback,
		{
			PCI_BAR_64BIT ,
			0 , 0 , 0 , 0 , 0 ,
		},
		IntelMeHandleBarCallback,
	};


	PCIMONITORCFG IntelUsb3DeviceInfo = {
		INTEL_USB3_BUS_NUMBER,
		INTEL_USB3_DEVICE_NUMBER,
		INTEL_USB3_FUNC_NUMBER ,
		{
			INTEL_USB3_BAR_LOWER_OFFSET,
			0,0,0,0,0,
		},
		1,
		{ 0 , 0 , 0 , 0 , 0 , 0 },
		{ 0 , 0 , 0 , 0 , 0 , 0 },
		IntelUsb3HandleMmioAccessCallback,
		{
			0 ,	0 , 0 , 0 , 0 , 0 ,
		},
		nullptr,
	};

	PCIMONITORCFG MarvellNicDeviceInfo = {
		MARVELL_NIC_BUS_NUMBER,
		MARVELL_NIC_DEVICE_NUMBER,
		MARVELL_NIC_FUNC_NUMBER ,
		{
			MARVELL_NIC_BAR_LOWER_OFFSET,
			MARVELL_NIC_BAR_HIGH_OFFSET,
			MARVELL_NIC_BAR1_LOWER_OFFSET,
			MARVELL_NIC_BAR1_HIGHT_OFFSET,
			0,0,
		},
		2,
		{ 0 , 0 , 0 , 0 , 0 , 0 },
		{ 0 , 0 , 0 , 0 , 0 , 0 },
		IntelNicHandleMmioAccessCallback,
		{
			PCI_BAR_64BIT ,	PCI_BAR_64BIT ,
			0 , 0 , 0 , 0 ,
		},
		IntelNicHandleBarCallback,
	};


	PCIMONITORCFG g_MonitorDeviceList[] =
	{
		SpiDeviceInfo,
		IntelMeDeviceInfo,
		IntelMe2DeviceInfo,
		IntelMe3DeviceInfo,
		IntelUsb3DeviceInfo,
		MarvellNicDeviceInfo,
	};

	//////////////////////////////////////////////
	//	Implementation
	//
	ULONG DmGetDeviceBarSize(
		_In_ UINT8 BusNumber,
		_In_ UINT8 DeviceNumber,
		_In_ UINT8 FunctionNumber,
		_In_ UINT8 Offset
	)
	{
		ULONG BaseAddr = 0;
		ULONG Size = 0;
		BaseAddr = ReadPCICfg(
			BusNumber,
			DeviceNumber,		
			FunctionNumber,     
			Offset,				
			sizeof(ULONG)
		); 
		
		WritePCICfg(
			BusNumber,
			DeviceNumber,		
			FunctionNumber,     
			Offset,				
			sizeof(ULONG),
			0xFFFFFFFF
		);

		Size = ReadPCICfg(
			BusNumber,
			DeviceNumber,		
			FunctionNumber,     
			Offset,				
			sizeof(ULONG)
		);

		WritePCICfg(
			BusNumber,
			DeviceNumber,		
			FunctionNumber,     
			Offset,				
			sizeof(ULONG),
			BaseAddr
		);

		Size &= 0xFFFFFFF0;
		Size = ((~Size) + 1);

		return Size;
	}


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

		PhysAddr.QuadPart = ((ULONG64)PciBarPa & 0x00000000FFFFFFF0);

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

	void InvalidateEptPages(EptData* ept_data, ULONG_PTR GuestPhysAddr, ULONG Size)
	{
		EptCommonEntry* Entry = nullptr;
		ULONG PciBarSpacePa = (ULONG)GuestPhysAddr;
		for (int k = 0; k < ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, Size); k++)
		{
			Entry = EptGetEptPtEntry(ept_data, PciBarSpacePa);
			if (!Entry || !Entry->all)
			{
				HYPERPLATFORM_LOG_DEBUG_SAFE(" - [PCI] PCIBAR0 never hasn't been memory-mapped, we map it now. \r\n");
				Entry = EptpMapGpaToHpa(PciBarSpacePa, ept_data);
				UtilInveptGlobal();
			}
			if (!Entry || !Entry->all)
			{
				break;
			}
			Entry->fields.read_access = false;
			Entry->fields.write_access = false;
			Entry->fields.execute_access = true;

			PciBarSpacePa = (ULONG)GuestPhysAddr + PAGE_SIZE * k;
		}
	}

	void ValidateEptPages(EptData* ept_data, ULONG_PTR GuestPhysAddr, ULONG Size)
	{
		EptCommonEntry* Entry = nullptr;
		ULONG PciBarSpacePa = (ULONG)GuestPhysAddr;
		for (int k = 0; k < ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, Size); k++)
		{
			Entry = EptGetEptPtEntry(ept_data, PciBarSpacePa);
			if (!Entry || !Entry->all)
			{
				HYPERPLATFORM_LOG_DEBUG_SAFE(" - [PCI] PCIBAR0 never hasn't been memory-mapped, we map it now. \r\n");
				PciBarSpacePa = (ULONG)GuestPhysAddr + PAGE_SIZE * k;
				continue;
			}
			if (!Entry || !Entry->all)
			{
				break;
			}

			Entry->fields.read_access	 = true;
			Entry->fields.write_access	 = true;
			Entry->fields.execute_access = true;

			PciBarSpacePa = (ULONG)GuestPhysAddr + PAGE_SIZE * k;
		}
	}

	void
	DmInitBarInfo(
		_In_ PCIMONITORCFG* Cfg,
		_In_ ULONG			BarOffsetIndex,
		_In_ ULONG			BarIndex)
	{
		ULONG64       LowerPa  = 0;
		ULONG64       PciBarPa = 0;
		ULONG			  size = 0;
		if (BarOffsetIndex >= MAX_BAR_INDEX)
		{
			return ;
		}

		size = DmGetDeviceBarSize(
			Cfg->BusNumber,
			Cfg->DeviceNum,
			Cfg->FuncNum,
			Cfg->BarOffset[BarOffsetIndex]
		);

		if (!size)
		{
			return ;
		}

		LowerPa = DmGetDeviceBarAddress(
			Cfg->BusNumber,
			Cfg->DeviceNum,
			Cfg->FuncNum,
			Cfg->BarOffset[BarOffsetIndex]
		);

		PciBarPa = LowerPa;
		if (Cfg->BarAddressWidth[BarIndex] & PCI_BAR_64BIT && ((BarOffsetIndex + 1) < MAX_BAR_INDEX))
		{
			ULONG64 UpperPa = DmGetDeviceBarAddress(
				Cfg->BusNumber,
				Cfg->DeviceNum,
				Cfg->FuncNum,
				Cfg->BarOffset[BarOffsetIndex + 1]
			);

			HYPERPLATFORM_LOG_DEBUG_SAFE("[IntelMe] Lower= %I64x_%I64x \r\n", UpperPa, LowerPa);

			PciBarPa = Cfg->Callback2(LowerPa, UpperPa);
		}
 		if (!PciBarPa)
		{
			HYPERPLATFORM_LOG_DEBUG_SAFE(" - [PCI] Empty PCIBAR, check your datasheet \r\n");
			return ;
		}

		Cfg->BarAddress[BarIndex] = PciBarPa;
		Cfg->BarLimit[BarIndex]   = PciBarPa + size;
		return ;
	}
	
	NTSTATUS DmEnableDeviceMonitor(
		_In_ EptData* ept_data)
	{
		for (int i = 0; i < sizeof(g_MonitorDeviceList) / sizeof(PCIMONITORCFG); i++)
		{
			ULONG BarIndex = 0;
			for (int BarOffsetIndex = 0; BarIndex <  g_MonitorDeviceList[i].BarCount; BarOffsetIndex++, BarIndex++)
			{ 
				DmInitBarInfo(&g_MonitorDeviceList[i], BarOffsetIndex, BarIndex);
				
				if (!g_MonitorDeviceList[i].BarAddress[BarIndex] || !g_MonitorDeviceList[i].BarLimit[BarIndex] ||
					((g_MonitorDeviceList[i].BarAddress[BarIndex] & 0xFFFFF00000000000) == 0xFFFFF00000000000) ||
					(g_MonitorDeviceList[i].BarLimit[BarIndex] < g_MonitorDeviceList[i].BarAddress[BarIndex]))
				{
					HYPERPLATFORM_LOG_DEBUG("- [PCI] PCIBAR Found Error \r\n");
					if (g_MonitorDeviceList[i].BarAddressWidth[BarIndex] & PCI_BAR_64BIT)
					{
						//Lower - Upper by default.
						BarOffsetIndex++;
					}
					continue;
				}
	
				HYPERPLATFORM_LOG_DEBUG("[PCI] Start= %p Limit= %p MMIO Size= %x Spanned Page= %d \r\n", 
					g_MonitorDeviceList[i].BarAddress[BarIndex],
					g_MonitorDeviceList[i].BarLimit[BarIndex], 
					g_MonitorDeviceList[i].BarLimit[BarIndex] - g_MonitorDeviceList[i].BarAddress[BarIndex] ,
					ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, g_MonitorDeviceList[i].BarLimit[BarIndex] - g_MonitorDeviceList[i].BarAddress[BarIndex])
				);

				InvalidateEptPages(ept_data, g_MonitorDeviceList[i].BarAddress[BarIndex], 
					(ULONG)(g_MonitorDeviceList[i].BarLimit[BarIndex] - g_MonitorDeviceList[i].BarAddress[BarIndex])
				);
 
				if (g_MonitorDeviceList[i].BarAddressWidth[BarIndex] & PCI_BAR_64BIT)
				{
					//Lower - Upper by default.
					BarOffsetIndex++;
				}
			}
		}
		return STATUS_SUCCESS;
	}

	NTSTATUS DmDisableDeviceMonitor(
		_In_ EptData* ept_data)
	{       
		NTSTATUS		status = STATUS_SUCCESS;
		for (int i = 0; i < sizeof(g_MonitorDeviceList) / sizeof(PCIMONITORCFG); i++)
		{
			ULONG BarIndex = 0;
			for (int BarOffsetIndex = 0; BarIndex < g_MonitorDeviceList[i].BarCount; BarOffsetIndex++, BarIndex++)
			{	
				DmInitBarInfo(&g_MonitorDeviceList[i], BarOffsetIndex, BarIndex);
				
				if (!g_MonitorDeviceList[i].BarAddress[BarIndex] || !g_MonitorDeviceList[i].BarLimit[BarIndex] ||
					((g_MonitorDeviceList[i].BarAddress[BarIndex] & 0xFFFFF00000000000) == 0xFFFFF00000000000) ||
					(g_MonitorDeviceList[i].BarLimit[BarIndex] < g_MonitorDeviceList[i].BarAddress[BarIndex]))
				{
					HYPERPLATFORM_LOG_DEBUG("- [PCI] PCIBAR Found Error \r\n"); 
					if (g_MonitorDeviceList[i].BarAddressWidth[BarIndex] & PCI_BAR_64BIT)
					{
						//Lower - Upper by default.
						BarOffsetIndex++;
					}
					continue;
				}


				HYPERPLATFORM_LOG_DEBUG("[PCI] Start= %p Limit= %p MMIO Size= %x Spanned Page= %d \r\n",
					g_MonitorDeviceList[i].BarAddress[BarIndex],
					g_MonitorDeviceList[i].BarLimit[BarIndex],
					g_MonitorDeviceList[i].BarLimit[BarIndex] - g_MonitorDeviceList[i].BarAddress[BarIndex],
					ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, g_MonitorDeviceList[i].BarLimit[BarIndex] - g_MonitorDeviceList[i].BarAddress[BarIndex])
				);

				ValidateEptPages(ept_data, g_MonitorDeviceList[i].BarAddress[BarIndex], 
					(ULONG)(g_MonitorDeviceList[i].BarLimit[BarIndex] - g_MonitorDeviceList[i].BarAddress[BarIndex])
				);

				if (g_MonitorDeviceList[i].BarAddressWidth[BarIndex] & PCI_BAR_64BIT)
				{
					//Lower - Upper by default.
					BarOffsetIndex++;
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
				if (MmioAddress >= (ULONG)g_MonitorDeviceList[i].BarAddress[j] && 
					MmioAddress <= (ULONG)g_MonitorDeviceList[i].BarLimit[j])
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
				if (MmioAddress >= (ULONG)g_MonitorDeviceList[i].BarAddress[j] &&
					MmioAddress <= (ULONG)g_MonitorDeviceList[i].BarLimit[j])
				{
					if (g_MonitorDeviceList[i].Callback)
					{
						ret = g_MonitorDeviceList[i].Callback(Context, InstPointer, MmioAddress, InstLen, Access);
					}
					break;
				}
			}
		}
		return ret;
	}
}
