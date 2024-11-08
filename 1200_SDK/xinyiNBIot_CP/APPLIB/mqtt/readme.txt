###########目录结构描述
|── readme.txt                        // help
|
|── MQTTPacket                        // mqtt开源代码
|
|—— utils                             //加密算法（用于阿里云秘钥计算）
|
|── xy_mqtt
	|
	|___inc                           //头文件
	|
	|___src
		|── at_mqtt_default.c         // 芯翼自研 at 命令实现
		|
		|—— at_mqtt_quectel.c         // 对标移远 at 命令实现
		|
		|—— xy_mqtt_api.c             // 芯翼提取的mqtt的相关API接口，可用于客户二次开发，但客户不得修改其源码！！！
		|
		|__ MQTTClient.c/MQTTTimer.c  // 底层SOCKET等适配部分
		