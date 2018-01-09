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
 * @file cmd_wait.c
 * @author Himanshu Chauhan <hchauhan@xvisor-x86.org>
 * @brief Implementation of wait command
 */

#include <vmm_error.h>
#include <vmm_stdio.h>
#include <vmm_version.h>
#include <vmm_modules.h>
#include <vmm_cmdmgr.h>
#include <arch_cpu.h>
#include <arch_delay.h>

#define MODULE_DESC			"Command wait"
#define MODULE_AUTHOR			"Himanshu Chauhan"
#define MODULE_LICENSE			"GPL"
#define MODULE_IPRIORITY		0
#define	MODULE_INIT			cmd_wait_init
#define	MODULE_EXIT			cmd_wait_exit

static void cmd_wait_usage(struct vmm_chardev *cdev)
{
	vmm_cprintf(cdev, "Usage: ");
	vmm_cprintf(cdev, "   wait <number of cycles>\n");
}

static int cmd_wait_exec(struct vmm_chardev *cdev, int argc, char **argv)
{
	if (argc < 2) {
		cmd_wait_usage(cdev);
		return VMM_EFAIL;
	}

	u32 cycles = atoi(argv[1]);
    u32 elapsed = 0;
    u32 estimated_cycles = arch_delay_loop_cycles(1);

    while (elapsed < cycles) {
        elapsed += estimated_cycles;
        arch_delay_loop(1);
    }

	return VMM_OK;
}

static struct vmm_cmd cmd_wait = {
	.name = "wait",
	.desc = "Sleep the terminal thread of execution for given cycles",
	.usage = cmd_wait_usage,
	.exec = cmd_wait_exec,
};

static int __init cmd_wait_init(void)
{
	return vmm_cmdmgr_register_cmd(&cmd_wait);
}

static void __exit cmd_wait_exit(void)
{
	vmm_cmdmgr_unregister_cmd(&cmd_wait);
}

VMM_DECLARE_MODULE(MODULE_DESC,
			MODULE_AUTHOR,
			MODULE_LICENSE,
			MODULE_IPRIORITY,
			MODULE_INIT,
			MODULE_EXIT);
