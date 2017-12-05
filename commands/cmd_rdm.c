/**
 * Copyright (c) 2015 Himanshu Chauhan
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @file cmd_rdm.c
 * @author Himanshu Chauhan <hchauhan@xvisor-x86.org>
 * @brief Implementation of rdm command
 */

#include <vmm_error.h>
#include <vmm_stdio.h>
#include <vmm_version.h>
#include <vmm_modules.h>
#include <vmm_cmdmgr.h>
#include <vmm_timer.h>

#define MODULE_DESC			"Command rdm"
#define MODULE_AUTHOR			"Himanshu Chauhan"
#define MODULE_LICENSE			"GPL"
#define MODULE_IPRIORITY		0
#define	MODULE_INIT			cmd_rdm_init
#define	MODULE_EXIT			cmd_rdm_exit

static void cmd_rdm_usage(struct vmm_chardev *cdev)
{
	vmm_cprintf(cdev, "Usage: ");
	vmm_cprintf(cdev, "   rdm max_int\n");
}

static int cmd_rdm_exec(struct vmm_chardev *cdev, int argc, char **argv)
{
    /* Reentrant random function from POSIX.1c.
       Copyright (C) 1996, 1999, 2009 Free Software Foundation, Inc.
       This file is part of the GNU C Library.
       Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.
    */
   // Hazardous convertion from u64 to u32
	u32 rdm = 0;
	if (vmm_timer_started()){
		rdm = vmm_timer_timestamp();
	}

    u32 next = rdm;
    u32 result;

    next *= 1103515245;
    next += 12345;
    result = (unsigned int) (next / 65536) % 2048;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int) (next / 65536) % 1024;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int) (next / 65536) % 1024;

    // *seed = next;

	vmm_cprintf(cdev,"%u\n", result);
	return VMM_OK;
}

static struct vmm_cmd cmd_rdm = {
	.name = "rdm",
	.desc = "Echo random int between 1 and max_int on standard output",
	.usage = cmd_rdm_usage,
	.exec = cmd_rdm_exec,
};

static int __init cmd_rdm_init(void)
{
	return vmm_cmdmgr_register_cmd(&cmd_rdm);
}

static void __exit cmd_rdm_exit(void)
{
	vmm_cmdmgr_unregister_cmd(&cmd_rdm);
}

VMM_DECLARE_MODULE(MODULE_DESC,
			MODULE_AUTHOR,
			MODULE_LICENSE,
			MODULE_IPRIORITY,
			MODULE_INIT,
			MODULE_EXIT);
