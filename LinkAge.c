/*
这是设备相应的信息，我们是网关，我们只要根据不同的设备状态，组成相应的广播帧发送就可以
*/
typedef enum
{
    GL_ALARM_DEVICE_NANE_NONE                                            = 0x00000000, /* 无报警                              */
    GL_ALARM_DEVICE_NANE_SMOKE                                           = 0x00020001, /* 烟雾报警                            */
    GL_ALARM_DEVICE_NANE_SITERWELL_SMOKE                                 = 0x0005530D, /* 烟雾报警(赛特威尔旧设备)            */
    GL_ALARM_DEVICE_NANE_GAS                                             = 0x00020002, /* 燃气报警                            */
    GL_ALARM_DEVICE_NANE_SITERWELL_GAS                                   = 0x06100000, /* 燃气报警(赛特威尔旧设备)            */
    GL_ALARM_DEVICE_NANE_SOS                                             = 0x00020003, /* SOS报警                             */
    GL_ALARM_DEVICE_NANE_SITERWELL_SOS                                   = 0x03000000, /* SOS报警(赛特威尔旧设备)             */
    GL_ALARM_DEVICE_NANE_WITH_SWITCH_GAS                                 = 0x00020004, /* 燃气报警(带流水开关)                */
    GL_ALARM_DEVICE_NANE_WITH_SOLENOID_BATTERY_CARBON_MONOXIDE           = 0x00020005, /* 一氧化碳报警(带电磁阀、电池)        */
    GL_ALARM_DEVICE_NANE_WITH_SOLENOID_BATTERY_GAS                       = 0x00020006, /* 燃气报警器(带电磁阀、电池)          */
    GL_ALARM_DEVICE_NANE_WITH_SOLENOID_BATTERY_GAS_AND_CARBON_MONOXIDE   = 0x00020007, /* 一氧化碳+燃气报警器(带电磁阀、电池) */
    GL_ALARM_DEVICE_NANE_WITH_SOLENOID_CARBON_MONOXIDE                   = 0x00020008, /* 一氧化碳报警器(带电磁阀)            */
	GL_ALARM_DEVICE_NANE_WITH_SOLENOID_GAS                               = 0x00020009, /* 燃气报警(带电磁阀)                  */
	GL_ALARM_DEVICE_NANE_WITH_SOLENOID_CARBON_MONOXIDE_AND_GAS           = 0x0002000A, /* 一氧化碳+燃气报警器(带电磁阀)       */
	GL_ALARM_DEVICE_NANE_WITH_SWITCH_GL                                  = 0x00020020, /* 个联流水开关                        */
	GL_ALARM_DEVICE_NANE_ELECTRIC                                        = 0x00030001, /* 电气报警电表                        */
}gl_alarm_device_name_type_e;

typedef enum
{
	GL_LINKAGE_CUT_NONE                     = 0x00,/* 啥都不做                   */
    GL_LINKAGE_CUT_GAS                      = 0x01,/* 切断燃气阀                    */
    GL_LINKAGE_CUT_ELECTRIC                 = 0x02,/* 切断电闸                      */
}gl_linkage_handle_type_e;

/**************************************************************************
*函数名      : gl_GetDeviceName
*作者        : 吴钊
*日期        : 2018/06/01
*功能描述    : 通过SN获取设备名
*参数        : 
              sn:       sn号
			  ucDevNme：获取的设备名
*返回值      :
              0：处理失败
			  1：处理成功
**************************************************************************/
gl_uint32 gl_GetDeviceName(gl_uint32 *sn, gl_uint8 *ucDevNme) 
{
    gl_dev_data_node_t *node1 = GL_NULL;
		
	if(ucDevNme == NULL)
	{
		gl_debug_info("ucDevNme space is NULL ------->%d\n", 2);
		return 0;
	}
	
    if(GL_NULL == sn)
    {
        gl_debug_info("sn is NULL ------->%d\n", 2);
        return 0;
    }
	
    if(0 == dev_data.node_count || GL_NULL == dev_data.node_list)
    {
        gl_debug_info("gl_gateway_del_dev(), queue empty, no need to del.\n");
        return 0;
    }
	
    node1 = dev_data.node_list;
    while(node1)//gl_find_dev_by_sn
    {
	    if(0 == gl_hexncmp(node1->sn, sn, GL_SN_LEN) && 0 != gl_hexncmp(sn, invalid_sn, GL_SN_LEN))
        {
            //gl_debug_info("Get Dev name ------->%d\n", 3);
	        gl_memcpy(ucDevNme, node1->name, GL_DEV_NAME_LEN + 1);//TODO
            return 1;
        }
        node1 = node1->next;   
    }
    return 0;
}

/**************************************************************************
*函数名      : gl_gateway_linkage_alarm_Hander
*作者        : 吴钊
*日期        : 2018/05/31
*功能描述    : 根据传入的告警状态，做相应的处理
               (个联流水开关要求同时切断燃气阀门和电闸 ,烟雾报警或燃气要求切断燃气阀门)
*参数        : 
              alarmVal: 告警状态
*返回值      :
              0：处理失败
			  1：处理成功
**************************************************************************/
//要根据设备IMEI处理相应设备名，然后再根据设备告警类型处理
#include<stdlib.h>

gl_int32 gl_gateway_linkage_alarm_Hander(gl_uint32 *sn, gl_uint8 alarmVal)
{
    gl_uint8 ucPackage[GL_PACKAGE_MAX_LEN];
    gl_uint8 ucDevNme[10 + 1];
    gl_linkage_handle_type_e e_type;
    gl_int32 i32Data;
    gl_char* str;
    
    e_type = GL_LINKAGE_CUT_NONE;

    //根据序列号得到相应的设备名
    if(gl_GetDeviceName(sn, ucDevNme) == 0)
    {
        gl_debug_info("can not get alarm device name ------->%d\n", 1);
	    return 0;//失败了
    }else{
        gl_debug_info("Alarm Device Name:%s\n", ucDevNme);
    }

    i32Data = strtol(ucDevNme, &str, 16);
    //gl_debug_info("Device Name i32Data:%d--------->\n", i32Data);
    switch(i32Data)
    {
        case GL_ALARM_DEVICE_NANE_NONE: //无报警
            gl_debug_info("%s ignore packet %d\n", __func__, __LINE__);
            //gl_debug_info("gl_gateway_linkage_alarm_Hander(), ignore packet.\n");
            return 1;//注意有了return之后此出break就无效了
            break;
                    
        case GL_ALARM_DEVICE_NANE_SMOKE:            //烟雾报警	
        case GL_ALARM_DEVICE_NANE_SITERWELL_SMOKE:  //烟雾报警(赛特威尔旧设备)
		case GL_ALARM_DEVICE_NANEE_GAS:             //燃气报警	
        case GL_ALARM_DEVICE_NANE_SITERWELL_GAS:    //燃气报警(赛特威尔旧设备)
		case GL_ALARM_DEVICE_NANE_WITH_SWITCH_GAS:  //燃气报警(带流水开关)
            if(GL_433_UPLOAD_V0_MSG_ALARM == alarmVal || GL_433_UPLOAD_V0_MSG_ALARM_GAS == alarmVal)
            {
                e_type = GL_LINKAGE_CUT_GAS;
            }
            break;     
        //这一类带有电磁阀
        case GL_ALARM_DEVICE_NANE_WITH_SOLENOID_BATTERY_GAS:                     //燃气报警器(带电磁阀、电池)		
        case GL_ALARM_DEVICE_NANE_WITH_SOLENOID_BATTERY_GAS_AND_CARBON_MONOXIDE: //一氧化碳+燃气报警器(带电磁阀、电池)
        case GL_ALARM_DEVICE_NANE_WITH_SOLENOID_GAS:                             //燃气报警(带电磁阀) 
        case GL_ALARM_DEVICE_NANE_WITH_SOLENOID_CARBON_MONOXIDE_AND_GAS:         //一氧化碳+燃气报警器(带电磁阀)
            if(GL_433_UPLOAD_V0_MSG_ALARM_GAS == alarmVal)
            {
                e_type = GL_LINKAGE_CUT_GAS;
            }		    
            break;
                    
        case GL_ALARM_DEVICE_NANE_WITH_SWITCH_GL: //个联流水开关 
            if(GL_433_UPLOAD_V0_MSG_FLOW_SW == alarmVal)
            {
                e_type = GL_LINKAGE_CUT_GAS | GL_LINKAGE_CUT_ELECTRIC;
            }
            break;
                    
        case GL_ALARM_DEVICE_NANE_SOS:                                   //SOS报警		
        case GL_ALARM_DEVICE_NANE_SITERWELL_SOS:                         //SOS报警(赛特威尔旧设备) 
        case GL_ALARM_DEVICE_NANE_WITH_SOLENOID_BATTERY_CARBON_MONOXIDE: //一氧化碳报警(带电磁阀、电池)
        case GL_ALARM_DEVICE_NANE_WITH_SOLENOID_CARBON_MONOXIDE:         //一氧化碳报警器(带电磁阀)	
        case GL_ALARM_DEVICE_NANE_ELECTRIC:                              //电气报警电表
            break;
                    
        default:
            gl_debug_info("%s ignore packet %d\n", __func__, __LINE__);
            ////gl_debug_info("gl_gateway_linkage_alarm_Hander(), ignore packet.\n");
            return 0;//注意有了return之后此出break就无效了
            break;
    }
    
    if(e_type != GL_LINKAGE_CUT_NONE)
    {
        if(gl_add_broadcast_cmd(e_type) == GL_RET_SUCCESS)
        {
            gl_debug_info("Add Broadcast CMD:------->%d\n", e_type);
        }
    }
	
    return 1;
}
//检查消息成功,就在gl_update_gateway_dev_node基础之上添加下面代码
//GL_CONTENT_TYPE_V1_DISCONNECTED_GAS /*燃气阀门已断开    */
case GL_433_UPLOAD_V0_MSG_COGAS_OFF://断开燃气阀门
	gl_debug_info("GL_433_UPLOAD_V0_MSG_COGAS_OFF ------->%d\n",13);
	tmp_node->val[1] = 0x01;
	tmp_node->need_upload_cloud |= GL_DEV_NEED_UPLOAD_STATUS;
	break;

/***************************************************************************
协议修改，增加下发布防/联动报警/设备联动状态
****************************************************************************/	
typedef enum
{	
	GL_GATEWAY_PACKET_DEPLOY,
	GL_GATEWAY_PACKET_GWALARM,
	GL_GATEWAY_PACKET_LINK,
}gl_gateway_packet_type_e;	
static const gl_char *deploy_field = ",\"val\":%d";
static const gl_char *gwalarm_field = ",\"val\":%d";
static const gl_char *link_field = ",\"val\":%d";
static const gl_char *act_deploy  = "deploy";
static const gl_char *act_gwalarm = "gwalarm";
static const gl_char *act_link    = "link";

typedef struct
{
    gl_uint8 ucVal;
}gl_gateway_deploy_arg_t;

typedef struct
{
    gl_uint8 ucVal;
}gl_gateway_gwalarm_arg_t;

typedef struct
{
    gl_uint8 ucVal;
}gl_gateway_link_arg_t;

typedef struct
{
    union
    {
		gl_gateway_deploy_arg_t deploy;
		gl_gateway_gwalarm_arg_t gwalarm;	
        gl_gateway_link_arg_t link;
    }arg;
}gl_gateway_packet_arg_t;

packets_opt[GL_GATEWAY_PACKET_DEPLOY].act = act_deploy;//下发布防
packets_opt[GL_GATEWAY_PACKET_DEPLOY].get_packet_func = GL_NULL;	
packets_opt[GL_GATEWAY_PACKET_GWALARM].act = act_gwalarm;//下发网关联动报警
packets_opt[GL_GATEWAY_PACKET_GWALARM].get_packet_func = GL_NULL;
packets_opt[GL_GATEWAY_PACKET_LINK].act = act_link;//上报设备联动状态
packets_opt[GL_GATEWAY_PACKET_LINK].get_packet_func = GL_NULL;	

/**************************************************************************
*函数名      : gl_gateway_opt_get_deploy_packet
*作者        : 吴钊
*日期        : 2018/06/02
*功能描述    : 组成下发布防数据包
*参数        : 
              arg    : 网关基本数据信息
			  data   : 目的数据包
			  max_len: 数据包大小
*返回值      :
              包数据长度
**************************************************************************/	
static gl_uint32 gl_gateway_opt_get_deploy_packet(gl_gateway_packet_arg_t *arg, gl_uint8 *data, gl_uint32 max_len)
{
    gl_uint32 len = 0;
    
    len = snprintf(data, max_len, deploy_field, arg->arg.deploy.ucVal);

    return len;	
}	

/**************************************************************************
*函数名      : gl_gateway_opt_get_gwalarm_packet
*作者        : 吴钊
*日期        : 2018/06/02
*功能描述    : 组成下发网关联动报警数据包
*参数        : 
              arg    : 网关基本数据信息
			  data   : 目的数据包
			  max_len: 数据包大小
*返回值      :
              包数据长度
**************************************************************************/	
static gl_uint32 gl_gateway_opt_get_gwalarm_packet(gl_gateway_packet_arg_t *arg, gl_uint8 *data, gl_uint32 max_len)
{
	gl_uint32 len = 0;
    
    len = snprintf(data, max_len, gwalarm_field, arg->arg.gwalarm.ucVal);

    return len;
}	

/**************************************************************************
*函数名      : gl_gateway_opt_get_link_packet
*作者        : 吴钊
*日期        : 2018/06/02
*功能描述    : 组成上报设备联动状态数据包
*参数        : 
              arg    : 网关基本数据信息
			  data   : 目的数据包
			  max_len: 数据包大小
*返回值      :
              包数据长度
**************************************************************************/
static gl_uint32 gl_gateway_opt_get_link_packet(gl_gateway_packet_arg_t *arg, gl_uint8 *data, gl_uint32 max_len)
{
	gl_uint32 len = 0;
    
    len = snprintf(data, max_len, link_field, arg->arg.link.ucVal);

    return len;	
}		
	
	
	
	
	
	