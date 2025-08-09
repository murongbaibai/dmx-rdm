#include "../inc/dr_internal.h"

/**
 * @brief RDM数据包默认配置
 */
rdm_package_t rdm_package = {
    .start = 0xCC,
    .sub_start = 0x01,
    .source_uid = RDM_DEFAULT_UID,
    .other = 0x0000000001000000,
    .check = 0,
};

/**
 * @brief 设置目标UID
 * @param uid 目标设备UID
 */
void rdm_package_set_uid(uint64_t uid)
{
    rdm_package.des_uid = uid;
}

/**
 * @brief 设置RDM命令
 * @param cmd_type 命令类型
 * @param cmd 具体命令
 */
void rdm_package_set_cmd(uint8_t cmd_type, uint16_t cmd)
{
    rdm_package.cmd_type = cmd_type;
    rdm_package.cmd = cmd;
}

/**
 * @brief 设置RDM数据
 * @param data_len 数据长度
 * @param data 数据指针
 * @note 数据必须在 rdm_package_send 前保持有效
 */
void rdm_package_set_data(uint8_t data_len, uint8_t data[])
{
    rdm_package.data_len = data_len;
    rdm_package.data = data;
}

/**
 * @brief 发送RDM数据包
 * @param dr_line DMX+RDM链路指针
 */
void rdm_package_send(dr_line_t *dr_line)
{
    // 计算包长度(24字节头 + 数据长度)
    rdm_package.message_len = 24 + rdm_package.data_len;

    // 填充包头数据
    __DR_TX_BUF(dr_line)[0] = rdm_package.start;
    __DR_TX_BUF(dr_line)[1] = rdm_package.sub_start;
    __DR_TX_BUF(dr_line)[2] = rdm_package.message_len;

    // 填充目标UID(6字节)
    __DR_TX_BUF(dr_line)[3] = (rdm_package.des_uid >> 40) & 0xFF;
    __DR_TX_BUF(dr_line)[4] = (rdm_package.des_uid >> 32) & 0xFF;
    __DR_TX_BUF(dr_line)[5] = (rdm_package.des_uid >> 24) & 0xFF;
    __DR_TX_BUF(dr_line)[6] = (rdm_package.des_uid >> 16) & 0xFF;
    __DR_TX_BUF(dr_line)[7] = (rdm_package.des_uid >> 8) & 0xFF;
    __DR_TX_BUF(dr_line)[8] = rdm_package.des_uid & 0xFF;

    // 填充源UID(6字节)
    __DR_TX_BUF(dr_line)[9]  = (rdm_package.source_uid >> 40) & 0xFF;
    __DR_TX_BUF(dr_line)[10] = (rdm_package.source_uid >> 32) & 0xFF;
    __DR_TX_BUF(dr_line)[11] = (rdm_package.source_uid >> 24) & 0xFF;
    __DR_TX_BUF(dr_line)[12] = (rdm_package.source_uid >> 16) & 0xFF;
    __DR_TX_BUF(dr_line)[13] = (rdm_package.source_uid >> 8) & 0xFF;
    __DR_TX_BUF(dr_line)[14] = rdm_package.source_uid & 0xFF;

    // 填充其他字段(5字节)
    __DR_TX_BUF(dr_line)[15] = (rdm_package.other >> 32) & 0xFF;
    __DR_TX_BUF(dr_line)[16] = (rdm_package.other >> 24) & 0xFF;
    __DR_TX_BUF(dr_line)[17] = (rdm_package.other >> 16) & 0xFF;
    __DR_TX_BUF(dr_line)[18] = (rdm_package.other >> 8) & 0xFF;
    __DR_TX_BUF(dr_line)[19] = rdm_package.other & 0xFF;

    // 填充命令字段(3字节)
    __DR_TX_BUF(dr_line)[20] = rdm_package.cmd_type;
    __DR_TX_BUF(dr_line)[21] = (rdm_package.cmd >> 8) & 0xFF;
    __DR_TX_BUF(dr_line)[22] = rdm_package.cmd & 0xFF;

    // 填充数据长度字段(1字节)
    __DR_TX_BUF(dr_line)[23] = rdm_package.data_len;

    // 填充数据内容
    uint8_t j = 0;
    for(int i = 24; i < rdm_package.message_len; i++) {
        __DR_TX_BUF(dr_line)[i] = rdm_package.data[j++];
    }

    // 计算校验和(2字节)
    rdm_package.check = 0;
    for(int i = 0; i < rdm_package.message_len; i++) {
        rdm_package.check += __DR_TX_BUF(dr_line)[i];
    }
    __DR_TX_BUF(dr_line)[rdm_package.message_len]   = (rdm_package.check >> 8) & 0xFF;
    __DR_TX_BUF(dr_line)[rdm_package.message_len+1] = rdm_package.check & 0xFF;

    //设置为发送模式
    __DR_SEND_STATUS(dr_line) = 1;
    
    // 发送起始信号
    dr_funs.change_mode(dr_line, DR_SEND_RESET);
    dr_funs.send_reset(dr_line);

    // 发送数据包
    dr_funs.change_mode(dr_line, DR_SEND_DATA);
    dr_funs.transmit(dr_line, __DR_TX_BUF(dr_line), rdm_package.message_len+2);
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
    
    // 遍历队列，判断是否有相同包类型且目标UID相同的数据包
    for (int i = __DR_RDM_QUEUE_HEAD(dr_line); 
         i != __DR_RDM_QUEUE_TAIL(dr_line); 
         i = (i + 1) % RDM_SEND_QUEUE_LENGTH)
    {
        if (__DR_RDM_QUEUE(dr_line)[i].package == package && 
            __DR_RDM_QUEUE(dr_line)[i].target_uid == target_uid)
        {
            // 重置发送计数
            __DR_RDM_QUEUE(dr_line)[i].send_cnt = 1;   
            // 直接返回，无需新增队列项
            return;  
        }
    }

    // 如果没有就填充数据包

    // 填充队列元素
    __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_TAIL(dr_line)].package = package;
    __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_TAIL(dr_line)].target_uid = target_uid;
    __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_TAIL(dr_line)].send_cnt = 1;

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
            // 记录最后发送的包类型
            __DR_RDM_LAST_PACKAGE(dr_line) = __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].package;
            rdm_send_finish(dr_line, RDM_ERROR_UNNOWM_PACKET);

            // 允许下一次发送
            DR_SEMAPHORE_GIVE(__DR_SEND_SEMAPHORE(dr_line));
            return;
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

        //回应超时重新发送
        case RDM_ERROR_RESPONE_TIMEOUT:
        //解包失败重新发送
        case RDM_ERROR_UNPACK_FAILD:
            //如果重发超过三次，取消发送
            if(__DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].send_cnt > 3)
            {
                // 不是广播节点处理离线
                if(__DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid != &__DR_RDM_UID_LIST(dr_line))
                {
                    // 将设备设置为离线
                    __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid->online_flag = 0;
                    rdm_device_offline_callback(dr_line, __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid);
                }
                // 移动头指针(循环队列)
                __DR_RDM_QUEUE_HEAD(dr_line) = (__DR_RDM_QUEUE_HEAD(dr_line) + 1) % RDM_SEND_QUEUE_LENGTH;
                // 清除队列满状态
                __DR_RDM_QUEUE_FULL(dr_line) = 0;
            }
            //设置重发计数
            else
                __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].send_cnt++;
        break;
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
__weak void rdm_device_respone_callback(dr_line_t *dr_line, uid_t *p, enum rdm_package package){}
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
    uid_t *p = rdm_uid_search(dr_line1, uid);
    
    // 新设备处理
    if(p == NULL)
    {
        // 添加新设备
        rdm_uid_add(dr_line1, uid);
        p = rdm_uid_search(dr_line1, uid);
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
    
    // 继续搜索当前分支...
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
    // 相同UID冲突处理
    if(__DR_RDM_STACK_DEPTH(dr_line) > 0 && 
        __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].low_uid == 
        __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].high_uid)
    {
        rdm_disc_success(dr_line);
        return;
    }
    
    uint64_t mid_uid;
    
    // 初始搜索范围处理
    if(__DR_RDM_STACK_DEPTH(dr_line) == 0)
    {
        mid_uid = RDM_BROADCAST_ADDR/2;
        __DR_RDM_STACK(dr_line)[0].low_uid = 0;
        __DR_RDM_STACK(dr_line)[0].high_uid = mid_uid;

        __DR_RDM_STACK(dr_line)[1].low_uid = mid_uid+1;
        __DR_RDM_STACK(dr_line)[1].high_uid = RDM_BROADCAST_ADDR;
        __DR_RDM_STACK_DEPTH(dr_line) = 2;
    }
    // 已有搜索范围处理
    else
    {
        // 计算中间UID
        mid_uid = (__DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].low_uid + 
                  __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].high_uid)/2;

        // 设置右分支
        __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)].low_uid = mid_uid+1;
        __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)].high_uid = 
            __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].high_uid;
        
        // 调整左分支
        __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].high_uid = mid_uid;

        // 栈深度+1
        __DR_RDM_STACK_DEPTH(dr_line)++;
    }
    
    // 下次处理右分支...
}

/**
 * @brief RDM搜索超时处理
 * @param dr_line DMX+RDM链路指针
 */
void rdm_disc_timeout(dr_line_t *dr_line)
{
    // 移除当前搜索分支
    if(__DR_RDM_STACK_DEPTH(dr_line) > 0)
        __DR_RDM_STACK_DEPTH(dr_line)--;
        
    // 下次处理左分支...
}

