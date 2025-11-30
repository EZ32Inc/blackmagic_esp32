
# ESP32 target,

This provides a wifi based, debug probe on the esp32
NOTE!! Not tested with latest changes!
Could be better to implement this instead,
   uint32_t platform_max_frequency_get(void)
   {
	return 0;
   }


# Build

Set ssid and password in main.c

. $HOME/esp/esp-idf/export.sh

./build-esp32.sh

# Flash

./run-esp32.sh  /dev/ttyUSB1

# Pins


So Pins are changed in platform.h

```
GND on ESP32 connects to GND on the target board
PIN 13 on ESP32 connects to SWD_CLK
PIN 14 on ESP32 connects to SWD_TMS
```
Pins are changed in platform.h

```
I (3119) event: sta ip: 192.168.1.117, mask: 255.255.255.0, gw: 192.168.1.1
I (3119) blackmagic: Connected to AP
I (3119) gpio: GPIO[13]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (3129) gpio: GPIO[24]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
```


# Start the debugger,
```
arm-none-eabi-gdb .pioenvs/rak811/firmware.elf

target  extended-remote 192.168.1.136:2345

(gdb) monitor help

(gdb) monitor swdp_scan
Target voltage: not supported
Available Targets:
No. Att Driver
 1      STM32L1x

https://github.com/blacksphere/blackmagic/wiki/Frequently-Asked-Questions

(gdb) attach 1

```

# Quicker download
```
arm-none-eabi-gdb .pioenvs/rak811/firmware.elf -ex 'target  extended-remote 192.168.1.136:2345'

(gdb) monitor swdp_scan
(gdb) attach 1
(gdb) load
(gdb) b main
(gdb) c


```