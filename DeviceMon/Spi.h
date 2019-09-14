// Copyright (c) 2019, Kelvin Chan. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#pragma once
#include "Util.h"

extern "C"
{
	
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
	bool SpiHandleMmioAccessCallback(
		GpRegisters*  Context,
		ULONG_PTR InstPointer,
		ULONG_PTR MmioAddress,
		ULONG		  InstLen,
		ULONG		   Access
	);
}