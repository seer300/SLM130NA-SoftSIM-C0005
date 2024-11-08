#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "fs_al.h"
#include "lfs_util.h"
#include "xy_flash.h"
#include "xy_memmap.h"
#include "xy_utils.h"
#include "xy_fs_user_config.h"

#define LFS_BLOCK_DEVICE_READ_SIZE      (16)
#define LFS_BLOCK_DEVICE_PROG_SIZE      (16)
#define LFS_BLOCK_DEVICE_CACHE_SIZE     (512)
#define LFS_BLOCK_DEVICE_ERASE_SIZE     (4096)
#define LFS_BLOCK_DEVICE_LOOK_AHEAD     (8)
#define LFS_BLOCK_DEVICE_BASE     		(FS_FLASH_BASE)
#define LFS_BLOCK_DEVICE_TOTAL_SIZE     (FS_FLASH_LENGTH)



static lfs_t *lfs1 = NULL;
static lfs_t *lfs2 = NULL;

#ifdef LFS_THREADSAFE
static osMutexId_t lfs_mutex = NULL;
#endif
static int lfs_flash_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size);

static int lfs_flash_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size);

static int lfs_flash_erase(const struct lfs_config *cfg, lfs_block_t block);

static int lfs_flash_sync(const struct lfs_config *cfg);
#ifdef LFS_THREADSAFE
static int lfs_lock(const struct lfs_config *cfg);
static int lfs_unlock(const struct lfs_config *cfg);
#endif

// configuration of the filesystem is provided by this struct

struct lfs_config *lfs_cfg1 = NULL;
struct lfs_config *lfs_cfg2 = NULL;


static int lfs_flash_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
    LFS_ASSERT(off  % cfg->read_size == 0);
    LFS_ASSERT(size % cfg->read_size == 0);
    LFS_ASSERT(block < cfg->block_count);

    if(cfg == lfs_cfg1)
    {

        // if(block >= (FS_FLASH_LENGTH/ cfg->block_size))
        // {
        //     block += (FS_FLASH_BASE_2 - (FS_FLASH_BASE +FS_FLASH_LENGTH))/ cfg->block_size;
        // }
        xy_Flash_Read(LFS_BLOCK_DEVICE_BASE + block * cfg->block_size + off, buffer, size);

    }
    else 
    {
        xy_Flash_Read(USER_LFS_FLASH_BASE + block * cfg->block_size + off, buffer, size);

    }


    return LFS_ERR_OK;
}

static int lfs_flash_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{

    LFS_ASSERT(off  % cfg->prog_size == 0);
    LFS_ASSERT(size % cfg->prog_size == 0);
    LFS_ASSERT(block < cfg->block_count);

    int ret;


    if(cfg == lfs_cfg1)
    {

        xy_Flash_Write_No_Erase(LFS_BLOCK_DEVICE_BASE + block * cfg->block_size + off, buffer, size);

    }
    else 
    {
        xy_Flash_Write_No_Erase(USER_LFS_FLASH_BASE  + block * cfg->block_size + off, buffer, size);
		
		
	
    }
   return LFS_ERR_OK;
}

static int lfs_flash_erase(const struct lfs_config *cfg, lfs_block_t block)
{
    LFS_ASSERT(block < cfg->block_count);

    int ret;

    if(cfg == lfs_cfg1)
    {
       xy_Flash_Erase(LFS_BLOCK_DEVICE_BASE + block * cfg->block_size, LFS_BLOCK_DEVICE_ERASE_SIZE);
    }
    else 
    {
       xy_Flash_Erase(USER_LFS_FLASH_BASE  + block * cfg->block_size, LFS_BLOCK_DEVICE_ERASE_SIZE);
    }

    return LFS_ERR_OK;
}
/* 如果底层写flash有cache需要用这个函数将cache中的数据同步到flash,
   xy1100写flash没有cache, 这里直接返回0
*/
static int lfs_flash_sync(const struct lfs_config *cfg)
{
	(void)cfg;
    return 0;
}

void fs_config_init(uint32_t disktype)
{

    if(disktype == FS_DEFAULT)
    {
        if(lfs1 == NULL)
        {
            lfs1 = lfs_malloc(sizeof(lfs_t));
            memset(lfs1,0,sizeof(lfs_t));
            lfs_cfg1 = lfs_malloc(sizeof(struct lfs_config));
            memset(lfs_cfg1,0,sizeof(struct lfs_config));
            lfs_cfg1->context = NULL;
            lfs_cfg1->read = lfs_flash_read;
            lfs_cfg1->prog = lfs_flash_prog;
            lfs_cfg1->sync = lfs_flash_sync;
            lfs_cfg1->erase = lfs_flash_erase;
    #ifdef LFS_THREADSAFE
            lfs_cfg1->lock = lfs_lock;
            lfs_cfg1->unlock = lfs_unlock;
    #endif
            // block device configuration
            lfs_cfg1->read_size = LFS_BLOCK_DEVICE_READ_SIZE;
            //写flash的时候按prog_size对齐去写，这个值设置小些可以提高flash的利用率特别是小文件（小于cache_size）较多时
            lfs_cfg1->prog_size = LFS_BLOCK_DEVICE_PROG_SIZE;
            //一次性擦除的大小也是分配块的大小
            lfs_cfg1->block_size = LFS_BLOCK_DEVICE_ERASE_SIZE;
            lfs_cfg1->block_count = LFS_BLOCK_DEVICE_TOTAL_SIZE/ LFS_BLOCK_DEVICE_ERASE_SIZE;
            //存放目录的block前四个字节记录revision，当revision大于block_cycles换新块
            lfs_cfg1->block_cycles = 200;
            //小于等于cache_size的文件直接存放到entry中，当小文件较多时可以适当增大这个值来提高flash的利用率
            //文件系统挂载后会为读写各分配一个cache_size大小的buffer，所以这个值增大需要更多的ram
            lfs_cfg1->cache_size = LFS_BLOCK_DEVICE_CACHE_SIZE;
            //配器bitmap的大小lookahead_size*8，滑窗大小
            lfs_cfg1->lookahead_size = LFS_BLOCK_DEVICE_LOOK_AHEAD; //fixed-size bitmap for allocator
            //文件名最大长度
            lfs_cfg1->name_max = LFS_NAME_MAX;
            //文件最大大小
            lfs_cfg1->file_max = LFS_FILE_MAX;
            lfs_cfg1->attr_max = 0;

		fs_mount(lfs1, lfs_cfg1);

        }
     }
	else{
        if(USER_LFS_FLASH_LEN == 0)
        {
            return LFS_ERR_INVAL;
        }
        else
        {

	    if(lfs2 == NULL)
	    {
	        lfs2 = lfs_malloc(sizeof(lfs_t));
	        memset(lfs2,0,sizeof(lfs_t));
	        lfs_cfg2 = lfs_malloc(sizeof(struct lfs_config));
	        memset(lfs_cfg2,0,sizeof(struct lfs_config));
	        lfs_cfg2->context = NULL;
	        lfs_cfg2->read = lfs_flash_read;
	        lfs_cfg2->prog = lfs_flash_prog;
	        lfs_cfg2->sync = lfs_flash_sync;
	        lfs_cfg2->erase = lfs_flash_erase;
#ifdef LFS_THREADSAFE
		lfs_cfg2->lock = lfs_lock;
		lfs_cfg2->unlock = lfs_unlock;
#endif
		// block device configuration
		lfs_cfg2->read_size = USER_LFS_BLOCK_DEVICE_READ_SIZE;
		//写flash的时候按prog_size对齐去写，这个值设置小些可以提高flash的利用率特别是小文件（小于cache_size）较多时
		lfs_cfg2->prog_size = USER_LFS_BLOCK_DEVICE_PROG_SIZE;
		//一次性擦除的大小也是分配块的大小
		lfs_cfg2->block_size = USER_LFS_BLOCK_DEVICE_ERASE_SIZE;
		lfs_cfg2->block_count = USER_LFS_FLASH_LEN 
						/ USER_LFS_BLOCK_DEVICE_ERASE_SIZE;
		//存放目录的block前四个字节记录revision，当revision大于block
		lfs_cfg2->block_cycles = USER_LFS_BLOCK_REVISION_NUM;
		//小于等于cache_size的文件直接存放到entry中，当小文件较多时可以适当增大这个值来提高flash的利用率
		//文件系统挂载后会为读写各分配一个cache_size大小的buffer，所以这个值增大需要更多的ram
		lfs_cfg2->cache_size = USER_LFS_BLOCK_DEVICE_CACHE_SIZE;
		//块分配器bitmap的大小lookahead_size*8
		lfs_cfg2->lookahead_size = USER_LFS_BLOCK_DEVICE_LOOK_AHEAD; //fixed-size bitmap for allocator
		lfs_cfg2->lookahead_buffer = NULL;
		//文件名最大长度
		lfs_cfg2->name_max = LFS_NAME_MAX;
		//文件最大大小
		lfs_cfg2->file_max = LFS_FILE_MAX;
		lfs_cfg2->attr_max = 0;

		fs_mount(lfs2, lfs_cfg2);
	}

        }
	}
}



void  fs_lock_init(void)
{
	osMutexAttr_t mutex_attr = {0};
	mutex_attr.attr_bits = osMutexRecursive;

	lfs_mutex = osMutexNew(&mutex_attr);

}


#ifdef LFS_THREADSAFE
static int lfs_lock(const struct lfs_config *cfg)
{
	(void)cfg;

	xy_mutex_acquire(lfs_mutex, osWaitForever);

	return 0;
}

static int lfs_unlock(const struct lfs_config *cfg)
{
	(void)cfg;

	xy_mutex_release(lfs_mutex);
	return 0;
}
#endif

int fs_mount(lfs_t *lfs, struct lfs_config *lfs_cfg)
{
    int err;

	//已经挂载过
	if(lfs->cfg != NULL)
	{
		return LFS_ERR_OK;
	}
	
	err = lfs_mount(lfs, lfs_cfg);

    //首次启动时flash全FF挂载会失败，需要格式化后再重新挂载
    if (err)
    {
        err = lfs_format(lfs, lfs_cfg);
        if(err)
            return err;

        err = lfs_mount(lfs, lfs_cfg);
        if(err)
            return err;
    }

    return LFS_ERR_OK;
}

int fs_unmount(lfs_t *lfs, struct lfs_config *lfs_cfg )
{
	int err;

	xy_mutex_acquire(lfs_mutex, osWaitForever);
	err = lfs_unmount(lfs);
	xy_free(lfs);
	xy_free(lfs_cfg);
	xy_mutex_release(lfs_mutex);

	return err;
}

int fs_open(fs_file_t *file, const char *path, int flags)
{
	int ret;
	uint32_t name_len;
    uint32_t type;

    type = file->diskType;
    xy_mutex_acquire(lfs_mutex, osWaitForever);
    fs_config_init(type);
    xy_mutex_release(lfs_mutex);
    if(type == FS_DEFAULT)
    {
        ret = lfs_file_open(lfs1, &file->file, path, flags);
        
    }
    else
    {

        ret = lfs_file_open(lfs2, &file->file, path, flags);
    }
	if(ret == LFS_ERR_OK)
	{
		name_len = strlen(path) + 1;
		file->filename = xy_malloc(name_len);
		strncpy(file->filename, path, name_len);
	}
    return ret;
}

int fs_close(fs_file_t *file)
{
    if(file == NULL)
    {
        return LFS_ERR_INVAL;
    }

	if(file->filename)
	{
		xy_free(file->filename);
		file->filename = NULL;
	}
    if (file->diskType == FS_DEFAULT)
    {
        return lfs_file_close(lfs1, &file->file);
    }
    else
    {
        return lfs_file_close(lfs2, &file->file);
    }
    
}

int fs_read(fs_file_t *file, void *buffer, uint32_t size)
{
    if(file == NULL || buffer == NULL)
    {
        return LFS_ERR_INVAL;
    }

    if(file->diskType == FS_DEFAULT)
    {
        return lfs_file_read(lfs1, &file->file, buffer, size);
    }
    else
    {
        return lfs_file_read(lfs2, &file->file, buffer, size);
    }

}

int fs_write(fs_file_t *file, const void *buffer, uint32_t size)
{
    if(file == NULL || buffer == NULL)
    {
        return LFS_ERR_INVAL;
    }

    if(file->diskType == FS_DEFAULT)
    {
        return lfs_file_write(lfs1, &file->file, buffer, size);
    }
    else
    {
        return lfs_file_write(lfs2, &file->file, buffer, size);
    }

}

int fs_sync(fs_file_t *file)
{
    xy_mutex_acquire(lfs_mutex, osWaitForever);
    fs_config_init(file->diskType);
    xy_mutex_release(lfs_mutex);
    if(file->diskType == FS_DEFAULT)
    {

    	 return lfs_file_sync(lfs1, &file->file);
    }
    else
    {

    	 return lfs_file_sync(lfs2, &file->file);
    }


}

int fs_seek(fs_file_t *file, int off, int whence)
{
     if(file->diskType == FS_DEFAULT)
    {

    	 return  lfs_file_seek(lfs1, &file->file, off, whence);
    }
    else
    {

    	 return  lfs_file_seek(lfs2, &file->file, off, whence);
    }

    
}

int fs_tell(fs_file_t *file)
{
    if(file->diskType == FS_DEFAULT)
    {

        	return  lfs_file_tell(lfs1, &file->file);
        }
        else
        {

        	 return  lfs_file_tell(lfs2, &file->file);
        }

}

int fs_size(fs_file_t *file)
{
    if(file->diskType == FS_DEFAULT)
    {

    	return  lfs_file_size(lfs1, &file->file);
    }
    else
    {

    	 return  lfs_file_size(lfs2, &file->file);
    }

    

}

int fs_truncate(fs_file_t *file, uint32_t size)
{
    if(file->diskType == FS_DEFAULT)
    {

            return  lfs_file_truncate(lfs1, &file->file, size);
    }
    else
    {

            return  lfs_file_truncate(lfs2, &file->file, size);
    }


}

int fs_remove(const char *path, uint32_t diskType)
{
    xy_mutex_acquire(lfs_mutex, osWaitForever);
    fs_config_init(diskType);
    xy_mutex_release(lfs_mutex);
    if(diskType == FS_DEFAULT)
    {

        return  lfs_remove(lfs1, path);
    }
    else
    {

        return  lfs_remove(lfs2, path);
    }
    
    
}

int fs_statfs(fs_stat_t *stat)
{
    int ret;
	
	stat->block_size = lfs_cfg1->block_size;
	stat->total_block = lfs_cfg1->block_count;
	ret = lfs_fs_size(lfs1);
	if(ret > 0)
	{
		stat->block_used = ret;
		ret = 0;
	}
	
    return ret;
}

int fs_rename(const char *oldpath, const char *newpath)
{
	return lfs_rename(lfs1, oldpath, newpath);
}

int fs_mkdir(const char *path)
{
	return lfs_mkdir(lfs1, path);
}

int fs_dir_open(fs_dir_t *dir, const char *path)
{
	return lfs_dir_open(lfs1, dir, path);
}

int fs_dir_read(fs_dir_t *dir, fs_info_t *info)
{
	return lfs_dir_read(lfs1, dir, info);
}

int fs_dir_close(fs_dir_t *dir)
{
	return lfs_dir_close(lfs1, dir);
}

