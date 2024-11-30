/** 
* @file     ss_fs.c
* @date     2024-06-25
* @author   Onomondo
* @brief    filesystem wrapper of the xy platform in accordance to onomondo-uicc expectations
*/

#include "xy_fs.h"
#include "xy_system.h"
#include "onomondo/softsim/fs.h"

#define SOFTSIM_DISK_TYPE FS_DEFAULT

ss_FILE ss_fopen(char *path, char *mode)
{
    const char * filename = path;
    const char * open_open = mode;
    return (ss_FILE)xy_fopen(path, open_open, SOFTSIM_DISK_TYPE);
};


int ss_fclose(ss_FILE fp)
{
    return xy_fclose((xy_file)fp);
};


size_t ss_fread(void *ptr, size_t size, size_t nmemb, ss_FILE fp)
{
    // nmemb equels number of bytes to read
    return xy_fread(ptr, (uint32_t)nmemb, (xy_file)fp);
};


size_t ss_fwrite(const void *prt, size_t size, size_t count, ss_FILE f)
{
    if (!f || !prt || !size || !count)
        return 0;

    int bytes = xy_fwrite(prt, (uint32_t)count, (xy_file)f);
    
    if (bytes < 0)
        return 0;
    
    return bytes;
};


int ss_file_size(char *path)
{
    int size = 0;
    xy_file fp = ss_fopen(path, "r");
    if (fp != NULL)
    {
        size = xy_fsize(fp);
        ss_fclose(fp);
    }
    
    return size;
};


int ss_delete_file(const char *path)
{
    return xy_fremove(path, SOFTSIM_DISK_TYPE);
};


int ss_delete_dir(const char *path)
{
    return xy_fremove(path, SOFTSIM_DISK_TYPE);
};


int ss_fseek(ss_FILE fp, long offset, int whence)
{
    int rc = xy_fseek((xy_file)fp, (int32_t)offset, (int32_t)whence);
    if (rc >= 0)
        return 0;
    else
        return -1;
};


int ss_access(const char *path, int amode)
{
    xy_file fp = ss_fopen(path, "r");
    ss_fclose(fp);
    return 0;
};
