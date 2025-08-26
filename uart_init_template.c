#include "main.h"

/* 
 *    DMX+RDM协议需要实现的接口:
 *          //串口接收完成接口
 *          void dr_uart_rxcomplete_handle(dr_line_t *dr_line);
 *          //串口断开信号接口
 *          void dr_break_handle(dr_line_t *dr_line);
 *          //串口发送完成接口
 *          void dr_uart_txcomplete_handle(dr_line_t *dr_line);
 *          //串口错误重启接口
 *          void dr_uart_error_handle(dr_line_t *dr_line);
 *          //DMX+RDM心跳接口
 *          void dr_tick_inc(uint8_t nms);
 *          //DMX+RDM任务处理接口
 *          void dr_task_handle(void);
 *    需要实现的操作集合
 *          dr_line_funs_t funs;
 */


/************************ DMX/UART硬件配置 ************************/
#if 0
/*
 * DMX LIN 1 硬件引脚配置
 * TX  PB8 (UART3)
 * RX  PB9 (UART3) 
 * RDE PF9 (方向控制)
 */
#define DR_LINE1_UART_CLK_ENABLE()     __HAL_RCC_USART3_CLK_ENABLE()
#define DR_LINE1_DMA_CLK_ENABLE()      __HAL_RCC_DMA_CLK_ENABLE()
#define DR_LINE1_UART                  USART3

/* GPIO 引脚配置 */
#define DR_LINE1_RDE_PORT              GPIOF
#define DR_LINE1_RDE_PIN               GPIO_PIN_9
#define DR_LINE1_TX_PORT               GPIOB  
#define DR_LINE1_TX_PIN                GPIO_PIN_8
#define DR_LINE1_TX_AF                 GPIO_AF10_USART3
#define DR_LINE1_RX_PORT               GPIOB
#define DR_LINE1_RX_PIN                GPIO_PIN_9
#define DR_LINE1_RX_AF                 GPIO_AF10_USART3

/* DMA 通道配置 */
#define DR_LINE1_TX_DMA_CHANNEL        DMA1_Channel1
#define DR_LINE1_TX_DMA_CHANNEL_MAP    DMA_CHANNEL_MAP_USART3_WR

/* 中断配置 */  
#define DR_LINE1_UART_IRQN             USART3_4_IRQn
#define DR_LINE1_UART_IRQHANDLER       USART3_4_IRQHandler
#define DR_LINE1_TX_DMA_IRQN           DMA1_Channel1_IRQn
#define DR_LINE1_TX_DMA_IRQHANDLER     DMA1_Channel1_IRQHandler

/*
 * DMX LIN 2 硬件引脚配置
 * TX  PA2 (UART2)
 * RX  PA3 (UART2) 
 * RDE PA1 (方向控制)
 */
#define DR_LINE2_UART_CLK_ENABLE()     __HAL_RCC_USART2_CLK_ENABLE()
#define DR_LINE2_DMA_CLK_ENABLE()      __HAL_RCC_DMA_CLK_ENABLE()
#define DR_LINE2_UART                  USART2

/* GPIO 引脚配置 */
#define DR_LINE2_RDE_PORT              GPIOA
#define DR_LINE2_RDE_PIN               GPIO_PIN_1
#define DR_LINE2_TX_PORT               GPIOA  
#define DR_LINE2_TX_PIN                GPIO_PIN_2
#define DR_LINE2_TX_AF                 GPIO_AF1_USART2
#define DR_LINE2_RX_PORT               GPIOA
#define DR_LINE2_RX_PIN                GPIO_PIN_3
#define DR_LINE2_RX_AF                 GPIO_AF1_USART2

/* DMA 通道配置 */
#define DR_LINE2_TX_DMA_CHANNEL        DMA1_Channel2
#define DR_LINE2_TX_DMA_CHANNEL_MAP    DMA_CHANNEL_MAP_USART2_WR

/* 中断配置 */  
#define DR_LINE2_UART_IRQN             USART2_IRQn
#define DR_LINE2_UART_IRQHANDLER       USART2_IRQHandler
#define DR_LINE2_TX_DMA_IRQN           DMA1_Channel2_3_IRQn
#define DR_LINE2_TX_DMA_IRQHANDLER     DMA1_Channel2_3_IRQHandler

/************************ DMX接口实现 ************************/

/* 全局变量声明 */
dr_line_t *dr_line1;
static UART_HandleTypeDef dr_line1_uart;
static DMA_HandleTypeDef dr_line1_tx_dma; 

dr_line_t *dr_line2;
static UART_HandleTypeDef dr_line2_uart;
static DMA_HandleTypeDef dr_line2_tx_dma; 

/* 方向控制宏定义 */
#define DMX_LIN1_RECV() HAL_GPIO_WritePin(DR_LINE1_RDE_PORT, DR_LINE1_RDE_PIN, GPIO_PIN_RESET)
#define DMX_LIN1_SEND() HAL_GPIO_WritePin(DR_LINE1_RDE_PORT, DR_LINE1_RDE_PIN, GPIO_PIN_SET)

#define DMX_LIN2_RECV() HAL_GPIO_WritePin(DR_LINE2_RDE_PORT, DR_LINE2_RDE_PIN, GPIO_PIN_RESET)
#define DMX_LIN2_SEND() HAL_GPIO_WritePin(DR_LINE2_RDE_PORT, DR_LINE2_RDE_PIN, GPIO_PIN_SET)

/**
 * @brief 改变DMX线路工作模式
 * @param dr_line DMX线路指针
 * @param mode 工作模式
 * @note 控制收发方向切换
 */
static void dr_line_change_mode(dr_line_t *dr_line, enum dr_mode mode)
{
    if(dr_line == dr_line1) {
        switch(mode) {
            case DR_SEND_DATA:
            case DR_SEND_RESET:
                DMX_LIN1_SEND();  // 设置为发送模式
                break;
                
            case DR_RECV_DATA: 
            case DR_RECV_RESET:
                DMX_LIN1_RECV();  // 设置为接收模式
                break;
        }
    }
    else if(dr_line == dr_line2) {
        switch(mode) {
            case DR_SEND_DATA:
            case DR_SEND_RESET:
                DMX_LIN2_SEND();  // 设置为发送模式
                break;
                
            case DR_RECV_DATA: 
            case DR_RECV_RESET:
                DMX_LIN2_RECV();  // 设置为接收模式
        }
    }
}

/**
 * @brief 发送BREAK起始信号
 * @param dr_line DMX线路指针
 * @note 用于DMX帧同步
 */
static void dr_line_send_reset(dr_line_t *dr_line)
{
    if(dr_line == dr_line1) {
        // 触发BREAK信号
        DR_LINE1_UART->CR1 |= USART_CR1_SBK;
        
        // 等待BREAK发送完成
        while(DR_LINE1_UART->CR1 & USART_CR1_SBK);
    }
    else if(dr_line == dr_line2) {
        // 触发BREAK信号
        DR_LINE2_UART->CR1 |= USART_CR1_SBK;
        
        // 等待BREAK发送完成
        while(DR_LINE2_UART->CR1 & USART_CR1_SBK);
    }
}

/**
 * @brief 关闭DMX线路
 * @param dr_line DMX线路指针 
 */
static void dr_line_close(dr_line_t *dr_line)
{
    if(dr_line == dr_line1) {
        //关闭BREAK中断
        DR_LINE1_UART->CR2 &= ~USART_CR2_LBDIE;
        // 中止当前UART传输
        HAL_UART_Abort(&dr_line1_uart);  
    }
    else if(dr_line == dr_line2) {
        //关闭BREAK中断
        DR_LINE2_UART->CR2 &= ~USART_CR2_LBDIE;
        // 中止当前UART传输
        HAL_UART_Abort(&dr_line2_uart);  
    }
}

/**
 * @brief 发送数据
 * @param dr_line DMX线路指针
 * @param data 发送数据缓冲区
 * @param size 数据长度
 */
static void dr_line_transmit(dr_line_t *dr_line, uint8_t *data, uint32_t size)
{
    if(dr_line == dr_line1) {
        HAL_UART_Transmit_DMA(&dr_line1_uart, data, size);  // DMA方式发送
    }
    else if(dr_line == dr_line2) {
        HAL_UART_Transmit_DMA(&dr_line2_uart, data, size);  // DMA方式发送
    }
}

/**
 * @brief 接收数据
 * @param dr_line DMX线路指针
 * @param data 接收数据缓冲区
 * @param size 接收数据长度
 */
static void dr_line_receive(dr_line_t *dr_line, uint8_t *data, uint32_t size)
{
    if(dr_line == dr_line1) {
        // 开启BREAK中断
        DR_LINE1_UART->CR2 |= USART_CR2_LBDIE;
        HAL_UART_Receive_IT(&dr_line1_uart, data, size);  // IT方式接收
    }
    else if(dr_line == dr_line2) {
        // 开启BREAK中断
        DR_LINE2_UART->CR2 |= USART_CR2_LBDIE;
        HAL_UART_Receive_IT(&dr_line2_uart, data, size);  // IT方式接收
    }
}

/************************ 单路DMX初始化实现 ************************/
//DMX+RDM串口初始化
void dr_line1_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    //使能相关外设时钟
    DR_LINE1_UART_CLK_ENABLE();
    DR_LINE1_DMA_CLK_ENABLE();

    GPIO_InitStruct.Pin = DR_LINE1_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = DR_LINE1_TX_AF; 
    HAL_GPIO_Init(DR_LINE1_TX_PORT, &GPIO_InitStruct);
    //复用映射可能不同！！！
    GPIO_InitStruct.Alternate = DR_LINE1_RX_AF; 
    GPIO_InitStruct.Pin = DR_LINE1_RX_PIN;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DR_LINE1_RX_PORT, &GPIO_InitStruct);
    
    //配置RDE引脚
    GPIO_InitStruct.Pin = DR_LINE1_RDE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DR_LINE1_RDE_PORT, &GPIO_InitStruct);
    
    //配置USART
    dr_line1_uart.Instance = DR_LINE1_UART;
    dr_line1_uart.Init.BaudRate = 250000;
    dr_line1_uart.Init.WordLength = UART_WORDLENGTH_8B;
    dr_line1_uart.Init.StopBits = UART_STOPBITS_2;
    dr_line1_uart.Init.Parity = UART_PARITY_NONE;
    dr_line1_uart.Init.Mode = UART_MODE_TX_RX;
    dr_line1_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    dr_line1_uart.Init.OverSampling = UART_OVERSAMPLING_8;
    HAL_UART_Init(&dr_line1_uart);

    //配置DMA发送
    dr_line1_tx_dma.Instance = DR_LINE1_TX_DMA_CHANNEL;
    dr_line1_tx_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    dr_line1_tx_dma.Init.PeriphInc = DMA_PINC_DISABLE;
    dr_line1_tx_dma.Init.MemInc = DMA_MINC_ENABLE;
    dr_line1_tx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    dr_line1_tx_dma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    dr_line1_tx_dma.Init.Mode = DMA_NORMAL;
    dr_line1_tx_dma.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&dr_line1_tx_dma);
    __HAL_LINKDMA(&dr_line1_uart, hdmatx, dr_line1_tx_dma);
    HAL_DMA_ChannelMap(&dr_line1_tx_dma, DR_LINE1_TX_DMA_CHANNEL_MAP);

    
    //开启BREAK中断
    DR_LINE1_UART->CR2 |= USART_CR2_LBDIE;
    //开启LIN模式
    DR_LINE1_UART->CR2 |= USART_CR2_LINEN;
    //检测11位低电平为断开
    DR_LINE1_UART->CR2 |= UART_LINBREAKDETECTLENGTH_11B;

    //配置NVIC
    HAL_NVIC_SetPriority(DR_LINE1_UART_IRQN, 0, 0);
    HAL_NVIC_EnableIRQ(DR_LINE1_UART_IRQN);
    HAL_NVIC_SetPriority(DR_LINE1_TX_DMA_IRQN, 0, 0);
    HAL_NVIC_EnableIRQ(DR_LINE1_TX_DMA_IRQN);

    dr_line_funs_t funs = {
        .change_mode = &dr_line_change_mode,
        .send_reset = &dr_line_send_reset,
        .close = &dr_line_close,
        .transmit = &dr_line_transmit,
        .receive = &dr_line_receive,
    };
    dr_line_funs_create(&funs);

    dr_line1 = dr_line_create();

    if(dr_line1 != NULL)
        dr_line_mode_set(dr_line1, DMX_INPUT);
}

/************************ 双路DMX初始化实现 ************************/
void dr_line2_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    //使能相关外设时钟
    DR_LINE2_UART_CLK_ENABLE();
    DR_LINE2_DMA_CLK_ENABLE();

    GPIO_InitStruct.Pin = DR_LINE2_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = DR_LINE2_TX_AF; 
    HAL_GPIO_Init(DR_LINE2_TX_PORT, &GPIO_InitStruct);
    //复用映射可能不同！！！
    GPIO_InitStruct.Alternate = DR_LINE2_RX_AF; 
    GPIO_InitStruct.Pin = DR_LINE2_RX_PIN;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DR_LINE2_RX_PORT, &GPIO_InitStruct);
    
    //配置RDE引脚 (PB5)
    GPIO_InitStruct.Pin = DR_LINE2_RDE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DR_LINE2_RDE_PORT, &GPIO_InitStruct);
    
    //配置USART
    dr_line2_uart.Instance = DR_LINE2_UART;
    dr_line2_uart.Init.BaudRate = 250000;
    dr_line2_uart.Init.WordLength = UART_WORDLENGTH_8B;
    dr_line2_uart.Init.StopBits = UART_STOPBITS_2;
    dr_line2_uart.Init.Parity = UART_PARITY_NONE;
    dr_line2_uart.Init.Mode = UART_MODE_TX_RX;
    dr_line2_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    dr_line2_uart.Init.OverSampling = UART_OVERSAMPLING_8;
    HAL_UART_Init(&dr_line2_uart);

    //配置DMA发送
    dr_line2_tx_dma.Instance = DR_LINE2_TX_DMA_CHANNEL;
    dr_line2_tx_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    dr_line2_tx_dma.Init.PeriphInc = DMA_PINC_DISABLE;
    dr_line2_tx_dma.Init.MemInc = DMA_MINC_ENABLE;
    dr_line2_tx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    dr_line2_tx_dma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    dr_line2_tx_dma.Init.Mode = DMA_NORMAL;
    dr_line2_tx_dma.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&dr_line2_tx_dma);
    __HAL_LINKDMA(&dr_line2_uart, hdmatx, dr_line2_tx_dma);
    HAL_DMA_ChannelMap(&dr_line2_tx_dma, DR_LINE2_TX_DMA_CHANNEL_MAP);

    
    //开启BREAK中断
    DR_LINE2_UART->CR2 |= USART_CR2_LBDIE;
    //开启LIN模式
    DR_LINE2_UART->CR2 |= USART_CR2_LINEN;
    //检测11位低电平为断开
    DR_LINE2_UART->CR2 |= UART_LINBREAKDETECTLENGTH_11B;

    //配置NVIC
    HAL_NVIC_SetPriority(DR_LINE2_UART_IRQN, 0, 0);
    HAL_NVIC_EnableIRQ(DR_LINE2_UART_IRQN);
    HAL_NVIC_SetPriority(DR_LINE2_TX_DMA_IRQN, 0, 0);
    HAL_NVIC_EnableIRQ(DR_LINE2_TX_DMA_IRQN);

    dr_line_funs_t funs = {
        .change_mode = &dr_line_change_mode,
        .send_reset = &dr_line_send_reset,
        .close = &dr_line_close,
        .transmit = &dr_line_transmit,
        .receive = &dr_line_receive,
    };
    dr_line_funs_create(&funs);

    dr_line2 = dr_line_create();

    if(dr_line2 != NULL)
        dr_line_mode_set(dr_line2, DMX_INPUT);
}

/***** 中断服务函数 *****/
/***
 * 使用HAL_Transmit_DMA()时，HAL库会更换发送完成逻辑，在DMA传输完成中断中开启串口传输完成中断
 * 但是在最后一个字节传输完成前，串口已经触发过传输完成中断，导致传输完成中断提前触发，此时如果进入发送完成回调函数，将会导致最后一位数据丢失
 * 简单的解决办法是等待TC置位，然后再进入发送完成完成回调函数，但是在开启多路时可能会导致其他路串口溢出
 * 所以需要修改HAL库的这个函数  UART_DMATransmitCplt
 * 
 *  CLEAR_BIT(huart->Instance->CR3, USART_CR3_DMAT);
 *
 *  在这两行中加入下面这个判断，这样在DMA传输过程中误触发了传输完成标志位，也能够清除
 *  if(huart->Instance == USART1)
 *  {
 *     __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_TC);
 *  }
 *
 *  SET_BIT(huart->Instance->CR1, USART_CR1_TCIE);
 *
 *
 *  这样就可以在正确时机由串口触发  HAL_UART_TxCpltCallback(huart); 回调函数
 *
 */
//DMA 中断
void DR_LINE1_TX_DMA_IRQHANDLER(void)
{
    HAL_DMA_IRQHandler(&dr_line1_tx_dma);
}

void DR_LINE2_TX_DMA_IRQHANDLER(void)
{
    HAL_DMA_IRQHandler(&dr_line2_tx_dma);
}

//串口中断
void DR_LINE1_UART_IRQHANDLER(void)
{
    //实现断开信号检测接口
    if(dr_line1_uart.Instance->CR2 & USART_CR2_LBDIE)
    {
        if(dr_line1_uart.Instance->SR & USART_SR_LBD)
        {
            //清除标志位
            __HAL_UART_CLEAR_FLAG(&dr_line1_uart,USART_SR_LBD);
            dr_break_handle(dr_line1);
        }
    }

    HAL_UART_IRQHandler(&dr_line1_uart);
}

void DR_LINE2_UART_IRQHANDLER(void)
{
    //实现断开信号检测接口
    if(dr_line2_uart.Instance->CR2 & USART_CR2_LBDIE)
    {
        if(dr_line2_uart.Instance->SR & USART_SR_LBD)
        {
            //清除标志位
            __HAL_UART_CLEAR_FLAG(&dr_line2_uart,USART_SR_LBD);
            dr_break_handle(dr_line2);
        }
    }

    HAL_UART_IRQHandler(&dr_line2_uart);
}

//实现发送完成接口
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == DR_LINE1_UART)
		dr_uart_txcomplete_handle(dr_line1);
    else if(huart->Instance == DR_LINE2_UART)
        dr_uart_txcomplete_handle(dr_line2);
}
//实现接收完成接口
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == DR_LINE1_UART)
		dr_uart_rxcomplete_handle(dr_line1);
    else if(huart->Instance == DR_LINE2_UART)
        dr_uart_rxcomplete_handle(dr_line2);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    __HAL_UART_CLEAR_FEFLAG(huart);
    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_PEFLAG(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
	//实现错误重启接口
	if(huart->Instance == DR_LINE1_UART)
		dr_uart_error_handle(dr_line1);
    else if(huart->Instance == DR_LINE2_UART)
        dr_uart_error_handle(dr_line2);
}


/******************************** 对用户开放的回调函数 ********************************/
//发送前回调函数，用于修改发送数组
void dmx_before_send_callback(dr_line_t *dr_line)
{    

}
//RDM解包完成回调函数
void rdm_unpack_complete_callback(dr_line_t *dr_line)
{

}
//DMX解包完成回调函数
void dmx_unpack_complete_callback(dr_line_t *dr_line)
{

}

#endif


