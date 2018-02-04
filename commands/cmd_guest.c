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
#include <vmm_delay.h>
#include <arch_delay.h>
#include <arch_cpu.h>
#include <libs/stringlib.h>
#include <cpu_defines.h>
#include <cpu_vcpu_helper.h>
#include <vmm_timer.h>
#include <asm/div64.h>
#include <libs/random_MT.h>
#include <libs/vserial.h>

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
	vmm_cprintf(cdev, "   guest reginject <guest_name> <gphys_addr> "
			  "<shift>\n");
	vmm_cprintf(cdev, "   guest reg     <guest_name> <reg_num> "
			  "<value>\n");
	vmm_cprintf(cdev, "   guest cycle_inject <guest_name> "
			  "<gphys_addr> <shift> <cycle>\n");
	vmm_cprintf(cdev, "   guest cycle_reginject <guest_name> "
			  "<gphys_addr> <shift> <cycle>\n");
	vmm_cprintf(cdev, "   guest region  <guest_name> <gphys_addr>\n");
	vmm_cprintf(cdev, "   guest inject_camp <guest_name> <gphys_addr_min>"
			  " <gphys_addr_max> <time_max> <nb>\n");
	vmm_cprintf(cdev, "   guest stat_camp <guest_name> <gphys_addr_min>"
			  " <gphys_addr_max> <time_max> <margin_err> <cut_off_pt>\n");
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

/*
 * Print the status of a guest.
 * This includes all registers state and the stack state.
 */
static int cmd_guest_status(struct vmm_chardev *cdev, const char *name)
{
    int i, mod;
    struct vmm_guest *guest = vmm_manager_guest_find(name);
    struct vmm_vcpu *vcpu = NULL;
    irq_flags_t flags;

    if (!guest) {
        vmm_cprintf(cdev, "Failed to find guest\n");
        return VMM_ENOTAVAIL;
    }
    vmm_cprintf(cdev, "     ***STATUS OF %s*****\n", name);
    // Lock state of guest in read mode
    vmm_read_lock_irqsave_lite(&guest->vcpu_lock, flags);

    // Print status for each virtual cpu of the guest
    list_for_each_entry(vcpu, &guest->vcpu_list, head) {
        // Print state of special registers and stack
        vmm_cprintf(cdev, "\t\tVCPU : ID:%d,SUBID:%d,ADDRESS:0x%08x\n", 
                vcpu->id, vcpu->subid, (unsigned int) vcpu);
        vmm_cprintf(cdev, "STACK: \n");
        vmm_cprintf(cdev, "    START PC : 0x%08x\n", vcpu->start_pc);
        vmm_cprintf(cdev, "    STACK SZ : 0x%08x\n", vcpu->stack_va);
        vmm_cprintf(cdev, "    STACK VA : 0x%08x\n", vcpu->stack_sz);
        vmm_cprintf(cdev, "REGISTER:\n");
        vmm_cprintf(cdev, "    Exception stack ptr : 0x%08x\n",
                vcpu->regs.sp_excp);
        vmm_cprintf(cdev, "    CPSR : 0x%08x\n", vcpu->regs.cpsr);
        vmm_cprintf(cdev, "    SP : 0x%08x", vcpu->regs.sp);
        vmm_cprintf(cdev, "    LR : 0x%08x", vcpu->regs.lr);
        vmm_cprintf(cdev, "    PC : 0x%08x\n", vcpu->regs.pc);
        mod = 0;
        // Print state of standard registers
        for (i = 0; i < CPU_GPR_COUNT; i++) {
            mod ++;
            vmm_cprintf(cdev, "    R%02d=0x%08x", i, vcpu->regs.gpr[i]);
            if (mod == 4 || mod == 8 || mod == 12)
                vmm_cprintf(cdev, "\n");
        }
        vmm_cprintf(cdev, "\n");
    }
    // Unlock guest
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

/*
 * Load a word into the guest memory
 */
static int  cmd_guest_loadmem(struct vmm_chardev *cdev, const char *name,
			     physical_addr_t gphys_addr, u32 word)
{
    struct vmm_guest *guest = vmm_manager_guest_find(name);
    if (!guest) {
	    vmm_cprintf(cdev,"Failed to find guest\n");
	    return VMM_ENOTAVAIL;
    }
    vmm_guest_memory_write(guest, gphys_addr, &word, 4, false);
    return VMM_OK;

}

/** REGISTER NUMBER TABLE:
 *  0 - 12 : R0-R12
 *  13 : SP
 *  14 : LR
 *  15 : PC
 **/
/*
 * Write a value into a register, in each virtual cpu of the guest
 * Why is that so? Because we do not know which vcpu the program will use.
 */
static int cmd_guest_reg(struct vmm_chardev * cdev, const char *name,
    			    physical_addr_t reg, u32 value)
{
    struct vmm_guest *guest = vmm_manager_guest_find(name);
    struct vmm_vcpu *vcpu = NULL;
    irq_flags_t flags;

    if (!guest) {
        return VMM_ENOTAVAIL;
    }
    // Lock guest state in write mode
    vmm_write_lock_irqsave_lite(&guest->vcpu_lock, flags);

    // Write register value for each virtual cpu
    list_for_each_entry(vcpu, &guest->vcpu_list, head) {
        cpu_vcpu_reg_write(vcpu, &vcpu->regs, (u32) reg, value);
    }
    // Unlock guest state
    vmm_write_unlock_irqrestore_lite(&guest->vcpu_lock, flags);
    return VMM_OK;
}

/** This function realize a bit flip on the register indicated by the
 *     shift 5..0 = bit that will be swap example: shift = 5 -> mask = 0b10000
 *     shift 6..6 = cpu id (0 or 1)
 **/
static int cmd_guest_reginject(struct vmm_chardev * cdev, const char *name,
    			    physical_addr_t reg, u32 shift)
{
    struct vmm_guest *guest = vmm_manager_guest_find(name);
    struct vmm_vcpu *vcpu = NULL;
    irq_flags_t flags;
    u32 mask = 1 << (shift % 32);
    u32 value = 0;
    u32 cpu_id = shift / 32;

    if (!guest) {
        return VMM_ENOTAVAIL;
    }
    // Lock guest state in read mode
    vmm_read_lock_irqsave_lite(&guest->vcpu_lock, flags);
    list_for_each_entry(vcpu, &guest->vcpu_list, head) {
        if (vcpu->id==cpu_id) {
            /* get the correct register in the correct cpu */
            value = vcpu->regs.gpr[(u32)reg];
            break;
        }
    }
    vmm_read_unlock_irqrestore_lite(&guest->vcpu_lock, flags);
    
    /* inject the error */
    value = value ^ mask;

    vmm_write_lock_irqsave_lite(&guest->vcpu_lock, flags);
    list_for_each_entry(vcpu, &guest->vcpu_list, head) {
        if(vcpu->id==cpu_id) {
            /* update the value of the register */
            cpu_vcpu_reg_write(vcpu,&vcpu->regs,(u32) reg,value);
            break;
        }
    }
    vmm_write_unlock_irqrestore_lite(&guest->vcpu_lock, flags);
    return VMM_OK;
}

/*
 * Realize a bit-flip at the address <gphys_addr>, shifted by the value
 * <shift> to access all bits in the byte
 */
static int cmd_guest_inject(struct vmm_chardev * cdev, const char *name,
    			    physical_addr_t gphys_addr, u32 shift)
{
    struct vmm_guest *guest = vmm_manager_guest_find(name);
    u32 loaded;
    u32 buf, mask;
    if (shift > 7) {
	vmm_cprintf(cdev, "Shift not valid\n");
	    return VMM_EFAIL;
    }

    mask = 1 << shift;
    if (!guest) {
	    vmm_cprintf(cdev,"Failed to find guest\n");
	    return VMM_ENOTAVAIL;
    }
    // Read memory value and store it into buf
    loaded = vmm_guest_memory_read(guest, gphys_addr, &buf, 4, FALSE);
    if (loaded != 4) {
	    vmm_cprintf(cdev,"Not able to read the memory!\n");
	    return VMM_EFAIL;
    }
    // Edit the value
    buf = buf ^ mask;
    // Write the edited value
    vmm_guest_memory_write(guest, gphys_addr, &buf, 4, false);
    return VMM_OK;
}

static int cmd_guest_dumpmem(struct vmm_chardev *cdev, const char *name,
			     physical_addr_t gphys_addr, u32 len)
{ //FINDME
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

/*
 * Inject a fault (bit-flip) at the address <gphys_addr> shifted of <shift>
 * bits, and at the cycle <cycle>
 */
static int cmd_guest_cycle_inject(struct vmm_chardev * cdev, const char *name,
                    physical_addr_t gphys_addr, u32 shift, u64 cycle)
{
    u64 time = 0;

    // Arch specific function that calculates the number of cycles that
    // one delay loop lasts
    u32 estimated_cycle_by_loop = arch_delay_loop_cycles(1);

    // Starts execution of the program
    cmd_guest_reset(cdev, name);
    cmd_guest_kick(cdev, name);

    // Wait the appropriate time
    while (time < cycle) {
        time += estimated_cycle_by_loop;
        arch_delay_loop(1);
    }

    // Pause the guest, inject the fault and resume execution
    cmd_guest_pause(cdev, name);
    vmm_cprintf(cdev, "%s: Injected fault (cycle %llu, address 0x%llx, shift %u)\n", name, time, (u64) gphys_addr, shift);
    cmd_guest_inject(cdev, name, gphys_addr, shift);
    cmd_guest_resume(cdev, name);
    return VMM_OK;
}

static int cmd_guest_cycle_reginject(struct vmm_chardev * cdev, const char *name,
                    physical_addr_t gphys_addr, u32 shift, u64 cycle)
{
    u64 time = 0;
    u32 estimated_cycle_by_loop = arch_delay_loop_cycles(1);
    cmd_guest_kick(cdev, name);
    while (time < cycle) {
        time += estimated_cycle_by_loop;
        arch_delay_loop(1);
    }
    cmd_guest_pause(cdev, name);
    vmm_cprintf(cdev, "%s: Injected fault (cycle %llu, reg %llu, shift %u)\n", name, time, (u64) gphys_addr, shift);
    cmd_guest_reginject(cdev, name, gphys_addr, shift);
    cmd_guest_resume(cdev, name);
    return VMM_OK;
}

/*
 * ARM Cortex A9 processor does not have a modulo operator
 */
static u32 mod(u32 a, u32 b){
	return (a-(b*do_div(a,b)));
}

/*
 * Wait for a given number of cycles
 */
static void wait(u64 nb_cycle){
	u64 cycles = nb_cycle;
    u64 elapsed = 0;
    u64 estimated_cycles = arch_delay_loop_cycles(1);

    while (elapsed < cycles) {
        elapsed += estimated_cycles;
        arch_delay_loop(1);
    }
}

/*
 * Begin an injection campaign of <nb_inject> injections
 *
 * <addr_min> and <addr_max> define the memory space where faults
 * can be injected
 * <cycle_max> defines the maximum number of cycles that the execution of the
 * program is supposed to last. 
 */
static int cmd_guest_inject_camp(struct vmm_chardev * cdev, const char *name,
                    u32 addr_min, u32 addr_max, u64 cycle_max, u32 nb_inject)
{
    // Init random (mandatory, values are set according to the source)
    // See random_MT.h for more infos
    unsigned long long init[4]={0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL}, length=4;
    init_by_array64(init, length); 

	// We do it nb_inject times
	int i = 0;
	while (i< nb_inject){
        // 32 bits random
		u64 rdm = genrand64_int64();

		// random between addr_min et addr_max
		physical_addr_t gphys_addr_inj = (mod(rdm, addr_max - addr_min)) + addr_min;

        // the offset is the number of the bit in the byte (so < 8)
		u32 shift = genrand64_int64() % 8; 
		u64 cycle = genrand64_int64();

        // It is now under cycle_max as requested
		cycle = mod(cycle, cycle_max); 

		cmd_guest_cycle_inject(cdev, name, gphys_addr_inj, shift, cycle);

		// After the inject only 'cycle' number of cycles have been executed
		wait(cycle_max - cycle);
        cmd_guest_pause(cdev, name);
        vserial_dump(cdev, "guest0/uart0", 2048);
        vmm_sdelay(2);
		cmd_guest_reset(cdev, name);
		i++;
	}
	return VMM_OK;
}

/*
 * Calculates all the parameters needed to launch an injection campaign
 * according to this paper provided by our supervisor
 * https://www.date-conference.com/proceedings-archive/PAPERS/2009/DATE09/PDFFILES/05.5_4.PDF
 * (Statistical Fault Injection, by R. Leveugle, A. Calvez, P. Maistri and
 *  P. Vanhauwaert)
 *
 *  <err_margin> is the margin of error (<e> in the paper)
 *  <cut_off_pt> is the cut-off point, or critical value (<t> in the paper)
 *  !!! Both need to be percents between 0 and 100 !!!
 */
static int cmd_guest_stat_camp (struct vmm_chardev *cdev,
                    const char *name, u32 addr_min, u32 addr_max,
                    u64 cycle_max, u32 err_margin, u32 cut_off_pt)
{
    // err_percent true value is 0.5 (see paper)
    u64 pop = (addr_max - addr_min) * cycle_max; // (N in paper)
    // Careful: possible overflow with pop
    
    u64 nb_faults = 0; // (n in paper)

    // cut_off_pt, err_margin have been multiplied by 100
    // calcul is err_margin^2 / cut_off^2 so its ok
    err_margin *= err_margin;
    cut_off_pt *= cut_off_pt;

    // Just computing the formula step by step
    nb_faults = do_div(err_margin * 4 * (pop - 1), cut_off_pt);
    nb_faults = do_div(pop, 1 + nb_faults);

    // Launching the campaign
    vmm_printf("%s: Launched injection campaign (pop %llu, nb %llu)\n", name, pop, nb_faults);
    cmd_guest_inject_camp(cdev, name, addr_min, addr_max, cycle_max, nb_faults);
    return VMM_OK;
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
			   physical_addr_t *src_addr, u32 *size, u32 *time)
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
    if (argc > 5) {
		*time = (physical_size_t)strtoull(argv[5], NULL, 0);
	} else {
		*time = 0;
	}
	return VMM_OK;
}

static int cmd_guest_exec(struct vmm_chardev *cdev, int argc, char **argv)
{
	int ret = VMM_OK;
	u32 size;
	u32 value;
    u32 time;
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
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &size, &time);
		if (VMM_OK != ret) {
			return ret;
		}
		return cmd_guest_dumpmem(cdev, argv[2], src_addr, size);
	} else if (strcmp(argv[1], "loadmem") == 0) {
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &value, &time);
		if (VMM_OK != ret) {
			return ret;
		}
		return cmd_guest_loadmem(cdev, argv[2], src_addr, value);
	} else if (strcmp(argv[1], "cycle_inject") == 0) {
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &value, &time);
		if (VMM_OK != ret) {
			return ret;
		}
		return cmd_guest_cycle_inject(cdev, argv[2], src_addr, value, time);
	} else if (strcmp(argv[1], "cycle_reginject") == 0) {
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &value, &time);
		if (VMM_OK != ret) {
			return ret;
		}
		return cmd_guest_cycle_reginject(cdev, argv[2], src_addr, value, time);
	} else if (strcmp(argv[1], "inject") == 0) {
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &value, &time);
		if (VMM_OK != ret) {
			return ret;
		}
		return cmd_guest_inject(cdev, argv[2], src_addr, value);
	} else if (strcmp(argv[1], "inject_camp") == 0){
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &value, &time);
		if (VMM_OK != ret) {
			return ret;
		}
		u32 nb_inject = (physical_size_t)strtoull(argv[6], NULL, 0);
		return cmd_guest_inject_camp(cdev, argv[2], src_addr, value, time, nb_inject);
	} else if (strcmp(argv[1], "stat_camp") == 0){
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &value, &time);
		if (VMM_OK != ret) {
			return ret;
		}
		u32 err_margin = (physical_size_t)strtoull(argv[6], NULL, 0);
		u32 cut_off_pt = (physical_size_t)strtoull(argv[7], NULL, 0);
		return cmd_guest_stat_camp(cdev, argv[2], src_addr, value, time, err_margin, cut_off_pt);
	} else if (strcmp(argv[1], "reginject") == 0) {
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &value, &time);
		if (VMM_OK != ret) {
			return ret;
		}
		return cmd_guest_reginject(cdev, argv[2], src_addr, value);
	} else if (strcmp(argv[1], "reg") == 0) {
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &value, &time);
		if (VMM_OK != ret) {
			return ret;
		}
		return cmd_guest_reg(cdev, argv[2], src_addr, value);
	} else if (strcmp(argv[1], "region") == 0) {
		ret = cmd_guest_param(cdev, argc, argv, &src_addr, &size, &time);
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
