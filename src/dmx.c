#include "dmx_rdm.h"

__weak void DMX_Send_Before_Callback(dmx_line_t *dmx_line)
{
}

static uint8_t dmx_black_buf[513] = {0};
//DMX发送 发送包越短，发送的越快，代表对设备的刷新率越高
void DMX_Send(dmx_line_t *dmx_line)
{
    DMX_Send_Before_Callback(dmx_line);
    //起始信号
    dmx_line->change_mode_fun(DMX_SEND_RESET);
    dmx_line->send_reset_fun();

    //发送包
    dmx_line->change_mode_fun(DMX_SEND_DATA);
#if DMX_ZHAN
    uint8_t start_code = 0x00;
    HAL_UART_Transmit(dmx_line,&start_code,1,HAL_MAX_DELAY);
    for(int i = 0; i < 512; i++)
    {
        HAL_UART_Transmit(dmx_line,&dmx_send_buf[i],1,HAL_MAX_DELAY);
        //延时产生占
        dmx_line->delay_nus_fun(DMX_ZHAN);
    }
#else
#ifdef USER_PRO_DATA_LEN
    DMX_Line_Transmit_DMA(dmx_line,dmx_line->dmx_send_buf,USER_PRO_DATA_LEN);
#else
    if(dmx_line->dmx_status == DMX_LINE_DMX_OUTPUT)
        DMX_Line_Transmit_DMA(dmx_line,dmx_line->dmx_send_buf,513);
    else if(dmx_line->dmx_status == DMX_LINE_BLACK_OUTPUT)
        DMX_Line_Transmit_DMA(dmx_line,dmx_black_buf,513);
#endif
#endif
}



//DMX解包
void DMX_Unpack(dmx_line_t *dmx_line)
{
    /*********!!! 不同于标准的DMX协议，是作为半主机的变种 !!!*********/
// #if (DEVICE_TYPE_SWITCH == DEVICE_TYPE_INPUT)
//     memcpy(dmx_line->dmx_package_prase, &dmx_line->dmx_rdm_package[device_info.info.dmx_start_addr], device_info.info.dmx_channel);
// #endif
    memcpy(dmx_line->dmx_package_prase, &dmx_line->dmx_rdm_package[2], MOTOR_NUM);
}

