#include "main.h"

/*
  ******************************************************************************
  * @author  MURONGBAI 3216147898@qq.com
  * @brief   本代码由慕容白开发、遵循MIT许可协议、转载需标明本词条
  * @date    2025.6.24
  ******************************************************************************
*/
/* 
 *    要实现DMX512协议，需要完成以下两个工作:
 *
 *    NO1、DMX512协议需要实现的接口:
 *          根据宏定义 DMX_INPUT_OUPUT_SUM 定义的链路个数，来实现相应的硬件初始化
 *          dmx_line_t *DMX_LINX_UART_Init(void);
 *          实现模式切换函数
 *          DMX_Change_Mode_Fun DMX_LINX_Change_Mode;
 *          实现起始信号产生函数
 *          DMX_Send_Reset_Fun DMX_LINX_Send_Reset;
 *
 *          微秒级定时器初始化
 *          void DMX_Tim_Init(void);
 *          微秒级定时器延时
 *          void DMX_Tim_Delay_Nus(uint16_t nus)
 *    NO1(扩展)、如果没有移植HAL库还需实现以下接口
 *          关闭发送接收功能
 *          void DMX_Line_Close(dmx_line_t *dmx_line)
 *          开启发送接收功能
 *          void DMX_Line_Oped(dmx_line_t *dmx_line)
 *          接收指定字节 DMA/IT 两种接收方式二选一
 *          void DMX_Line_Receive_DMA(dmx_line_t *dmx_line, uint8_t *rx_buf, uint32_t sum)
 *          void DMX_Line_Receive_IT(dmx_line_t *dmx_line, uint8_t *rx_buf, uint32_t sum)
 *          发送指定字节 DMA/IT 两种发送方式二选一
 *          void DMX_Line_Transmit_DMA(dmx_line_t *dmx_line, uint8_t *tx_buf, uint32_t sum)
 *          void DMX_Line_Transmit_IT(dmx_line_t *dmx_line, uint8_t *tx_buf, uint32_t sum)
 * 
 *    NO2、DMX512协议需要调用的回调函数:
 *          //串口接收完成接口
 *          void DMX_UART_RxCompleteHandle(dmx_line_t* dmx_line);
 *          //串口断开信号接口
 *          void DMX_UART_BreakHandle(dmx_line_t* dmx_line);
 *          //串口发送完成接口
 *          void DMX_UART_TxCompleteHandle(dmx_line_t* dmx_line);
 *          //串口错误重启接口
 *          void DMX_UART_ErrorRebootHandle(dmx_line_t* dmx_line);
 *          //DMX512任务处理接口
 *          void DMX_TaskHandle(void);
 */




/******************************** 微秒级延时初始化示例代码 ********************************/
//DMX定时器初始化
//定义TIM句柄
static TIM_HandleTypeDef htim14;
void DMX_Tim_Init(void)
{
    // 使能TIM14时钟
    __HAL_RCC_TIM14_CLK_ENABLE();
    
    htim14.Instance = TIM14;
    htim14.Init.Prescaler = 72 - 1;  // 72MHz/72 = 1MHz (1us计数一次)
    htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim14.Init.Period = 0xFFFF - 1; // 最大计数值(16位定时器)
    htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    HAL_TIM_Base_Init(&htim14);

    // 启动定时器
    HAL_TIM_Base_Start(&htim14);
}
//微秒级延时函数实现
static void DMX_Tim_Delay_Nus(uint16_t nus)
{
    __HAL_TIM_SET_COUNTER(&htim14, 0);
    
    // 等待计数器达到指定值
    while(__HAL_TIM_GET_COUNTER(&htim14) < nus);
}

/******************************** 单路DMX初始化示例代码 ********************************/
dmx_line_t *dmx_line1;

#define DMX_LIN1_RECV HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET)
#define DMX_LIN1_SEND HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET)
//改变模式
static void DMX_LIN1_Change_Mode(enum dmx_mode mode)
{
    //DMX发送
    if(mode == DMX_SEND_DATA)
    {
        DMX_LIN1_SEND;
    }
    //DMX输出起始信号
    else if(mode == DMX_SEND_RESET)
    {
        DMX_LIN1_SEND;
    }
    //DMX接收数据
    else if(mode == DMX_RECV_DATA)
    {
        DMX_LIN1_RECV;
    }
    //DMX接收起始信号
    else if(mode == DMX_RECV_RESET)
    {
        DMX_LIN1_RECV;
    }
}
//产生起始信号
static void DMX_LIN1_Send_Reset(void)
{
    //发送起始信号
    USART2->CR1 |= USART_CR1_SBK;
    //等待起始信号发送完毕
    while(USART2->CR1&USART_CR1_SBK);
}


//DMX串口初始化
/*
 *  T1
 *  TX  PA4 (UART2)
 *  RX  PA3 (UART2)
 *  RDE PA5
 */
static UART_HandleTypeDef huart2;
static DMA_HandleTypeDef hdma_usart2_tx;
static DMA_HandleTypeDef hdma_usart2_rx;
dmx_line_t *DMX_LIN1_UART_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    //使能相关外设时钟
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_DMA_CLK_ENABLE();

    //PA4 as USART2_TX, PA3 as USART2_RX
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_USART2; 
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    //复用映射不同！！！
    GPIO_InitStruct.Alternate = GPIO_AF1_USART2; 
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    //配置RDE引脚 (PA5)
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    //配置USART
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 250000;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_2;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_8;
    HAL_UART_Init(&huart2);

    //配置DMA发送
    hdma_usart2_tx.Instance = DMA1_Channel1;
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdma_usart2_tx);
    __HAL_LINKDMA(&huart2, hdmatx, hdma_usart2_tx);
    HAL_DMA_ChannelMap(&hdma_usart2_tx, DMA_CHANNEL_MAP_USART2_WR);

    //配置DMA接收
    hdma_usart2_rx.Instance = DMA1_Channel2;
    hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx.Init.Mode = DMA_NORMAL;
    hdma_usart2_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    HAL_DMA_Init(&hdma_usart2_rx);
    __HAL_LINKDMA(&huart2, hdmarx, hdma_usart2_rx);
    HAL_DMA_ChannelMap(&hdma_usart2_rx, DMA_CHANNEL_MAP_USART2_RD);

    

    //开启BREAK中断
    USART2->CR2 |= USART_CR2_LBDIE;
    //开启LIN模式
    USART2->CR2 |= USART_CR2_LINEN;
    // //检测11位低电平为断开
    USART2->CR2 |= UART_LINBREAKDETECTLENGTH_11B;

    //配置NVIC
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

    dmx_line1 = DMX_Line_Create();
    //绑定改变模式函数
    dmx_line1->change_mode_fun = &DMX_LIN1_Change_Mode;
    //绑定产生起始信号函数
    dmx_line1->send_reset_fun = &DMX_LIN1_Send_Reset;
    //绑定延时函数
    dmx_line1->delay_nus_fun = &DMX_Tim_Delay_Nus;
    //绑定串口
    dmx_line1->huart = &huart2; 

    return dmx_line1;
}

/***** 中断服务函数 *****/
//DMX TX
void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart2_tx);
}
//DMX_RX
void DMA1_Channel2_3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart2_rx);
}
//DMX串口中断
void USART2_IRQHandler(void)
{
    //如果长时间没有中断就重启
	HAL_IWDG_Refresh(&hiwdg);
    //实现断开信号检测接口
    if(huart2.Instance->CR2 & USART_CR2_LBDIE)
    {
        if(huart2.Instance->SR & USART_SR_LBD)
        {
            //清除标志位
            __HAL_UART_CLEAR_FLAG(&huart2,USART_SR_LBD);
            DMX_UART_BreakHandle(dmx_line1);
        }
    }

    HAL_UART_IRQHandler(&huart2);
}
/******************************** 单路DMX初始化示例代码结束 ********************************/

//实现发送完成接口
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART2)
		DMX_UART_TxCompleteHandle(dmx_line1);
}
//实现接收完成接口
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART2)
		DMX_UART_RxCompleteHandle(dmx_line1);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	//实现错误重启接口
	if(huart->Instance == USART2)
		DMX_UART_ErrorRebootHandle(dmx_line1);
}


/******************************** 对用户开放的回调函数（在中断环境中回调、代码需要简短） ********************************/
dmx_line_t *rdm_unpack_ok_line = NULL;

static inline uint8_t check(uint8_t data)
{
    uint8_t sum = 0;
    for(int i = 0; i < 7; i++)
        if(data&(1<<i))
            sum++;
    //奇数个1，校验位是1。偶数个1，校验位是0
    return sum%2 == (data>>7)&0x01;
}
static inline uint8_t compute(uint8_t data)
{
    uint8_t sum = 0;
    for(int i = 0; i < 7; i++)
        if(data&(1<<i))
            sum++;
    return data | ((sum%2)<<7);
}
//发送前回调函数，用于修改发送数组
void DMX_Send_Before_Callback(dmx_line_t *dmx_line)
{    
    dmx_line->dmx_send_buf[0] = 0x00;
    for(int i = 0; i < MOTOR_NUM; i++)
        dmx_line->dmx_send_buf[i+1] = compute(motor_status[i]);
}

//RDM解包完成回调函数
void RDM_UnpackComplete_Callback(dmx_line_t *dmx_line)
{
    rdm_unpack_ok_line = dmx_line;
}
//DMX解包完成回调函数
void DMX_UnpackComplete_Callback(dmx_line_t *dmx_line)
{
    //按键按下后屏蔽一包，等待更新完成
    if(key_update_flag)
    {
        key_update_flag = 0;
        return;
    }
    //根据收到的数据包设置自身的状态
    for(int i = 0; i < MOTOR_NUM; i++)
    {
        if(check(dmx_line->dmx_package_prase[i]) == 0)
            return;
        dmx_line->dmx_package_prase[i] &= 0x7F;
        //按键值发生变化
        if(motor_status[i] != dmx_line->dmx_package_prase[i])
        {
            //如果当前房间不是溢出房间，或者没有启用溢出功能
            if(spill_motor-1 != i || !spill_flag)
            {
                //更新状态
                motor_status[i] = dmx_line->dmx_package_prase[i];
            }
            //如果在熄屏中
            if(standby_flag)
            {
                led_exit_standby_mode();
                //取消按键屏蔽
                key_mask = 0;
            }
            else
            {
                //重新熄屏计时
                led_run_time = 0;
                led_set_status(i, motor_status[i]);
            }
            //保存参数
            motor_arg_save();
        }
    }
}
#ifdef USE_REFRESH_TIM
//断开信号时间到回调
void DMX_Refresh_Tim_CompleteHandle(dmx_line_t* dmx_line)
{

}
#endif

