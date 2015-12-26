/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Patrick Rudolph
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <types.h>
#include <string.h>
#include <device/device.h>
#include <device/pci_def.h>
#include <device/pci_ops.h>
#include <device/pci.h>
#include <console/console.h>
#include <southbridge/intel/common/gpio.h>

#define VENDOR_ID_NVIDIA 0x10de
#define VENDOR_ID_AMD 0x1002

/* Hybrid graphics allows to connect LVDS interface to either iGPU
 * or dGPU depending on GPIO level.
 * On systems without dGPU this code is either not linked in or is never called
 * and has no effect.
 *
 * Note: Once native gfx init is done for AMD or Nvida graphic
 *       cards merge this code.
 */

static struct device *get_audio_dev(struct device *dev)
{
	/* returns the GPU HDMI audio device */
	if (dev->vendor == VENDOR_ID_NVIDIA) {
		switch (dev->device) {
		case 0x0def:
			return dev_find_device(dev->vendor, 0x0bea, dev->bus->dev);
		case 0x0dfc:
			return dev_find_device(dev->vendor, 0x0bea, dev->bus->dev);
		case 0x1056:
			return dev_find_device(dev->vendor, 0x0e08, dev->bus->dev);
		case 0x1057:
			return dev_find_device(dev->vendor, 0x0bea, dev->bus->dev);
		case 0x0a6c:
			return dev_find_device(dev->vendor, 0x0be3, dev->bus->dev);
		default:
			break;
		}
	} else {
		switch (dev->device) {
		case 0x9591:
			return dev_find_device(dev->vendor, 0xaa20, dev->bus->dev);
		case 0x95c4:
			return dev_find_device(dev->vendor, 0xaa28, dev->bus->dev);
		default:
			break;
		}
	}
	return NULL;
}

static void hybrid_graphics_enable_peg(struct device *dev)
{
	struct device *audio_dev;

	/* connect LVDS interface to dGPU */
	set_gpio(CONFIG_HYBRID_GRAPHICS_GPIO_NUM, GPIO_LEVEL_LOW);
	printk(BIOS_DEBUG, "Switching panel to discrete GPU\n");
	dev->enabled = 1;

	audio_dev = get_audio_dev(dev);
	if (audio_dev)
		audio_dev->enabled = 1;
}

static void hybrid_graphics_disable_peg(struct device *dev)
{
	struct device *audio_dev;

	/* connect LVDS interface to iGPU */
	set_gpio(CONFIG_HYBRID_GRAPHICS_GPIO_NUM, GPIO_LEVEL_HIGH);
	printk(BIOS_DEBUG, "Switching panel to integrated GPU\n");
	printk(BIOS_DEBUG, "Disabling device 0x%04x:0x%04x on bus %s\n",
			dev->vendor, dev->device, bus_path(dev->bus));
	dev->enabled = 0;

	audio_dev = get_audio_dev(dev);
	if (audio_dev) {
		printk(BIOS_DEBUG, "Disabling device 0x%04x:0x%04x on bus %s\n",
				audio_dev->vendor, audio_dev->device, bus_path(audio_dev->bus));
		audio_dev->enabled = 0;
	}
}

static struct pci_operations pci_dev_ops_pci = {
	.set_subsystem = pci_dev_set_subsystem,
};

struct device_operations hybrid_graphics_ops = {
	.read_resources   = pci_dev_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init             = pci_dev_init,
	.scan_bus         = 0,
	.enable           = hybrid_graphics_enable_peg,
	.disable          = hybrid_graphics_disable_peg,
	.ops_pci          = &pci_dev_ops_pci,
};

static const unsigned short pci_device_ids_nvidia[] = {
		0x0def, /* NVidia NVS 5400m Lenovo T430/T530 */
		0x0dfc, /* NVidia NVS 5200m Lenovo T430s */
		0x1056, /* NVidia NVS 4200m Lenovo T420/T520 */
		0x1057, /* NVidia NVS 4200m Lenovo T420/T520 */
		0x0a6c, /* NVidia NVS 3100m Lenovo T410/T510 */
		0 };

static const struct pci_driver hybrid_peg_nvidia __pci_driver = {
	.ops	 = &hybrid_graphics_ops,
	.vendor	 = VENDOR_ID_NVIDIA,
	.devices = pci_device_ids_nvidia,
};

static const unsigned short pci_device_ids_amd[] = {
		0x9591, /* ATI Mobility Radeon HD 3650 Lenovo T500/W500 */
		0x95c4, /* ATI Mobility Radeon HD 3470 Lenovo T400/R400 */
		0 };

static const struct pci_driver hybrid_peg_amd __pci_driver = {
	.ops	 = &hybrid_graphics_ops,
	.vendor	 = VENDOR_ID_AMD,
	.devices = pci_device_ids_amd,
};

