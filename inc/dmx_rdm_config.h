#ifndef DMX_RDM_CONFIG_H
#define DMX_RDM_CONFIG_H
/*
  ******************************************************************************
  * @author  MURONGBAI 3216147898@qq.com
  * @brief   本代码由慕容白开发、遵循MIT许可协议、转载需标明本词条
  * @date    2025.6.24
  ******************************************************************************
*/

#define DMX_INPUT_OUPUT_SUM  1  //选择开启几路DMX信号输出


#define DEVICE_TYPE_INPUT    0   
#define DEVICE_TYPE_OUTPUT   1
#define DEVICE_TYPE_SWITCH   DEVICE_TYPE_OUTPUT  //选择设备类型 

#define RDM_DEFAULT_UID      0x19BA00000000 /* 设置默认UID */
#if (DEVICE_TYPE_SWITCH == DEVICE_TYPE_INPUT)
    #define USE_REFRESH_TIM  0       /* 刷新率计算模块 Refresh rate calculation */
    #if USE_REFRESH_TIM
        #define DMX_SIGNAL_BREAK_TIME 500  /* 信号断开检测 */
    #endif
    
    #define DMX_MODE1_CHANNEL 6       
    #define DMX_MODE2_CHANNEL 8       
    #define DMX_MODE3_CHANNEL 0       
    #define DMX_MODE4_CHANNEL 0       
    #define DMX_MODE5_CHANNEL 0       
    #define DMX_MODE6_CHANNEL 0       
    #define DMX_MODE7_CHANNEL 0       
    #define DMX_MODE8_CHANNEL 0    

    #define DMX_CHANNEL_MAX  DMX_MODE2_CHANNEL      /* 设备最大的通道数 MIN=1 MAX=512 */
    #define RDM_MODE_MAX     2                      /* 设备最大的模式数 MIN=1 MAX=8 */
#endif

#define DMX_ZHAN             0  //DMX数据包帧间隔
#define DMX_PACKET_DELAY     200  //DMX数据包包间隔
#define IDLE_TIME_OVER       4  //空闲检测定时器检测时长
#define SEND_DELAY_TIME_OVER DMX_PACKET_DELAY //发包延时定时器检测时长


//扩展协议模块
#define USE_DMX_USER_PRO            0
#if USE_DMX_USER_PRO
    //用户扩展协议数据包头
    #define USER_PRO_START_CODE  0x01
    //用户扩展协议数据包长
    #define USER_PRO_DATA_LEN    520
#endif




/***************** 接收端配置 Receiver Configuration *****************/


#endif /* DMX_CONFIG_H */
