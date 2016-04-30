## What is labsupply-stm-main?
labsupply-stm-main is project of simple labsupply main module based on stm32f100c4, the basic characteristics:

* In                       -  38 V
* Out                      -  2.5V - 32V
* Maximum out current      -  2.5A
* Ripple (2.3A)            -  less 20 mV
* Load Regulation (2.3A)   -  less 50mV
* Software limiting out V/I
* hardware current protection based on LM339(or fastest analog) comparator
* USART for communicate with 'slave' module


## Hardware
Modules connection - [modules connection](https://github.com/LeftRadio/labsupply-stm-main/tree/master/.doc/modules_connection_6.jpg), also see docs folder for schematics/pcb - [docs sch](https://github.com/LeftRadio/labsupply-stm-main/tree/master/.doc)


## Build script

1. Install [Python3](https://www.python.org/downloads/)
2. Install [gcc toolchain](https://launchpad.net/gcc-arm-embedded/+download).
2. Add arm-none-eabi (for example - C:\GNU Tools ARM Embedded\5.3 2016q1\bin\ ) path to system paths.
3. Download and unpack this repository
4. From unpacked repository put in console:
>> python build.py

firmware files default location - 'workdir\.out\'

