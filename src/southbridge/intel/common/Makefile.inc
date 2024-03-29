##
## This file is part of the coreboot project.
##
## Copyright (C) 2008-2009 coresystems GmbH
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; version 2 of the License.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

# CONFIG_HAVE_INTEL_FIRMWARE protects doing anything to the build.
subdirs-y += firmware

ifeq ($(CONFIG_SOUTHBRIDGE_INTEL_COMMON),y)

romstage-$(CONFIG_USBDEBUG_IN_ROMSTAGE) += usb_debug.c
ramstage-$(CONFIG_USBDEBUG) += usb_debug.c

ifneq ($(CONFIG_INTEL_LYNXPOINT_LP),y)
romstage-y += gpio.c
ramstage-y += gpio.c
smm-$(CONFIG_HAVE_SMI_HANDLER) += gpio.c
endif
endif
