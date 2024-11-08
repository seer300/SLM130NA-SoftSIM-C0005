#pragma once

#if WEBLOG
    #include <stdbool.h>
    #include <stdint.h>
    // 服务器下发的log命令类型
    typedef enum
    {
        LOG_DISABLE = 0,
        LOG_ENABLE,
        LOG_CLEAN,
        LOG_READ_AP,
        LOG_READ_CP,
        LOG_END,
    } log_cmd_t;
    
    // 远程log配置信息，每次buff存满写入flash时，配置信息也需更新到前4Kflash中，断电上电后可读取
    typedef struct
    {
    	uint32_t log_base_addr;     // 保存log的起始地址
    	uint32_t flash_size;        // 保存log实际内容的大小，不包含log配置信息
    	uint32_t save_offset;       // FLASH环形队列中log的最新待写入位置偏移
    	uint16_t flash_rewrite_num; // FLASH环形队列写满次数
    	uint16_t log_buff_size;     // 保存log的RAM上的缓冲区大小
    	uint8_t  web_log_enable;    // 保存log的开关
    	uint8_t  unused[3];
    } log_cfg_t;
    
    /**
     * @brief    获取web_log的配置信息
     * @param	 log_read_mode  [IN]  可选择获取AP或者CP的web_log配置信息，可设置为LOG_READ_AP或LOG_READ_CP
     * @param	 log_config     [OUT] 用来保存AP或者CP的web_log配置信息
     * @return   0：获取失败，1: 获取成功
     */
    bool web_log_config(int log_read_mode, log_cfg_t *log_config);

    /**
     * @brief    读取web_log的内容
     * @param	 base_addr      [IN]  读取AP或者CP的web_log的起始地址
     * @param	 size           [IN]  读取的web_log的内容大小
     * @param	 buff           [OUT] 用来保存AP或者CP的web_log的内容
     * @note     使用此接口之前可通过web_log_config接口获取log的配置信息
     */
    bool web_log_read(uint32_t base_addr, int size, void *buff);

    int log_printf_save_buff(int dyn_id, int src_id, const char *fmt, ...);

    int at_LOGENABLE_req(char *at_buf, char **prsp_cmd);
    int at_LOGREAD_req(char *at_buf, char **prsp_cmd);
#else
    #define log_printf_save_buff(...)
#endif