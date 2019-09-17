# DeviceMon

DeviceMon is a Windows Driver that intercepts the communication between your PCI devices and kernel driver.

# Description
 DeviceMon will be an on-going development to support more PCI devices.  Currently it supports monitoring SPI controller behavior.  With SPI behavior monitoring, anyone who sends a cycle to the SPI controller will captured by DeviceMon.  This means theoretically Flash ROM attacks could be captured by DeviceMon. By intercepting a MMIO translation path, the communication between driver and devices could be easily exposed.
 
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
 
 # How to use
 
  * Step 1: Collect the following information of your testing device.
``` 
typedef struct _PCI_MONITOR_CFG
{
  UINT8         BusNumber;		// Device Bus
  UINT8         DeviceNum;		// Device Number
  UINT8         FuncNum;		// Device Function
  
  UINT8		BarOffset[6];		// BAR offset in PCI Config , check your chipset datasheet
  					// By default is 6 32bit BAR, if your device is 64bit BAR maxium is 3
					// for 64bit, please adding BarAddressWidth
					// LOWER and UPPER offset have to be in order as IntelMeDeviceInfo
					
  UINT8	       BarCount;		// Number of offset need to me monitored, check your chipset datasheet
  ULONG64      BarAddress[6];		// Obtained BAR address, it will be filled out runtime
  MMIOCALLBACK Callback;		// MMIO handler
  ULONG        BarAddressWidth[6];   	// 0 by default, PCI_64BIT_DEVICE affect BarOffset parsing n, n+1 => 64bit 
  
  OFFSETMAKECALLBACK Callback2;		// callback that indicate offset is 64bit 
					// offset combination is compatible for those 64bit combined BAR,
					// and it should take device-dependent bitwise operation. 
					
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
    0,0,0,0,0				
  },	
  1,					
  { 0 , 0 , 0 , 0 , 0 , 0 },		
  SpiHandleMmioAccessCallback,		
  { 0 , 0 , 0 , 0 , 0 , 0 },		
  nullptr,				
};



PCIMONITORCFG IntelMeDeviceInfo = 
{
  INTEL_ME_BUS_NUMBER,			
  INTEL_ME_DEVICE_NUMBER,		
  INTEL_ME_FUNC_NUMBER ,		
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




PCIMONITORCFG IntelMe2DeviceInfo = 
{
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



PCIMONITORCFG IntelMe3DeviceInfo = 
{
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




PCIMONITORCFG IntelUsb3DeviceInfo = 
{
  INTEL_USB3_BUS_NUMBER,
  INTEL_USB3_DEVICE_NUMBER,
  INTEL_USB3_FUNC_NUMBER ,
  {
    INTEL_USB3_BAR_LOWER_OFFSET,
    0,0,0,0,0,
  },
  1,
  { 0 , 0 , 0 , 0 , 0 , 0 },
  IntelUsb3HandleMmioAccessCallback,
  {
    0 ,	0 , 0 , 0 , 0 , 0 ,
  },
  nullptr,
};

	
```
```

//Put your device config here. Engine will be able to distract them automatically.
PCIMONITORCFG g_MonitorDeviceList[] =
{
  SpiDeviceInfo,
  IntelMeDeviceInfo,
  IntelMe2DeviceInfo,
  IntelMe3DeviceInfo,
  IntelUsb3DeviceInfo,
};
 
```
* Step 3: Implement your callback with your device logic 
It will be eventually get invoke your callback on access (R/W) with the following prototype
 
```
typedef bool(*MMIOCALLBACK)(
		GpRegisters*  Context,
		ULONG_PTR InstPointer,
		ULONG_PTR MmioAddress,
		ULONG	  InstLen,
		ULONG	  Access
		);
 
 ``` 
Because huge differences between PCI devices, you have to check device config from your data-sheet from your hardware manufacture.

# Windows 10 RS4 Test demo 
 A demo has captured a malware that starting the attack and dumping the SPI Flash ROM.
 Also, as following figure shown, two binary compared there's no any effect on dumped SPI Flash when VMM in the middle.
 
 * SPI Device Monitoring
 
 <img src="https://user-images.githubusercontent.com/22551808/64905333-77c97380-d68b-11e9-83a7-d6e75cef5dc2.png" width="70%" height="70%" align="middle"> </img>

 * Intel ME Interface Driver Monitoring 
   - https://downloadcenter.intel.com/download/26136/Intel-Management-Engine-Corporate-Driver-for-Windows-7-8-1-10-for-NUC5i5MY
   
 <img src="https://user-images.githubusercontent.com/22551808/64921244-b3406c80-d775-11e9-92a9-1d43a5e68a15.png" width="70%" height="70%" align="middle"> </img>
 
 * xHCI(USB3) controller Interface Driver Monitoring
 
 <img src="https://user-images.githubusercontent.com/22551808/65013340-a59efa00-d8cf-11e9-93a2-3a1050d2eb06.png" width="70%"
 height="70%" align="middle"> </img>
 
 # Supported Devices
  * SPI Controller Interface
  * Intel ME Controller Interface 
  * xHCI (USB3) Controller Interface
  
 Moreover, Except for the malware behavior capturing, DeviceMon is also a good helper for analysis device driver protocol. :)
 
 Request for more device monitoring is welcome. please feel free to contact via kelvin.chan@microsoft.com
