当前电信SDK版本号：NB SDK V2.1.1.006


1、xy_ctlw_module_init去除入参，默认xy_tcpip_is_ok必须为是；xy_ctlw_module_init接口与恢复流程如何关联的？
2、g_ctlw_dns_ip和g_ctlw_host_name定义时直接赋值{0}，去除memset 0的操作
3、新增一个函数uint8_t xy_get_ipaddr_type(char *ipaddr)，对应xy_ipaddr_is_valid接口，返回值为地址类型；//唐晓玲
4、inet_pton6等涉及IP地址转换的函数，提供好注释说明，最好举例；与inet_pton的差异在哪？//唐晓玲
5、allow_execute_onCurStatus需要深究下，我理解无需特别关注状态，也无需过多考虑深睡恢复场景；
6、xy_ctlw_dns_task_proc走查下，感觉嵌套过深了
7、ctiot_set_server接口内部已经做了非常严格的检查，外部调用者无需再防范式编程；
8、xy_ctlw_module_recover为何到处调用执行？
9、xy_ctlw_ctiot_get_context大把地方用于防错，价值不大，交由开源内部自行判断；
10、XY_CTLW_AUTO_REG、XY_CTLW_MANUAL_REG的场景化差异说明；
11、cloud_proxy.c、ctwing_proxy.c宏观思想没问题，但优先级低，都是些小客户有此需求；
12、xy_ctlw_autoRegister_task的使用介绍，与外部触发的注册差异性在哪？ctiot_engine_entry接口为何未用？配置类的无需ctiot_start_send_recv_thread，但涉及远程通信必须调用该接口
13、ctwing_util.c中全局存在价值的排查；
14、睡眠机制是如何控制的？两个文件系统，一个是配置类NV，应该由SDK来实时保存，且xy_ctlw_module_init中应该就可以恢复了；另一个是运行状态机，由芯翼实现稳态下的写文件和再次上电的恢复




/*update超时唤醒/下行数据/fota重启后*/
void ctwing_resume()
{
	if(FS(session) is exist)
    {
        if(!xy_tcpip_is_ok())
			return;
			
		osMutexAcquire
		xy_ctlw_module_init(true);
		xy_ctlw_module_recover();
		osMutexRelease
    }
}


/*ctwing的AT命令解析时调用*/
void ctwing_init_resume()
{
	if(!xy_tcpip_is_ok())
		return error;
	xy_ctlw_module_init();
	if(FS(session) is exist)
    {
		osMutexAcquire
		xy_ctlw_module_recover();
		osMutexRelease
    }	
}