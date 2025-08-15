#include "../inc/dr_internal.h"

uint64_t rdm_uid = 0;

/**
 * @brief 设置目标UID
 * @param uid 目标设备UID
 */
void rdm_package_set_uid(dr_line_t *dr_line, uint64_t uid)
{
    __DR_TX_BUF(dr_line)[3] = (uid >> 40) & 0xFF;
    __DR_TX_BUF(dr_line)[4] = (uid >> 32) & 0xFF;
    __DR_TX_BUF(dr_line)[5] = (uid >> 24) & 0xFF;
    __DR_TX_BUF(dr_line)[6] = (uid >> 16) & 0xFF;
    __DR_TX_BUF(dr_line)[7] = (uid >> 8)  & 0xFF;
    __DR_TX_BUF(dr_line)[8] = (uid >> 0)  & 0xFF;
}

/**
 * @brief 设置RDM命令
 * @param cmd_type 命令类型
 * @param cmd 具体命令
 */
void rdm_package_set_cmd(dr_line_t *dr_line, uint8_t cmd_type, uint16_t cmd)
{
    __DR_TX_BUF(dr_line)[20] = cmd_type;
    __DR_TX_BUF(dr_line)[21] = (cmd >> 8) & 0xFF;
    __DR_TX_BUF(dr_line)[22] = (cmd >> 0) & 0xFF;
}

/**
 * @brief 设置RDM数据
 * @param data_len 数据长度
 * @param data 数据指针
 * @note 数据必须在 rdm_package_send 前保持有效
 */
void rdm_package_set_data(dr_line_t *dr_line, uint8_t data_len, uint8_t data[])
{
    __DR_TX_BUF(dr_line)[23] = data_len;

    for(int i = 0; i < data_len; i++) {
        __DR_TX_BUF(dr_line)[24 + i] = data[i];
    }
}

/**
 * @brief 发送RDM数据包
 * @param dr_line DMX+RDM链路指针
 */
void rdm_package_send(dr_line_t *dr_line)
{
    // 计算包长度(24字节头 + 数据长度)
    uint16_t message_len = 24 + __DR_TX_BUF(dr_line)[23];

    // 填充包头数据
    __DR_TX_BUF(dr_line)[0] = 0xCC;
    __DR_TX_BUF(dr_line)[1] = 0x01;
    __DR_TX_BUF(dr_line)[2] = message_len;

    // 填充源UID(6字节)
    __DR_TX_BUF(dr_line)[9]  = (rdm_uid >> 40) & 0xFF;
    __DR_TX_BUF(dr_line)[10] = (rdm_uid >> 32) & 0xFF;
    __DR_TX_BUF(dr_line)[11] = (rdm_uid >> 24) & 0xFF;
    __DR_TX_BUF(dr_line)[12] = (rdm_uid >> 16) & 0xFF;
    __DR_TX_BUF(dr_line)[13] = (rdm_uid >> 8)  & 0xFF;
    __DR_TX_BUF(dr_line)[14] = (rdm_uid >> 0)  & 0xFF;

    // 填充其他字段(5字节)
    __DR_TX_BUF(dr_line)[15] = 0x00;
    __DR_TX_BUF(dr_line)[16] = 0x01;
    __DR_TX_BUF(dr_line)[17] = 0x00;
    __DR_TX_BUF(dr_line)[18] = 0x00;
    __DR_TX_BUF(dr_line)[19] = 0x00;

    // 计算校验和(2字节)
    uint32_t check = 0;
    for(int i = 0; i < message_len; i++) {
        check += __DR_TX_BUF(dr_line)[i];
    }
    __DR_TX_BUF(dr_line)[message_len]   = (check >> 8) & 0xFF;
    __DR_TX_BUF(dr_line)[message_len+1] = (check >> 0) & 0xFF;

    //设置为发送模式
    __DR_SEND_STATUS(dr_line) = 1;
    
    // 发送起始信号
    dr_funs.change_mode(dr_line, DR_SEND_RESET);
    dr_funs.send_reset(dr_line);

    // 发送数据包
    dr_funs.change_mode(dr_line, DR_SEND_DATA);
    dr_funs.transmit(dr_line, __DR_TX_BUF(dr_line), message_len+2);
}

/**
 * @brief 添加RDM包到发送队列
 * @param dr_line DMX+RDM链路指针
 * @param package RDM包类型
 * @param target_uid 目标设备UID指针(uid_list表示广播)
 * @param callback 发送完成回调函数
 */
void rdm_send(dr_line_t *dr_line, enum rdm_package package, uid_t *target_uid)
{
    // 检查队列是否已满
    if(__DR_RDM_QUEUE_FULL(dr_line))
    {
        rdm_send_finish(dr_line, RDM_ERROR_QUEUE_FULL);
        return;
    }

    // 填充队列元素
    __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_TAIL(dr_line)].package = package;
    __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_TAIL(dr_line)].target_uid = target_uid;

    // 移动尾指针(循环队列)
    __DR_RDM_QUEUE_TAIL(dr_line) = (__DR_RDM_QUEUE_TAIL(dr_line) + 1) % RDM_SEND_QUEUE_LENGTH;

    // 更新队列满状态标志
    __DR_RDM_QUEUE_FULL(dr_line) = (__DR_RDM_QUEUE_TAIL(dr_line) == __DR_RDM_QUEUE_HEAD(dr_line));
}

/**
 * @brief 从发送队列取出并发送RDM包
 * @param dr_line DMX+RDM链路指针
 */
void rdm_send_queue_pop(dr_line_t *dr_line)
{
    // 根据包类型调用相应处理函数
    switch(__DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].package)
    {
        case DISC_MUTE:
            rdm_disc_mute(dr_line, __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid);
            break;
        case DISC_UN_MUTE:
            rdm_disc_un_mute(dr_line, __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid);
            break;
        case SET_DRIVER_DMX_ADDR:
            rdm_set_dmx_addr(dr_line, __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid);
            break;
        case SET_DRIVER_FLAG:
            rdm_set_flag(dr_line, __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid);
            break;
        case SET_DRIVER_MODE:
            rdm_set_mode(dr_line, __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid);
            break;
        case GET_DRIVER_DMX_ADDR:
            rdm_get_dmx_addr(dr_line, __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid);
            break;
        case GET_DRIVER_FLAG:
            rdm_get_flag(dr_line, __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid);
            break;
        case GET_DRIVER_INFO:
            rdm_get_info(dr_line, __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid);
            break;
        case GET_DRIVER_MODE:
            rdm_get_mode(dr_line, __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid);
            break;
        case GET_DRIVER_MODE_INFO:
            rdm_get_mode_info(dr_line, __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid);
            break;
        case GET_DRIVER_VERSION:
            rdm_get_version(dr_line, __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid);
            break;    
        // 未知包类型处理
        default:
            rdm_send_finish(dr_line, RDM_ERROR_UNNOWM_PACKET);
        break;
    }
    // 记录最后发送的包类型
    __DR_RDM_LAST_PACKAGE(dr_line) = __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].package;
}

/**
 * @brief RDM发送完成处理
 * @param dr_line DMX+RDM链路指针
 * @param errors 错误状态
 */
void rdm_send_finish(dr_line_t *dr_line, enum rdm_output_errors errors)
{
    // 如果发送队列非空，来自队列发送
    if((__DR_RDM_QUEUE_HEAD(dr_line) != __DR_RDM_QUEUE_TAIL(dr_line) && __DR_RDM_QUEUE_FULL(dr_line) == 0) || 
        __DR_RDM_QUEUE_FULL(dr_line) == 1)
    {
        switch(errors)
        {
            //发送成功，取消发送
            case RDM_ERROR_NULL:
                //发送回应成功回调
                rdm_device_respone_callback(dr_line, 
                    __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid, 
                    __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].package);

                    // 移动头指针(循环队列)
                    __DR_RDM_QUEUE_HEAD(dr_line) = (__DR_RDM_QUEUE_HEAD(dr_line) + 1) % RDM_SEND_QUEUE_LENGTH;
                    // 清除队列满状态
                    __DR_RDM_QUEUE_FULL(dr_line) = 0;
            break;
            // 发送队列已满，取消发送
            case RDM_ERROR_QUEUE_FULL:

            break;
            // 不支持的包类型，取消发送
            case RDM_ERROR_UNNOWM_PACKET:
                // 移动头指针(循环队列)
                __DR_RDM_QUEUE_HEAD(dr_line) = (__DR_RDM_QUEUE_HEAD(dr_line) + 1) % RDM_SEND_QUEUE_LENGTH;
                // 清除队列满状态
                __DR_RDM_QUEUE_FULL(dr_line) = 0;
            break;

            //回应超时取消发送
            case RDM_ERROR_RESPONE_TIMEOUT:
            //解包失败取消发送
            case RDM_ERROR_UNPACK_FAILD:
                // 移动头指针(循环队列)
                __DR_RDM_QUEUE_HEAD(dr_line) = (__DR_RDM_QUEUE_HEAD(dr_line) + 1) % RDM_SEND_QUEUE_LENGTH;
                // 清除队列满状态
                __DR_RDM_QUEUE_FULL(dr_line) = 0;
            break;
        }
    }
}

/**
 * @brief RDM设备离线回调(弱定义)
 * @param dr_line DMX+RDM链路指针
 * @param p 设备UID指针
 * @weak 默认实现为空函数
 */
__weak void rdm_device_offline_callback(dr_line_t *dr_line, uid_t *p){}

/**
 * @brief RDM设备添加回调(弱定义) 
 * @param dr_line DMX+RDM链路指针
 * @param p 设备UID指针
 * @weak 默认实现为空函数
 */
__weak void rdm_device_add_callback(dr_line_t *dr_line, uid_t *p){}

/**
 * @brief RDM设备上线回调(弱定义)
 * @param dr_line DMX+RDM链路指针 
 * @param p 设备UID指针
 * @weak 默认实现为空函数
 */
__weak void rdm_device_online_callback(dr_line_t *dr_line, uid_t *p){}

/**
 * @brief RDM设备回应回调(弱定义)
 * @param dr_line DMX+RDM链路指针
 * @param p 设备UID指针
 * @param package 回应的数据包类型
 * @weak 默认实现为空函数
 */
__weak void rdm_device_respone_callback(dr_line_t *dr_line, uid_t *p, uint16_t package){}
/**
 * @brief RDM设备搜索成功处理
 * @param dr_line DMX+RDM链路指针
 */
void rdm_disc_success(dr_line_t *dr_line)
{
    uint64_t uid = 0;
    
    // 相同UID情况处理
    if(__DR_RDM_STACK_DEPTH(dr_line) > 0 && 
        __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].low_uid == 
        __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].high_uid)
    {
        uid = __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].low_uid;
    }
    // 正常响应处理
    else 
    {
        // 解析UID(6字节)
        for(int j = 0; j < 5; j++)
        {
            uid |= __DR_RDM_PARSE(dr_line).data[j];
            uid <<= 8;
        }
        uid |= __DR_RDM_PARSE(dr_line).data[5];
    }
    
    // 搜索设备UID
    uid_t *p = rdm_uid_search(dr_line, uid);
    
    // 新设备处理
    if(p == NULL)
    {
        // 添加新设备
        rdm_uid_add(dr_line, uid);
        p = rdm_uid_search(dr_line, uid);
        p -> online_flag = 1;
        // 设备添加回调
        rdm_device_add_callback(dr_line, p);
    }
    // 离线设备重新上线
    else if(p->online_flag == 0)
    {
        p->online_flag = 1;
        // 设备上线回调
        rdm_device_online_callback(dr_line, p);
    }
    
    // 哑音设备
    rdm_send(dr_line, DISC_MUTE, p);
    
    //移除当前分支
    if(__DR_RDM_STACK_DEPTH(dr_line) > 0)
        __DR_RDM_STACK_DEPTH(dr_line)--;
}

/**
 * @brief RDM搜索冲突处理
 * @param dr_line DMX+RDM链路指针
 * @note 触发条件:
 *       1. 未收到起始信号且接收错误
 *       2. 未收到起始信号且解包错误
 */
void rdm_disc_conflict(dr_line_t *dr_line)
{
    if(__DR_RDM_STACK_DEPTH(dr_line) == 0)
        while(1);
    // 相同UID冲突处理
    if(__DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].low_uid == 
        __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].high_uid)
    {
        rdm_disc_success(dr_line);
        return;
    }
    
    uint64_t mid_uid;
    
    // 计算中间UID
    mid_uid = __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].low_uid + 
             ((__DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].high_uid - 
              __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].low_uid)>>1);

    // 设置右分支
    __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)].low_uid = mid_uid+1;
    __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)].high_uid = 
    __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].high_uid;

    // 覆盖左分支
    __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].high_uid = mid_uid;

    // 栈深度+1
    __DR_RDM_STACK_DEPTH(dr_line)++;
    
    // 下次处理右分支...
}

/**
 * @brief RDM搜索超时处理
 * @param dr_line DMX+RDM链路指针
 */
void rdm_disc_timeout(dr_line_t *dr_line)
{
    // 如果是唯一UID搜索处理超时
    if(__DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].low_uid ==
       __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].high_uid)
    {
        uid_t *p = rdm_uid_search(dr_line, __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].low_uid);

        if(p != NULL && p->online_flag == 1)
        {
            p->online_flag = 0;
            rdm_device_offline_callback(dr_line, p);
        }
    }
    // 移除当前搜索分支
    if(__DR_RDM_STACK_DEPTH(dr_line) > 0)
        __DR_RDM_STACK_DEPTH(dr_line)--;
    // 下次处理左分支...
}

