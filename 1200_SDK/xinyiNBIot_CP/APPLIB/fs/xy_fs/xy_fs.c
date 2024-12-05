#include <stdio.h>
#include "xy_fs.h"
#include "xy_utils.h"
#include "xy_flash.h"
#include "ipc_msg.h"

volatile int lpm_fs_flag = 0;


/*该函数仅用于XY_FS接口的异常信息上报*/
static void fs_debug_info_print(const char * fileName, const char * function)
{
	if(Is_OpenCpu_Ver() || lpm_fs_flag || fileName == NULL)
	{
		return;
	}
	char * debug_str = xy_malloc(64);
	snprintf(debug_str,64,"+DBGINFO: %s %s failed\r\n", fileName, function);
	send_debug_by_at_uart(debug_str);
	xy_free(debug_str);

	xy_printf(0,XYAPP, WARN_LOG, "[XY_FS] can't %s flie: %s", function, fileName);
}

xy_file xy_fopen(const char * fileName, const char * mode, uint32_t diskType)
{
	int32_t open_mode;
	fs_file_t * pf;

	/* Parse the specified mode. */
	open_mode = LFS_O_RDONLY;
	if (*mode != 'r') {			/* Not read... */
		open_mode = (LFS_O_WRONLY | LFS_O_CREAT);
		if (*mode != 'w') {		/* Not write (create or truncate)... */
			open_mode = (LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
			if (*mode != 'a') {	/* Not write (create or append)... */
				return NULL;
			}
		}
	}

	if ((mode[1] == 'b')) {		/* Binary mode (NO-OP currently). */
		++mode;
	}

	if (mode[1] == '+') {			/* Read and Write. */
		++mode;
		open_mode |= (LFS_O_RDONLY | LFS_O_WRONLY);
		open_mode += (LFS_O_RDWR - (LFS_O_RDONLY | LFS_O_WRONLY));
	}

    pf = xy_malloc(sizeof(fs_file_t));
    pf->diskType = diskType;
	if(fs_open(pf, fileName, open_mode))
	{
		xy_free(pf);
		pf = NULL;
		fs_debug_info_print(fileName, "open");
	}
	
    return pf;
}

int32_t xy_fclose(xy_file fp)
{
	int ret;

	ret = fs_close(fp);
	if(ret < 0)
	{
		fs_debug_info_print(fp->filename, "close");
	}
	
	xy_free(fp);
    return ret;
}

int32_t xy_fread(void * buf, uint32_t len, xy_file fp)
{
	if(len == 0)
		return  0;
	
	int ret_len = fs_read(fp, buf, len);

	if(ret_len < 0)
	{
		fs_debug_info_print(fp->filename, "read");
	}
	
	return ret_len;
}

int32_t xy_fwrite(void *buf, uint32_t len, xy_file fp)
{
	if(len == 0)
		return  0;

	int ret_len = fs_write(fp, buf, len);

	if(ret_len < 0)
	{
		fs_debug_info_print(fp->filename, "write");
	}
	else
	{
		fs_sync(fp);
	}
		
	return ret_len;
}

int32_t xy_fwrite_safe(void *buf, uint32_t size, xy_file fp)
{
	int32_t len_r = 0;
	int32_t len_w = 0;
	uint8_t * buf_r = NULL;
		
	if(size == 0)
		return  0;

	buf_r = xy_malloc(size);
	len_r = fs_read(fp, buf_r, size);


	//对比数据有没有改变，没有改变不写
	if(size==len_r && !memcmp(buf_r, buf, len_r))
	{
		len_w = size;
	}
	else
	{
		fs_seek(fp, 0, 0);
		len_w = fs_write(fp, buf, size);
		
		if(len_w <= 0)
		{
			fs_debug_info_print(fp->filename, "write_safe");
		}
		else
		{
			fs_sync(fp);
		}
	}
	
	xy_free(buf_r);
	
	return len_w;
}

int32_t xy_ftell(xy_file fp)
{
	return fs_tell(fp);
}

int32_t xy_fseek(xy_file fp, int32_t offset, int32_t seekType)
{
	return fs_seek(fp, offset, seekType);
}

int32_t xy_fsync(xy_file fp)
{
	return fs_sync(fp);
}

int32_t xy_fsize(xy_file fp)
{
	return fs_size(fp);
}

int32_t xy_fremove(const char *fileName, uint32_t diskType)
{
	return fs_remove(fileName, diskType);
}

xy_file lpm_fs_fopen(const char * fileName, const char * mode, uint32_t diskType)
{
	/*OPENCPU产品，不能写flash，否则会卡AP核几百毫秒*/
	if(Is_OpenCpu_Ver())
		return NULL;

	if(fileName == NULL || mode == NULL)
	{
		return NULL;
	}

	volatile flash_notice_t* flash_notice = (flash_notice_t*)BAK_MEM_FLASH_NOTICE;

	lpm_fs_flag = 1;
	flash_notice->cp_status = cp_status_write;
	shm_msg_write((void *)osThreadGetName(osThreadGetId()), 8, ICM_FLASHWRT_NOTICE); //通知ap挂起，写入线程名为了查问题方便
	while(flash_notice->ap_status == ap_status_run); //循环等待ap核进入suspend态，这里不能用osDelay切出去

	xy_file ret = xy_fopen(fileName, mode, diskType);

	flash_notice->cp_status = cp_status_writedone; //通知ap退出suspend态
	while(flash_notice->ap_status == ap_status_sus)  //循环等待ap核退出suspend态，这里不能用osDelay切出去

	flash_notice->cp_status = cp_status_idle; //cp_status回到idle态
	
	lpm_fs_flag = 0;

	return ret;
}

int32_t lpm_fs_fwrite(void *buf, uint32_t len, xy_file fp)
{
	volatile flash_notice_t* flash_notice = (flash_notice_t*)BAK_MEM_FLASH_NOTICE;

	/*OPENCPU产品，不能写flash，否则会卡AP核几百毫秒*/
	if(Is_OpenCpu_Ver())
		return XY_OK;

	lpm_fs_flag = 1;
	flash_notice->cp_status = cp_status_write;
	shm_msg_write((void *)osThreadGetName(osThreadGetId()), 8, ICM_FLASHWRT_NOTICE); //通知ap挂起，写入线程名为了查问题方便
	while(flash_notice->ap_status == ap_status_run); //循环等待ap核进入suspend态，这里不能用osDelay切出去

	int ret = fs_write(fp, buf, len);

	fs_sync(fp);

	flash_notice->cp_status = cp_status_writedone; //通知ap退出suspend态
	while(flash_notice->ap_status == ap_status_sus)  //循环等待ap核退出suspend态，这里不能用osDelay切出去

	flash_notice->cp_status = cp_status_idle; //cp_status回到idle态
	
	lpm_fs_flag = 0;
		
	return ret;
}

int32_t lpm_fs_fread(void *buf, uint32_t len, xy_file fp)
{
	volatile flash_notice_t* flash_notice = (flash_notice_t*)BAK_MEM_FLASH_NOTICE;

	/*OPENCPU产品，不能写flash，否则会卡AP核几百毫秒*/
	if(Is_OpenCpu_Ver())
		return XY_OK;

	lpm_fs_flag = 1;
	flash_notice->cp_status = cp_status_write;
	shm_msg_write((void *)osThreadGetName(osThreadGetId()), 8, ICM_FLASHWRT_NOTICE); //通知ap挂起，写入线程名为了查问题方便
	while(flash_notice->ap_status == ap_status_run); //循环等待ap核进入suspend态，这里不能用osDelay切出去

	int ret = fs_read(fp, buf, len);

	flash_notice->cp_status = cp_status_writedone; //通知ap退出suspend态
	while(flash_notice->ap_status == ap_status_sus)  //循环等待ap核退出suspend态，这里不能用osDelay切出去

	flash_notice->cp_status = cp_status_idle; //cp_status回到idle态
	
	lpm_fs_flag = 0;
		
	return ret;
}

int32_t lpm_fs_fclose(xy_file fp)
{
	volatile flash_notice_t* flash_notice = (flash_notice_t*)BAK_MEM_FLASH_NOTICE;

	/*OPENCPU产品，不能写flash，否则会卡AP核几百毫秒*/
	if(Is_OpenCpu_Ver())
		return XY_OK;

	lpm_fs_flag = 1;
	flash_notice->cp_status = cp_status_write;
	shm_msg_write((void *)osThreadGetName(osThreadGetId()), 8, ICM_FLASHWRT_NOTICE); //通知ap挂起，写入线程名为了查问题方便
	while(flash_notice->ap_status == ap_status_run); //循环等待ap核进入suspend态，这里不能用osDelay切出去

	int ret = xy_fclose(fp);

	flash_notice->cp_status = cp_status_writedone; //通知ap退出suspend态
	while(flash_notice->ap_status == ap_status_sus)  //循环等待ap核退出suspend态，这里不能用osDelay切出去

	flash_notice->cp_status = cp_status_idle; //cp_status回到idle态
	
	lpm_fs_flag = 0;
		
	return ret;
}


