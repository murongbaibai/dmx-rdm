#include "../inc/dr_internal.h"

static const uint16_t dmx_channel_map[8] = {DMX_MODE1_CHANNEL, DMX_MODE2_CHANNEL, DMX_MODE3_CHANNEL, DMX_MODE4_CHANNEL, DMX_MODE5_CHANNEL, DMX_MODE6_CHANNEL, DMX_MODE7_CHANNEL, DMX_MODE8_CHANNEL};

/************************ 标准最小RDM协议实现 ************************/

/**
 * @brief RDM广播包响应
 * @param dr_line DMX+RDM链路指针
 */
void rdm_disc_driver_respone(dr_line_t *dr_line)
{
    for(int i = 0; i < 7; i++)
        __DR_TX_BUF(dr_line)[i] = 0xFE;
    __DR_TX_BUF(dr_line)[7] = 0xAA;

    __DR_TX_BUF(dr_line)[8]  = ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 8*5) & 0xFF) | 0xAA;
    __DR_TX_BUF(dr_line)[9]  = ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 8*5) & 0xFF) | 0x55;
    __DR_TX_BUF(dr_line)[10] = ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 8*4) & 0xFF) | 0xAA;
    __DR_TX_BUF(dr_line)[11] = ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 8*4) & 0xFF) | 0x55;
    __DR_TX_BUF(dr_line)[12] = ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 8*3) & 0xFF) | 0xAA;
    __DR_TX_BUF(dr_line)[13] = ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 8*3) & 0xFF) | 0x55;
    __DR_TX_BUF(dr_line)[14] = ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 8*2) & 0xFF) | 0xAA;
    __DR_TX_BUF(dr_line)[15] = ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 8*2) & 0xFF) | 0x55;
    __DR_TX_BUF(dr_line)[16] = ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 8*1) & 0xFF) | 0xAA;
    __DR_TX_BUF(dr_line)[17] = ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 8*1) & 0xFF) | 0x55;
    __DR_TX_BUF(dr_line)[18] = ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 8*0) & 0xFF) | 0xAA;
    __DR_TX_BUF(dr_line)[19] = ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 8*0) & 0xFF) | 0x55;

    uint32_t check_sum = 0;
    for(int i = 8; i < 20; i++)
    {
        check_sum += __DR_TX_BUF(dr_line)[i];
    }

    __DR_TX_BUF(dr_line)[20] = ((check_sum >> 8*1) & 0xFF) | 0xAA;
    __DR_TX_BUF(dr_line)[21] = ((check_sum >> 8*1) & 0xFF) | 0x55;
    __DR_TX_BUF(dr_line)[22] = ((check_sum >> 8*0) & 0xFF) | 0xAA;
    __DR_TX_BUF(dr_line)[23] = ((check_sum >> 8*0) & 0xFF) | 0x55;

    //设置为发送模式
    __DR_SEND_STATUS(dr_line) = 1;
    
    dr_funs.change_mode(dr_line, DR_SEND_DATA);
    dr_funs.transmit(dr_line, __DR_TX_BUF(dr_line), 24);
}

/**
 * @brief RDM哑音包响应
 * @param dr_line DMX+RDM链路指针
 * @param uid 目标设备UID
 */
void rdm_disc_mute_respone(dr_line_t *dr_line, uint64_t uid)
{
    rdm_package_set_uid(dr_line, uid);
    rdm_package_set_cmd(dr_line, 0x11, 0x0002);  // DISC_MUTE命令
    uint8_t data[2] = {0x00, 0x00};
    rdm_package_set_data(dr_line, 2, data);
    rdm_package_send(dr_line);
}

/**
 * @brief RDM解除哑音包响应
 * @param dr_line DMX+RDM链路指针
 * @param uid 目标设备UID
 */
void rdm_disc_un_mute_respone(dr_line_t *dr_line, uint64_t uid)
{
    rdm_package_set_uid(dr_line, uid);
    rdm_package_set_cmd(dr_line, 0x11, 0x0003);  // DISC_UN_MUTE命令
    uint8_t data[2] = {0x00, 0x00};
    rdm_package_set_data(dr_line, 2, data);
    rdm_package_send(dr_line);
}

/**
 * @brief RDM获取设备版本响应
 * @param dr_line DMX+RDM链路指针
 * @param uid 目标设备UID
 */
void rdm_get_version_respone(dr_line_t *dr_line, uint64_t uid)
{
    rdm_package_set_uid(dr_line, uid);
    rdm_package_set_cmd(dr_line, 0x21, 0x00C0);  // GET_DEVICE_VERSION命令
    
    uint8_t data[4];
    data[0] = (__DR_RDM_DEVICE_INFO(dr_line).info.soft_version >> 24) & 0xFF;
    data[1] = (__DR_RDM_DEVICE_INFO(dr_line).info.soft_version >> 16) & 0xFF;
    data[2] = (__DR_RDM_DEVICE_INFO(dr_line).info.soft_version >> 8) & 0xFF;
    data[3] = __DR_RDM_DEVICE_INFO(dr_line).info.soft_version & 0xFF;
    
    rdm_package_set_data(dr_line, 4, data);
    rdm_package_send(dr_line);
}

/**
 * @brief RDM获取设备信息响应
 * @param dr_line DMX+RDM链路指针
 * @param uid 目标设备UID
 */
void rdm_get_info_respone(dr_line_t *dr_line, uint64_t uid)
{
    rdm_package_set_uid(dr_line, uid);
    rdm_package_set_cmd(dr_line, 0x21, 0x0060);  // GET_DEVICE_INFO命令
    
    uint8_t data[19] = {0};
    
    // RDM版本
    data[0] = (__DR_RDM_DEVICE_INFO(dr_line).info.rdm_version >> 8) & 0xFF;
    data[1] = __DR_RDM_DEVICE_INFO(dr_line).info.rdm_version & 0xFF;
    
    // 设备ID
    data[2] = (__DR_RDM_DEVICE_INFO(dr_line).info.device_id >> 8) & 0xFF;
    data[3] = __DR_RDM_DEVICE_INFO(dr_line).info.device_id & 0xFF;
    
    // 产品类别
    data[4] = (__DR_RDM_DEVICE_INFO(dr_line).info.product >> 8) & 0xFF;
    data[5] = __DR_RDM_DEVICE_INFO(dr_line).info.product & 0xFF;
    
    // 软件版本
    data[6] = (__DR_RDM_DEVICE_INFO(dr_line).info.soft_version >> 24) & 0xFF;
    data[7] = (__DR_RDM_DEVICE_INFO(dr_line).info.soft_version >> 16) & 0xFF;
    data[8] = (__DR_RDM_DEVICE_INFO(dr_line).info.soft_version >> 8) & 0xFF;
    data[9] = __DR_RDM_DEVICE_INFO(dr_line).info.soft_version & 0xFF;
    
    // DMX通道数
    data[10] = (__DR_DMX_CHANNEL(dr_line) >> 8) & 0xFF;
    data[11] = __DR_DMX_CHANNEL(dr_line) & 0xFF;
    
    // 工作模式
    data[12] = __DR_DMX_MODE(dr_line);
    data[13] = RDM_MODE_MAX;
    
    // DMX起始地址
    data[14] = (__DR_DMX_ADDR(dr_line) >> 8) & 0xFF;
    data[15] = __DR_DMX_ADDR(dr_line) & 0xFF;
    
    // 子设备数和传感器数
    data[16] = (__DR_RDM_DEVICE_INFO(dr_line).info.sub_device >> 8) & 0xFF;
    data[17] = __DR_RDM_DEVICE_INFO(dr_line).info.sub_device & 0xFF;
    data[18] = __DR_RDM_DEVICE_INFO(dr_line).info.sensor;
    
    rdm_package_set_data(dr_line, 19, data);
    rdm_package_send(dr_line);
}

/**
 * @brief RDM获取设备状态响应
 * @param dr_line DMX+RDM链路指针
 * @param uid 目标设备UID
 */
void rdm_get_flag_respone(dr_line_t *dr_line, uint64_t uid)
{
    rdm_package_set_uid(dr_line, uid);
    rdm_package_set_cmd(dr_line, 0x21, 0x1000);  // GET_DEVICE_FLAG命令
    rdm_package_set_data(dr_line, 1, &__DR_RDM_DEVICE_INFO(dr_line).flag);
    rdm_package_send(dr_line);
}

/**
 * @brief RDM设置设备状态响应
 * @param dr_line DMX+RDM链路指针
 * @param uid 目标设备UID
 */
void rdm_set_flag_respone(dr_line_t *dr_line, uint64_t uid)
{
    rdm_package_set_uid(dr_line, uid);
    rdm_package_set_cmd(dr_line, 0x31, 0x1000);  // SET_DEVICE_FLAG命令
    rdm_package_set_data(dr_line, 0, NULL);
    rdm_package_send(dr_line);
}

/**
 * @brief RDM获取DMX地址响应
 * @param dr_line DMX+RDM链路指针
 * @param uid 目标设备UID
 */
void rdm_get_dmx_addr_respone(dr_line_t *dr_line, uint64_t uid)
{
    rdm_package_set_uid(dr_line, uid);
    rdm_package_set_cmd(dr_line, 0x21, 0x00F0);  // GET_DMX_ADDRESS命令
    
    uint8_t data[2];
    data[0] = (__DR_DMX_ADDR(dr_line) >> 8) & 0xFF;
    data[1] = __DR_DMX_ADDR(dr_line) & 0xFF;
    
    rdm_package_set_data(dr_line, 2, data);
    rdm_package_send(dr_line);
}

/**
 * @brief RDM设置DMX地址响应
 * @param dr_line DMX+RDM链路指针
 * @param uid 目标设备UID
 */
void rdm_set_dmx_addr_respone(dr_line_t *dr_line, uint64_t uid)
{
    rdm_package_set_uid(dr_line, uid);
    rdm_package_set_cmd(dr_line, 0x31, 0x00F0);  // SET_DMX_ADDRESS命令
    rdm_package_set_data(dr_line, 0, NULL);
    rdm_package_send(dr_line);
}

/**
 * @brief RDM获取支持命令响应(未实现)
 */
void rdm_get_pid_respone(dr_line_t *dr_line, uint64_t uid)
{
    // TODO: 待实现
}

/**
 * @brief RDM获取自定义命令响应(未实现)
 */
void rdm_get_cust_pid_respone(dr_line_t *dr_line, uint64_t uid)
{
    // TODO: 待实现
}


/************************ 标准扩展RDM协议实现 ************************/

/**
 * @brief RDM获取设备模式响应
 * @param dr_line DMX+RDM链路指针
 * @param uid 目标设备UID
 * @note 响应包含当前模式和支持的最大模式值
 */
void rdm_get_mode_respone(dr_line_t *dr_line, uint64_t uid)
{
    // 设置目标UID
    rdm_package_set_uid(dr_line, uid);
    
    // 设置命令类型和PID (GET_DEVICE_MODE)
    rdm_package_set_cmd(dr_line, 0x21, 0x00E0);

    // 准备响应数据
    uint8_t data[2] = {
        __DR_DMX_MODE(dr_line),    // 当前模式
        RDM_MODE_MAX               // 支持的最大模式
    };
    
    // 设置数据并发送
    rdm_package_set_data(dr_line, 2, data);
    rdm_package_send(dr_line);
}

/**
 * @brief RDM设置设备模式响应
 * @param dr_line DMX+RDM链路指针
 * @param uid 目标设备UID
 * @note 此响应不包含数据，仅确认设置成功
 */
void rdm_set_mode_respone(dr_line_t *dr_line, uint64_t uid)
{
    // 设置目标UID
    rdm_package_set_uid(dr_line, uid);
    
    // 设置命令类型和PID (SET_DEVICE_MODE)
    rdm_package_set_cmd(dr_line, 0x31, 0x00E0);

    // 设置空数据并发送
    rdm_package_set_data(dr_line, 0, NULL);
    rdm_package_send(dr_line);
}

/**
 * @brief RDM获取设备模式描述响应
 * @param dr_line DMX+RDM链路指针
 * @param uid 目标设备UID
 * @note 此功能暂未实现
 * @todo 需要实现模式描述信息的返回
 */
void rdm_get_mode_info_respone(dr_line_t *dr_line, uint64_t uid)
{
    // TODO: 实现模式描述信息的返回
    // 应包括各模式的文本描述信息
}






/************************ RDM解包 ************************/
/**
 * @brief RDM输入模式数据包解包处理
 * @param dr_line DMX+RDM链路指针
 * @note 根据RDM协议解析接收到的数据包，并设置相应的包类型
 */
void rdm_input_unpack(dr_line_t *dr_line)
{
    // 检查目标UID是否匹配
    if(RDM_UID_TRUE)
    {
        // 根据命令类别处理不同类型的包
        switch(__DR_RX_BUF(dr_line)[20])  // 命令类别字节
        {
            // 设备发现类命令 (0x10)
            case 0x10:
                // 设备唯一性搜索命令
                if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0x01)
                {
                    if(__DR_RDM_DEVICE_INFO(dr_line).mute == 1)
                    {
                        __DR_RDM_PARSE(dr_line).rdm_package = RDM_PACKAGE_CHECK_ERROR;
                        break;
                    }
                    
                    // 解析搜索范围的低UID和高UID
                    uint64_t low_uid = ((uint64_t)__DR_RX_BUF(dr_line)[24] << 40) |
                                      ((uint64_t)__DR_RX_BUF(dr_line)[25] << 32) |
                                      ((uint64_t)__DR_RX_BUF(dr_line)[26] << 24) |
                                      ((uint64_t)__DR_RX_BUF(dr_line)[27] << 16) |
                                      ((uint64_t)__DR_RX_BUF(dr_line)[28] << 8)  |
                                       (uint64_t)__DR_RX_BUF(dr_line)[29];
                    
                    uint64_t high_uid = ((uint64_t)__DR_RX_BUF(dr_line)[30] << 40) |
                                       ((uint64_t)__DR_RX_BUF(dr_line)[31] << 32) |
                                       ((uint64_t)__DR_RX_BUF(dr_line)[32] << 24) |
                                       ((uint64_t)__DR_RX_BUF(dr_line)[33] << 16) |
                                       ((uint64_t)__DR_RX_BUF(dr_line)[34] << 8)  |
                                        (uint64_t)__DR_RX_BUF(dr_line)[35];
                    
                    // 检查设备UID是否在搜索范围内
                    if(__DR_RDM_DEVICE_INFO(dr_line).uid >= low_uid && 
                       __DR_RDM_DEVICE_INFO(dr_line).uid <= high_uid)
                    {
                        __DR_RDM_PARSE(dr_line).rdm_package = DISC_UNIQUE;
                    }
                    else
                    {
                        __DR_RDM_PARSE(dr_line).rdm_package = RDM_PACKAGE_CHECK_ERROR;
                    }
                }
                // 设备哑音命令
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0x02)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = DISC_MUTE;
                }
                // 设备解除哑音命令
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0x03)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = DISC_UN_MUTE;
                }
                break;
                
            // 获取类命令 (0x20)
            case 0x20:
                // 获取设备状态
                if(__DR_RX_BUF(dr_line)[21] == 0x10 && __DR_RX_BUF(dr_line)[22] == 0x00)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = GET_DRIVER_FLAG;
                }
                // 获取DMX地址
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0xF0)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = GET_DRIVER_DMX_ADDR;
                }
                // 获取设备版本
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0xC0)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = GET_DRIVER_VERSION;
                }
                // 获取设备信息
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0x60)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = GET_DRIVER_INFO;
                }
                // 获取设备模式
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0xE0)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = GET_DRIVER_MODE;
                }
                break;
                
            // 设置类命令 (0x30)
            case 0x30:
                // 设置设备状态
                if(__DR_RX_BUF(dr_line)[21] == 0x10 && __DR_RX_BUF(dr_line)[22] == 0x00)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = SET_DRIVER_FLAG;
                }
                // 设置DMX地址
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0xF0)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = SET_DRIVER_DMX_ADDR;
                }
                // 设置设备模式
                else if(__DR_RX_BUF(dr_line)[21] == 0x00 && __DR_RX_BUF(dr_line)[22] == 0xE0)
                {
                    __DR_RDM_PARSE(dr_line).rdm_package = SET_DRIVER_MODE;
                }
                break;
                
            // 不支持的命令类型
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
            ((uint64_t)__DR_RX_BUF(dr_line)[13] << 8)  |
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
    // 目标UID不匹配，不需要响应
    else
    {
        __DR_RDM_PARSE(dr_line).rdm_package = RDM_PACKAGE_CHECK_ERROR;
    }
}

/**
 * @brief RDM设备设置回调(弱定义)
 * @param dr_line DMX+RDM链路指针
 * @param p 设备UID指针
 * @weak 默认实现为空函数
 */
__weak void rdm_device_set_callback(dr_line_t *dr_line)
{
    
}

/**
 * @brief RDM自动响应处理
 * @param dr_line DMX+RDM链路指针
 * @note 根据解包结果自动生成相应的响应包
 */
void rdm_auto_response(dr_line_t *dr_line)
{ 
    switch(__DR_RDM_PARSE(dr_line).rdm_package)
    {
        case DISC_UNIQUE:  
            rdm_disc_driver_respone(dr_line);
            break;
            
        case DISC_MUTE:
            rdm_disc_mute_respone(dr_line, __DR_RDM_PARSE(dr_line).source_uid);
            __DR_RDM_DEVICE_INFO(dr_line).mute = 1;  // 进入哑音状态
            break;
            
        case DISC_UN_MUTE:
            rdm_disc_un_mute_respone(dr_line, __DR_RDM_PARSE(dr_line).source_uid);
            __DR_RDM_DEVICE_INFO(dr_line).mute = 0;  // 退出哑音状态
            break;
            
        case GET_DRIVER_FLAG:
            rdm_get_flag_respone(dr_line, __DR_RDM_PARSE(dr_line).source_uid);
            break;
            
        case GET_DRIVER_DMX_ADDR:
            rdm_get_dmx_addr_respone(dr_line, __DR_RDM_PARSE(dr_line).source_uid);
            break;
            
        case GET_DRIVER_VERSION:
            rdm_get_version_respone(dr_line, __DR_RDM_PARSE(dr_line).source_uid);
            break;
            
        case GET_DRIVER_INFO:
            rdm_get_info_respone(dr_line, __DR_RDM_PARSE(dr_line).source_uid);
            break;
            
        case GET_DRIVER_MODE:
            rdm_get_mode_respone(dr_line, __DR_RDM_PARSE(dr_line).source_uid);
            break;
            
        case SET_DRIVER_FLAG:
            rdm_set_flag_respone(dr_line, __DR_RDM_PARSE(dr_line).source_uid);
            __DR_RDM_DEVICE_INFO(dr_line).flag = __DR_RDM_PARSE(dr_line).data[0];
            rdm_device_set_callback(dr_line);
            break;
            
        case SET_DRIVER_DMX_ADDR:
            rdm_set_dmx_addr_respone(dr_line, __DR_RDM_PARSE(dr_line).source_uid);
            __DR_DMX_ADDR(dr_line) = (__DR_RDM_PARSE(dr_line).data[0] << 8) | 
                                     __DR_RDM_PARSE(dr_line).data[1];
            rdm_device_set_callback(dr_line);
            break;
            
        case SET_DRIVER_MODE:
            rdm_set_mode_respone(dr_line, __DR_RDM_PARSE(dr_line).source_uid);
            __DR_DMX_MODE(dr_line) = __DR_RDM_PARSE(dr_line).data[0];
            __DR_DMX_CHANNEL(dr_line) = dmx_channel_map[__DR_DMX_MODE(dr_line)-1];
            rdm_device_set_callback(dr_line);
            break;
    }
}
