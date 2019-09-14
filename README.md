# DeviceMon

DeviceMon is a Windows Driver that implemented Virtualization Based PCI Device Monitor

# Environment
  * Visual Studio 2015 update 3 
  * Windows SDK 10
  * Windowr Driver Kit 10 
  * VMware 12 with EPT environment. 
  * Supports Multi-core processor environment
  * Test environment with Windows 10 x64 RS4
  
# Description
 DeviceMon will be on-going developing, and initialed by monitoring SPI controller cycles, with SPI behavior monitoring, anyone send a cycle to SPI controller it can be captured by DeviceMon, means, in case of someone whose are trying to attack the ROM, theoretically could be capture by DeviceMon.
 
 # Installation

   *  Compiled DeviceMon.sys 

   *  Enable Testsigning on x64:
   
         `bcdedit /set testsigning on` 
   
   *  Install DeviceMon.sys 
   
         `sc create DeviceMon type= kernel binPath= C:\DeviceMon.sys`
                  
   * start a service as following screen capture with its expected output
   
 # Windows 10 RS4 Test demo 
 A demo has captured a malware that starting the attack and dumping the SPI Flash ROM.
 
 <img src="https://user-images.githubusercontent.com/22551808/64902638-dbd64280-d65f-11e9-97a1-4535a446daef.PNG" width="70%" height="70%"> </img>
