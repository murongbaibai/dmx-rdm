#ifndef DMX_H
#define DMX_H

/**
 * @brief 发送DMX512数据帧
 * @param dmx_line 目标链路实例
 * @note 包含BREAK信号+数据发送完整流程
 */
void dmx_send(dr_line_t *dr_line);

/**
 * @brief 解析DMX512数据帧
 * @param dmx_line 目标链路实例
 * @note 自动跳过起始码，提取有效通道数据
 */
void dmx_unpack(dr_line_t *dr_line);

#endif
