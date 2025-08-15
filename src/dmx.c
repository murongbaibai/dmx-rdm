#include "../inc/dr_internal.h"

static const uint8_t dmx_black_buf[513] = {0};  ///< 黑场数据缓冲区（全0数据包）

/**
 * @brief DMX发送前回调函数(弱定义)
 * @param dr_line DMX+RDM链路指针
 * @note 用户可重写此函数实现发送前自定义处理
 * @weak 默认实现为空函数
 */
__weak void dmx_before_send_callback(dr_line_t *dr_line)
{
    // 默认空实现
}

/**
 * @brief 发送DMX数据包
 * @param dr_line DMX+RDM链路指针
 * @note 发送流程:
 *       1. 触发发送前回调
 *       2. 发送BREAK起始信号
 *       3. 发送数据包(根据状态选择有效数据或黑场)
 */
void dmx_send(dr_line_t *dr_line)
{
    // 前置处理回调
    dmx_before_send_callback(dr_line);

    //设置为发送模式
    __DR_SEND_STATUS(dr_line) = 1;
    
    // 发送BREAK起始信号
    dr_funs.change_mode(dr_line, DR_SEND_RESET);
    dr_funs.send_reset(dr_line);

    // 切换至数据发送模式
    dr_funs.change_mode(dr_line, DR_SEND_DATA);

    // 根据链路状态选择数据内容
    if(__DR_DMX_OUTPUT_BLACK(dr_line) == 0) {
        dr_funs.transmit(dr_line, __DR_TX_BUF(dr_line), 513);  // 发送有效数据
    }
    else if(__DR_DMX_OUTPUT_BLACK(dr_line) == 1) {
        dr_funs.transmit(dr_line, (uint8_t *)dmx_black_buf, 513);  // 发送黑场数据
    }
}

/**
 * @brief DMX数据解包
 * @param dr_line DMX+RDM链路指针
 * @note 解包规则:
 *       - 跳过前1字节头
 *       - 根据设备通道数确定有效数据长度
 */
void dmx_unpack(dr_line_t *dr_line)
{
    memcpy(__DR_DMX_PARSE(dr_line), 
           &__DR_RX_BUF(dr_line)[__DR_DMX_ADDR(dr_line)],    // 按DMX地址确定读取的首地址
           __DR_DMX_CHANNEL(dr_line));                       // 按设备通道数确定长度
}
