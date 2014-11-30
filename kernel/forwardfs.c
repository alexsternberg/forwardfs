#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <uapi/linux/stat.h>
// #include "forwardfs_connector.h"
#include "forwardfs_netlink.h"
#include "forwardfs.h"

#define FORWARDFS_MAGIC 0x416F5

#define OKAY "ok"

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

struct inode* ramfs_get_inode ( struct super_block *sb, const struct inode *dir, umode_t mode, dev_t dev );

static int
ramfs_mknod ( struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev )
{
        struct inode * inode = ramfs_get_inode ( dir->i_sb, dir, mode, dev );
        int error = -ENOSPC;

        if ( inode ) {
                d_instantiate ( dentry, inode );
//                 dget ( dentry ); /* Extra count - pin the dentry in core */
                error = 0;
                dir->i_mtime = dir->i_ctime = CURRENT_TIME;
        }
        return error;
}

static int ramfs_mkdir ( struct inode * dir, struct dentry * dentry, umode_t mode )
{
        int retval = ramfs_mknod ( dir, dentry, mode | S_IFDIR, 0 );
        if ( !retval )
                inc_nlink ( dir );
        return retval;
}

static int ramfs_create ( struct inode *dir, struct dentry *dentry, umode_t mode, bool excl )
{
        return ramfs_mknod ( dir, dentry, mode | S_IFREG, 0 );
}

static int ramfs_symlink ( struct inode * dir, struct dentry *dentry, const char * symname )
{
        struct inode *inode;
        int error = -ENOSPC;

        inode = ramfs_get_inode ( dir->i_sb, dir, S_IFLNK|S_IRWXUGO, 0 );
        if ( inode ) {
                int l = strlen ( symname ) +1;
//                 error = page_symlink ( inode, symname, l );
                if ( !error ) {
                        d_instantiate ( dentry, inode );
//                         dget ( dentry );
                        dir->i_mtime = dir->i_ctime = CURRENT_TIME;
                } else
                        iput ( inode );
        }
        return error;
}

struct inode_operations forwardfs_dir_inode_operations = {
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

struct inode_operations ramfs_file_inode_operations = {};

struct file_operations ramfs_file_operations = {};

struct inode* ramfs_get_inode ( struct super_block *sb, const struct inode *dir, umode_t mode, dev_t dev )
{
        printk(KERN_INFO "called %s\n", __func__);
        struct sk_buff *skb;
        struct inode * inode = new_inode ( sb );

        if ( inode ) {
                inode->i_ino = get_next_ino();
                inode_init_owner ( inode, dir, mode );
//                 inode->i_mapping->a_ops = &ramfs_aops;
//                 inode->i_mapping->backing_dev_info = &ramfs_backing_dev_info;
//                 mapping_set_gfp_mask ( inode->i_mapping, GFP_HIGHUSER );
//                 mapping_set_unevictable ( inode->i_mapping );
                inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
//                 printk(KERN_INFO "got io mode %i, posibilities: %i, %i, %i\n ", mode,S_IFREG,S_IFDIR,S_IFLNK);
                switch ( mode & S_IFMT ) {
                default:
                        printk ( KERN_INFO "getting specl inode\n" );
                        init_special_inode ( inode, mode, dev );
                        break;
                case S_IFREG:
                        printk ( KERN_INFO "getting file inode\n" );
                        inode->i_op = &ramfs_file_inode_operations;
                        inode->i_fop = &ramfs_file_operations;
                        break;
                case S_IFDIR:
                        printk ( KERN_INFO "getting dir inode\n" );
                        char *msg = "get dir";
//                         forward_snd_msg(msg, strlen(msg));
                        inode->i_op = &forwardfs_dir_inode_operations;
                        inode->i_fop = &simple_dir_operations;
                        mutex_lock(&forward_nl_mutex); //get lock for socket
                        forward_snd_msg(msg, strlen(msg));
                        netlink_rcv_skb(skb, &forward_rcv_msg);
                        mutex_unlock(&forward_nl_mutex);
                        /* directory inodes start off with i_nlink == 2 (for "." entry) */
                        inc_nlink ( inode );
                        break;
                case S_IFLNK:
                        printk ( KERN_INFO "getting link inode\n" );
                        inode->i_op = &page_symlink_inode_operations;
                        break;
                }
        }
        return inode;
}

int ramfs_fill_super ( struct super_block *sb, void *data, int silent )
{
        struct forward_fs_info *fsi;
        struct inode *inode;
        int err = 0;

        save_mount_options ( sb, data );

        printk ( KERN_INFO "allocating memory\n" );
        fsi = kzalloc ( sizeof ( struct forward_fs_info ), GFP_KERNEL );
        sb->s_fs_info = fsi;
        if ( !fsi ) {
                printk ( KERN_ERR "could not allocate for fs_info\n" );
                return -ENOMEM;
        }

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
        if ( !sb->s_root ) {
                printk ( KERN_ERR "could not get root dentry\n" );
                return -ENOMEM;
        }

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
        printk ( KERN_INFO "ForwardFS Init\n" );
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
