# DeviceMon

DeviceMon is a Windows Driver that intercept the communication between your PCI devices and kernel driver.

# Description
 DeviceMon will be on-going developed for support more PCI devices, and currently support monitoring SPI controller behavior, with SPI behavior monitoring, anyone send a cycle to SPI controller it can be captured by DeviceMon, means, in case of someone whose are trying to attack the Flash ROM, theoretically could be capture by DeviceMon. By intercepting a MMIO translation path, the communication between driver and devices could be easily exposed.
 
# Environment
  * Visual Studio 2015 update 3 
  * Windows SDK 10
  * Windowr Driver Kit 10 
  * VMware 12 with EPT environment. 
  * Supports Multi-core processor environment
  * Test environment with Windows 10 x64 RS4
  * With VT-x enabled machine
  
 # Installation

   *  Compiled DeviceMon.sys 

   *  Enable Testsigning on x64:
   
         `bcdedit /set testsigning on` 
   
   *  Install DeviceMon.sys 
   
         `sc create DeviceMon type= kernel binPath= C:\DeviceMon.sys`
         
         `sc start DeviceMon`

                  
   * start a service as following screen capture with its expected output

# Mechanism

 * With VT-x and EPT assisted, we are able to intercept the address translation between guest physical address to host physical address,
 PCI device communication heavily rely on MMIO, before host physical address is sent out to the address bus and perform I/O operation, the final step could be simplify as address translation, so we could take an advantage from address translation intercept and for runtime mal-behave detection, and analysis device driver protocol.  See my recent blogpost for more detail.
 
 <img src="https://user-images.githubusercontent.com/22551808/64904101-b8b88c80-d679-11e9-8073-3e283f4e6500.jpg" width="70%" height="70%" align="middle"> </img>
 

 # Windows 10 RS4 Test demo 
 A demo has captured a malware that starting the attack and dumping the SPI Flash ROM.
 Also, as following figure shown, two binary compared there's no any effect on dumped SPI Flash when VMM in the middle.
 
 <img src="https://user-images.githubusercontent.com/22551808/64905333-77c97380-d68b-11e9-83a7-d6e75cef5dc2.png" width="70%" height="70%" align="middle"> </img>
 
 Moreover, Except for the malware behavior capturing, DeviceMon is also a good helper for analysis device driver protocol. :)
 
 
