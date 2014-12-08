#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <uapi/linux/stat.h>
#include "forwardfs_netlink.h"
#include "forwardfs.h"

#define FORWARDFS_MAGIC 0x416F5

/**
 * file_system_type for forwardfs
 */
// static struct file_system_type forwardfs_fs_type;
struct forwardfs_mount_opts {
        umode_t mode;
};

struct forward_fs_info {
        struct forwardfs_mount_opts mount_opts;
};

static const struct super_operations forwardfs_ops = {
        .statfs         = simple_statfs,
        .drop_inode     = generic_delete_inode,
        .show_options   = generic_show_options,
};

struct inode* forwardfs_get_inode ( struct super_block *sb, const struct inode *dir, umode_t mode, dev_t dev );

static int
forwardfs_mknod ( struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev )
{
        struct inode * inode = forwardfs_get_inode ( dir->i_sb, dir, mode, dev );
        int error = -ENOSPC;

        if ( inode ) {
                d_instantiate ( dentry, inode );
//                 dget ( dentry ); /* Extra count - pin the dentry in core */
                error = 0;
                dir->i_mtime = dir->i_ctime = CURRENT_TIME;
        }
        return error;
}

static int forwardfs_mkdir ( struct inode * dir, struct dentry * dentry, umode_t mode )
{
        int retval = forwardfs_mknod ( dir, dentry, mode | S_IFDIR, 0 );
        if ( !retval )
                inc_nlink ( dir );
        return retval;
}

static int forwardfs_create ( struct inode *dir, struct dentry *dentry, umode_t mode, bool excl )
{
        return forwardfs_mknod ( dir, dentry, mode | S_IFREG, 0 );
}

static int forwardfs_symlink ( struct inode * dir, struct dentry *dentry, const char * symname )
{
        struct inode *inode;
        int error = -ENOSPC;

        inode = forwardfs_get_inode ( dir->i_sb, dir, S_IFLNK|S_IRWXUGO, 0 );
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
        .create         = forwardfs_create,
        .lookup         = simple_lookup,
        .link           = simple_link,
        .unlink         = simple_unlink,
        .symlink        = forwardfs_symlink,
        .mkdir          = forwardfs_mkdir,
        .rmdir          = simple_rmdir,
        .mknod          = forwardfs_mknod,
        .rename         = simple_rename,
        
};

static ssize_t forward_file_open(struct inode *inod, struct file* filp){
        if(!nl_sk) return -EXDEV; //must have a socket
        
        mutex_lock(&forward_nl_mutex);//lock socket global
        mutex_lock(&forward_nl_recv_mutex); //lock recv buffer
        
        //send open
        
        struct payload p;
        p.data = kmalloc(sizeof(inod->i_ino),0);
        p.length = sizeof(inod->i_ino);
        memcpy(p.data, inod->i_ino, sizeof(inod->i_ino));
        forward_send_simple(FFS_OPEN, &p);
        
        
        mutex_lock(&forward_nl_recv_mutex); //block until buffer filled
        
        
        
        
        filp->private_data = (void*) inod->i_ino;
        inod->i_size = payload.data;
        
        
        
        kfree(payload.data); payload.data = NULL; payload.length = 0;
        
        mutex_unlock(&forward_nl_recv_mutex);
        mutex_unlock(&forward_nl_mutex); //release socket
        return 0; 
        
}

static ssize_t forward_file_read(struct file *filp, char __user *buf, size_t count, loff_t *offset){
        if(!nl_sk) return -EXDEV; //must have a socket
        return; 
        
}

static ssize_t forward_file_write(struct file *filp, char __user *buf, size_t count, loff_t *offset){
        if(!nl_sk) return -EXDEV; //must have a socket
        return; 
        
}

struct inode_operations forwardfs_file_inode_operations;

struct file_operations forwardfs_file_operations = {
        .open           = forward_file_open,
        .read           = forward_file_read,
        .write          = forward_file_write,
};

struct inode* forwardfs_get_inode ( struct super_block *sb, const struct inode *dir, umode_t mode, dev_t dev )
{
        printk(KERN_INFO "called %s\n", __func__);
        struct inode * inode = new_inode ( sb );

        if ( inode ) {
                inode->i_ino = get_next_ino();
                inode_init_owner ( inode, dir, mode );
//                 inode->i_mapping->a_ops = &forwardfs_aops;
//                 inode->i_mapping->backing_dev_info = &forwardfs_backing_dev_info;
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
                        inode->i_op = &forwardfs_file_inode_operations;
                        inode->i_fop = &forwardfs_file_operations;
                        break;
                case S_IFDIR:
                        printk ( KERN_INFO "getting dir inode\n" );
                        char *msg = "get dir";
//                         forward_snd_msg(msg, strlen(msg));
                        inode->i_op = &forwardfs_dir_inode_operations;
                        inode->i_fop = &simple_dir_operations;
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

int forwardfs_fill_super ( struct super_block *sb, void *data, int silent )
{
        struct forward_fs_info *fsi;
        struct inode *inode;

        sb->s_magic             = FORWARDFS_MAGIC;
        sb->s_op                = &forwardfs_ops;
        sb->s_time_gran         = 1;

        inode = forwardfs_get_inode ( sb, NULL, S_IFDIR | fsi->mount_opts.mode, 0 );
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
        return mount_nodev ( fs_type, flags, data, forwardfs_fill_super );
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
