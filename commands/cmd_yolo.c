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
 * @file cmd_yolo.c
 * @author Himanshu Chauhan <hchauhan@xvisor-x86.org>
 * @brief Implementation of yolo command
 */

#include <vmm_error.h>
#include <vmm_stdio.h>
#include <vmm_version.h>
#include <vmm_modules.h>
#include <vmm_cmdmgr.h>

#define MODULE_DESC			"Command yolo"
#define MODULE_AUTHOR			"Anonyme"
#define MODULE_LICENSE			"GPL"
#define MODULE_IPRIORITY		0
#define	MODULE_INIT			cmd_yolo_init
#define	MODULE_EXIT			cmd_yolo_exit

static void cmd_yolo_usage(struct vmm_chardev *cdev)
{
	vmm_cprintf(cdev, "Usage: ");
	vmm_cprintf(cdev, "   yolo\n");
}

static int cmd_yolo_exec(struct vmm_chardev *cdev, int argc, char **argv)
{
	int print_new_line = 1;
	int i;

	if (argc < 1) {
		cmd_yolo_usage(cdev);
		return VMM_EFAIL;
	}

	vmm_cprintf(cdev, "Geoffrey Bonobo\n");

	return VMM_OK;
}

static struct vmm_cmd cmd_yolo = {
	.name = "yolo",
	.desc = "yolo given message on standard output",
	.usage = cmd_yolo_usage,
	.exec = cmd_yolo_exec,
};

static int __init cmd_yolo_init(void)
{
	return vmm_cmdmgr_register_cmd(&cmd_yolo);
}

static void __exit cmd_yolo_exit(void)
{
	vmm_cmdmgr_unregister_cmd(&cmd_yolo);
}

VMM_DECLARE_MODULE(MODULE_DESC,
			MODULE_AUTHOR,
			MODULE_LICENSE,
			MODULE_IPRIORITY,
			MODULE_INIT,
			MODULE_EXIT);
