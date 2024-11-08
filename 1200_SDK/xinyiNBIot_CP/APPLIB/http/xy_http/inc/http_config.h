#pragma once

#define HTTPCLIENT_HEADER_SIZE           1024    /*http header buffer*/
#define HTTPCLIENT_RESP_SIZE             1024    /*http response buffer*/

#define CONFIG_HTTP_FILE_OPERATE     0    /*是否使用文件操作，暂未适配，默认关闭*/
#define CONFIG_HTTP_SECURE			 1	  /*是否开启HTTPS支持，默认开启*/

#define HARDCODE_USER_CERT_PK        1    /*是否将server cert，client cert，private key 存储到flash中，默认开启*/

