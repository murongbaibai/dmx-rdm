#ifndef DMX_H
#define DMX_H

/**************************************** 结构体定义 ****************************************/



/**************************************** 变量外部声明 ****************************************/



/**************************************** 函数声明 ****************************************/
/// @brief 发送前回调函数，用于修改发送数组
/// @param dmx_line DMX数据链路
__weak void DMX_Send_Before_Callback(dmx_line_t *dmx_line);

/// @brief 发送DMX数据包
/// @param dmx_line DMX数据链路
void DMX_Send(dmx_line_t *dmx_line);

/// @brief 解析DMX数据包
/// @param dmx_line DMX数据链路
void DMX_Unpack(dmx_line_t *dmx_line);

#endif
