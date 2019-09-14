# DeviceMon

DeviceMon is a Windows Driver that intercept the communication between your PCI devices and kernel driver.

# Description
 DeviceMon will be on-going developed for support more PCI devices, and currently support monitoring SPI controller behavior, with SPI behavior monitoring, anyone send a cycle to SPI controller it can be captured by DeviceMon, means, in case of someone whose are trying to attack the Flash ROM, theoretically could be capture by DeviceMon. By intercepting a MMIO translation path, the communication between driver and devices could be easily exposed.
 
# Environment
  * Visual Studio 2015 update 3 
  * Windows SDK 10
  * Windowr Driver Kit 10 
  * Windows 10 x64 RS4
  * With VT-x enabled machine
  * Series 100 / 200 / 300 Chipset's SPI Interface
  
 # Installation

   *  Compiled DeviceMon.sys 

   *  Enable Testsigning on x64:
   
         `bcdedit /set testsigning on` 
   
   *  Install DeviceMon.sys 
   
         `sc create DeviceMon type= kernel binPath= C:\DeviceMon.sys`
         
         `sc start DeviceMon`

# Mechanism

 * With VT-x and EPT assisted, we are able to intercept the address translation between guest physical address to host physical address,
 PCI device communication heavily rely on MMIO, before host physical address is sent out to the address bus and perform I/O operation, the final step could be simplify as address translation, so we could take an advantage from address translation intercept and for runtime mal-behave detection, and analysis device driver protocol.  See my recent blogpost for more detail.
 
 <img src="https://user-images.githubusercontent.com/22551808/64904101-b8b88c80-d679-11e9-8073-3e283f4e6500.jpg" width="70%" height="70%" align="middle"> </img>
 
 # Test it
 
  * Step 1: Collect the following information of your testing device.
``` 
typedef struct _PCI_MONITOR_CFG
{
	UINT8	BusNumber;		//
	UINT8	DeviceNum;		//
	UINT8	FuncNum;		//
	UINT8	BarOffset[6];		// BAR offset in PCI Config , check your chipset datasheet
	UINT8	BarCount;		// Number of BAR in PCI Config , check your chipset datasheet
	//...
}PCIMONITORCFG, *PPCIMONITORCFG;
 
 ```
 * Step 2: Construct it and fill into the global config as follow
 ```
 PCIMONITORCFG SpiDeviceInfo =
 {
	SPI_INTERFACE_BUS_NUMBER,
	SPI_INTERFACE_DEVICE_NUMBER,
	SPI_INTERFACE_FUNC_NUMBER ,
	{
	  SPI_INTERFACE_SPIBAR_OFFSET,
	},
	1,                          	//SPI device has only one BAR 
	{ 0 , 0 , 0 , 0 , 0, 0 },  	//automatically filled when initial monitor. Just fill 6's zero 
	SpiHandleMmioAccessCallback,
};
```
```
//Put your device config here. Engine will be able to distract them automatically.
PCIMONITORCFG g_MonitorDeviceList[] =
{
    SpiDeviceInfo, 
};
 
 ```
 * Step 3: Implement your callback with your device logic 
 It will be eventually get invoke your callback on access (R/W) with the following prototype
 
 ```
 typedef bool(*MMIOCALLBACK)(GpRegisters*  Context,
		ULONG_PTR InstPointer,
		ULONG_PTR MmioAddress,
		ULONG		  InstLen,
		ULONG		   Access
	);
 
 ``` 
Because huge differences between PCI devices, you have to check device config from your data-sheet from your hardware manufacture.

# Windows 10 RS4 Test demo 
 A demo has captured a malware that starting the attack and dumping the SPI Flash ROM.
 Also, as following figure shown, two binary compared there's no any effect on dumped SPI Flash when VMM in the middle.
 
 <img src="https://user-images.githubusercontent.com/22551808/64905333-77c97380-d68b-11e9-83a7-d6e75cef5dc2.png" width="70%" height="70%" align="middle"> </img>
 
 Moreover, Except for the malware behavior capturing, DeviceMon is also a good helper for analysis device driver protocol. :)
 
 Request for more device monitoring is welcome. please feel free to contact via kelvin.chan@microsoft.com / kelvin1272011@gmail.com
 or directly create an issue. 
