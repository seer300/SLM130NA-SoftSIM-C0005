# SDK说明

## 编译环境构建 

1. 编译kernel需要armcc编译工具。提供以下两种方式使用armcc。
    (1) 使用在ql-cross-tool\win32\owtoolchain目录下提供的ammcc编译工具，但需要在ql-cross-tool\win32\owtoolchain目录放入正版license。
	(2) 安装DS-5开发软件，并将armcc编译工具所在目录加入环境变量PATH中，添加变量ARMLMD_LICENSE_FILE到环境变量中，ARMLMD_LICENSE_FILE设置为armcc所使用的license；
2. 编译APP（ql-application）需要arm-gcc编译工具，SDK中已提供。

## 目录结构

### xinyiNBIot_AP	根目录 

	---APPLIB         业务库，用户可修改，自行维护
		---at_cmd	    AT服务端
		---cJSON	    开源库，客户自行维护
		---cloud	    公有云参考代码
			---cdp	        华为云参考代码
			---ctwing	    电信云参考代码
			---onenet	    中移云参考代码
			---utils	    公有云公共函数
		---dm	      联通/电信自注册参考代码
		---Dtls	      DTLS开源库
		---fs	      littlefs文件系统，支持两个分区
		---http	      http参考代码
		---libcoap	  coap开源库
		---lwip	      tcpip协议栈参考代码，不建议客户修改
		---mqtt	      mqtt参考代码
		---netled	  网路指示灯参考代码
		---perf	      灌包调试perf参考代码，仅芯翼内部使用
		---ping	      PING包参考代码，客户自行维护
		---socket	  socket的AT代理参考代码
		---wakaama	  lwm2m开源库
		---wireshark  抓包调试代码，仅芯翼内部使用
		---xy_fota	  芯翼自研FOTA参考代码

		
	---ARCH           cortex-M3架构相关代码，包括系统启动相关
		
		
	---DRIVERS        芯翼底层驱动库函数
	
	
	---KERNEL         freertos，用户仅能调用cmsis_os2.h中的接口

	---NBIotPs	      3GPP协议栈

	---NBPHY	      3GPP物理层
	
	---TARGETS	      工程入口，用户可以自行重定义各种__WEAK弱函数
	
		
	---SYSAPP	  系统内核代码及相关库，用户不得修改，只能调用xy_打头的头文件中API接口
		---at_ctrl	   AT主框架
		---at_uart     AT串口驱动相关，仅当AT通道挂接在CP核放有效
		---diag_ctrl   log控制模块
		---flash       flash操作接口，含eftl磨损算法接口
		---rtc_tmr     RTC模块
		---shm_msg     核间消息机制
		---smartcard   SIM卡驱动
		---system      基础平台系统级接口，包括看门狗/省电等


## 编译指令：

1. ql-sdk目录下，执行 build.bat app		编译APP镜像(application)，若对app未做任何修改，可以不执行
2. ql-sdk目录下，执行 build.bat kernel		编译kernel，若对kernel未做任何修改，可以不执行
3. ql-sdk目录下，执行 build.bat bootloader	编译bootloader，若对bootloader未做任何修改，可以不执行
4. ql-sdk目录下，执行 build.bat firmware	生成固件

>> 注：未执行编译操作的组件，默认使用ql-sdk\ql-config\quec-project\aboot\images目录下的镜像


## Release History：

**[XINYI1200] 2022-10-15**
- 1、SDK 初版，提供编译框架。
- 2、提供内核编译指令。
- 3、添加SIM、设备信息、网络信息、UART、GPIO、数据拨号、OS、socket、文件系统等功能及API。


