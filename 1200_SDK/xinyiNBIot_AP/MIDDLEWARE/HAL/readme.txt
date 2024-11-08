
在CONFIG_DYN_LOAD_SELECT、BAN_WRITE_FLASH两个宏都开启前提下，
从.build\elf\xinyiNBSoC.map中，确认产品用到的源文件后，把MIDDLEWARE\HAL\so文件夹下用到的源文件拷贝到SO\HAL\src下，
进而可以增加用户的RAM空间。

通常用户仅需重点关注MIDDLEWARE\HAL\so、MIDDLEWARE\mcu_adapt\so两个文件夹中的有效源文件。

具体请参阅《芯翼XY1200S&XY2100S产品动态内存复用方案》