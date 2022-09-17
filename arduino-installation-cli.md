How to install arduino-cli on Ubuntu
====================================

From this URL: https://arduino.github.io/arduino-cli/0.21/installation/

```
# The below installs arduino-cli in ~/bin
curl -O -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh
sh install.sh

# the below downloads what it takes to manage arduino:avr boards
arduino-cli core install arduino:avr
```

Example for 01_generic.ino (example of RF433recv library):

```
# Compilation

$ arduino-cli compile -b arduino:avr:nano:cpu=atmega328old --build-path build --libraries /home/sebastien/travail/cpp/seb/arduino/libraries 01_generic.ino

Le croquis utilise 7950 octets (25%) de l'espace de stockage de programmes. Le maximum est de 30720 octets.
Les variables globales utilisent 243 octets (11%) de mémoire dynamique, ce qui laisse 1805 octets pour les variables locales. Le maximum est de 2048 octets.

Used library Version Path                                                       
RF433recv    0.3.0   /home/sebastien/travail/cpp/seb/arduino/libraries/RF433recv

Used platform Version Path                                                          
arduino:avr   1.8.5   /home/sebastien/.arduino15/packages/arduino/hardware/avr/1.8.5

# Upload

$ arduino-cli upload -v -b arduino:avr:nano:cpu=atmega328old --input-dir build -p /dev/ttyUSB0

"/home/sebastien/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/bin/avrdude" "-C/home/sebastien/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/etc/avrdude.conf" -v -V -patmega328p -carduino "-P/dev/ttyUSB0" -b57600 -D "-Uflash:w:build/01_generic.ino.hex:i"

avrdude: Version 6.3-20190619
         Copyright (c) 2000-2005 Brian Dean, http://www.bdmicro.com/
         Copyright (c) 2007-2014 Joerg Wunsch

         System wide configuration file is "/home/sebastien/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/etc/avrdude.conf"
         User configuration file is "/home/sebastien/.avrduderc"
         User configuration file does not exist or is not a regular file, skipping

         Using Port                    : /dev/ttyUSB0
         Using Programmer              : arduino
         Overriding Baud Rate          : 57600
         AVR Part                      : ATmega328P
         Chip Erase delay              : 9000 us
         PAGEL                         : PD7
         BS2                           : PC2
         RESET disposition             : dedicated
         RETRY pulse                   : SCK
         serial program mode           : yes
         parallel program mode         : yes
         Timeout                       : 200
         StabDelay                     : 100
         CmdexeDelay                   : 25
         SyncLoops                     : 32
         ByteDelay                     : 0
         PollIndex                     : 3
         PollValue                     : 0x53
         Memory Detail                 :

                                  Block Poll               Page                       Polled
           Memory Type Mode Delay Size  Indx Paged  Size   Size #Pages MinW  MaxW   ReadBack
           ----------- ---- ----- ----- ---- ------ ------ ---- ------ ----- ----- ---------
           eeprom        65    20     4    0 no       1024    4      0  3600  3600 0xff 0xff
           flash         65     6   128    0 yes     32768  128    256  4500  4500 0xff 0xff
           lfuse          0     0     0    0 no          1    0      0  4500  4500 0x00 0x00
           hfuse          0     0     0    0 no          1    0      0  4500  4500 0x00 0x00
           efuse          0     0     0    0 no          1    0      0  4500  4500 0x00 0x00
           lock           0     0     0    0 no          1    0      0  4500  4500 0x00 0x00
           calibration    0     0     0    0 no          1    0      0     0     0 0x00 0x00
           signature      0     0     0    0 no          3    0      0     0     0 0x00 0x00

         Programmer Type : Arduino
         Description     : Arduino
         Hardware Version: 2
         Firmware Version: 1.16
         Vtarget         : 0.0 V
         Varef           : 0.0 V
         Oscillator      : Off
         SCK period      : 0.1 us

avrdude: AVR device initialized and ready to accept instructions

Reading | ################################################## | 100% 0.00s

avrdude: Device signature = 0x1e950f (probably m328p)
avrdude: reading input file "build/01_generic.ino.hex"
avrdude: writing flash (7950 bytes):

Writing | ################################################## | 100% 2.35s

avrdude: 7950 bytes of flash written

avrdude done.  Thank you.
```

