/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2010 coresystems GmbH
 * Copyright (C) 2011 The ChromiumOS Authors.  All rights reserved.
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

#include <stdint.h>
#include <string.h>
#include <lib.h>
#include <timestamp.h>
#include <arch/io.h>
#include <device/pci_def.h>
#include <device/pnp_def.h>
#include <cpu/x86/lapic.h>
#include <pc80/mc146818rtc.h>
#include <arch/acpi.h>
#include <cbmem.h>
#include <console/console.h>
#include <northbridge/intel/sandybridge/sandybridge.h>
#include <northbridge/intel/sandybridge/raminit_native.h>
#include <southbridge/intel/bd82x6x/pch.h>
#include <southbridge/intel/common/gpio.h>
#include <arch/cpu.h>
#include <cpu/x86/msr.h>
#include <halt.h>
#if CONFIG_CHROMEOS
#include <vendorcode/google/chromeos/chromeos.h>
#endif
#include <cbfs.h>

void pch_enable_lpc(void)
{
	/* EC Decode Range Port60/64 and Port62/66 */
	/* Enable EC and PS/2 Keyboard/Mouse*/
	pci_write_config16(PCH_LPC_DEV, LPC_EN, KBC_LPC_EN | MC_LPC_EN);

	/* EC Decode Range Port68/6C */
	pci_write_config32(PCH_LPC_DEV, LPC_GEN1_DEC, (0x68 & ~3) | 0x40001);

	/* EC Decode Range Port 380-387 */
	pci_write_config32(PCH_LPC_DEV, LPC_GEN2_DEC, 0x380 | 0x40001);

}

void rcba_config(void)
{
	u32 reg32;

	/*
	 *             GFX    INTA -> PIRQA (MSI)
	 * D28IP_P1IP  WLAN   INTA -> PIRQB
	 * D28IP_P2IP  ETH0   INTB -> PIRQF
	 * D28IP_P3IP  SDCARD INTC -> PIRQD
	 * D29IP_E1P   EHCI1  INTA -> PIRQD
	 * D26IP_E2P   EHCI2  INTA -> PIRQF
	 * D31IP_SIP   SATA   INTA -> PIRQB (MSI)
	 * D31IP_SMIP  SMBUS  INTB -> PIRQH
	 * D31IP_TTIP  THRT   INTC -> PIRQA
	 * D27IP_ZIP   HDA    INTA -> PIRQA (MSI)
	 *
	 * Trackpad interrupt is edge triggered and cannot be shared.
	 * TRACKPAD                -> PIRQG

	 */

	/* Device interrupt pin register (board specific) */
	RCBA32(D31IP) = (INTC << D31IP_TTIP) | (NOINT << D31IP_SIP2) |
			(INTB << D31IP_SMIP) | (INTA << D31IP_SIP);
	RCBA32(D29IP) = (INTA << D29IP_E1P);
	RCBA32(D28IP) = (INTA << D28IP_P1IP) | (INTB << D28IP_P2IP) |
			(INTC << D28IP_P3IP);
	RCBA32(D27IP) = (INTA << D27IP_ZIP);
	RCBA32(D26IP) = (INTA << D26IP_E2P);
	RCBA32(D25IP) = (NOINT << D25IP_LIP);
	RCBA32(D22IP) = (NOINT << D22IP_MEI1IP);

	/* Device interrupt route registers */
	DIR_ROUTE(D31IR, PIRQB, PIRQH, PIRQA, PIRQC);
	DIR_ROUTE(D29IR, PIRQD, PIRQE, PIRQF, PIRQG);
	DIR_ROUTE(D28IR, PIRQB, PIRQF, PIRQD, PIRQE);
	DIR_ROUTE(D27IR, PIRQA, PIRQH, PIRQA, PIRQB);
	DIR_ROUTE(D26IR, PIRQF, PIRQE, PIRQG, PIRQH);
	DIR_ROUTE(D25IR, PIRQA, PIRQB, PIRQC, PIRQD);
	DIR_ROUTE(D22IR, PIRQA, PIRQB, PIRQC, PIRQD);

	/* Enable IOAPIC (generic) */
	RCBA16(OIC) = 0x0100;
	/* PCH BWG says to read back the IOAPIC enable register */
	(void) RCBA16(OIC);

	/* Disable unused devices (board specific) */
	reg32 = RCBA32(FD);
	reg32 |= PCH_DISABLE_ALWAYS;
	/* Disable PCI bridge so MRC does not probe this bus */
	reg32 |= PCH_DISABLE_P2P;
	RCBA32(FD) = reg32;
}

const struct southbridge_usb_port mainboard_usb_ports[] = {
	/* enabled   usb oc pin    length */
	{ 1, 0, 0x0040 }, /* P0: Right USB 3.0 #1 (no OC) */
	{ 1, 0, 0x0040 }, /* P1: Right USB 3.0 #2 (no OC) */
	{ 1, 0, 0x0040 }, /* P2: Camera (no OC) */
	{ 0, 0, 0x0000 }, /* P3: Empty */
	{ 0, 0, 0x0000 }, /* P4: Empty */
	{ 0, 0, 0x0000 }, /* P5: Empty */
	{ 0, 0, 0x0000 }, /* P6: Empty */
	{ 0, 0, 0x0000 }, /* P7: Empty */
	{ 0, 4, 0x0000 }, /* P8: Empty */
	{ 1, 4, 0x0080 }, /* P9: Left USB 1 (no OC) */
	{ 1, 4, 0x0040 }, /* P10: Mini PCIe - WLAN / BT (no OC) */
	{ 0, 4, 0x0000 }, /* P11: Empty */
	{ 0, 4, 0x0000 }, /* P12: Empty */
	{ 0, 4, 0x0000 }, /* P13: Empty */
};

void mainboard_get_spd(spd_raw_data *spd) {
	read_spd(&spd[0], 0x50);
	read_spd(&spd[2], 0x52);
}
