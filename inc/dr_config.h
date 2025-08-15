#ifndef DR_CONFIG_H
#define DR_CONFIG_H
/*
  ******************************************************************************
  * @author  MURONGBAI 3216147898@qq.com
  * @brief   本代码由慕容白开发、遵循MIT许可协议、转载需标明本词条
  * @date    2025.6.24
  ******************************************************************************
*/

/**
 * @file dmx_config.h
 * @brief DMX/RDM协议栈核心配置参数
 */

/* 硬件抽象层配置 -----------------------------------------------------------*/
//指定内存管理函数
#define DR_MEM_INCLUDE "stdlib.h"
#define DR_MALLOC malloc
#define DR_FREE   free

/* 协议栈基础配置 ---------------------------------------------------------*/
#define DMX_MAX_LINE          4     ///< 最大并发DMX链路数（内存占用：接收模式，每链路约2.8KB.发送模式，每链路约3.6K）

/* RDM设备标识配置 */
#define RDM_VENDOR_ID         0x19BA                   //厂商ID
#define RDM_DEVICE_ID         *(uint32_t *)0x0801FFF0  //设备ID

/* 定时器参数（单位：ms） */
#define IDLE_TIM_OVER         10    ///< 链路空闲检测超时阈值（ms）
#define SEND_DELAY_TIM_OVER   0     ///< 数据包间延迟（ms）
#define REFRESH_TIM_OVER      500   ///< 链路信号断开检测时间（ms）
#define RDM_RECV_TIM_OVER     40    ///< RDM接收超时时间（ms）
#define SEND_TIMEOUT_TIM_OVER 300   ///< 发送超时时间（ms） 


/* DMX通道模式配置 
 * @note 通道数必须满足 ANSI E1.11-2008 标准
 */
#define USE_DMX_QUICK_UNPACK  1     ///< 快速解包模式（中断中完成DMX解包,开启多路时对主频有一定要求）
#define DMX_MODE1_CHANNEL     6     ///< 模式1通道数（6CH）
#define DMX_MODE2_CHANNEL     8     ///< 模式2通道数（8CH）
#define DMX_MODE3_CHANNEL     0     ///< 模式3通道数
#define DMX_MODE4_CHANNEL     0     ///< 模式4通道数
#define DMX_MODE5_CHANNEL     0     ///< 模式5通道数
#define DMX_MODE6_CHANNEL     0     ///< 模式6通道数
#define DMX_MODE7_CHANNEL     0     ///< 模式7通道数
#define DMX_MODE8_CHANNEL     0     ///< 模式8通道数

#define DMX_CHANNEL_MAX       DMX_MODE2_CHANNEL  ///< 设备最大通道数（范围：1-512）
#define RDM_MODE_MAX          2     ///< 设备支持的模式数（范围：1-8）

/* 输出设备配置 ---------------------------------------------------------*/
#define RDM_SEND_QUEUE_LENGTH 50    ///< RDM发送队列深度（单位：数据包）

#endif /* DMX_CONFIG_H */
