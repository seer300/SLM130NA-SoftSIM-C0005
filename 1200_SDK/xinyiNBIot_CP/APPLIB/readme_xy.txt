开源信息说明
======
  ARM核使用开源系统freertos，版本FreeRTOS Kernel V10.3.1；云业务使用了CDP sdk和ONENET sdk；
  同时芯翼SDK集成了mbedtls、libcoap和wakaama开源库，这些开源库已经调试编译好，用户可以直接使用。该文件介绍了开源库的编译链接和使用。

用户二次开发
========
DEMO的使用，请阅读《芯翼XY1200平台开发指南_V1.1》，以及demo路径下的readme

如何编译链接开源库？？
===========
  在SDK中，\xinyiNBIoT_OS_custom\TARGETS\xinyiNBSoC\GCC-ARM\make\define.mk中使能相关开源库的宏定义即可。
  
  ARM mbedtls开源库 --- MBEDTLS_SUPPORT
  
  libcoap开源库     --- LIBCOAP_SUPPORT
  
  wakaama开源库     --- WAKAAMA_SUPPORT
  
  eg：
  	LIBCOAP_SUPPORT=y//编译链接libcoap开源库
  	
  	LIBCOAP_SUPPORT=n//不编译链接libcoap开源库


FreeRTOS
======
  ARM核使用开源系统FreeRTOS。
  FreeRTOS遵循的开源协议是MIT许可证。根据MIT许可证，您可以自由使用、修改和分发FreeRTOS，甚至用于商业目的，只需在您的软件副本中包含MIT许可证的版权声明和许可声明。MIT许可证是一种宽松的开源许可证，允许开发人员在其项目中集成和使用FreeRTOS而无需支付费用或受到限制。
版本： V10.3.1
路径：https://www.freertos.org/a00104.html


CDP sdk
======
  cdp业务开发使用的sdk，支持对电信CTWing，联通和华为云平台的连接
  CDP sdk是集成在LiteOS内的，版本与获取路径参照LiteOS
版本：LiteOSV200R001C50B021
路径：https://github.com/LiteOS/LiteOS/releases/tag/tag_LiteOS_V200R001C50B021_20180629  
  
ONENET sdk  
======
  中移提供的物联网业务sdk，支持与中移onenet 云平台的对接
版本：2.3.0
路径：https://open.iot.10086.cn/doc/nb-iot/book/device-develop/get_SDK.html
  

ARM mbedtls开源库
======
  ARM mbedtls使开发人员可以非常轻松地在（嵌入式产品中加入加密和 SSL/TLS 功能）。它提供了具有直观的 API 和可读源代码的 SSL 
库。该工具即开即用，可以在大部分系统上直接构建它，也可以手动选择和配置各项功能。

  mbedtls库提供了一组可单独使用和编译的加密组件，还可以使用单个配置头文件加入或排除这些组件。
  
  从功能角度来看，该mbedtls分为三个主要部分：   
	- SSL/TLS 协议实施。 	
	- 一个加密库。 	
	- 一个 X.509 证书处理库。
  mbed TLS遵循的开源协议是Apache许可证2.0。根据Apache许可证2.0，您可以自由使用、修改和分发mbed TLS，包括用于商业目的，只需遵守许可证的条款，如在衍生产品中包含原始版权声明和许可声明。Apache许可证2.0是一种宽松的开源许可证，允许开发人员在其项目中集成和使用mbed TLS而无需支付费用或受到限制。	
版本：V3.2.1
路径：https://tls.mbed.org	

libcoap开源库https://libcoap.net
======
  libcoap是CoAP协议的C语言实现，libcoap提供server和client功能，它是调试CoAP的有力工具，sdk将
集成了libcoap，并通过已个简单的例子说明libcoap的使用方法。
  CoAP（Constrained Application Protocol）是一种专为受限环境（如物联网设备）设计的应用层协议，通常用于在资源受限的网络中进行低功耗通信。
CoAP协议本身并没有特定的开源许可协议，而是由IETF（Internet Engineering Task Force）进行标准化。因此，CoAP协议的使用不受特定的开源许可协议限制，而是遵循IETF的标准化过程和规范。
版本：release-4.2.0  
路径：https://github.com/obgm/libcoap/tree/release-4.2.0


wakaama是LWM2M开源协议栈https://github.com/eclipse/wakaama
======
  wakaama呈现形式并不是一个C的库文件，而是以源代码的形式，直接和项目代码联合编译。  
  SDK只集成了客户端代码，用户可以自己配置使用。
  LwM2M协议本身并没有特定的开源许可协议，而是由OMA（Open Mobile Alliance）制定和管理。OMA通常采用RAND（Reasonable and Non-Discriminatory）许可模式，这意味着LwM2M协议的标准可以在遵循合理和非歧视性条件下使用。因此，LwM2M协议的使用不受特定的开源许可协议限制，但需要遵循OMA的标准化规范。
版本： 
路径：https://github.com/eclipse/wakaama
  

MQTT是mqtt开源协议栈https://github.com/eclipse/paho.mqtt.embedded-c/tree/master
======
  MQTT是一个基于客户端-服务器的消息发布/订阅传输协议，以源代码的形式呈现，直接和项目代码联合编译。
  MQTT（Message Queuing Telemetry Transport）协议是一种轻量级的消息传输协议，通常用于物联网设备之间的通信。
  MQTT协议本身并没有特定的开源许可协议，但根据OASIS的规定，MQTT协议的标准是开放的，并且可以免费使用和实现。
  因此，MQTT协议的使用不受特定的开源许可协议限制，但需要遵循OASIS的标准化规范。
版本: MQTT 开源代码官方已不再更新，目前使用的是 master 分支的最新版本 
路径：https://github.com/eclipse/paho.mqtt.embedded-c/tree/master


HTTP开源库
======
  HTTP协议是一款用于传输超文本的应用层协议。
  HTTP开源库是集成在AliOS Things内的，版本与获取路径参照AliOS Things。
  HTTP（Hypertext Transfer Protocol）是一种用于传输超文本的应用层协议，用于在Web浏览器和Web服务器之间传输数据。HTTP协议本身并没有特定的开源许可协议，而是由IETF（Internet Engineering Task Force）进行标准化。
  HTTP协议的标准是公开的，并且可以免费使用和实现，因此它不受特定的开源许可协议限制。开发人员可以自由地使用HTTP协议来构建Web应用程序，而无需担心开源许可问题。
版本：rel_3.3.0
路径：https://gitee.com/alios-things-admin/AliOS-Things/tree/rel_3.3.0/

LwIP遵循以下开源协议：
  BSD（Berkeley Software Distribution）开源许可证。
  MIT开源许可证。
  GPL（GNU General Public License）。
版本：rel_2.1.3
路径：https://savannah.nongnu.org/projects/lwip/
	
test

 
