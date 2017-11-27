/**
 * Copyright (c) 2010 Anup Patel.
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
 * @file cmd_guest.c
 * @author Anup Patel (anup@brainfault.org)
 * @brief Implementation of guest command
 */

#include <vmm_error.h>
#include <vmm_stdio.h>
#include <vmm_devtree.h>
#include <vmm_manager.h>
#include <vmm_host_aspace.h>
#include <vmm_guest_aspace.h>
#include <vmm_modules.h>
#include <vmm_cmdmgr.h>
#include <vmm_devemu.h>
#include <libs/stringlib.h>
#include <cpu_defines.h>
#include <cpu_vcpu_helper.h>

#define MODULE_DESC			"Command guest"
#define MODULE_AUTHOR			"Anup Patel"
#define MODULE_LICENSE			"GPL"
#define MODULE_IPRIORITY		0
#define	MODULE_INIT			cmd_guest_init
#define	MODULE_EXIT			cmd_guest_exit

static void cmd_guest_usage(struct vmm_chardev *cdev)
{
	vmm_cprintf(cdev, "Usage:\n");
	vmm_cprintf(cdev, "   guest help\n");
	vmm_cprintf(cdev, "   guest list\n");
	vmm_cprintf(cdev, "   guest create  <guest_name>\n");
	vmm_cprintf(cdev, "   guest destroy <guest_name>\n");
	vmm_cprintf(cdev, "   guest reset   <guest_name>\n");
	vmm_cprintf(cdev, "   guest kick    <guest_name>\n");
	vmm_cprintf(cdev, "   guest pause   <guest_name>\n");
	vmm_cprintf(cdev, "   guest resume  <guest_name>\n");
	vmm_cprintf(cdev, "   guest status  <guest_name>\n");
	vmm_cprintf(cdev, "   guest halt    <guest_name>\n");
	vmm_cprintf(cdev, "   guest dumpmem <guest_name> <gphys_addr> "
			  "[mem_sz]\n");
	vmm_cprintf(cdev, "   guest loadmem <guest_name> <gphys_addr> "
			  "<value>\n");
	vmm_cprintf(cdev, "   guest inject  <guest_name> <gphys_addr> "
			  "<shift>\n");
	vmm_cprintf(cdev, "   guest reginject  <guest_name> <gphys_addr> "
			  "<shift>\n");
	vmm_cprintf(cdev, "   guest reg     <guest_name> <reg_num> "
			  "<value>\n");
	vmm_cprintf(cdev, "   guest region  <guest_name> <gphys_addr>\n");
	vmm_cprintf(cdev, "Note:\n");
	vmm_cprintf(cdev, "   <guest_name> = node name under /guests "
			  "device tree node\n");
}

static int guest_list_iter(struct vmm_guest *guest, void *priv)
{
	int rc;
	char path[256];
	struct vmm_chardev *cdev = priv;

	rc = vmm_devtree_getpath(path, sizeof(path), guest->node);
	if (rc) {
		vmm_snprintf(path, sizeof(path),
			     "----- (error %d)", rc);
	}
	vmm_cprintf(cdev, " %-6d %-17s %-13s %-39s\n",
		    guest->id, guest->name,
		    (guest->is_big_endian) ? "big" : "little",
		    path);

	return VMM_OK;
}

static void cmd_guest_list(struct vmm_chardev *cdev)
{
	vmm_cprintf(cdev, "----------------------------------------"
			  "---------------------------------------\n");
	vmm_cprintf(cdev, " %-6s %-17s %-13s %-39s\n",
			 "ID ", "Name", "Endianness", "Device Path");
	vmm_cprintf(cdev, "----------------------------------------"
			  "---------------------------------------\n");
	vmm_manager_guest_iterate(guest_list_iter, cdev);
	vmm_cprintf(cdev, "----------------------------------------"
			  "---------------------------------------\n");
}

static int cmd_guest_create(struct vmm_chardev *cdev, const char *name)
{
	struct vmm_guest *guest = NULL;
	struct vmm_devtree_node *pnode = NULL, *node = NULL;

	pnode = vmm_devtree_getnode(VMM_DEVTREE_PATH_SEPARATOR_STRING
					VMM_DEVTREE_GUESTINFO_NODE_NAME);
	node = vmm_devtree_getchild(pnode, name);
	vmm_devtree_dref_node(pnode);
	if (!node) {
		vmm_cprintf(cdev, "Error: failed to find %s node under %s\n",
				  name, VMM_DEVTREE_PATH_SEPARATOR_STRING
					VMM_DEVTREE_GUESTINFO_NODE_NAME);
		return VMM_EFAIL;
	}

	guest = vmm_manager_guest_create(node);
	vmm_devtree_dref_node(node);
	if (!guest) {
		vmm_cprintf(cdev, "%s: Failed to create\n", name);
		return VMM_EFAIL;
	}

	vmm_cprintf(cdev, "%s: Created\n", name);

	return VMM_OK;
}

static int cmd_guest_destroy(struct vmm_chardev *cdev, const char *name)
{
	int ret;
	struct vmm_guest *guest = vmm_manager_guest_find(name);

	if (!guest) {
		vmm_cprintf(cdev, "Failed to find guest\n");
		return VMM_ENOTAVAIL;
	}

	if ((ret = vmm_manager_guest_destroy(guest))) {
		vmm_cprintf(cdev, "%s: Failed to destroy\n", name);
	} else {
		vmm_cprintf(cdev, "%s: Destroyed\n", name);
	}

	return ret;
}

static int cmd_guest_reset(struct vmm_chardev *cdev, const char *name)
{
	int ret;
	struct vmm_guest *guest = vmm_manager_guest_find(name);

	if (!guest) {
		vmm_cprintf(cdev, "Failed to find guest\n");
		return VMM_ENOTAVAIL;
	}

	if ((ret = vmm_manager_guest_reset(guest))) {
		vmm_cprintf(cdev, "%s: Failed to reset\n", name);
	} else {
		vmm_cprintf(cdev, "%s: Reset\n", name);
	}

	return ret;
}

static int cmd_guest_kick(struct vmm_chardev *cdev, const char *name)
{
	int ret;
	struct vmm_guest *guest = vmm_manager_guest_find(name);

	if (!guest) {
		vmm_cprintf(cdev, "Failed to find guest\n");
		return VMM_ENOTAVAIL;
	}

	if ((ret = vmm_manager_guest_kick(guest))) {
		vmm_cprintf(cdev, "%s: Failed to kick\n", name);
	} else {
		vmm_cprintf(cdev, "%s: Kicked\n", name);
	}

	return ret;
}

static int cmd_guest_pause(struct vmm_chardev *cdev, const char *name)
{
	int ret;
	struct vmm_guest *guest = vmm_manager_guest_find(name);

	if (!guest) {
		vmm_cprintf(cdev, "Failed to find guest\n");
		return VMM_ENOTAVAIL;
	}

	if ((ret = vmm_manager_guest_pause(guest))) {
		vmm_cprintf(cdev, "%s: Failed to pause\n", name);
	} else {
		vmm_cprintf(cdev, "%s: Paused\n", name);
	}

	return ret;
}

static int cmd_guest_status(struct vmm_chardev *cdev, const char *name) 
{
    int i,mod;
    struct vmm_guest *guest = vmm_manager_guest_find(name);
    struct vmm_vcpu *vcpu = NULL;
    irq_flags_t flags;

    if (!guest) {
        vmm_cprintf(cdev, "Failed to find guest\n");
        return VMM_ENOTAVAIL;
    }
    vmm_cprintf(cdev, "     ***STATUS OF %s*****\n",name);
    vmm_read_lock_irqsave_lite(&guest->vcpu_lock, flags);

    list_for_each_entry(vcpu, &guest->vcpu_list, head) {
        vmm_cprintf(cdev, "\t\tVCPU : ID:%d,SUBID:%d,ADDRESS:0x%08x\n",vcpu->id,vcpu->subid,(unsigned int)vcpu);
        vmm_cprintf(cdev, "STACK: \n");
        vmm_cprintf(cdev, "    START PC : 0x%08x\n",vcpu->start_pc);
        vmm_cprintf(cdev, "    STACK SZ : 0x%08x\n",vcpu->stack_va);
        vmm_cprintf(cdev, "    STACK VA : 0x%08x\n",vcpu->stack_sz);
        vmm_cprintf(cdev, "REGISTER:\n");
        vmm_cprintf(cdev, "    Exception stack ptr : 0x%08x\n", 
                vcpu->regs.sp_excp);
        vmm_cprintf(cdev, "    CPSR : 0x%08x\n", vcpu->regs.cpsr);
        vmm_cprintf(cdev, "    SP : 0x%08x", vcpu->regs.sp);
        vmm_cprintf(cdev, "    LR : 0x%08x", vcpu->regs.lr);
        vmm_cprintf(cdev, "    PC : 0x%08x\n", vcpu->regs.pc);
        mod = 0;
        for(i=0;i<CPU_GPR_COUNT;i++) {
            mod ++;
            vmm_cprintf(cdev, "    R%02d=0x%08x",i,vcpu->regs.gpr[i]);
            if (mod==4||mod==8||mod==12)
                vmm_cprintf(cdev,"\n");
        }
        vmm_cprintf(cdev,"\n");
    }
    vmm_read_unlock_irqrestore_lite(&guest->vcpu_lock, flags);
    return VMM_OK;
}

static int cmd_guest_resume(struct vmm_chardev *cdev, const char *name)
{
	int ret;
	struct vmm_guest *guest = vmm_manager_guest_find(name);

	if (!guest) {
		vmm_cprintf(cdev, "Failed to find guest\n");
		return VMM_ENOTAVAIL;
	}

	if ((ret = vmm_manager_guest_resume(guest))) {
		vmm_cprintf(cdev, "%s: Failed to resume\n", name);
	} else {
		vmm_cprintf(cdev, "%s: Resumed\n", name);
	}

	return ret;
}

static int cmd_guest_halt(struct vmm_chardev *cdev, const char *name)
{
	int ret;
	struct vmm_guest *guest = vmm_manager_guest_find(name);

	if (!guest) {
		vmm_cprintf(cdev, "Failed to find guest\n");
		return VMM_ENOTAVAIL;
	}

	if ((ret = vmm_manager_guest_halt(guest))) {
		vmm_cprintf(cdev, "%s: Failed to halt\n", name);
	} else {
		vmm_cprintf(cdev, "%s: Halted\n", name);
	}

	return ret;
}

static int  cmd_guest_loadmem(struct vmm_chardev *cdev, const char *name,
			     physical_addr_t gphys_addr, u32 word)
{
    struct vmm_guest *guest = vmm_manager_guest_find(name);
    if(!guest) {
	vmm_cprintf(cdev,"Failed to find guest\n");
	return VMM_ENOTAVAIL;
    }
    vmm_guest_memory_write(guest, gphys_addr, &word, 4, true);
    return VMM_OK;

}
/** REGISTER NUMBER TABLE:
 *  0 - 12 : R0-R12
 *  13 : SP
 *  14 : LR
 *  15 : PC
 **/
static int cmd_guest_reg(struct vmm_chardev * cdev, const char *name,
    			    physical_addr_t reg, u32 value)
{
    struct vmm_guest *guest = vmm_manager_guest_find(name);
    struct vmm_vcpu *vcpu = NULL;
    irq_flags_t flags;

    if (!guest) {
        return VMM_ENOTAVAIL;
    }
    vmm_write_lock_irqsave_lite(&guest->vcpu_lock, flags);

    list_for_each_entry(vcpu, &guest->vcpu_list, head) {
        cpu_vcpu_reg_write(vcpu,&vcpu->regs,(u32) reg,value);
    }
    vmm_write_unlock_irqrestore_lite(&guest->vcpu_lock, flags);
    return VMM_OK;
}

/** This function realize a bit swap on the register indicated by the 
 *    id = shitf/32
 **/
static int cmd_guest_reginject(struct vmm_chardev * cdev, const char *name,
    			    physical_addr_t reg, u32 shift)
{
    struct vmm_guest *guest = vmm_manager_guest_find(name);
    struct vmm_vcpu *vcpu = NULL;
    irq_flags_t flags;
    u32 mask = 1<<(shift%32);
    u32 value;
    u32 cpu_id = shift/32;
    
    if (!guest) {
        return VMM_ENOTAVAIL;
    }

    vmm_read_lock_irqsave_lite(&guest->vcpu_lock, flags);
    list_for_each_entry(vcpu, &guest->vcpu_list, head) {
        if (vcpu->id==cpu_id) {
            value = vcpu->regs.gpr[(u32)reg];
            break;
        }
    }
    vmm_read_unlock_irqrestore_lite(&guest->vcpu_lock, flags);
   
    value = value ^ mask;

    vmm_write_lock_irqsave_lite(&guest->vcpu_lock, flags);
    list_for_each_entry(vcpu, &guest->vcpu_list, head) {
        if(vcpu->id==cpu_id) {
            cpu_vcpu_reg_write(vcpu,&vcpu->regs,(u32) reg,value);
            break;
        }
    }
    vmm_write_unlock_irqrestore_lite(&guest->vcpu_lock, flags);
    return VMM_OK;
}

static int cmd_guest_inject(struct vmm_chardev * cdev, const char *name,
    			    physical_addr_t gphys_addr, u32 shift)
{
    struct vmm_guest *guest = vmm_manager_guest_find(name);
    u32 loaded;
    u32 buf,mask;
    if(shift>7) {
	vmm_cprintf(cdev,"Shift not valid\n");
	return VMM_EFAIL;
    }

    mask = 1 << shift;
    if(!guest) {
	vmm_cprintf(cdev,"Failed to find guest\n");
	return VMM_ENOTAVAIL;
    }
    loaded = vmm_guest_memory_read(guest, gphys_addr,
					       &buf, 4, FALSE);
    if (loaded != 4) {
	vmm_cprintf(cdev,"Not able to read the memory!\n");
	return VMM_EFAIL;
    }
    buf = buf ^ mask;
    vmm_guest_memory_write(guest, gphys_addr, &buf, 4, true);
    return VMM_OK;
}

static int cmd_guest_dumpmem(struct vmm_chardev *cdev, const char *name,
			     physical_addr_t gphys_addr, u32 len)
{
#define BYTES_PER_LINE 16
	u8 buf[BYTES_PER_LINE];
	u32 total_loaded = 0, loaded = 0, *mem;
	struct vmm_guest *guest = vmm_manager_guest_find(name);

	if (!guest) {
		vmm_cprintf(cdev, "Failed to find guest\n");
		return VMM_ENOTAVAIL;
	}

	len = (len + (BYTES_PER_LINE - 1)) & ~(BYTES_PER_LINE - 1);

	vmm_cprintf(cdev, "%s physical memory ", name);
	vmm_cprintf(cdev, "0x%"PRIPADDR" - 0x%"PRIPADDR":\n",
			    gphys_addr, (gphys_addr + len));
	while (total_loaded < len) {
		loaded = vmm_guest_memory_read(guest, gphys_addr,
					       buf, BYTES_PER_LINE, FALSE);
		if (loaded != BYTES_PER_LINE) {
			break;
		}
		mem = (u32 *)buf;
		vmm_cprintf(cdev, "0x%"PRIPADDR":", gphys_addr);
		vmm_cprintf(cdev, " %08x %08x %08x %08x\n",
			    mem[0], mem[1], mem[2], mem[3]);
		gphys_addr += BYTES_PER_LINE;
		total_loaded += BYTES_PER_LINE;
	}
#undef BYTES_PER_LINE
	if (total_loaded == len) {
		return VMM_OK;
	}

	return VMM_EFAIL;
}

static int cmd_guest_region(struct vmm_chardev *cdev, const char *name,
			    physical_addr_t gphys_addr)
{
	struct vmm_guest *guest = vmm_manager_guest_find(name);
	struct vmm_region *reg = NULL;
	struct vmm_emudev *emudev;

	if (!guest) {
		vmm_cprintf(cdev, "Failed to find guest\n");
		return VMM_ENOTAVAIL;
	}

	reg = vmm_guest_find_region(guest, gphys_addr, VMM_REGION_MEMORY,
				    TRUE);
	if (!reg) {
		reg = vmm_guest_find_region(guest, gphys_addr, VMM_REGION_IO,
					    TRUE);
	}
	if (!reg) {
		vmm_cprintf(cdev, "Memory region not found\n");
		return VMM_EFAIL;
	}

	vmm_cprintf(cdev, "Region guest physical address: 0x%"PRIPADDR"\n",
		    reg->gphys_addr);

	vmm_cprintf(cdev, "Region host physical address : 0x%"PRIPADDR"\n",
		    reg->hphys_addr);

	vmm_cprintf(cdev, "Region physical size         : 0x%"PRIPSIZE"\n",
		    reg->phys_size);

	vmm_cprintf(cdev, "Region flags                 : 0x%08x\n",
		    reg->flags);

	vmm_cprintf(cdev, "Region node name             : %s\n",
		    reg->node->name);

	if (!reg->devemu_priv) {
		return VMM_OK;
	}
	emudev = reg->devemu_priv;
	if (!emudev->emu) {
		return VMM_OK;
	}

	vmm_cprintf(cdev, "Region emulator name         : %s\n",
		    emudev->emu->name);

	return VMM_OK;
}

static int cmd_guest_param(struct vmm_chardev *cdev, int argc, char **argv,
			   physical_addr_t *src_addr, u32 *size)
{
	if (argc < 4) {
		vmm_cprintf(cdev, "Error: Insufficient argument for "
			    "command dumpmem.\n");
		cmd_guest_usage(cdev);
		return VMM_EFAIL;
	}
	*src_addr = (physical_addr_t)strtoull(argv[3], NULL, 0);
	if (argc > 4) {
		*size = (physical_size_t)strtoull(argv[4], NULL, 0);
	} else {
		*size = 64;
	}
	return VMM_OK;
}

static int cmd_guest_exec(struct vmm_chardev *cdev, int argc, char **argv)
{
	int ret = VMM_OK;
	u32 size;
	u32 value;
	physical_addr_t src_addr;
	if (argc == 2) {
		if (strcmp(argv[1], "help") == 0) {
			cmd_guest_usage(cdev);
			return VMM_OK;
		} else if (strcmp(argv[1], "list") == 0) {
			cmd_guest_list(cdev);
			return VMM_OK;
		}
	}
	if (argc < 3) {
		cmd_guest_usage(cdev);
		return VMM_EFAIL;
	}
	if (strcmp(argv[1], "create") == 0) {
		return cmd_guest_create(cdev, argv[2]);
	} else if (strcmp(argv[1], "destroy") == 0) {
		return cmd_guest_destroy(cdev, argv[2]);
	} else if (strcmp(argv[1], "reset") == 0) {
		return cmd_guest_reset(cdev, argv[2]);
	} else if (strcmp(argv[1], "kick") == 0) {
		return cmd_guest_kick(cdev, argv[2]);
	} else if (strcmp(argv[1], "pause") == 0) {
		return cmd_guest_pause(cdev, argv[2]);
	} else if (strcmp(argv[1], "resume") == 0) {
		return cmd_guest_resume(cdev, argv[2]);
	} else if (strcmp(argv[1], "status") == 0) {
		return cmd_guest_status(cdev, argv[2]);
	} else if (strcmp(argv[1], "halt") == 0) {
		return cmd_guest_halt(cdev, argv[2]);
	} else if (strcmp(argv[1], "dumpmem") == 0) {
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &size);
		if (VMM_OK != ret) {
			return ret;
		}
		return cmd_guest_dumpmem(cdev, argv[2], src_addr, size);
	} else if (strcmp(argv[1], "loadmem") == 0) {
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &value);
		if (VMM_OK != ret) {
			return ret;
		}
		return cmd_guest_loadmem(cdev, argv[2], src_addr, value);
	} else if (strcmp(argv[1], "inject") == 0) {
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &value);
		if (VMM_OK != ret) {
			return ret;
		}
		return cmd_guest_inject(cdev, argv[2], src_addr, value);
	} else if (strcmp(argv[1], "reginject") == 0) {
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &value);
		if (VMM_OK != ret) {
			return ret;
		}
		return cmd_guest_reginject(cdev, argv[2], src_addr, value);
	} else if (strcmp(argv[1], "reg") == 0) {
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &value);
		if (VMM_OK != ret) {
			return ret;
		}
		return cmd_guest_reg(cdev, argv[2], src_addr, value);
	} else if (strcmp(argv[1], "region") == 0) {
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &size);
		if (VMM_OK != ret) {
			return ret;
		}
		return cmd_guest_region(cdev, argv[2], src_addr);
	} else {
		cmd_guest_usage(cdev);
		return VMM_EFAIL;
	}
	return VMM_OK;
}

static struct vmm_cmd cmd_guest = {
	.name = "guest",
	.desc = "control commands for guest",
	.usage = cmd_guest_usage,
	.exec = cmd_guest_exec,
};

static int __init cmd_guest_init(void)
{
	return vmm_cmdmgr_register_cmd(&cmd_guest);
}

static void __exit cmd_guest_exit(void)
{
	vmm_cmdmgr_unregister_cmd(&cmd_guest);
}

VMM_DECLARE_MODULE(MODULE_DESC,
			MODULE_AUTHOR,
			MODULE_LICENSE,
			MODULE_IPRIORITY,
			MODULE_INIT,
			MODULE_EXIT);
