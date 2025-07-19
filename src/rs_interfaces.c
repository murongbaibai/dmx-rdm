#include "dmx_rdm.h"

/*
  ******************************************************************************
  * @author  MURONGBAI 3216147898@qq.com
  * @brief   本代码由慕容白开发、遵循MIT许可协议、转载需标明本词条
  * @date    2025.6.19
  ******************************************************************************
*/
//DMX链路抽象结构体
static dmx_line_t *dmx_line_buf[8];
//解包函数
static void DMX_RDM_Unpack(dmx_line_t *dmx_line);
/******************初始化******************/
//定时器初始化
__weak void DMX_Tim_Init(void)
{
}
//DMX硬件初始化
__weak dmx_line_t * DMX_LIN1_UART_Init(void)
{
    return NULL;
}
__weak dmx_line_t *DMX_LIN2_UART_Init(void)
{
    return NULL;
}
__weak dmx_line_t *DMX_LIN3_UART_Init(void)
{
    return NULL;
}
__weak dmx_line_t *DMX_LIN4_UART_Init(void)
{
    return NULL;
}
__weak dmx_line_t *DMX_LIN5_UART_Init(void)
{
    return NULL;
}
__weak dmx_line_t *DMX_LIN6_UART_Init(void)
{
    return NULL;
}
__weak dmx_line_t *DMX_LIN7_UART_Init(void)
{
    return NULL;
}
__weak dmx_line_t *DMX_LIN8_UART_Init(void)
{
    return NULL;
}
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
__weak void DMX_Line_Close(dmx_line_t *dmx_line)
{
}
__weak void DMX_Line_Oped(dmx_line_t *dmx_line)
{
}
__weak void DMX_Receive_DMA_Stop(dmx_line_t *dmx_line)
{
}
__weak void DMX_Line_Receive_DMA(dmx_line_t *dmx_line, uint8_t *rx_buf, uint32_t data_len)
{
}
__weak void DMX_Line_Receive_IT(dmx_line_t *dmx_line, uint8_t *rx_buf, uint32_t data_len)
{
}
__weak void DMX_Line_Transmit_DMA(dmx_line_t *dmx_line, uint8_t *tx_buf, uint32_t data_len)
{
}
__weak void DMX_Line_Transmit_IT(dmx_line_t *dmx_line, uint8_t *tx_buf, uint32_t data_len)
{
}
#endif
__weak void DMX_UnpackComplete_Callback(dmx_line_t *dmx_line)
{
}
__weak void USER_DMX_UnpackComplete_Callback(dmx_line_t *dmx_line)
{
}
__weak void RDM_UnpackComplete_Callback(dmx_line_t *dmx_line)
{
}
//DMX+RDM+DEVICE初始化
void DMX_Init(void)
{
    //微秒级延时定时器
    DMX_Tim_Init();

#if (DEVICE_TYPE_SWITCH == DEVICE_TYPE_OUTPUT)
    //特有数据结构初始化
    RDM_Uid_Init();
#endif

    //根据开启的链路进行初始化
    switch(DMX_INPUT_OUPUT_SUM)
    {
        case 8:
            dmx_line_buf[7] = DMX_LIN8_UART_Init();
        case 7:
            dmx_line_buf[6] = DMX_LIN7_UART_Init();
        case 6:
            dmx_line_buf[5] = DMX_LIN6_UART_Init();
        case 5:
            dmx_line_buf[4] = DMX_LIN5_UART_Init();
        case 4:
            dmx_line_buf[3] = DMX_LIN4_UART_Init();
        case 3:
            dmx_line_buf[2] = DMX_LIN3_UART_Init();
        case 2:
            dmx_line_buf[1] = DMX_LIN2_UART_Init();
        case 1:
            dmx_line_buf[0] = DMX_LIN1_UART_Init();
        break;
        default:

        break;
    }

    for(int i = 0; i < DMX_INPUT_OUPUT_SUM; i++)
    {
#if (DEVICE_TYPE_SWITCH == DEVICE_TYPE_INPUT)
        DMX_Line_Mode_Set(dmx_line_buf[i], DMX_LINE_INPUT);
#else
        DMX_Line_Mode_Set(dmx_line_buf[i], DMX_LINE_DMX_OUTPUT);
#endif
    }
}
/* 
 * 统一更换模式接口
 * DMX_LINE_OFF ：         关闭发送接收
 * DMX_LINE_DMX_OUTPUT ：  DMX信号输出模式
 * DMX_LINE_BLACK_OUTPUT ：DMX信号全零输出模式
 * DMX_LINE_RDM_OUTPUT ：  RDM信号输出模式
 * DMX_LINE_INPUT ：       信号输入模式
 */
void DMX_Line_Mode_Set(dmx_line_t *dmx_line, uint8_t status)
{
#ifndef USE_HAL_DEVICE
    //重新开启串口
    if(dmx_line->dmx_status == DMX_LINE_OFF)
        DMX_Line_Oped(dmx_line);
#endif
    dmx_line->dmx_status = status;
    switch(status)
    {
        //转为接收然后关闭串口
        case DMX_LINE_OFF:
            dmx_line->change_mode_fun(DMX_RECV_DATA);
            DMX_Line_Close(dmx_line);
        break;
        //转为输入模式，并开启接收
        case DMX_LINE_INPUT:
            dmx_line->change_mode_fun(DMX_RECV_DATA);
            DMX_Line_Receive_DMA(dmx_line, dmx_line->rx_buf.buf, 1);
        break;
        //开启一次传输，然后自动续传
        case DMX_LINE_DMX_OUTPUT:
        case DMX_LINE_BLACK_OUTPUT:
#ifdef USE_HAL_DEVICE
            //如果在发送中，就会在发送完成中断中续传，不用打断发送
            if(dmx_line->huart->gState != HAL_UART_STATE_BUSY_TX)
                DMX_Send(dmx_line);                
#else
            DMX_Send(dmx_line);   
#endif
        break;
        //由其他接口实现RDM数据包发送
        case DMX_LINE_RDM_OUTPUT:

        break;
    }

}


//串口接收完成接口
void DMX_UART_RxCompleteHandle(dmx_line_t* dmx_line)
{
	//开启接收下一位
    dmx_line->rx_buf.index++;
	if(dmx_line->rx_buf.index >= 520)
		dmx_line->rx_buf.index = 0;
    //如果接收过程中关闭，就停止接收
    if(dmx_line->dmx_status != DMX_LINE_OFF)
        DMX_Line_Receive_DMA(dmx_line,&dmx_line->rx_buf.buf[dmx_line->rx_buf.index],1);
    else
        dmx_line->rx_buf.index = 0;
    //重新设置定时器计算包结束
    dmx_line->idle_tim.count = 0;
    dmx_line->idle_tim.state = DMX_TIM_START;
#if USE_REFRESH_TIM
    //重新设置刷新率检测定时器
    dmx_line->refresh_tim.count = 0;
    dmx_line->refresh_tim.state = DMX_TIM_START;
#endif
}


//串口断开信号接口
void DMX_UART_BreakHandle(dmx_line_t* dmx_line)
{
    /* 
     *  RDM协议二分搜索时会产生冲突包
     *  冲突会导致头字节错乱，此时需要将情况反映给二分搜索算法
     *  然而DMX数据链路上的其他协议也会导致识别为冲突，此时通过是否接收到起始信号进行区分
     */
    dmx_line->dmx_recv_break = 1;
    /* 
     *  如果空闲检测定时器没有结包 意味着DMX数据包连发，间隔时间过短
     *  此时以起始信号作为包的间隔，提前结包并且关闭空闲检测定时器
     */
	if(dmx_line->rx_buf.index > 1)
    {
        //关闭定时器
        dmx_line->idle_tim.count = 0;
        dmx_line->idle_tim.state = DMX_TIM_STOP;
        
        //提取数据包到二级缓冲区
        memcpy(dmx_line->dmx_rdm_package, dmx_line->rx_buf.buf, dmx_line->rx_buf.index);
        //清空计数值
        dmx_line->rx_buf.index = 0;
        //执行解包任务
        DMX_RDM_Unpack(dmx_line);
        dmx_line->dmx_recv_break = 0;
        return;
    }
    /* 
    *  收到起始信号代表新的包开始，不管之前是否结包
    *  都进行新一轮的数据包读取
    */
    /* 
    *  使用HAL库时，有几点不确定的特性
    *  1、接收BREAK信号会进入接收完成中断？，然后把BREAK数据识别成一个特殊的数据？这个数据没有实在意义，但会占一位数据接收位
    *  2、会先识别到接收完成中断，然后识别到断开信号中断
    *  3、开启DMA接收的第一位会丢失？但是接收数组的地址又增加了？
    */
}


//串口发送完成接口
void DMX_UART_TxCompleteHandle(dmx_line_t* dmx_line)
{
    //延时等待最后一位发出
    dmx_line->delay_nus_fun(88);
    //DMX数据包发送完成接口
    if(dmx_line->dmx_status == DMX_LINE_DMX_OUTPUT || dmx_line->dmx_status == DMX_LINE_BLACK_OUTPUT)
    {
#if DMX_PACKET_DELAY
        //开启下一轮接收
        DMX_Line_Mode_Set(dmx_line, DMX_LINE_INPUT);
        //设置发送延时定时器
        dmx_line->send_delay_tim.count = 0;
        dmx_line->send_delay_tim.state = DMX_TIM_START;
#else
        //继续发送下一包
        DMX_Send(dmx_line);
#endif
    }
    //RDM数据包发送完成接口
    else if(dmx_line->dmx_status == DMX_LINE_RDM_OUTPUT)
    {
        //开启下一轮接收
        DMX_Line_Mode_Set(dmx_line, DMX_LINE_INPUT);
    }
    //串口已被关闭，不继续接收
    else if(dmx_line->dmx_status == DMX_LINE_OFF)
    {

    }
}


//串口错误重启接口
void DMX_UART_ErrorRebootHandle(dmx_line_t* dmx_line)
{
    __HAL_UART_CLEAR_FEFLAG(dmx_line->huart);
    __HAL_UART_CLEAR_PEFLAG(dmx_line->huart);
    __HAL_UART_CLEAR_NEFLAG(dmx_line->huart);
    __HAL_UART_CLEAR_OREFLAG(dmx_line->huart);
    //发送错误
    if(dmx_line->dmx_status == DMX_LINE_DMX_OUTPUT || dmx_line->dmx_status == DMX_LINE_BLACK_OUTPUT || dmx_line->dmx_status == DMX_LINE_RDM_OUTPUT)
    {
        //明确停止DMA
        DMX_Send_DMA_Stop(dmx_line);
        //直接算作发送完成
        DMX_UART_TxCompleteHandle(dmx_line);
    }
    //接收错误
    else if(dmx_line->dmx_status == DMX_LINE_INPUT)
    {
        //重新设置定时器计算包结束
        dmx_line->idle_tim.count = 0;
        dmx_line->idle_tim.state = DMX_TIM_START;
        //清空计算值
        dmx_line->rx_buf.index = 0;
        //开启下一轮接收
        DMX_Line_Receive_DMA(dmx_line,dmx_line->rx_buf.buf,1);
        //设置接收错误标志位
        dmx_line->dmx_recv_error = 1;
    }
}


//空闲检测定时器时间到接口
static void DMX_Idle_Tim_CompleteHandle(dmx_line_t* dmx_line)
{
    //提取数据包到二级缓冲区
    memcpy(dmx_line->dmx_rdm_package, dmx_line->rx_buf.buf, dmx_line->rx_buf.index);
    //清空计数值
    dmx_line->rx_buf.index = 0;
    //执行解包任务
    DMX_RDM_Unpack(dmx_line);
    dmx_line->dmx_recv_break = 0;
}


//发送延时时间到接口
void DMX_Send_Delay_Tim_CompleteHandle(dmx_line_t* dmx_line)
{
    //如果还是在发送状态就续传数据包
    if(dmx_line->dmx_status != DMX_LINE_OFF)
        DMX_Line_Mode_Set(dmx_line, DMX_LINE_DMX_OUTPUT);
}


#if USE_REFRESH_TIM
//断开信号时间到到接口
__weak void DMX_Refresh_Tim_CompleteHandle(dmx_line_t* dmx_line)
{

}
#endif


//DMX任务处理接口
void DMX_TaskHandle(uint32_t nms)
{
    for(int i = 0; i < DMX_INPUT_OUPUT_SUM; i++)
    {
        //确认初始化完成
        if(dmx_line_buf[i] != NULL)
        {
            //增加定时器的时间
            if(dmx_line_buf[i]->idle_tim.state == DMX_TIM_START)
                dmx_line_buf[i]->idle_tim.count += nms;
            if(dmx_line_buf[i]->send_delay_tim.state == DMX_TIM_START)
                dmx_line_buf[i]->send_delay_tim.count += nms;
#if USE_REFRESH_TIM
            if(dmx_line_buf[i]->refresh_tim.state == DMX_TIM_START)
                dmx_line_buf[i]->refresh_tim.count += nms;
#endif

            //溢出判断
            if(dmx_line_buf[i]->idle_tim.count >= dmx_line_buf[i]->idle_tim.over)
            {
                dmx_line_buf[i]->idle_tim.count = 0;
                dmx_line_buf[i]->idle_tim.state = DMX_TIM_STOP;
                DMX_Idle_Tim_CompleteHandle(dmx_line_buf[i]);
            }
//确保开启了发送延时
#if SEND_DELAY_TIME_OVER
            if(dmx_line_buf[i]->send_delay_tim.count >= dmx_line_buf[i]->send_delay_tim.over)
            {
                dmx_line_buf[i]->send_delay_tim.count = 0;
                dmx_line_buf[i]->send_delay_tim.state = DMX_TIM_STOP;
                DMX_Send_Delay_Tim_CompleteHandle(dmx_line_buf[i]);
            }
#endif
#if USE_REFRESH_TIM
            if(dmx_line_buf[i]->refresh_tim.count >= dmx_line_buf[i]->refresh_tim.over)
            {
                dmx_line_buf[i]->refresh_tim.count = 0;
                dmx_line_buf[i]->refresh_tim.state = DMX_TIM_STOP;
                DMX_Refresh_Tim_CompleteHandle(dmx_line_buf[i]);
            }
#endif
        }

    }
}


/******************DMX512链路对外接口******************/
//创建新的DMX512链路
dmx_line_t * DMX_Line_Create(void)
{
#ifdef INC_FREERTOS_H
    dmx_line_t *dmx_line = (dmx_line_t *)pvPortMalloc(sizeof(dmx_line_t));
#else
    dmx_line_t *dmx_line = (dmx_line_t *)malloc(sizeof(dmx_line_t));
#endif
    dmx_line->idle_tim.over = IDLE_TIME_OVER;
    dmx_line->send_delay_tim.over = SEND_DELAY_TIME_OVER; 
#if USE_REFRESH_TIM
    dmx_line->refresh_tim.over = DMX_SIGNAL_BREAK_TIME; 
#endif
    memset(dmx_line->dmx_send_buf, 0, 513);
    dmx_line->rx_buf.buf[0] = 0x00;
     dmx_line->rx_buf.index = 0;
#if USE_DMX_USER_PRO
    //定义包头
    dmx_line->dmx_send_buf[0] = USER_PRO_START_CODE;
#endif

    return dmx_line;
}
//获取RDM解包结果
rdm_package_prase_t *DMX_Get_rdm_package_prase(dmx_line_t* dmx_line)
{
    if(dmx_line == NULL)
        return NULL;
    return &(dmx_line->rdm_package_prase);
}
//获取DMX解包结果
uint8_t* DMX_Get_dmx_package_prase(dmx_line_t* dmx_line)
{
#if (DEVICE_TYPE_SWITCH == DEVICE_TYPE_INPUT)
    if(dmx_line == NULL)
        return NULL;
    return dmx_line->dmx_package_prase;
#else
	return NULL;
#endif
}
#if USE_DMX_USER_PRO
//获取用户DMX解包结果
uint8_t* USER_DMX_Get_dmx_package_prase(dmx_line_t* dmx_line)
{
    if(dmx_line == NULL)
        return NULL;
    return dmx_line->user_pro_prase;
}
#endif
//获取刷新率







/******************组包函数******************/
//设置目的地UID
void RDM_Package_Set_Uid(uint64_t uid)
{
    rdm_package.des_uid = uid;
}
//设置命令
void RDM_Package_Set_Cmd(uint8_t cmd_type,uint16_t cmd)
{
    rdm_package.cmd_type = cmd_type;
    rdm_package.cmd = cmd;
}
//设置完data后要在内存释放前发送数据
void RDM_Package_Set_Data(uint8_t data_len,uint8_t data[])
{
    rdm_package.data_len = data_len;
    rdm_package.data = data;
}
//计算包长 校验位 然后发送数据
void RDM_Package_Send(dmx_line_t *dmx_line)
{
    //24 + data_len
    rdm_package.message_len = 24+rdm_package.data_len;

    dmx_line->rdm_send_buf[0] = rdm_package.start;
    dmx_line->rdm_send_buf[1] = rdm_package.sub_start;
    dmx_line->rdm_send_buf[2] = rdm_package.message_len;

    dmx_line->rdm_send_buf[3] = (rdm_package.des_uid >> 40) & 0xFF;
    dmx_line->rdm_send_buf[4] = (rdm_package.des_uid >> 32) & 0xFF;
    dmx_line->rdm_send_buf[5] = (rdm_package.des_uid >> 24) & 0xFF;
    dmx_line->rdm_send_buf[6] = (rdm_package.des_uid >> 16) & 0xFF;
    dmx_line->rdm_send_buf[7] = (rdm_package.des_uid >> 8) & 0xFF;
    dmx_line->rdm_send_buf[8] = rdm_package.des_uid & 0xFF;

    dmx_line->rdm_send_buf[9]  = (rdm_package.source_uid >> 40) & 0xFF;
    dmx_line->rdm_send_buf[10] = (rdm_package.source_uid >> 32) & 0xFF;
    dmx_line->rdm_send_buf[11] = (rdm_package.source_uid >> 24) & 0xFF;
    dmx_line->rdm_send_buf[12] = (rdm_package.source_uid >> 16) & 0xFF;
    dmx_line->rdm_send_buf[13] = (rdm_package.source_uid >> 8) & 0xFF;
    dmx_line->rdm_send_buf[14] = rdm_package.source_uid & 0xFF;

    dmx_line->rdm_send_buf[15] = (rdm_package.other >> 32) & 0xFF;
    dmx_line->rdm_send_buf[16] = (rdm_package.other >> 24) & 0xFF;
    dmx_line->rdm_send_buf[17] = (rdm_package.other >> 16) & 0xFF;
    dmx_line->rdm_send_buf[18] = (rdm_package.other >> 8) & 0xFF;
    dmx_line->rdm_send_buf[19] = rdm_package.other & 0xFF;

    dmx_line->rdm_send_buf[20] = rdm_package.cmd_type;
    dmx_line->rdm_send_buf[21] = (rdm_package.cmd >> 8) & 0xFF;
    dmx_line->rdm_send_buf[22] = rdm_package.cmd & 0xFF;

    dmx_line->rdm_send_buf[23] = rdm_package.data_len;
    uint8_t j = 0;

    for(int i = 24; i < rdm_package.message_len; i++)
    {
        dmx_line->rdm_send_buf[i] = rdm_package.data[j++];
    }

    //计算校验和
	rdm_package.check = 0;
    for(int i = 0; i < rdm_package.message_len; i++)
    {
        rdm_package.check += dmx_line->rdm_send_buf[i];
    }
    dmx_line->rdm_send_buf[rdm_package.message_len]   = (rdm_package.check >> 8) & 0xFF;
    dmx_line->rdm_send_buf[rdm_package.message_len+1] = rdm_package.check & 0xFF;

    DMX_Line_Mode_Set(dmx_line, DMX_LINE_RDM_OUTPUT);
    //起始信号
    dmx_line->change_mode_fun(DMX_SEND_RESET);
    dmx_line->send_reset_fun();

    //发送包
    dmx_line->change_mode_fun(DMX_SEND_DATA);
    //发送数据长度message_len+两位校验位
    DMX_Line_Transmit_DMA(dmx_line,dmx_line->rdm_send_buf,rdm_package.message_len+2);
}


/****************** 统一对外接口 ******************/
static void DMX_RDM_Unpack(dmx_line_t *dmx_line)
{
    if(dmx_line->dmx_recv_error)
    {
        dmx_line->dmx_recv_error = 0;
        return;
    }
    switch(dmx_line->dmx_rdm_package[1])
    {
        case 0x01:
            DMX_Unpack(dmx_line);
            //设置解包完成回调
            DMX_UnpackComplete_Callback(dmx_line);
        break;
#if USE_DMX_USER_PRO
        //帕灯主从机协议
        case USER_PRO_START_CODE:
            memcpy(dmx_line->user_pro_prase, dmx_line->dmx_rdm_package, USER_PRO_DATA_LEN);
            //设置解包完成回调
            USER_DMX_UnpackComplete_Callback(dmx_line);
        break;
#endif
        case 0xCC:
		case 0xFE:
            RDM_Unpack(dmx_line);
            //设置状态为RDM输出
            DMX_Line_Mode_Set(dmx_line, DMX_LINE_RDM_OUTPUT);
			//自动回应
#if (DEVICE_TYPE_SWITCH == DEVICE_TYPE_INPUT)                                                                                                                         
            switch(dmx_line->rdm_package_prase.rdm_package)
            {
                case DISC_UNIQUE:  
                    RDM_Disc_Driver_Respone(dmx_line);
                break;
                case DISC_MUTE:
                    RDM_Disc_Mute_Respone(dmx_line, dmx_line->rdm_package_prase.source_uid);
                    //进入哑音状态
                    device_info.mute = 1;
                break;
                case DISC_UN_MUTE:
                    RDM_Disc_Un_Mute_Respone(dmx_line, dmx_line->rdm_package_prase.source_uid);
                    //退出哑音状态
                    device_info.mute = 0;
                break;
                case GET_DRIVER_FLAG:
                    RDM_Get_Flag_Respone(dmx_line, dmx_line->rdm_package_prase.source_uid);
                break;
                case GET_DRIVER_DMX_ADDR:
                    RDM_Get_DMX_Addr_Respone(dmx_line, dmx_line->rdm_package_prase.source_uid);
                break;
                case GET_DRIVER_VERSION:
                    RDM_Get_Version_Respone(dmx_line, dmx_line->rdm_package_prase.source_uid);
                break;
                case GET_DRIVER_INFO:
                    RDM_Get_Info_Respone(dmx_line, dmx_line->rdm_package_prase.source_uid);
                break;
                case GET_DRIVER_MODE:
                    RDM_Get_Mode_Respone(dmx_line, dmx_line->rdm_package_prase.source_uid);
                break;
                case SET_DRIVER_FLAG:
                    RDM_Set_Flag_Respone(dmx_line, dmx_line->rdm_package_prase.source_uid);
                    device_info.flag = dmx_line->rdm_package_prase.data[0];
                break;
                case SET_DRIVER_DMX_ADDR:
                    RDM_Set_DMX_Addr_Respone(dmx_line, dmx_line->rdm_package_prase.source_uid);
                    device_info.info.dmx_start_addr = dmx_line->rdm_package_prase.data[0];
                    device_info.info.dmx_start_addr <<= 8;
                    device_info.info.dmx_start_addr |= dmx_line->rdm_package_prase.data[1];
                break;
                case SET_DRIVER_MODE:
                    RDM_Set_Mode_Respone(dmx_line, dmx_line->rdm_package_prase.source_uid);
                    device_info.info.dmx_cur_mode = dmx_line->rdm_package_prase.data[0];
                    //修改通道值
                    switch(device_info.info.dmx_cur_mode)
                    {
                        case 1:
                            device_info.info.dmx_channel = DMX_MODE1_CHANNEL;
                        break;
                        case 2:
                            device_info.info.dmx_channel = DMX_MODE2_CHANNEL;
                        break;
                    }
                break;
                default:
                    
                break;
            }
#endif 
            RDM_UnpackComplete_Callback(dmx_line);
        break;
        default:
            //如果没有收到起始信号、判断为RDM广播包回应冲突
            if(dmx_line->dmx_recv_break == 0)
            {
                dmx_line->rdm_package_prase.rdm_package = ERROR_PACKAGE;
				RDM_UnpackComplete_Callback(dmx_line);
            }
        break;
    }
}

