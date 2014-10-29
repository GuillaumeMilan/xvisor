/**
 * Copyright (c) 2012 Anup Patel.
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
 * @file cmd_vfs.c
 * @author Anup Patel (anup@brainfault.org)
 * @author Himanshu Chauhan (hschauhan@nulltrace.org)
 * @brief Implementation of vfs command
 */

#include <vmm_error.h>
#include <vmm_heap.h>
#include <vmm_stdio.h>
#include <vmm_devtree.h>
#include <vmm_wallclock.h>
#include <vmm_manager.h>
#include <vmm_host_aspace.h>
#include <vmm_guest_aspace.h>
#include <vmm_modules.h>
#include <vmm_cmdmgr.h>
#include <libs/libfdt.h>
#include <libs/stringlib.h>
#include <libs/vfs.h>

#if CONFIG_CRYPTO_HASH_MD5
#include <libs/md5.h>
#endif

#if CONFIG_CRYPTO_HASH_SHA256
#include <libs/sha256.h>
#endif

#define MODULE_DESC			"Command vfs"
#define MODULE_AUTHOR			"Anup Patel"
#define MODULE_LICENSE			"GPL"
#define MODULE_IPRIORITY		(VFS_IPRIORITY+1)
#define	MODULE_INIT			cmd_vfs_init
#define	MODULE_EXIT			cmd_vfs_exit

#define VFS_MAX_FDT_SZ			(32*1024)
#define VFS_LOAD_BUF_SZ			256

static void cmd_vfs_usage(struct vmm_chardev *cdev)
{
	vmm_cprintf(cdev, "Usage:\n");
	vmm_cprintf(cdev, "   vfs help\n");
	vmm_cprintf(cdev, "   vfs fslist\n");
	vmm_cprintf(cdev, "   vfs mplist\n");
	vmm_cprintf(cdev, "   vfs mount <bdev_name> <path_to_mount>\n");
	vmm_cprintf(cdev, "   vfs umount <path_to_unmount>\n");
	vmm_cprintf(cdev, "   vfs ls <path_to_dir>\n");
	vmm_cprintf(cdev, "   vfs cat <path_to_file>\n");
#if CONFIG_CRYPTO_HASH_MD5
	vmm_cprintf(cdev, "   vfs md5 <path_to_file>\n");
#endif
#if CONFIG_CRYPTO_HASH_SHA256
	vmm_cprintf(cdev, "   vfs sha256 <path_to_file>\n");
#endif
	vmm_cprintf(cdev, "   vfs run <path_to_file>\n");
	vmm_cprintf(cdev, "   vfs mv <old_path> <new_path>\n");
	vmm_cprintf(cdev, "   vfs rm <path_to_file>\n");
	vmm_cprintf(cdev, "   vfs mkdir <path_to_dir>\n");
	vmm_cprintf(cdev, "   vfs rmdir <path_to_dir>\n");	
	vmm_cprintf(cdev, "   vfs fdt_load <devtree_path> "
			  "<devtree_root_name> <path_to_fdt_file> "
			  "[<alias0>,<attr_name>,<attr_type>,<value>] "
			  "[<alias1>,<attr_name>,<attr_type>,<value>] ...\n");
	vmm_cprintf(cdev, "   vfs host_load <host_phys_addr> "
			  "<path_to_file> [<file_offset>] [<byte_count>]\n");
	vmm_cprintf(cdev, "   vfs host_load_list <path_to_list_file>\n");
	vmm_cprintf(cdev, "   vfs guest_load <guest_name> <guest_phys_addr> "
			  "<path_to_file> [<file_offset>] [<byte_count>]\n");
	vmm_cprintf(cdev, "   vfs guest_load_list <guest_name> "
			  "<path_to_list_file>\n");
	vmm_cprintf(cdev, "Note:\n");
	vmm_cprintf(cdev, "   <attr_type> = unknown|string|bytes|"
					   "uint32|uint64|"
			  		   "physaddr|physsize|"
					   "virtaddr|virtsize\n");
}

static int cmd_vfs_fslist(struct vmm_chardev *cdev)
{
	int num, count;
	struct filesystem *fs;

	vmm_cprintf(cdev, "----------------------------------------"
			  "----------------------------------------\n");
	vmm_cprintf(cdev, " %-9s %-69s\n", "Num", "Name");
	vmm_cprintf(cdev, "----------------------------------------"
			  "----------------------------------------\n");
	count = vfs_filesystem_count();
	for (num = 0; num < count; num++) {
		fs = vfs_filesystem_get(num);
		vmm_cprintf(cdev, " %-9d %-69s\n", num, fs->name);
	}
	vmm_cprintf(cdev, "----------------------------------------"
			  "----------------------------------------\n");

	return VMM_OK;
}

static int cmd_vfs_mplist(struct vmm_chardev *cdev)
{
	int num, count;
	const char *mode;
	struct mount *m;

	vmm_cprintf(cdev, "----------------------------------------"
			  "----------------------------------------\n");
	vmm_cprintf(cdev, " %-15s %-11s %-11s %-39s\n", 
			  "BlockDev", "Filesystem", "Mode", "Path");
	vmm_cprintf(cdev, "----------------------------------------"
			  "----------------------------------------\n");
	count = vfs_mount_count();
	for (num = 0; num < count; num++) {
		m = vfs_mount_get(num);
		switch (m->m_flags & MOUNT_MASK) {
		case MOUNT_RDONLY:
			mode = "read-only";
			break;
		case MOUNT_RW:
			mode = "read-write";
			break;
		default:
			mode = "unknown";
			break;
		};
		vmm_cprintf(cdev, " %-15s %-11s %-11s %-39s\n", 
				  m->m_dev->name, m->m_fs->name, 
				  mode, m->m_path);
	}
	vmm_cprintf(cdev, "----------------------------------------"
			  "----------------------------------------\n");

	return VMM_OK;
}

static int cmd_vfs_mount(struct vmm_chardev *cdev, 
			 const char *dev, const char *path)
{
	int rc;
	bool found;
	int fd, num, count;
	struct vmm_blockdev *bdev;
	struct filesystem *fs;

	bdev = vmm_blockdev_find(dev);
	if (!bdev) {
		vmm_cprintf(cdev, "Block device %s not found\n", dev);
		return VMM_ENODEV;
	}

	if (strcmp(path, "/") != 0) {
		fd = vfs_opendir(path);
		if (fd < 0) {
			vmm_cprintf(cdev, "Directory %s not found\n", path);
			return fd;
		} else {
			vfs_closedir(fd);
		}
	}

	found = FALSE;
	count = vfs_filesystem_count();
	vmm_cprintf(cdev, "Trying:");
	for (num = 0; num < count; num++) {
		fs = vfs_filesystem_get(num);
		vmm_cprintf(cdev, " %s", fs->name);
		rc = vfs_mount(path, fs->name, dev, MOUNT_RW);
		if (!rc) {
			found = TRUE;
			vmm_cprintf(cdev, "\n"); 
			break;
		}
	}

	if (!found) {
		vmm_cprintf(cdev, "\nMount failed\n");
		return VMM_ENOSYS;
	}

	vmm_cprintf(cdev, "Mounted %s using %s at %s\n", dev, fs->name, path);

	return VMM_OK;
}

static int cmd_vfs_umount(struct vmm_chardev *cdev, const char *path)
{
	int rc;

	rc = vfs_unmount(path);
	if (rc) {
		vmm_cprintf(cdev, "Unmount failed\n"); 
	} else {
		vmm_cprintf(cdev, "Unmount successful\n"); 
	}

	return rc;
}

static int cmd_vfs_ls(struct vmm_chardev *cdev, const char *path)
{
	char type[11];
	char dpath[VFS_MAX_PATH];
	int fd, rc, plen, total_ent;
	struct vmm_timeinfo ti;
	struct stat st;
	struct dirent d;

	fd = vfs_opendir(path);
	if (fd < 0) {
		vmm_cprintf(cdev, "Failed to opendir %s\n", path);
		return fd;
	}

	if ((plen = strlcpy(dpath, path, sizeof(dpath))) >= sizeof(dpath)) {
		rc = VMM_EOVERFLOW;
		goto closedir_fail;
	}

	if (path[plen-1] != '/') {
		if ((plen = strlcat(dpath, "/", sizeof(dpath))) >=
		    sizeof(dpath)) {
			rc = VMM_EOVERFLOW;
			goto closedir_fail;
		}
	}

	total_ent = 0;
	while (!vfs_readdir(fd, &d)) {
		dpath[plen] = '\0';
		if (strlcat(dpath, d.d_name, sizeof(dpath)) >= sizeof(dpath)) {
			rc = VMM_EOVERFLOW;
			goto closedir_fail;
		}
		rc = vfs_stat(dpath, &st);
		if (rc) {
			vmm_cprintf(cdev, "Failed to get %s stat\n", dpath);
			goto closedir_fail;
		}
		strcpy(type, "----------");
		if (st.st_mode & S_IFDIR) {
			type[0]= 'd';
		} else if (st.st_mode & S_IFCHR) {
			type[0]= 'c';
		} else if (st.st_mode & S_IFBLK) {
			type[0]= 'b';
		} else if (st.st_mode & S_IFLNK) {
			type[0]= 'l';
		}
		if (st.st_mode & S_IRUSR) {
			type[1] = 'r';
		}
		if (st.st_mode & S_IWUSR) {
			type[2] = 'w';
		}
		if (st.st_mode & S_IXUSR) {
			type[3] = 'x';
		}
		if (st.st_mode & S_IRGRP) {
			type[4] = 'r';
		}
		if (st.st_mode & S_IWGRP) {
			type[5] = 'w';
		}
		if (st.st_mode & S_IXGRP) {
			type[6] = 'x';
		}
		if (st.st_mode & S_IROTH) {
			type[7] = 'r';
		}
		if (st.st_mode & S_IWOTH) {
			type[8] = 'w';
		}
		if (st.st_mode & S_IXOTH) {
			type[9] = 'x';
		}
		vmm_cprintf(cdev, "%10s ", type);
		vmm_cprintf(cdev, "%10ll ", st.st_size);
		vmm_wallclock_mkinfo(st.st_mtime, 0, &ti);
		switch (ti.tm_mon) {
		case 0:
			vmm_cprintf(cdev, "%s ", "Jan");
			break;
		case 1:
			vmm_cprintf(cdev, "%s ", "Feb");
			break;
		case 2:
			vmm_cprintf(cdev, "%s ", "Mar");
			break;
		case 3:
			vmm_cprintf(cdev, "%s ", "Apr");
			break;
		case 4:
			vmm_cprintf(cdev, "%s ", "May");
			break;
		case 5:
			vmm_cprintf(cdev, "%s ", "Jun");
			break;
		case 6:
			vmm_cprintf(cdev, "%s ", "Jul");
			break;
		case 7:
			vmm_cprintf(cdev, "%s ", "Aug");
			break;
		case 8:
			vmm_cprintf(cdev, "%s ", "Sep");
			break;
		case 9:
			vmm_cprintf(cdev, "%s ", "Oct");
			break;
		case 10:
			vmm_cprintf(cdev, "%s ", "Nov");
			break;
		case 11:
			vmm_cprintf(cdev, "%s ", "Dec");
			break;
		default:
			break;
		};
		vmm_cprintf(cdev, "%02d %d %02d:%02d:%02d ", 
				  ti.tm_mday, ti.tm_year + 1900, 
				  ti.tm_hour, ti.tm_min, ti.tm_sec);
		vmm_cprintf(cdev, "%s\n", d.d_name);
		total_ent++;
	}
	vmm_cprintf(cdev, "total %d\n", total_ent);
	rc = vfs_closedir(fd);
	if (rc) {
		vmm_cprintf(cdev, "Failed to closedir %s\n", path);
		return rc;
	}

	return VMM_OK;

closedir_fail:
	vfs_closedir(fd);
	return rc;
}

static int cmd_vfs_run(struct vmm_chardev *cdev, const char *path)
{
	int fd, rc;
	u32 len;
	size_t buf_rd;
	char buf[VFS_LOAD_BUF_SZ];
	struct stat st;
	u32 tok_len;
	char *token, *save;
	const char *delim = "\n";
	u32 end, cleanup = 0;

	fd = vfs_open(path, O_RDONLY, 0);
	if (fd < 0) {
		vmm_cprintf(cdev, "Failed to open %s\n", path);
		return fd;
	}

	rc = vfs_fstat(fd, &st);
	if (rc) {
		vfs_close(fd);
		vmm_cprintf(cdev, "Failed to stat %s\n", path);
		return rc;
	}

	if (!(st.st_mode & S_IFREG)) {
		vfs_close(fd);
		vmm_cprintf(cdev, "Cannot read %s\n", path);
		return VMM_EINVALID;
	}

	len = st.st_size;
	while (len) {
		memset(buf, 0, sizeof(buf));

		buf_rd = (len < VFS_LOAD_BUF_SZ) ? len : VFS_LOAD_BUF_SZ;
		buf_rd = vfs_read(fd, buf, buf_rd);
		if (buf_rd < 1) {
			break;
		}

		end = buf_rd - 1;
		while (buf[end] != '\n') {
			buf[end] = 0;
			end--;
			cleanup++;
		}

		if (cleanup) {
			vfs_lseek(fd, (buf_rd - cleanup), SEEK_SET);
			cleanup = 0;
		}

		for (token = strtok_r(buf, delim, &save); token;
		     token = strtok_r(NULL, delim, &save)) {
			tok_len = strlen(token);
			if (*token != '#' && *token != '\n') {
				vmm_cmdmgr_execute_cmdstr(cdev, token, NULL);
			}

			len -= (tok_len + 1);
		}
	}

	rc = vfs_close(fd);
	if (rc) {
		vmm_cprintf(cdev, "Failed to close %s\n", path);
		return rc;
	}

	return VMM_OK;
}

#if CONFIG_CRYPTO_HASH_MD5
static int cmd_vfs_md5(struct vmm_chardev *cdev, const char *path)
{
	int fd, rc, i;
	u32 len;
	size_t buf_rd;
	u8 buf[VFS_LOAD_BUF_SZ];
	struct stat st;
	struct md5_context md5c;
	u8 digest[16];

	fd = vfs_open(path, O_RDONLY, 0);
	if (fd < 0) {
		vmm_cprintf(cdev, "Failed to open %s\n", path);
		return fd;
	}

	rc = vfs_fstat(fd, &st);
	if (rc) {
		vfs_close(fd);
		vmm_cprintf(cdev, "Failed to stat %s\n", path);
		return rc;
	}

	if (!(st.st_mode & S_IFREG)) {
		vfs_close(fd);
		vmm_cprintf(cdev, "Cannot read %s\n", path);
		return VMM_EINVALID;
	}

	len = st.st_size;
	md5_init(&md5c);

	while (len) {
		memset(buf, 0, sizeof(buf));

		buf_rd = (len < VFS_LOAD_BUF_SZ) ? len : VFS_LOAD_BUF_SZ;
		buf_rd = vfs_read(fd, buf, buf_rd);
		if (buf_rd < 1) {
			break;
		}
		md5_update(&md5c, buf, buf_rd);
	}

	md5_final(digest, &md5c);

	vmm_cprintf(cdev, "MD5 Digest: ");
	for (i = 0; i < 16; i++)
		vmm_cprintf(cdev, "%x", digest[i]);
	vmm_cprintf(cdev, "\n");

	rc = vfs_close(fd);
	if (rc) {
		vmm_cprintf(cdev, "Failed to close %s\n", path);
		return rc;
	}

	return VMM_OK;
}
#endif

#if CONFIG_CRYPTO_HASH_SHA256
static int cmd_vfs_sha256(struct vmm_chardev *cdev, const char *path)
{
	int fd, rc, i;
	u32 len;
	size_t buf_rd;
	u8 buf[VFS_LOAD_BUF_SZ];
	struct stat st;
	struct sha256_context sha256c;
	sha256_digest_t digest;

	fd = vfs_open(path, O_RDONLY, 0);
	if (fd < 0) {
		vmm_cprintf(cdev, "Failed to open %s\n", path);
		return fd;
	}

	rc = vfs_fstat(fd, &st);
	if (rc) {
		vfs_close(fd);
		vmm_cprintf(cdev, "Failed to stat %s\n", path);
		return rc;
	}

	if (!(st.st_mode & S_IFREG)) {
		vfs_close(fd);
		vmm_cprintf(cdev, "Cannot read %s\n", path);
		return VMM_EINVALID;
	}

	len = st.st_size;
	sha256_init(&sha256c);

	while (len) {
		memset(buf, 0, sizeof(buf));

		buf_rd = (len < VFS_LOAD_BUF_SZ) ? len : VFS_LOAD_BUF_SZ;
		buf_rd = vfs_read(fd, buf, buf_rd);
		if (buf_rd < 1) {
			break;
		}
		sha256_update(&sha256c, buf, buf_rd);
	}

	sha256_final(digest, &sha256c);

	vmm_cprintf(cdev, "SHA-256 Digest: ");
	for (i = 0; i < SHA256_DIGEST_LEN; i++)
		vmm_cprintf(cdev, "%x", digest[i]);
	vmm_cprintf(cdev, "\n");

	rc = vfs_close(fd);
	if (rc) {
		vmm_cprintf(cdev, "Failed to close %s\n", path);
		return rc;
	}

	return VMM_OK;
}
#endif

static int cmd_vfs_cat(struct vmm_chardev *cdev, const char *path)
{
	int fd, rc;
	u32 i, off, len;
	bool found_non_printable;
	size_t buf_rd;
	char buf[VFS_LOAD_BUF_SZ];
	struct stat st;

	fd = vfs_open(path, O_RDONLY, 0);
	if (fd < 0) {
		vmm_cprintf(cdev, "Failed to open %s\n", path);
		return fd;
	}

	rc = vfs_fstat(fd, &st);
	if (rc) {
		vfs_close(fd);
		vmm_cprintf(cdev, "Failed to stat %s\n", path);
		return rc;
	}

	if (!(st.st_mode & S_IFREG)) {
		vfs_close(fd);
		vmm_cprintf(cdev, "Cannot read %s\n", path);
		return VMM_EINVALID;
	}

	off = 0;
	len = st.st_size;
	while (len) {
		buf_rd = (len < VFS_LOAD_BUF_SZ) ? len : VFS_LOAD_BUF_SZ;
		buf_rd = vfs_read(fd, buf, buf_rd);
		if (buf_rd < 1) {
			break;
		}

		found_non_printable = FALSE;
		for (i = 0; i < buf_rd; i++) {
			if (!vmm_isprintable(buf[i])) {
				found_non_printable = TRUE;
				break;
			}
			vmm_cputc(cdev, buf[i]);
		}
		if (found_non_printable) {
			vmm_cprintf(cdev, "\nFound non-printable char %d "
					  "at offset %d\n", buf[i], (off + i));
			break;
		}

		off += buf_rd;
		len -= buf_rd;
	}

	rc = vfs_close(fd);
	if (rc) {
		vmm_cprintf(cdev, "Failed to close %s\n", path);
		return rc;
	}

	return VMM_OK;
}

static int cmd_vfs_mv(struct vmm_chardev *cdev, 
			const char *old_path, const char *new_path)
{
	int rc;
	struct stat st;

	rc = vfs_stat(old_path, &st);
	if (rc) {
		vmm_cprintf(cdev, "Path %s does not exist.\n", old_path);
		return rc;
	}

	rc = vfs_rename(old_path, new_path);
	if (rc) {
		vmm_cprintf(cdev, "Failed to rename.\n");
		return rc;
	}

	return VMM_OK;
}

static int cmd_vfs_rm(struct vmm_chardev *cdev, const char *path)
{
	int rc;
	struct stat st;

	rc = vfs_stat(path, &st);
	if (rc) {
		vmm_cprintf(cdev, "Path %s does not exist.\n", path);
		return rc;
	}

	if (!(st.st_mode & S_IFREG)) {
		vmm_cprintf(cdev, "Path %s should be regular file.\n", path);
		return VMM_EINVALID;
	}

	return vfs_unlink(path);
}

static int cmd_vfs_mkdir(struct vmm_chardev *cdev, const char *path)
{
	int rc;
	struct stat st;

	rc = vfs_stat(path, &st);
	if (!rc) {
		vmm_cprintf(cdev, "Path %s already exist.\n", path);
		return VMM_EEXIST;
	}

	return vfs_mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);
}

static int cmd_vfs_rmdir(struct vmm_chardev *cdev, const char *path)
{
	int rc;
	struct stat st;

	rc = vfs_stat(path, &st);
	if (rc) {
		vmm_cprintf(cdev, "Path %s does not exist.\n", path);
		return rc;
	}

	if (!(st.st_mode & S_IFDIR)) {
		vmm_cprintf(cdev, "Path %s should be directory.\n", path);
		return VMM_EINVALID;
	}

	return vfs_rmdir(path);
}

static int cmd_vfs_fdt_load(struct vmm_chardev *cdev,
			    const char *devtree_path,
			    const char *devtree_root_name,
			    const char *path,
			    int aliasc, char **aliasv)
{
	int a, fd, rc = VMM_OK;
	char *astr;
	const char *aname, *apath, *aattr, *atype;
	size_t fdt_rd;
	void *fdt_data, *val = NULL;
	u32 val_type, val_len = 0;
	struct stat st;
	struct vmm_devtree_node *root, *anode, *node;
	struct vmm_devtree_node *parent;
	struct fdt_fileinfo fdt;

	parent = vmm_devtree_getnode(devtree_path);
	if (!parent) {
		vmm_cprintf(cdev, "Devtree path %s does not exist.\n",
			    devtree_path);
		rc = VMM_EINVALID;
		goto fail;
	}

	root = vmm_devtree_getchild(parent, devtree_root_name);
	if (root) {
		vmm_cprintf(cdev, "Devtree path %s/%s already exist.\n",
			    devtree_path, devtree_root_name);
		rc = VMM_EINVALID;
		goto fail;
	}

	fd = vfs_open(path, O_RDONLY, 0);
	if (fd < 0) {
		vmm_cprintf(cdev, "Failed to open %s\n", path);
		rc = fd;
		goto fail;
	}

	rc = vfs_fstat(fd, &st);
	if (rc) {
		vmm_cprintf(cdev, "Path %s does not exist.\n", path);
		goto fail_closefd;
	}

	if (!(st.st_mode & S_IFREG)) {
		vmm_cprintf(cdev, "Path %s should be regular file.\n", path);
		rc = VMM_EINVALID;
		goto fail_closefd;
	}

	if (!st.st_size) {
		vmm_cprintf(cdev, "File %s has zero %d bytes.\n", path);
		rc = VMM_EINVALID;
		goto fail_closefd;
	}

	if (st.st_size > VFS_MAX_FDT_SZ) {
		vmm_cprintf(cdev, "File %s has size %d bytes (> %d bytes).\n",
			    path, (long)st.st_size, VFS_MAX_FDT_SZ);
		rc = VMM_EINVALID;
		goto fail_closefd;
	}

	fdt_data = vmm_zalloc(VFS_MAX_FDT_SZ);
	if (!fdt_data) {
		rc = VMM_ENOMEM;
		goto fail_closefd;
	}

	fdt_rd = vfs_read(fd, fdt_data, VFS_MAX_FDT_SZ);
	if (fdt_rd < st.st_size) {
		rc = VMM_EIO;
		goto fail_freedata;
	}

	rc = libfdt_parse_fileinfo((virtual_addr_t)fdt_data, &fdt);
	if (rc) {
		goto fail_freedata;
	}

	root = NULL;
	rc = libfdt_parse_devtree(&fdt, &root, devtree_root_name, parent);
	if (rc) {
		goto fail_freedata;
	}

	anode = vmm_devtree_getchild(root, VMM_DEVTREE_ALIASES_NODE_NAME);

	for (a = 0; a < aliasc; a++) {
		if (!anode) {
			vmm_cprintf(cdev, "Error: %s node not available\n",
				    VMM_DEVTREE_ALIASES_NODE_NAME);
			continue;
		}

		astr = aliasv[a];

		aname = astr;
		while (*astr != '\0' && *astr != ',') {
			astr++;
		}
		if (*astr == ',') {
			*astr = '\0';
			astr++;
		}

		if (*astr == '\0') {
			continue;
		}
		aattr = astr;
		while (*astr != '\0' && *astr != ',') {
			astr++;
		}
		if (*astr == ',') {
			*astr = '\0';
			astr++;
		}

		if (*astr == '\0') {
			continue;
		}
		atype = astr;
		while (*astr != '\0' && *astr != ',') {
			astr++;
		}
		if (*astr == ',') {
			*astr = '\0';
			astr++;
		}

		if (*astr == '\0') {
			continue;
		}

		if (vmm_devtree_read_string(anode, aname, &apath)) {
			vmm_cprintf(cdev, "Error: Failed to read %s attribute "
				    "of %s node\n", aname,
				    VMM_DEVTREE_ALIASES_NODE_NAME);
			continue;
		}

		node = vmm_devtree_getchild(root, apath);
		if (!node) {
			vmm_cprintf(cdev, "Error: %s node not found under "
				    "%s/%s\n", apath, devtree_path,
				    devtree_root_name);
			continue;
		}

		if (!strcmp(atype, "unknown")) {
			val = NULL;
			val_len = 0;
			val_type = VMM_DEVTREE_MAX_ATTRTYPE;
		} else if (!strcmp(atype, "string")) {
			val_len = strlen(astr) + 1;
			val = vmm_zalloc(val_len);
			if (!val) {
				vmm_cprintf(cdev, "Error: vmm_zalloc(%d) "
					    "failed\n", val_len);
				continue;
			}
			strcpy(val, astr);
			val_type = VMM_DEVTREE_ATTRTYPE_STRING;
		} else if (!strcmp(atype, "bytes")) {
			val_len = 1;
			val = vmm_zalloc(val_len);
			if (!val) {
				vmm_cprintf(cdev, "Error: vmm_zalloc(%d) "
					    "failed\n", val_len);
				continue;
			}
			*((u8 *)val) = strtoul(astr, NULL, 0);
			val_type = VMM_DEVTREE_ATTRTYPE_BYTEARRAY;
		} else if (!strcmp(atype, "uint32")) {
			val_len = 4;
			val = vmm_zalloc(val_len);
			if (!val) {
				vmm_cprintf(cdev, "Error: vmm_zalloc(%d) "
					    "failed\n", val_len);
				continue;
			}
			*((u32 *)val) = strtoul(astr, NULL, 0);
			val_type = VMM_DEVTREE_ATTRTYPE_UINT32;
		} else if (!strcmp(atype, "uint64")) {
			val_len = 8;
			val = vmm_zalloc(val_len);
			if (!val) {
				vmm_cprintf(cdev, "Error: vmm_zalloc(%d) "
					    "failed\n", val_len);
				continue;
			}
			*((u64 *)val) = strtoull(astr, NULL, 0);
			val_type = VMM_DEVTREE_ATTRTYPE_UINT64;
		} else if (!strcmp(atype, "physaddr")) {
			val_len = sizeof(physical_addr_t);
			val = vmm_zalloc(val_len);
			if (!val) {
				vmm_cprintf(cdev, "Error: vmm_zalloc(%d) "
					    "failed\n", val_len);
				continue;
			}
			*((physical_addr_t *)val) = strtoull(astr, NULL, 0);
			val_type = VMM_DEVTREE_ATTRTYPE_PHYSADDR;
		} else if (!strcmp(atype, "physsize")) {
			val_len = sizeof(physical_size_t);
			val = vmm_zalloc(val_len);
			if (!val) {
				vmm_cprintf(cdev, "Error: vmm_zalloc(%d) "
					    "failed\n", val_len);
				continue;
			}
			*((physical_size_t *)val) = strtoull(astr, NULL, 0);
			val_type = VMM_DEVTREE_ATTRTYPE_PHYSSIZE;
		} else if (!strcmp(atype, "virtaddr")) {
			val_len = sizeof(virtual_addr_t);
			val = vmm_zalloc(val_len);
			if (!val) {
				vmm_cprintf(cdev, "Error: vmm_zalloc(%d) "
					    "failed\n", val_len);
				continue;
			}
			*((virtual_addr_t *)val) = strtoull(astr, NULL, 0);
			val_type = VMM_DEVTREE_ATTRTYPE_VIRTADDR;
		} else if (!strcmp(atype, "virtsize")) {
			val_len = sizeof(virtual_size_t);
			val = vmm_zalloc(val_len);
			if (!val) {
				vmm_cprintf(cdev, "Error: vmm_zalloc(%d) "
					    "failed\n", val_len);
				continue;
			}
			*((virtual_size_t *)val) = strtoull(astr, NULL, 0);
			val_type = VMM_DEVTREE_ATTRTYPE_VIRTSIZE;
		} else {
			vmm_cprintf(cdev, "Error: Invalid attribute type %s\n",
				    atype);
			continue;
		}

		if (val && (val_len > 0)) {
			vmm_devtree_setattr(node, aattr, val,
					    val_type, val_len, FALSE);
			vmm_free(val);
		}
	}

fail_freedata:
	vmm_free(fdt_data);
fail_closefd:
	vfs_close(fd);
fail:
	return rc;
}

static int cmd_vfs_load(struct vmm_chardev *cdev,
			struct vmm_guest *guest,
			physical_addr_t pa, 
			const char *path, u32 off, u32 len)
{
	int fd, rc;
	loff_t rd_off;
	physical_addr_t wr_pa;
	size_t buf_wr, buf_rd, buf_count, wr_count;
	char buf[VFS_LOAD_BUF_SZ];
	struct stat st;

	fd = vfs_open(path, O_RDONLY, 0);
	if (fd < 0) {
		vmm_cprintf(cdev, "Failed to open %s\n", path);
		return fd;
	}

	rc = vfs_fstat(fd, &st);
	if (rc) {
		vfs_close(fd);
		vmm_cprintf(cdev, "Failed to stat %s\n", path);
		return rc;
	}

	if (!(st.st_mode & S_IFREG)) {
		vfs_close(fd);
		vmm_cprintf(cdev, "Cannot read %s\n", path);
		return VMM_EINVALID;
	}

	if (off >= st.st_size) {
		vfs_close(fd);
		vmm_cprintf(cdev, "Offset greater than file size\n");
		return VMM_EINVALID;
	}

	len = ((st.st_size - off) < len) ? (st.st_size - off) : len;

	rd_off = 0;
	wr_count = 0;
	wr_pa = pa;
	while (len) {
		buf_rd = (len < VFS_LOAD_BUF_SZ) ? len : VFS_LOAD_BUF_SZ;
		buf_count = vfs_read(fd, buf, buf_rd);
		if (buf_count < 1) {
			vmm_cprintf(cdev, "Failed to read "
					  "%d bytes @ 0x%llx from %s\n",
					   buf_rd, (u64)rd_off, path);
			break;
		}
		rd_off += buf_count;
		if (guest) {
			buf_wr = vmm_guest_memory_write(guest, wr_pa,
							buf, buf_count, FALSE);
		} else {
			buf_wr = vmm_host_memory_write(wr_pa,
						       buf, buf_count, FALSE);
		}
		if (buf_wr != buf_count) {
			vmm_cprintf(cdev, "Failed to write "
					  "%d bytes @ 0x%llx (%s)\n",
					  buf_count, (u64)wr_pa,
					  (guest) ? (guest->name) : "host");
			break;
		}
		len -= buf_wr;
		wr_count += buf_wr;
		wr_pa += buf_wr;
	}

	vmm_cprintf(cdev, "%s: Loaded 0x%llx with %d bytes\n",
			  (guest) ? (guest->name) : "host",
			  (u64)pa, wr_count);

	rc = vfs_close(fd);
	if (rc) {
		vmm_cprintf(cdev, "Failed to close %s\n", path);
		return rc;
	}

	return VMM_OK;
}

static int cmd_vfs_load_list(struct vmm_chardev *cdev,
			     struct vmm_guest *guest,
			     const char *path)
{
	loff_t rd_off;
	struct stat st;
	int fd, rc, pos;
	physical_addr_t pa;
	char c, *addr, *file, buf[VFS_LOAD_BUF_SZ+1];

	fd = vfs_open(path, O_RDONLY, 0);
	if (fd < 0) {
		vmm_cprintf(cdev, "Failed to open %s\n", path);
		return fd;
	}

	rc = vfs_fstat(fd, &st);
	if (rc) {
		vfs_close(fd);
		vmm_cprintf(cdev, "Failed to stat %s\n", path);
		return rc;
	}

	if (!(st.st_mode & S_IFREG)) {
		vfs_close(fd);
		vmm_cprintf(cdev, "Cannot read %s\n", path);
		return VMM_EINVALID;
	}

	rd_off = 0;
	pos = 0;
	while (vfs_read(fd, &c, 1) == 1) {
		if (pos == VFS_LOAD_BUF_SZ) {
			vmm_cprintf(cdev, "Line exceeds limit of "
				    "%d chars at offset 0x%llx\n",
				    VFS_LOAD_BUF_SZ, (u64)rd_off);
			break;
		}
		if (c == '\n' || c == '\r') {
			buf[pos] = '\0';
			while ((pos > 0) &&
			       ((buf[pos - 1] == ' ') ||
				(buf[pos - 1] == '\t'))) {
				pos--;
				buf[pos] = '\0';
			}

			addr = &buf[0];
			while ((*addr == ' ') || (*addr == '\t')) {
				addr++;
			}
			if (*addr == '\0') {
				goto skip_line;
			}

			file = addr;
			while ((*file != ' ') &&
			       (*file != '\t') &&
			       (*file != '\0')) {
				file++;
			}
			if (*file == '\0') {
				goto skip_line;
			}

			while ((*file == ' ') || (*file == '\t')) {
				*file = '\0';
				file++;
			}
			if (*file == '\0') {
				goto skip_line;
			}

			pa = (physical_addr_t)strtoull(addr, NULL, 0);
			vmm_cprintf(cdev, "%s: Loading 0x%llx with file %s\n",
				    (guest) ? (guest->name) : "host",
				    (u64)pa, file);
			rc = cmd_vfs_load(cdev, guest, pa,
					  file, 0, 0xFFFFFFFF);
			if (rc) {
				vmm_cprintf(cdev, "error %d\n", rc);
				break;
			}
skip_line:
			pos = 0;
		} else if (vmm_isprintable(c)) {
			buf[pos] = c;
			pos++;
		} else {
			vmm_cprintf(cdev, "Non-printable char at "
				    "offset 0x%llx\n", (u64)rd_off);
			break;
		}
		rd_off++;
	}

	rc = vfs_close(fd);
	if (rc) {
		vmm_cprintf(cdev, "Failed to close %s\n", path);
		return rc;
	}

	return VMM_OK;
}

static int cmd_vfs_exec(struct vmm_chardev *cdev, int argc, char **argv)
{
	u32 off, len;
	physical_addr_t pa;
	struct vmm_guest *guest;
	if (argc < 2) {
		cmd_vfs_usage(cdev);
		return VMM_EFAIL;
	}
	if (strcmp(argv[1], "help") == 0) {
		cmd_vfs_usage(cdev);
		return VMM_OK;
	} else if ((strcmp(argv[1], "fslist") == 0) && (argc == 2)) {
		return cmd_vfs_fslist(cdev);
	} else if ((strcmp(argv[1], "mplist") == 0) && (argc == 2)) {
		return cmd_vfs_mplist(cdev);
	} else if ((strcmp(argv[1], "mount") == 0) && (argc == 4)) {
		return cmd_vfs_mount(cdev, argv[2], argv[3]);
	} else if ((strcmp(argv[1], "umount") == 0) && (argc == 3)) {
		return cmd_vfs_umount(cdev, argv[2]);
	} else if ((strcmp(argv[1], "ls") == 0) && (argc == 3)) {
		return cmd_vfs_ls(cdev, argv[2]);
	} else if ((strcmp(argv[1], "run") == 0) && (argc == 3)) {
		return cmd_vfs_run(cdev, argv[2]);
#if CONFIG_CRYPTO_HASH_MD5
	} else if ((strcmp(argv[1], "md5") == 0) && (argc == 3)) {
		return cmd_vfs_md5(cdev, argv[2]);
#endif
#if CONFIG_CRYPTO_HASH_SHA256
	} else if ((strcmp(argv[1], "sha256") == 0) && (argc == 3)) {
		return cmd_vfs_sha256(cdev, argv[2]);
#endif
	} else if ((strcmp(argv[1], "cat") == 0) && (argc == 3)) {
		return cmd_vfs_cat(cdev, argv[2]);
	} else if ((strcmp(argv[1], "mv") == 0) && (argc == 4)) {
		return cmd_vfs_mv(cdev, argv[2], argv[3]);
	} else if ((strcmp(argv[1], "rm") == 0) && (argc == 3)) {
		return cmd_vfs_rm(cdev, argv[2]);
	} else if ((strcmp(argv[1], "mkdir") == 0) && (argc == 3)) {
		return cmd_vfs_mkdir(cdev, argv[2]);
	} else if ((strcmp(argv[1], "rmdir") == 0) && (argc == 3)) {
		return cmd_vfs_rmdir(cdev, argv[2]);
	} else if ((strcmp(argv[1], "fdt_load") == 0) && (argc >= 5)) {
		return cmd_vfs_fdt_load(cdev, argv[2], argv[3], argv[4],
					argc-5, (argc-5) ? &argv[5] : NULL);
	} else if ((strcmp(argv[1], "host_load") == 0) && (argc > 3)) {
		pa = (physical_addr_t)strtoull(argv[2], NULL, 0);
		off = (argc > 4) ? strtoul(argv[4], NULL, 0) : 0;
		len = (argc > 5) ? strtoul(argv[5], NULL, 0) : 0xFFFFFFFF;
		return cmd_vfs_load(cdev, NULL, pa, argv[3], off, len);
	} else if ((strcmp(argv[1], "host_load_list") == 0) && (argc == 3)) {
		return cmd_vfs_load_list(cdev, NULL, argv[2]);
	} else if ((strcmp(argv[1], "guest_load") == 0) && (argc > 4)) {
		guest = vmm_manager_guest_find(argv[2]);
		if (!guest) {
			vmm_cprintf(cdev, "Failed to find guest %s\n",
				    argv[2]);
			return VMM_ENOTAVAIL;
		}
		pa = (physical_addr_t)strtoull(argv[3], NULL, 0);
		off = (argc > 5) ? strtoul(argv[5], NULL, 0) : 0;
		len = (argc > 6) ? strtoul(argv[6], NULL, 0) : 0xFFFFFFFF;
		return cmd_vfs_load(cdev, guest, pa, argv[4], off, len);
	} else if ((strcmp(argv[1], "guest_load_list") == 0) && (argc == 4)) {
		guest = vmm_manager_guest_find(argv[2]);
		if (!guest) {
			vmm_cprintf(cdev, "Failed to find guest %s\n",
				    argv[2]);
			return VMM_ENOTAVAIL;
		}
		return cmd_vfs_load_list(cdev, guest, argv[3]);
	}
	cmd_vfs_usage(cdev);
	return VMM_EFAIL;
}

static struct vmm_cmd cmd_vfs = {
	.name = "vfs",
	.desc = "vfs related commands",
	.usage = cmd_vfs_usage,
	.exec = cmd_vfs_exec,
};

static int __init cmd_vfs_init(void)
{
	return vmm_cmdmgr_register_cmd(&cmd_vfs);
}

static void __exit cmd_vfs_exit(void)
{
	vmm_cmdmgr_unregister_cmd(&cmd_vfs);
}

VMM_DECLARE_MODULE(MODULE_DESC, 
			MODULE_AUTHOR, 
			MODULE_LICENSE, 
			MODULE_IPRIORITY, 
			MODULE_INIT, 
			MODULE_EXIT);