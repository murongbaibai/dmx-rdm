#include "../inc/dr_internal.h"

/************************ UID链表管理函数 ************************/

/**
 * @brief 添加新UID到链表(头插法)
 * @param dr_line DMX+RDM链路指针
 * @param uid 要添加的设备UID
 * @return 添加成功返回1，失败返回0
 */
uint8_t rdm_uid_add(dr_line_t *dr_line, uint64_t uid)
{
    // 检查UID是否已存在
    if(rdm_uid_search(dr_line, uid) != NULL)
        return 0;

    // 动态分配内存
    uid_t *new_uid = (uid_t *)DR_MALLOC(sizeof(uid_t));

    if(new_uid == NULL)
        return 0;

    // 初始化新节点
    new_uid->uid = uid;
    new_uid->online_flag = 1;

    // 头插法插入链表
    new_uid->next = __DR_RDM_UID_LIST(dr_line)->next;
    new_uid->prev = __DR_RDM_UID_LIST(dr_line);
    new_uid->next->prev = new_uid;
    __DR_RDM_UID_LIST(dr_line)->next = new_uid;

    return 1;
}

/**
 * @brief 搜索指定UID的节点
 * @param dr_line DMX+RDM链路指针
 * @param uid 要搜索的设备UID
 * @return 找到返回节点指针，未找到返回NULL
 */
uid_t *rdm_uid_search(dr_line_t *dr_line, uint64_t uid)
{
    uid_t *p = __DR_RDM_UID_LIST(dr_line)->next;
    while(p != __DR_RDM_UID_LIST(dr_line))
    {
        if(p->uid == uid)
            return p;
        p = p->next;
    }
    return NULL;
}

/**
 * @brief 销毁整个UID链表
 * @param dr_line DMX+RDM链路指针
 */
void rdm_uid_distory(dr_line_t *dr_line)
{
    uid_t *p = __DR_RDM_UID_LIST(dr_line)->next;

    // 遍历释放所有节点
    while(p != __DR_RDM_UID_LIST(dr_line))
    {
        __DR_RDM_UID_LIST(dr_line)->next = p->next;
        DR_FREE(p);
        p = __DR_RDM_UID_LIST(dr_line)->next;
    }

    // 重置链表头
    __DR_RDM_UID_LIST(dr_line)->next = __DR_RDM_UID_LIST(dr_line);
    __DR_RDM_UID_LIST(dr_line)->prev = __DR_RDM_UID_LIST(dr_line);
}

/**
 * @brief 删除指定UID节点
 * @param p 要删除的节点指针
 */
void rdm_uid_delete(uid_t *p)
{
    // 从链表中移除节点
    p->prev->next = p->next;
    p->next->prev = p->prev;

    // 释放节点内存
    DR_FREE(p);
}

/**
 * @brief 获取当前设备总数
 * @param dr_line DMX+RDM链路指针
 * @return 设备数量
 */
uint8_t rdm_uid_get_sum(dr_line_t *dr_line)
{
    uint8_t device_max = 0;
    uid_t *p = __DR_RDM_UID_LIST(dr_line)->next;

    // 遍历统计节点数
    while(p != __DR_RDM_UID_LIST(dr_line))
    {
        device_max++;
        p = p->next;
    }
    return device_max;
}

/**
 * @brief 获取指定索引的设备UID
 * @param dr_line DMX+RDM链路指针
 * @param index 设备索引 0->返回头节点
 * @return 设备UID指针  NULL->无此设备
 */
uid_t* rdm_uid_get(dr_line_t *dr_line, uint16_t index)
{
    uid_t *p = __DR_RDM_UID_LIST(dr_line);

    // 返回头节点
    if(index == 0)
        return p;
    
    //搜索指定节点
    for(int i = 0; i < index; i++)
    {
        p = p->next;

        // 搜索到最后一个节点
        if(p == __DR_RDM_UID_LIST(dr_line))
            return NULL;
    }

    //返回搜索到的节点
    return p;
    
}

/************************ 标准RDM协议实现 ************************/

/**
 * @brief 发送RDM广播包
 * @param dr_line DMX+RDM链路指针
 * @param low_uid 搜索范围最低UID
 * @param high_uid 搜索范围最高UID
 */
void rdm_disc_driver(dr_line_t *dr_line, uint64_t low_uid, uint64_t high_uid)
{
    // 设置广播地址
    rdm_package_set_uid(dr_line, RDM_BROADCAST_ADDR);
    
    // 设置发现设备命令
    rdm_package_set_cmd(dr_line, 0x10, 0x0001);

    // 准备UID范围数据
    uint8_t data[12] = {
        (uint8_t)((low_uid >> 40) & 0xFF),
        (uint8_t)((low_uid >> 32) & 0xFF),
        (uint8_t)((low_uid >> 24) & 0xFF),
        (uint8_t)((low_uid >> 16) & 0xFF),
        (uint8_t)((low_uid >> 8) & 0xFF),
        (uint8_t)(low_uid & 0xFF),
        (uint8_t)((high_uid >> 40) & 0xFF),
        (uint8_t)((high_uid >> 32) & 0xFF),
        (uint8_t)((high_uid >> 24) & 0xFF),
        (uint8_t)((high_uid >> 16) & 0xFF),
        (uint8_t)((high_uid >> 8) & 0xFF),
        (uint8_t)(high_uid & 0xFF)
    };
    
    rdm_package_set_data(dr_line, 0x0C, data);
    rdm_package_send(dr_line);
}

/**
 * @brief 发送设备哑音命令
 * @param dr_line DMX+RDM链路指针
 * @param p 目标设备UID指针
 */
void rdm_disc_mute(dr_line_t *dr_line, uid_t *p)
{
    if(p == NULL) return;

    rdm_package_set_uid(dr_line, p->uid);
    rdm_package_set_cmd(dr_line, 0x10, 0x0002);  // DISC_MUTE命令
    rdm_package_set_data(dr_line, 0, NULL);
    rdm_package_send(dr_line);
}

/**
 * @brief 发送设备解除哑音命令
 * @param dr_line DMX+RDM链路指针
 * @param p 目标设备UID指针
 */
void rdm_disc_un_mute(dr_line_t *dr_line, uid_t *p)
{
    if(p == NULL) return;

    rdm_package_set_uid(dr_line, p->uid);
    rdm_package_set_cmd(dr_line, 0x10, 0x0003);  // DISC_UN_MUTE命令
    rdm_package_set_data(dr_line, 0, NULL);
    rdm_package_send(dr_line);
}

/**
 * @brief 查询设备软件版本
 * @param dr_line DMX+RDM链路指针
 * @param p 目标设备UID指针
 */
void rdm_get_version(dr_line_t *dr_line, uid_t *p)
{
    if(p == NULL) return;

    rdm_package_set_uid(dr_line, p->uid);
    rdm_package_set_cmd(dr_line, 0x20, 0x00C0);  // GET_DEVICE_VERSION命令
    rdm_package_set_data(dr_line, 0, NULL);
    rdm_package_send(dr_line);
}

/**
 * @brief 查询设备信息
 * @param dr_line DMX+RDM链路指针
 * @param p 目标设备UID指针
 */
void rdm_get_info(dr_line_t *dr_line, uid_t *p)
{
    if(p == NULL) return;

    rdm_package_set_uid(dr_line, p->uid);
    rdm_package_set_cmd(dr_line, 0x20, 0x0060);  // GET_DEVICE_INFO命令
    rdm_package_set_data(dr_line, 0, NULL);
    rdm_package_send(dr_line);
}

/**
 * @brief 查询设备状态
 * @param dr_line DMX+RDM链路指针
 * @param p 目标设备UID指针
 */
void rdm_get_flag(dr_line_t *dr_line, uid_t *p)
{
    if(p == NULL) return;

    rdm_package_set_uid(dr_line, p->uid);
    rdm_package_set_cmd(dr_line, 0x20, 0x1000);  // GET_DEVICE_FLAG命令
    rdm_package_set_data(dr_line, 0, NULL);
    rdm_package_send(dr_line);
}

/**
 * @brief 设置设备状态
 * @param dr_line DMX+RDM链路指针
 * @param p 目标设备UID指针
 */
void rdm_set_flag(dr_line_t *dr_line, uid_t *p)
{
    if(p == NULL) return;

    rdm_package_set_uid(dr_line, p->uid);
    rdm_package_set_cmd(dr_line, 0x30, 0x1000);  // SET_DEVICE_FLAG命令
    rdm_package_set_data(dr_line, 1, &p->device_flag);
    rdm_package_send(dr_line);
}

/**
 * @brief 查询设备DMX地址
 * @param dr_line DMX+RDM链路指针
 * @param p 目标设备UID指针
 */
void rdm_get_dmx_addr(dr_line_t *dr_line, uid_t *p)
{
    if(p == NULL) return;

    rdm_package_set_uid(dr_line, p->uid);
    rdm_package_set_cmd(dr_line, 0x20, 0x00F0);  // GET_DMX_ADDRESS命令
    rdm_package_set_data(dr_line, 0, NULL);
    rdm_package_send(dr_line);
}

/**
 * @brief 设置设备DMX地址
 * @param dr_line DMX+RDM链路指针
 * @param p 目标设备UID指针
 */
void rdm_set_dmx_addr(dr_line_t *dr_line, uid_t *p)
{
    if(p == NULL) return;

    rdm_package_set_uid(dr_line, p->uid);
    rdm_package_set_cmd(dr_line, 0x30, 0x00F0);  // SET_DMX_ADDRESS命令
    
    uint8_t data[2] = {
        (uint8_t)((p->device_dmx >> 8) & 0xFF),
        (uint8_t)(p->device_dmx & 0xFF)
    };
    
    rdm_package_set_data(dr_line, 2, data);
    rdm_package_send(dr_line);
}

/************************ 扩展RDM协议命令 ************************/

/**
 * @brief 查询设备工作模式
 * @param dr_line DMX+RDM链路指针
 * @param p 目标设备UID指针
 */
void rdm_get_mode(dr_line_t *dr_line, uid_t *p)
{
    if(p == NULL) return;

    rdm_package_set_uid(dr_line, p->uid);
    rdm_package_set_cmd(dr_line, 0x20, 0x00E0);  // GET_DEVICE_MODE命令
    rdm_package_set_data(dr_line, 0, NULL);
    rdm_package_send(dr_line);
}

/**
 * @brief 设置设备工作模式
 * @param dr_line DMX+RDM链路指针
 * @param p 目标设备UID指针
 */
void rdm_set_mode(dr_line_t *dr_line, uid_t *p)
{
    if(p == NULL) return;

    rdm_package_set_uid(dr_line, p->uid);
    rdm_package_set_cmd(dr_line, 0x30, 0x00E0);  // SET_DEVICE_MODE命令
    rdm_package_set_data(dr_line, 1, &p->device_cur_mode);
    rdm_package_send(dr_line);
}

/**
 * @brief 查询设备模式描述信息
 * @param dr_line DMX+RDM链路指针
 * @param p 目标设备UID指针
 */
void rdm_get_mode_info(dr_line_t *dr_line, uid_t *p)
{
    if(p == NULL) return;

    rdm_package_set_uid(dr_line, p->uid);
    rdm_package_set_cmd(dr_line, 0x20, 0x00E1);  // GET_DEVICE_MODE_INFO命令
    rdm_package_set_data(dr_line, 1, &p->device_cur_mode);
    rdm_package_send(dr_line);
}

/************************ RDM发送端解包处理 ************************/

/**
 * @brief RDM输出端数据包解包处理
 * @param dr_line DMX+RDM链路指针
 * @note 解析接收到的RDM数据包并设置相应的包类型
 */
void rdm_output_unpack(dr_line_t *dr_line)
{
    //处理广播搜寻设备回应包
    if(__DR_RECV_PACKAGE(dr_line) == DISC_RESPONE_PACKAGE)
    {
        uint16_t i = 0;
        // 跳过前导0xFE
        while(__DR_RX_BUF(dr_line)[++i] == 0xFE);
        
        // 验证广播包格式
        if(__DR_RX_BUF(dr_line)[i++] == 0xAA)
        {
            // 解码UID数据 (6字节)
            for(int j = 0; j < 6; j++)
            {
                // 合并交替的0xAA和0x55掩码数据
                __DR_RDM_PARSE(dr_line).data[j] = (__DR_RX_BUF(dr_line)[i] & 0x55) | 
                                                         (__DR_RX_BUF(dr_line)[i+1] & 0xAA);
                i += 2;
            }
            
            // 计算校验和
            uint32_t sum = 0;
            for(int j = 0; j < 6; j++)
            {
                sum += (__DR_RDM_PARSE(dr_line).data[j] | 0xAA);
                sum += (__DR_RDM_PARSE(dr_line).data[j] | 0x55);
            }
            
            // 生成预期校验值
            uint8_t checksum[4] = {
                (uint8_t)((sum >> 8) | 0xAA),
                (uint8_t)((sum >> 8) | 0x55),
                (uint8_t)(sum | 0xAA),
                (uint8_t)(sum | 0x55)
            };
            
            // 验证校验和
            if(memcmp(&__DR_RX_BUF(dr_line)[i], checksum, 4) == 0)
            {
                __DR_RDM_PARSE(dr_line).rdm_package = DISC_UNIQUE;
            }
            else
            {
                __DR_RDM_PARSE(dr_line).rdm_package = RDM_PACKAGE_CHECK_ERROR;
            }
            i += 4;
        }
    }
    // 处理标准RDM包
    else if(__DR_RECV_PACKAGE(dr_line) == RDM_PACKAGE)
    {
        // 根据命令类型解析包
        switch(__DR_RX_BUF(dr_line)[20])
        {
            // 响应类命令
            case 0x11:
                if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0x02)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = DISC_MUTE;
                }
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0x03)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = DISC_UN_MUTE;
                }
                break;
                
            // 获取类命令
            case 0x21:
                if(__DR_RX_BUF(dr_line)[21] == 0x10 && __DR_RX_BUF(dr_line)[22] == 0x00)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = GET_DRIVER_FLAG;
                }
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0xF0)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = GET_DRIVER_DMX_ADDR;
                }
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0xC0)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = GET_DRIVER_VERSION;
                }
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0x60)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = GET_DRIVER_INFO;
                }
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0xE0)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = GET_DRIVER_MODE;
                }
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0xE1)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = GET_DRIVER_MODE_INFO;
                }
                break;
                
            // 设置类命令
            case 0x31:
                if(__DR_RX_BUF(dr_line)[21] == 0x10 && __DR_RX_BUF(dr_line)[22] == 0x00)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = SET_DRIVER_FLAG;
                }
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0xF0)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = SET_DRIVER_DMX_ADDR;
                }
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0xE0)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = SET_DRIVER_MODE;
                }
                break;
                
            default:
                __DR_RDM_PARSE(dr_line).rdm_package = RDM_PACKAGE_CHECK_ERROR;
                break;
        }
        
        // 记录数据部分
        uint8_t data_len = __DR_RX_BUF(dr_line)[23];
        memcpy(__DR_RDM_PARSE(dr_line).data, &__DR_RX_BUF(dr_line)[24], data_len);
        
        // 解析源UID (6字节)
        __DR_RDM_PARSE(dr_line).source_uid = 
            ((uint64_t)__DR_RX_BUF(dr_line)[9] << 40) |
            ((uint64_t)__DR_RX_BUF(dr_line)[10] << 32) |
            ((uint64_t)__DR_RX_BUF(dr_line)[11] << 24) |
            ((uint64_t)__DR_RX_BUF(dr_line)[12] << 16) |
            ((uint64_t)__DR_RX_BUF(dr_line)[13] << 8) |
            (uint64_t)__DR_RX_BUF(dr_line)[14];
        
        // 计算并验证校验和
        uint32_t sum = 0;
        for(int i = 0; i < 24+data_len; i++)
        {
            sum += __DR_RX_BUF(dr_line)[i];
        }
        
        if(((sum >> 8) & 0xFF) != __DR_RX_BUF(dr_line)[24+data_len] || 
           (sum & 0xFF) != __DR_RX_BUF(dr_line)[25+data_len])
        {
            __DR_RDM_PARSE(dr_line).rdm_package = RDM_PACKAGE_CHECK_ERROR;
        }
    }
}

/**
 * @brief 根据解包结果自动设置设备信息
 * @param dr_line DMX+RDM链路指针
 */
void rdm_auto_set(dr_line_t *dr_line)
{
    switch(__DR_RDM_PARSE(dr_line).rdm_package)
    {
        case GET_DRIVER_FLAG:
            __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid->device_flag = 
                __DR_RDM_PARSE(dr_line).data[0];
            break;
            
        case GET_DRIVER_DMX_ADDR:
            __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid->device_dmx = 
                (__DR_RDM_PARSE(dr_line).data[0] << 8) | 
                __DR_RDM_PARSE(dr_line).data[1];
            break;
            
        case GET_DRIVER_VERSION:
            __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid->device_version = 
                (__DR_RDM_PARSE(dr_line).data[0] << 8) | 
                __DR_RDM_PARSE(dr_line).data[1];
            break;
            
        case GET_DRIVER_INFO:
            // 解析DMX地址
            __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid->device_dmx = 
                (__DR_RDM_PARSE(dr_line).data[14] << 8) | 
                __DR_RDM_PARSE(dr_line).data[15];
                
            // 解析通道数
            __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid->device_cur_channel = 
                (__DR_RDM_PARSE(dr_line).data[10] << 8) | 
                __DR_RDM_PARSE(dr_line).data[11];
                
            // 解析模式信息
            __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid->device_cur_mode = 
                __DR_RDM_PARSE(dr_line).data[12];
            __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid->device_mode_max = 
                __DR_RDM_PARSE(dr_line).data[13];
            break;
            
        case GET_DRIVER_MODE:
            __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid->device_cur_mode = 
                __DR_RDM_PARSE(dr_line).data[0];
            __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid->device_mode_max = 
                __DR_RDM_PARSE(dr_line).data[1];
            break;
            
        case GET_DRIVER_MODE_INFO:
            __DR_RDM_QUEUE(dr_line)[__DR_RDM_QUEUE_HEAD(dr_line)].target_uid->device_cur_channel = 
                (__DR_RDM_PARSE(dr_line).data[0] << 8) | 
                __DR_RDM_PARSE(dr_line).data[1];
            break;
    }
}
