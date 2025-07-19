#ifndef RS_INTERFACES_H
#define RS_INTERFACES_H

#define MOTOR_NUM 10
/**************************************** 结构体定义 ****************************************/
//接收数组定义
typedef struct rx_buf_t
{
    uint8_t buf[520];
    uint16_t index;
}rx_buf_t;

//软件定时器状态定义
enum dmx_tim_state{DMX_TIM_STOP = 0, DMX_TIM_START};
//软件定时器结构体定义
typedef struct dmx_tim_t
{
    uint8_t state;
    uint16_t count;
    uint16_t over;
}dmx_tim_t;

//DMX发送接收状态定义
//             发送起始信号    发送数据       接收数据      接收起始信号
enum dmx_mode {DMX_SEND_RESET,DMX_SEND_DATA,DMX_RECV_DATA,DMX_RECV_RESET};
//DMX链路状态定义
//             关闭链路       链路DMX信号输出       链路DMX黑场输出         链路RDM信号输出       链路接收信号
enum dmx_flag {DMX_LINE_OFF, DMX_LINE_DMX_OUTPUT, DMX_LINE_BLACK_OUTPUT, DMX_LINE_RDM_OUTPUT, DMX_LINE_INPUT};


//发送改变模式函数
typedef void (*DMX_Change_Mode_Fun)(enum dmx_mode mode);
//发送起始信号函数
typedef void (*DMX_Send_Reset_Fun)(void);
//微秒级延时函数
typedef void (*DMX_Delay_Nus_Fun)(uint16_t nus);

//DMX512链路主结构体定义
typedef struct dmx_line_t
{
    //设备状态标志位
    uint8_t dmx_status;
    //接收起始信号标志位
    uint8_t dmx_recv_break;
    //接收错误标志位
    uint8_t dmx_recv_error;
    //一级缓冲区 串口接收数据缓冲区
    rx_buf_t rx_buf;
    //一级缓冲区 串口DMX数据包发送缓冲区
    uint8_t dmx_send_buf[513];
    //一级缓冲区 串口RDM数据包发送缓冲区
    uint8_t rdm_send_buf[256];
    //二级缓冲区 DMX/RDM包缓冲区
    uint8_t dmx_rdm_package[520];
    uint8_t dmx_rdm_package_ne;

    /*********!!! 不同于标准的DMX协议，是作为半主机的变种 !!!*********/
// #if (DEVICE_TYPE_SWITCH == DEVICE_TYPE_INPUT)
//     //三级缓冲区 DMX解包结果缓冲区
//     uint8_t dmx_package_prase[DMX_CHANNEL_MAX];
// #endif
    //三级缓冲区 DMX解包结果缓冲区
    uint8_t dmx_package_prase[MOTOR_NUM];


    //三级级缓冲区 RDM解包结果缓冲区
    rdm_package_prase_t rdm_package_prase;
#if USE_DMX_USER_PRO
    //三级缓冲区 用户自定义协议解包结果
    uint8_t user_pro_prase[USER_PRO_DATA_LEN];
#endif

    //DMX系统软件定时器
    dmx_tim_t idle_tim;
    dmx_tim_t send_delay_tim;
    #if USE_REFRESH_TIM
    dmx_tim_t refresh_tim;
    #endif

    //切换接收/发送的函数
    DMX_Change_Mode_Fun change_mode_fun;
    //发送起始信号函数
    DMX_Send_Reset_Fun send_reset_fun;
    //微秒级延时函数
    DMX_Delay_Nus_Fun delay_nus_fun;
#ifdef USE_HAL_DRIVER
    //HAL库串口
    UART_HandleTypeDef *huart;
#endif

}dmx_line_t;



/**************************************** 变量外部声明 ****************************************/


/**************************************** 函数声明 ****************************************/
__weak void DMX_Tim_Init(void);
__weak dmx_line_t * DMX_LIN1_UART_Init(void);
__weak dmx_line_t * DMX_LIN2_UART_Init(void);
__weak dmx_line_t * DMX_LIN3_UART_Init(void);
__weak dmx_line_t * DMX_LIN4_UART_Init(void);
__weak dmx_line_t * DMX_LIN5_UART_Init(void);
__weak dmx_line_t * DMX_LIN6_UART_Init(void);
__weak dmx_line_t * DMX_LIN7_UART_Init(void);
__weak dmx_line_t * DMX_LIN8_UART_Init(void);
#ifdef USE_HAL_DEVICE
//重定义函数
#define DMX_Line_Close(dmx_line)                   HAL_UART_Abort((dmx_line)->huart)
#define DMX_Receive_DMA_Stop(dmx_line)             HAL_DMA_Abort((dmx_line)->huart->hdmarx)
#define DMX_Send_DMA_Stop(dmx_line)                HAL_DMA_Abort((dmx_line)->huart->hdmatx)
#define DMX_Line_Receive_IT(dmx_line, buf, size)   HAL_UART_Receive_IT((dmx_line)->huart, buf, size)
#define DMX_Line_Receive_DMA(dmx_line, buf, size)  HAL_UART_Receive_DMA((dmx_line)->huart, buf, size)
#define DMX_Line_Transmit_IT(dmx_line, buf, size)  HAL_UART_Transmit_IT((dmx_line)->huart, buf, size)
#define DMX_Line_Transmit_DMA(dmx_line, buf, size) HAL_UART_Transmit_DMA((dmx_line)->huart, buf, size)
#else
//如果没有HAL库需要实现以下接口
__weak void DMX_Line_Close(dmx_line_t *dmx_line);
__weak void DMX_Line_Oped(dmx_line_t *dmx_line);
__weak void DMX_Receive_DMA_Stop(dmx_line_t *dmx_line);
__weak void DMX_Line_Receive_DMA(dmx_line_t *dmx_line, uint8_t *rx_buf, uint32_t data_len);
__weak void DMX_Line_Receive_IT(dmx_line_t *dmx_line, uint8_t *rx_buf, uint32_t data_len);
__weak void DMX_Line_Transmit_DMA(dmx_line_t *dmx_line, uint8_t *tx_buf, uint32_t data_len);
__weak void DMX_Line_Transmit_IT(dmx_line_t *dmx_line, uint8_t *tx_buf, uint32_t data_len);
#endif
__weak void DMX_UnpackComplete_Callback(dmx_line_t *dmx_line);
__weak void USER_DMX_UnpackComplete_Callback(dmx_line_t *dmx_line);
__weak void RDM_UnpackComplete_Callback(dmx_line_t *dmx_line);

/// @brief 初始化所有DMX数据链路
/// @param 无
void DMX_Init(void);

/// @brief 设置DMX数据链路模式
/// @param dmx_line DMX数据链路
/// @param status   模式
void DMX_Line_Mode_Set(dmx_line_t *dmx_line, uint8_t status);

/// @brief 串口接收完成接口
/// @param dmx_line DMX数据链路
void DMX_UART_RxCompleteHandle(dmx_line_t* dmx_line);

/// @brief 串口断开信号接口
/// @param dmx_line DMX数据链路
void DMX_UART_BreakHandle(dmx_line_t* dmx_line);

/// @brief 串口发送完成接口
/// @param dmx_line DMX数据链路
void DMX_UART_TxCompleteHandle(dmx_line_t* dmx_line);

/// @brief 串口错误重启接口
/// @param dmx_line DMX数据链路
void DMX_UART_ErrorRebootHandle(dmx_line_t* dmx_line);

/// @brief DMX512任务处理接口
/// @param nms 系统定时器时基
void DMX_TaskHandle(uint32_t nms);

/// @brief 创建新的DMX512链路
/// @param  无
/// @return 初始化好的DMX数据链路
dmx_line_t * DMX_Line_Create(void);

/// @brief 获取RDM解包结果
/// @param dmx_line DMX数据链路
/// @return         RDM解包后结构体
rdm_package_prase_t* DMX_Get_rdm_package_prase(dmx_line_t* dmx_line);

/// @brief 获取DMX解包结果
/// @param dmx_line DMX数据链路
/// @return         DMX解包后数组
uint8_t* DMX_Get_dmx_package_prase(dmx_line_t* dmx_line);

#if USE_DMX_USER_PRO
/// @brief 获取用户DMX解包结果
/// @param dmx_line DMX数据链路
/// @return         用户DMX解包后数组
uint8_t* USER_DMX_Get_dmx_package_prase(dmx_line_t* dmx_line);
#endif


/// @brief 设置RDM发送包UID
/// @param uid UID
void RDM_Package_Set_Uid(uint64_t uid);

/// @brief 设置RDM发送包类型
/// @param cmd_type 命令类型
/// @param cmd 具体命令
void RDM_Package_Set_Cmd(uint8_t cmd_type,uint16_t cmd);

/// @brief 设置RDM发送包数据
/// @param data_len 数据长度
/// @param data 数据内容
void RDM_Package_Set_Data(uint8_t data_len,uint8_t data[]);

/// @brief 将设置好的数据包发送出去
/// @param dmx_line 发送到哪个DMX数据链路
void RDM_Package_Send(dmx_line_t *dmx_line);

#endif
