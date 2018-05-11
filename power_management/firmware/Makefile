##
## This file is part of the libopencm3 project.
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

DEVICE=stm32f042
#DEVICE=stm32f103c8t6
BINARY = power_manager
#OPENCM3_DIR=/home/spock/chmura/lib/libopencm3
OPENCM3_DIR=../libopencm3
OBJS += rtc.o pwm.o
CPPFLAGS += -I$(CURDIR)/include
LDSCRIPT = ./stm32.ld
JLINK_EXE = /usr/local/bin/JLinkExe
include ./libopencm3.target.mk

prog: power_manager.bin
	sudo dfu-util -a 0 -d 0483:df11 -s 0x08000000:leave -D power_manager.bin
#	stm32flash -e 255 -v -w blink.bin /dev/ttyUSB0
