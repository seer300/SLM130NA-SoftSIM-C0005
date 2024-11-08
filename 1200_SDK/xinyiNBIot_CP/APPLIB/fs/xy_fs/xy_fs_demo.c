#include "xy_fs.h"
#include "fs_al.h"
#include "xy_system.h"

#define  DEMO_NAME  "xyfile"

/*用户只能使用FS_USER分区，注意正确配置USER_LFS_FLASH_BASE和USER_LFS_FLASH_LEN值*/
void fs_demo(void)
{
	xy_file fp = NULL;
	int readCount = XY_ERR;
	int writeCount = XY_ERR;
	int pos = XY_ERR;
	int fp_size = XY_ERR;
	int offset = XY_ERR;
	uint32_t boot_count = 0;
	//xy_file fp = NULL;
    char writebuf[64] = {0};
    char readbuf[64] = {0};

	/*用户只能使用FS_USER分区，注意正确配置USER_LFS_FLASH_BASE和USER_LFS_FLASH_LEN值！当前为了方便演示，使用默认分区*/
	fp = xy_fopen(DEMO_NAME, "w+", FS_DEFAULT);

	if(fp != NULL)
	{
		sprintf(writebuf, "xy_fs demo test");
		writeCount = xy_fwrite(writebuf, strlen(writebuf), fp);
		//查询文件大小
		fp_size = xy_fsize(fp);
        //文件位置指针移动到文件头
		offset = xy_fseek(fp, 0, 0);
		//查询文件位置指针
		//读文件，返回读取的字节数
		pos = xy_ftell(fp);
		readCount = xy_fread(readbuf, 15, fp);
		user_printf("%s%d",readbuf, pos);

		sprintf(writebuf, "!!!");
		//文件位置指针移动到原字符串结束符,修改原文件的数据
		offset = xy_fseek(fp, -1, 1);
		writeCount = xy_fwrite(writebuf, strlen(writebuf), fp);
		//查询文件大小
		fp_size = xy_fsize(fp);
        //文件位置指针移动到文件头，读取现在文件中的数据
		offset = xy_fseek(fp, 0, 0);
		readCount = xy_fread(readbuf, 17, fp);
		pos = xy_ftell(fp);
		user_printf("%s%d",readbuf, pos);
		//读写完毕，关闭文件
		xy_fclose(fp);


	}


}

