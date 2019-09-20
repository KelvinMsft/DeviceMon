// Copyright (c) 2019, Kelvin Chan. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "Spi.h"
#include "log.h"
#include "DevMon.h"
#include "Fuzzer.h"

FUZZ SpiFuzzer = { 0 };
extern "C"
{
//////////////////////////////////////////////
//	Types
//

typedef union _BIOS_HSFSTS_CTRL {
	unsigned int all;
	struct {
		unsigned int FlashCycleDone : 1;	 //!<    [0]
		unsigned int FlashCycleError : 1;    //!<    [1]
		unsigned int AccessErrorLog : 1;	 //!<    [2]
		unsigned int Reserved : 2;			 //!<  [4:3]
		unsigned int SpiCycleInProgress : 1; //!<    [5]
		unsigned int Reserved2 : 5;			 //!< [10:6]
		unsigned int FlashCfgLockdown : 1;   //!<   [11]
		unsigned int PRR34Lockdown : 1;		 //!<   [12]
		unsigned int FdoPss : 1;			 //!<   [13]
		unsigned int FlashDescValid : 1;	 //!<   [14]
		unsigned int FLOCKDN : 1;			 //!<   [15]
		unsigned int FlashCycleGo : 1;		 //!<   [16]
		unsigned int FlashCycleCmd : 4;		 //!< [20:17]
		unsigned int WriteEnableType : 1;    //!<   [21]
		unsigned int Reserved3 : 2;			 //!< [23:22]
		unsigned int FlashDataByteCount : 6; //!< [29:24]
		unsigned int Reserved4 : 1;			 //!<   [30]
		unsigned int FlashSpiSmiPinEnable : 1; //!<   [31]
	} fields;
}HSSFSTSCTL, *PHSSFSTSCTL;
 


//////////////////////////////////////////////
//	Variables
//

char* AccessStr[2] = {
	"Read",
	"Write"
};

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

bool SpiHandleMmioAccessCallback(
	GpRegisters*  Context,
	ULONG_PTR InstPointer,
	ULONG_PTR MmioAddress,
	ULONG		  InstLen,
	ULONG		   Access
)
{
	ULONG Offset = 0;

	Offset = MmioAddress & 0xFFF;

	HYPERPLATFORM_LOG_DEBUG("[SPI %s] RIP= %p MMIO Address= %p Inst Len= %d \r\n",
		AccessStr[Access - 1], InstPointer, MmioAddress, InstLen);

	HYPERPLATFORM_LOG_DEBUG("[SPI %s] Offset= 0x%X Register= %s \r\n",
		AccessStr[Access - 1], Offset, SpiGetNameByOffset(Offset));

	if (Access == 0x1 )
	{
		StartFuzz(Context, InstPointer, MmioAddress, InstLen, &SpiFuzzer);
	}

	return false;
}
}