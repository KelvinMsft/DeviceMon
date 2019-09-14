#include <fltKernel.h>
#include "ept.h"
#include "log.h"
#include "SpiMon.h"
#include "common.h"
#include "util.h"
extern "C"
{
	//////////////////////////////////////////////
	//	Types
	// 
	char* AccessStr[2] = { 
		"Read",
		"Write"
	};

	typedef union _BIOS_HSFSTS_CTRL {
		unsigned int all;
		struct {
			unsigned int FlashCycleDone : 1;   //!<    [0]
			unsigned int FlashCycleError : 1;   //!<    [1]
			unsigned int AccessErrorLog : 1;   //!<    [2]
			unsigned int Reserved : 2;   //!<  [4:3]
			unsigned int SpiCycleInProgress : 1;   //!<    [5]
			unsigned int Reserved2 : 5;   //!< [10:6]
			unsigned int FlashCfgLockdown : 1;   //!<   [11]
			unsigned int PRR34Lockdown : 1;   //!<   [12]
			unsigned int FdoPss : 1;   //!<   [13]
			unsigned int FlashDescValid : 1;   //!<   [14]
			unsigned int FLOCKDN : 1;   //!<   [15]
			unsigned int FlashCycleGo : 1;   //!<   [16]
			unsigned int FlashCycleCmd : 4;   //!< [20:17]
			unsigned int WriteEnableType : 1;   //!<   [21]
			unsigned int Reserved3 : 2;   //!< [23:22]
			unsigned int FlashDataByteCount : 6;   //!< [29:24]
			unsigned int Reserved4 : 1;   //!<   [30]
			unsigned int FlashSpiSmiPinEnable : 1;   //!<   [31]
		} fields;
	}HSSFSTSCTL, *PHSSFSTSCTL;



	//////////////////////////////////////////////
	//	Variable
	// 
	//custome structure for compatible to all board.
	typedef struct {
		USHORT addr;    //relative offset to SPIBAR 
		int    size;
		char   *name;
	} IoRegister;

	IoRegister SpiBarRegister[] = {
		{ 0x00, 4, "BFPR - BIOS Flash primary region" },
		{ 0x04, 2, "HSFSTS - Hardware Sequencing Flash Status" },
		{ 0x06, 2, "HSFCTL - Hardware Sequencing Flash Control" },
		{ 0x08, 4, "FADDR - Flash Address" },
		{ 0x0c, 4, "Reserved" },
		{ 0x10, 4, "FDATA0" },
		{ 0x14, 4, "FDATA2" },
		{ 0x18, 4, "FDATA3" },
		{ 0x1C, 4, "FDATA4" },
		{ 0x20, 4, "FDATA5" },
		{ 0x24, 4, "FDATA6" },
		{ 0x28, 4, "FDATA7" },
		{ 0x2C, 4, "FDATA8" },
		{ 0x30, 4, "FDATA9" },
		{ 0x34, 4, "FDATA10" },
		{ 0x38, 4, "FDATA11" },
		{ 0x3C, 4, "FDATA12" },
		{ 0x40, 4, "FDATA13" },
		{ 0x44, 4, "FDATA14" },
		{ 0x48, 4, "FDATA15" },
		{ 0x4C, 4, "FDATA16" },
		{ 0x50, 4, "FRACC - Flash Region Access Permissions" },
		{ 0x54, 4, "Flash Region 0" },
		{ 0x58, 4, "Flash Region 1" },
		{ 0x5c, 4, "Flash Region 2" },
		{ 0x60, 4, "Flash Region 3" },
		{ 0x64, 4, "Flash Region 4" },
		{ 0x74, 4, "FPR0 Flash Protected Range 0" },
		{ 0x78, 4, "FPR0 Flash Protected Range 1" },
		{ 0x7c, 4, "FPR0 Flash Protected Range 2" },
		{ 0x80, 4, "FPR0 Flash Protected Range 3" },
		{ 0x84, 4, "FPR0 Flash Protected Range 4" },
		{ 0x90, 1, "SSFSTS - Software Sequencing Flash Status" },
		{ 0x91, 3, "SSFSTS - Software Sequencing Flash Status" },
		{ 0x94, 2, "PREOP - Prefix opcode Configuration" },
		{ 0x96, 2, "OPTYPE - Opcode Type Configuration" },
		{ 0x98, 8, "OPMENU - Opcode Menu Configuration" },
		{ 0xa0, 1, "BBAR - BIOS Base Address Configuration" },
		{ 0xb0, 4, "FDOC - Flash Descriptor Observability Control" },
		{ 0xb8, 4, "Reserved" },
		{ 0xc0, 4, "AFC - Additional Flash Control" },
		{ 0xc4, 4, "LVSCC - Host Lower Vendor Specific Component Capabilities" },
		{ 0xc8, 4, "UVSCC - Host Upper Vendor Specific Component Capabilities" },
		{ 0xd0, 4, "FPB - Flash Partition Boundary" },
	};

	///////////////////////////////////////////////
	//	Macro
	//

	#define SPI_READ_CYCLE				    0
	#define SPI_RESEVERD_CYCLE			    1
	#define SPI_WRITE_CYCLE				    2
	#define SPI_4K_ERASE_CYCLE			    3
	#define SPI_64K_ERASE_CYCLE			    4
	#define SPI_READ_SFDP_CYCLE			    5
	#define SPI_READ_JEDEC_ID_CYCLE		    6
	#define SPI_WRITE_STATUS_CYCLE		    7
	#define SPI_READ_STATUS_CYCLE		    8
	#define SPI_RPMC_OP1_CYCLE			    9
	#define SPI_RPMC_OP2_CYCLE			   10

	#define SPI_INTERFACE_BUS_NUMBER		0
	#define SPI_INTERFACE_DEVICE_NUMBER  0x1F
	#define SPI_INTERFACE_FUNC_NUMBER	  0x5
	#define SPI_INTERFACE_SPIBAR_OFFSET  0x10
	//////////////////////////////////////////////
	//	Implementation
	//

	ULONG64 PciGetDeviceMmioAddress(UINT8 BusNumber, UINT8 DeviceNumber, UINT8 FunctionNumber, UINT8 Offset)
	{
		PHYSICAL_ADDRESS PhysAddr = { 0 };

		ULONG SpiBarPa = ReadPCICfg(
			BusNumber,
			DeviceNumber,   //0x1F
			FunctionNumber,    //SPI: 0x05 / LPC: 0
			Offset,   //SPI: 0x10 / LPC: F0
			sizeof(ULONG)
		);

		PhysAddr.QuadPart = ((ULONG64)SpiBarPa & 0x00000000FFFFFFFF);

		return PhysAddr.QuadPart;
	}

	void SpiSetMonitorTrapFlag(void* sh_data, bool enable) 
	{
		UNREFERENCED_PARAMETER(sh_data);
		VmxProcessorBasedControls vm_procctl = {
			static_cast<unsigned int>(UtilVmRead(VmcsField::kCpuBasedVmExecControl)) };
		vm_procctl.fields.monitor_trap_flag = enable;
		UtilVmWrite(VmcsField::kCpuBasedVmExecControl, vm_procctl.all);
	}

	NTSTATUS SpiEnableMonitor()
	{
		return UtilForEachProcessor(
			[](void* context) {
			UNREFERENCED_PARAMETER(context);
			return UtilVmCall(HypercallNumber::kEnableVmmDeviceMonitor, nullptr);
		},
			nullptr);
	}

	NTSTATUS SpiDisableMonitor()
	{
		return UtilForEachProcessor(
			[](void* context) {
			UNREFERENCED_PARAMETER(context);
			return UtilVmCall(HypercallNumber::kDisableVmmDeviceMonitor, nullptr);
		},
			nullptr);
	} 

	ULONG64 SpiGetSpiBar0()
	{
		ULONG64 SpiBarPa = PciGetDeviceMmioAddress(
								SPI_INTERFACE_BUS_NUMBER, 
								SPI_INTERFACE_DEVICE_NUMBER, 
								SPI_INTERFACE_FUNC_NUMBER, 
								SPI_INTERFACE_SPIBAR_OFFSET
							);
		if (!SpiBarPa)
		{
			HYPERPLATFORM_LOG_DEBUG("  - [SPI] Empty SPIBAR, check your datasheet \r\n");
			return SpiBarPa;
		}
		HYPERPLATFORM_LOG_DEBUG("  + [SPI] SPIBAR= 0x%p \r\n", SpiBarPa);
		return SpiBarPa;
	}

	EptCommonEntry* SpiGetSpiBarEptEntry(EptData* ept_data)
	{
		EptCommonEntry*  Entry = nullptr;
		ULONG64 SpiBarPa = SpiGetSpiBar0();
		if (!SpiBarPa || !ept_data)
		{
			HYPERPLATFORM_LOG_DEBUG(" - [SPI] Empty SPIBAR, check your datasheet \r\n");
			return Entry;
		}

		 Entry = EptGetEptPtEntry(ept_data, SpiBarPa);
		 
		 //Cannot find in current EPT means it is device memory.
		 if (!Entry || !Entry->all)
		 {
			 HYPERPLATFORM_LOG_DEBUG(" - [SPI] SPIBAR0 never hasn't been memory-mapped, we map it now. \r\n");
			 Entry = EptpMapGpaToHpa(SpiBarPa, ept_data);
			 UtilInveptGlobal();
		 }

		 if (!Entry || !Entry->all)
		 {
			 HYPERPLATFORM_LOG_DEBUG(" - [SPI] Create mapping from SPIBAR mmio is failed, terminate now \r\n");
			 return Entry;
		 }

		 HYPERPLATFORM_LOG_DEBUG(" + [SPI] SPIBAR Ept Entry found 0x%I64x \r\n", Entry->all);

		 return Entry;
	}

	EptCommonEntry* SpiEnableSpiMonitor(EptData* ept_data)
	{
		EptCommonEntry*  Entry = nullptr;

		Entry = SpiGetSpiBarEptEntry(ept_data);
		if (!Entry)
		{
			HYPERPLATFORM_LOG_DEBUG("- [SPI] SPIBAR ept Entry Not Found \r\n");
			return Entry;
		}

		HYPERPLATFORM_LOG_DEBUG("+ [SPI] Entry= %p \r\n", Entry->all);

		//cause misconfig
		Entry->fields.read_access	 = false;
		Entry->fields.write_access	 = false;	
		Entry->fields.execute_access = true;
	
		
		HYPERPLATFORM_LOG_DEBUG("+ [SPI] Entry= %p \r\n", Entry->all);

		return Entry;
	}

	NTSTATUS SpiDisableSpiMonitor(EptData* ept_data)
	{
		NTSTATUS		status = STATUS_SUCCESS;
		EptCommonEntry*  Entry = nullptr;
		Entry = SpiGetSpiBarEptEntry(ept_data);
		if (!Entry)
		{
			HYPERPLATFORM_LOG_DEBUG("- [SPI] SPIBAR ept Entry Not Found \r\n");
			status = STATUS_UNSUCCESSFUL;
			return status;
		}

		HYPERPLATFORM_LOG_DEBUG("+ [SPI] Entry= %p \r\n", Entry->all);

		//cause misconfig
		Entry->fields.read_access	 = true;
		Entry->fields.write_access	 = true;
		Entry->fields.execute_access = true;
		
		return status;
	}
	char* SpiGetNameByOffset(ULONG Offset)
	{
		char* ret = nullptr;
		int i = 0;
		for (i = 0; i < sizeof(SpiBarRegister) / sizeof(IoRegister); i++)
		{
			if (SpiBarRegister[i].addr == Offset)
			{
				ret = SpiBarRegister[i].name;
				break;
			}
		}
		return ret;
	}

	bool SpiHandleMmioAccess(
		GpRegisters* Context,
		ULONG_PTR Rip, 
		ULONG_PTR MmioAddress, 
		ULONG Width, 
		ULONG Access
	)
	{
		ULONG Offset = 0;

		Offset = MmioAddress & 0xFFF;
		
		HYPERPLATFORM_LOG_DEBUG("[SPI %s] RIP= %p MMIO Address= %p Inst Len= %d \r\n", 
			AccessStr[Access - 1], Rip, MmioAddress, Width);

		HYPERPLATFORM_LOG_DEBUG("[SPI %s] Offset= 0x%X Register= %s \r\n",
			AccessStr[Access - 1], Offset, SpiGetNameByOffset(Offset));
	
		if (*(UCHAR*)Rip == 0xF3)
		{
			HYPERPLATFORM_LOG_DEBUG("[SPI %s] Dst=%p  Src= %p Len= %d", 
				AccessStr[Access - 1 ],
				Context->di, 
				Context->si, 
				Context->cx );
		}
		
		return true;
	}
}