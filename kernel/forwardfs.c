#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include "forwardfs_connector.h"
#include "forwardfs_netlink.h"
#include "forwardfs.h"

#define FORWARDFS_MAGIC 0x416F5

/**
 * file_system_type for forwardfs
 */
// static struct file_system_type forwardfs_fs_type;
struct ramfs_mount_opts {
        umode_t mode;
};

struct forward_fs_info {
        struct ramfs_mount_opts mount_opts;
};

static const struct super_operations ramfs_ops = {
        .statfs         = simple_statfs,
        .drop_inode     = generic_delete_inode,
        .show_options   = generic_show_options,
};

struct inode* ramfs_get_inode(struct super_block *sb, const struct inode *dir, umode_t mode, dev_t dev){
        return NULL;
}

int ramfs_fill_super ( struct super_block *sb, void *data, int silent )
{
        struct forward_fs_info *fsi;
        struct inode *inode;
        int err;

        save_mount_options ( sb, data );

        fsi = kzalloc ( sizeof ( struct forward_fs_info ), GFP_KERNEL );
        sb->s_fs_info = fsi;
        if ( !fsi )
                return -ENOMEM;

        //err = ramfs_parse_options ( data, &fsi->mount_opts );
        if ( err )
                return err;

        sb->s_maxbytes          = MAX_LFS_FILESIZE;
        sb->s_blocksize         = PAGE_CACHE_SIZE;
        sb->s_blocksize_bits    = PAGE_CACHE_SHIFT;
        sb->s_magic             = FORWARDFS_MAGIC;
        sb->s_op                = &ramfs_ops;
        sb->s_time_gran         = 1;

        inode = ramfs_get_inode ( sb, NULL, S_IFDIR | fsi->mount_opts.mode, 0 );
        sb->s_root = d_make_root ( inode );
        if ( !sb->s_root )
                return -ENOMEM;

        return 0;
}

struct dentry *forward_mount ( struct file_system_type *fs_type,
                                 int flags, const char *dev_name, void *data )
{
        return mount_nodev ( fs_type, flags, data, ramfs_fill_super );
}

static void forward_kill_sb ( struct super_block *sb )
{
        kfree ( sb->s_fs_info );
        kill_litter_super ( sb );
}

static struct file_system_type forwardfs_fs_type = {
        .name           = "forwardfs",
        .mount          = forward_mount,
        .kill_sb        = forward_kill_sb,
        .fs_flags       = FS_USERNS_MOUNT,
};

/*
static const struct inode_operations forwardfs_dir_inode_operations = {
         .create         = ramfs_create,
         .lookup         = simple_lookup,
         .link           = simple_link,
         .unlink         = simple_unlink,
         .symlink        = ramfs_symlink,
         .mkdir          = ramfs_mkdir,
         .rmdir          = simple_rmdir,
         .mknod          = ramfs_mknod,
         .rename         = simple_rename,
};
*/





/**
 * Module init
 */
static int
forward_init ( void )
{
        pid = 0;
        printk ( KERN_INFO "ForwardFS Init" );
        nl_sk = netlink_kernel_create ( &init_net, NETLINK_USERSOCK, &cfg ); //initialize netlink socket
        if ( !nl_sk ) {
                printk ( KERN_ERR "%s: receive handler registration failed\n", __func__ );
                return -EXDEV;
        }
        return register_filesystem ( &forwardfs_fs_type ); //register the filesystem
}

/**
 * Module exit
 */
static void
forward_exit ( void )
{
        if ( nl_sk ) {
                netlink_kernel_release ( nl_sk );
        }
        return unregister_filesystem ( &forwardfs_fs_type );
}

module_init ( forward_init );
module_exit ( forward_exit );
