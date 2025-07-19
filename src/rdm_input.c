#include "dmx_rdm.h"

#if (DEVICE_TYPE_SWITCH == DEVICE_TYPE_INPUT)
static const uint16_t device_channel_buf[8] = {DMX_MODE1_CHANNEL, DMX_MODE2_CHANNEL, DMX_MODE3_CHANNEL, DMX_MODE4_CHANNEL, DMX_MODE5_CHANNEL, DMX_MODE6_CHANNEL, DMX_MODE7_CHANNEL, DMX_MODE8_CHANNEL};
//设备信息结构体定义（参数由dmx_config_h配置）
device_info_t device_info =
{
    .mute = 0,                              //无哑音
    .uid  = RDM_DEFAULT_UID,                //默认UID
    .flag = 0,                              //状态关

    .info.device_id = 1,                    //设备类型
    .info.dmx_channel = DMX_MODE1_CHANNEL,  //当前通道数
    .info.dmx_cur_mode = 1,                 //当前模式
    .info.dmx_mode_max = RDM_MODE_MAX,      //最大模式数
    .info.dmx_start_addr = 1,               //DMX地址
    .info.product = 1,                      //产品类别
    .info.rdm_version = 0,                  //RDM版本
    .info.sensor = 0,                       //传感器个数
    .info.soft_version = 0x0100,            //软件版本
    .info.sub_device = 0                    //子设备个数
};

//RDM包初始化
rdm_package_t rdm_package = {
    .start = 0xCC,
    .sub_start = 0x01,
    .source_uid = RDM_DEFAULT_UID,
    .other = 0x0000000001000000,
    .check = 0,
};
/****************** 获取/设置设备信息 ******************/
uint16_t Device_Get_DMX_Addr(void)
{
    return device_info.info.dmx_start_addr;
}
uint8_t Device_Get_Mode(void)
{
    return device_info.info.dmx_cur_mode;
}
uint16_t Device_Get_Channel(void)
{
    return device_info.info.dmx_channel;
}
uint8_t Device_Get_Flag(void)
{
    return device_info.flag;
}
uint8_t Device_Get_Mute(void)
{
    return device_info.mute;
}

void Device_Set_DMX_Addr(uint16_t dmx_addr)
{
    if(dmx_addr == 0 || dmx_addr > 512)
        return;
    device_info.info.dmx_start_addr = dmx_addr;
}
void Device_Set_Mode(uint8_t mode)
{
    if(mode == 0 || mode > device_info.info.dmx_mode_max)
        return;
    device_info.info.dmx_cur_mode = mode;
    device_info.info.dmx_channel = device_channel_buf[mode-1];
}
void Device_Set_Flag(uint8_t flag)
{
    device_info.flag = flag ? 1 : 0;
}
void Device_Set_Mute(uint8_t mute)
{
    device_info.mute = mute ? 1 : 0;
}


/****************** RDM发送 ******************/
/****** 标准最小RDM协议 ******/
//广播包回应
void RDM_Disc_Driver_Respone(dmx_line_t *dmx_line)
{
    for(int i = 0; i < 7; i++)
        dmx_line->rdm_send_buf[i] = 0xFE;
    dmx_line->rdm_send_buf[7] = 0xAA;

    dmx_line->rdm_send_buf[8]  = ((device_info.uid >> 8*5) & 0xFF) | 0xAA;
    dmx_line->rdm_send_buf[9]  = ((device_info.uid >> 8*5) & 0xFF) | 0x55;
    dmx_line->rdm_send_buf[10] = ((device_info.uid >> 8*4) & 0xFF) | 0xAA;
    dmx_line->rdm_send_buf[11] = ((device_info.uid >> 8*4) & 0xFF) | 0x55;
    dmx_line->rdm_send_buf[12] = ((device_info.uid >> 8*3) & 0xFF) | 0xAA;
    dmx_line->rdm_send_buf[13] = ((device_info.uid >> 8*3) & 0xFF) | 0x55;
    dmx_line->rdm_send_buf[14] = ((device_info.uid >> 8*2) & 0xFF) | 0xAA;
    dmx_line->rdm_send_buf[15] = ((device_info.uid >> 8*2) & 0xFF) | 0x55;
    dmx_line->rdm_send_buf[16] = ((device_info.uid >> 8*1) & 0xFF) | 0xAA;
    dmx_line->rdm_send_buf[17] = ((device_info.uid >> 8*1) & 0xFF) | 0x55;
    dmx_line->rdm_send_buf[18] = ((device_info.uid >> 8*0) & 0xFF) | 0xAA;
    dmx_line->rdm_send_buf[19] = ((device_info.uid >> 8*0) & 0xFF) | 0x55;

    uint32_t check_sum = 0;
    for(int i = 8; i < 20; i++)
    {
        check_sum += dmx_line->rdm_send_buf[i];
    }

    dmx_line->rdm_send_buf[20] = ((check_sum >> 8*1) & 0xFF) | 0xAA;
    dmx_line->rdm_send_buf[21] = ((check_sum >> 8*1) & 0xFF) | 0x55;
    dmx_line->rdm_send_buf[22] = ((check_sum >> 8*0) & 0xFF) | 0xAA;
    dmx_line->rdm_send_buf[23] = ((check_sum >> 8*0) & 0xFF) | 0x55;

    //发送包
    dmx_line->change_mode_fun(DMX_SEND_DATA);
    DMX_Line_Transmit_DMA(dmx_line,dmx_line->rdm_send_buf,24);
}
//哑音包回应
void RDM_Disc_Mute_Respone(dmx_line_t *dmx_line, uint64_t uid)
{
    RDM_Package_Set_Uid(uid);

    RDM_Package_Set_Cmd(0x11,0x0002);

    uint8_t data[2] = {0x00,0x00};
    RDM_Package_Set_Data(2,data);

    RDM_Package_Send(dmx_line);
}
//解除哑音包回应
void RDM_Disc_Un_Mute_Respone(dmx_line_t *dmx_line, uint64_t uid)
{
    RDM_Package_Set_Uid(uid);

    RDM_Package_Set_Cmd(0x11,0x0003);

    uint8_t data[2] = {0x00,0x00};
    RDM_Package_Set_Data(2,data);

    RDM_Package_Send(dmx_line);
}
//查设备软件版本回应
void RDM_Get_Version_Respone(dmx_line_t *dmx_line, uint64_t uid)
{
    RDM_Package_Set_Uid(uid);

    RDM_Package_Set_Cmd(0x21,0x00C0);

    uint8_t data[4] = {0};
    data[0] = device_info.info.soft_version>>24 & 0xFF;
    data[1] = device_info.info.soft_version>>16 & 0xFF;
    data[2] = device_info.info.soft_version>>8 & 0xFF;
    data[3] = device_info.info.soft_version>>0 & 0xFF;
    RDM_Package_Set_Data(4,data);

    RDM_Package_Send(dmx_line);
}
//查设备信息回应
void RDM_Get_Info_Respone(dmx_line_t *dmx_line, uint64_t uid)
{
    RDM_Package_Set_Uid(uid);

    RDM_Package_Set_Cmd(0x21,0x0060);

    uint8_t data[19];
    data[0]  = (device_info.info.rdm_version >> 8) & 0xFF;
    data[1]  = device_info.info.rdm_version & 0xFF;

    data[2]  = (device_info.info.device_id >> 8) & 0xFF;
    data[3]  = device_info.info.device_id & 0xFF;

    data[4]  = (device_info.info.product >> 8) & 0xFF;
    data[5]  = device_info.info.product & 0xFF;

    data[6]  = (device_info.info.soft_version >> 24) & 0xFF;
    data[7]  = (device_info.info.soft_version >> 16)& 0xFF;
    data[8]  = (device_info.info.soft_version >> 8) & 0xFF;
    data[9]  = device_info.info.soft_version & 0xFF;

    data[10] = (device_info.info.dmx_channel >> 8) & 0xFF;
    data[11] = device_info.info.dmx_channel & 0xFF;

    data[12] = device_info.info.dmx_cur_mode;
    data[13] = device_info.info.dmx_mode_max;

    data[14] = (device_info.info.dmx_start_addr >> 8) & 0xFF;
    data[15] = device_info.info.dmx_start_addr & 0xFF;

    data[16] = (device_info.info.sub_device >> 8) & 0xFF;
    data[17] = device_info.info.sub_device & 0xFF;
    data[18] = device_info.info.sensor;

    RDM_Package_Set_Data(19,data);

    RDM_Package_Send(dmx_line);
}
//查设备状态回应
void RDM_Get_Flag_Respone(dmx_line_t *dmx_line, uint64_t uid)
{
    RDM_Package_Set_Uid(uid);

    RDM_Package_Set_Cmd(0x21,0x1000);

    RDM_Package_Set_Data(1,&device_info.flag);

    RDM_Package_Send(dmx_line);
}
//设置设备状态回应
void RDM_Set_Flag_Respone(dmx_line_t *dmx_line, uint64_t uid)
{
    RDM_Package_Set_Uid(uid);

    RDM_Package_Set_Cmd(0x31,0x1000);

    RDM_Package_Set_Data(0,NULL);

    RDM_Package_Send(dmx_line);
}
//查设备DMX地址回应
void RDM_Get_DMX_Addr_Respone(dmx_line_t *dmx_line, uint64_t uid)
{
    RDM_Package_Set_Uid(uid);

    RDM_Package_Set_Cmd(0x21,0x00F0);

    uint8_t data[2] = {0};
    data[0] = device_info.info.dmx_start_addr>>8 & 0xFF;
    data[1] = device_info.info.dmx_start_addr>>0 & 0xFF;
    RDM_Package_Set_Data(2,data);

    RDM_Package_Send(dmx_line);
}
//设置设备DMX地址回应
void RDM_Set_DMX_Addr_Respone(dmx_line_t *dmx_line, uint64_t uid)
{
    RDM_Package_Set_Uid(uid);

    RDM_Package_Set_Cmd(0x31,0x00F0);

    RDM_Package_Set_Data(0,NULL);

    RDM_Package_Send(dmx_line);
}
//查设备支持的命令回应

//查设备自定义命令回应


/****** 标准扩展RDM协议 ******/
//查设备模式回应
void RDM_Get_Mode_Respone(dmx_line_t *dmx_line, uint64_t uid)
{
    RDM_Package_Set_Uid(uid);

    RDM_Package_Set_Cmd(0x21,0x00E0);

    uint8_t data[2] = {0};
    data[0] = device_info.info.dmx_cur_mode;
    data[1] = device_info.info.dmx_mode_max;
    RDM_Package_Set_Data(2,data);

    RDM_Package_Send(dmx_line);
}
//设置设备模式回应
void RDM_Set_Mode_Respone(dmx_line_t *dmx_line, uint64_t uid)
{
    RDM_Package_Set_Uid(uid);

    RDM_Package_Set_Cmd(0x31,0x00E0);

    RDM_Package_Set_Data(0,NULL);

    RDM_Package_Send(dmx_line);
}
//查设备模式描述回应
//void RDM_Get_Mode_Info_Respone(dmx_line_t *dmx_line, uint64_t uid)
//{

//}






/****************** RDM解包 ******************/
void RDM_Unpack(dmx_line_t *dmx_line)
{
    if(RDM_UID_TRUE)
    {
        switch(dmx_line->dmx_rdm_package[20])
        {
            //寻设备包
            case 0x10:
                if(dmx_line->dmx_rdm_package[21] == 0x00 && dmx_line->dmx_rdm_package[22] == 0x01)
                {
                    if(device_info.mute == 1)
                    {
                        dmx_line->rdm_package_prase.rdm_package = IRR_PACKAGE;
                        break;
                    }
                    //判断UID是否在低UID和高UID之间
                    uint64_t low_uid = 0;
                    uint64_t high_uid = 0;
                    low_uid = dmx_line->dmx_rdm_package[24]; low_uid <<= 8;
                    low_uid |= dmx_line->dmx_rdm_package[25]; low_uid <<= 8;
                    low_uid |= dmx_line->dmx_rdm_package[26]; low_uid <<= 8;
                    low_uid |= dmx_line->dmx_rdm_package[27]; low_uid <<= 8;
                    low_uid |= dmx_line->dmx_rdm_package[28]; low_uid <<= 8;
                    low_uid |= dmx_line->dmx_rdm_package[29];
                    high_uid = dmx_line->dmx_rdm_package[30]; high_uid <<= 8;
                    high_uid |= dmx_line->dmx_rdm_package[31]; high_uid <<= 8;
                    high_uid |= dmx_line->dmx_rdm_package[32]; high_uid <<= 8;
                    high_uid |= dmx_line->dmx_rdm_package[33]; high_uid <<= 8;
                    high_uid |= dmx_line->dmx_rdm_package[34]; high_uid <<= 8;
                    high_uid |= dmx_line->dmx_rdm_package[35];
                    if(device_info.uid >= low_uid && device_info.uid <= high_uid)
                    {
                        dmx_line->rdm_package_prase.rdm_package = DISC_UNIQUE;
                    }
                    else
                    {
                        //不用回应
                        dmx_line->rdm_package_prase.rdm_package = IRR_PACKAGE;
                    }
                }
                else if(dmx_line->dmx_rdm_package[21] == 0x00 && dmx_line->dmx_rdm_package[22] == 0x02)
                {
                    dmx_line->rdm_package_prase.rdm_package = DISC_MUTE;
                }
                else if(dmx_line->dmx_rdm_package[21] == 0x00 && dmx_line->dmx_rdm_package[22] == 0x03)
                {
                    dmx_line->rdm_package_prase.rdm_package = DISC_UN_MUTE;
                }
            break;
            //读信息包
            case 0x20:
                if(dmx_line->dmx_rdm_package[21] == 0x10 && dmx_line->dmx_rdm_package[22] == 0x00)
                {
                    dmx_line->rdm_package_prase.rdm_package = GET_DRIVER_FLAG;
                }
                else if(dmx_line->dmx_rdm_package[21] == 0x00 && dmx_line->dmx_rdm_package[22] == 0xF0)
                {
                    dmx_line->rdm_package_prase.rdm_package = GET_DRIVER_DMX_ADDR;
                }
                else if(dmx_line->dmx_rdm_package[21] == 0x00 && dmx_line->dmx_rdm_package[22] == 0xC0)
                {
                    dmx_line->rdm_package_prase.rdm_package = GET_DRIVER_VERSION;
                }
                else if(dmx_line->dmx_rdm_package[21] == 0x00 && dmx_line->dmx_rdm_package[22] == 0x60)
                {
                    dmx_line->rdm_package_prase.rdm_package = GET_DRIVER_INFO;
                }
                else if(dmx_line->dmx_rdm_package[21] == 0x00 && dmx_line->dmx_rdm_package[22] == 0xE0)
                {
                    dmx_line->rdm_package_prase.rdm_package = GET_DRIVER_MODE;
                }
            break;
            //设置包
            case 0x30:
                if(dmx_line->dmx_rdm_package[21] == 0x10 && dmx_line->dmx_rdm_package[22] == 0x00)
                {
                    dmx_line->rdm_package_prase.rdm_package = SET_DRIVER_FLAG;
                }
                else if(dmx_line->dmx_rdm_package[21] == 0x00 && dmx_line->dmx_rdm_package[22] == 0xF0)
                {
                    dmx_line->rdm_package_prase.rdm_package = SET_DRIVER_DMX_ADDR;
                }   
				else if(dmx_line->dmx_rdm_package[21] == 0x00 && dmx_line->dmx_rdm_package[22] == 0xE0)
                {
                    dmx_line->rdm_package_prase.rdm_package = SET_DRIVER_MODE;
                }
            break;
            //不支持的数据包
            default:
                dmx_line->rdm_package_prase.rdm_package = IRR_PACKAGE;
            break;
        }
        //读取数据
        for(int j = 0; j < dmx_line->dmx_rdm_package[23]; j++)
        {
            dmx_line->rdm_package_prase.data[j] = dmx_line->dmx_rdm_package[24+j];
        }
        //读取源UID
        dmx_line->rdm_package_prase.source_uid  = dmx_line->dmx_rdm_package[9];  dmx_line->rdm_package_prase.source_uid <<= 8;
        dmx_line->rdm_package_prase.source_uid |= dmx_line->dmx_rdm_package[10]; dmx_line->rdm_package_prase.source_uid <<= 8;
        dmx_line->rdm_package_prase.source_uid |= dmx_line->dmx_rdm_package[11]; dmx_line->rdm_package_prase.source_uid <<= 8;
        dmx_line->rdm_package_prase.source_uid |= dmx_line->dmx_rdm_package[12]; dmx_line->rdm_package_prase.source_uid <<= 8;
        dmx_line->rdm_package_prase.source_uid |= dmx_line->dmx_rdm_package[13]; dmx_line->rdm_package_prase.source_uid <<= 8;
        dmx_line->rdm_package_prase.source_uid |= dmx_line->dmx_rdm_package[14];
        //检测校验位
        uint32_t sum = 0;
        for(int j = 0; j < 24+dmx_line->dmx_rdm_package[23]; j++)
        {
            sum += dmx_line->dmx_rdm_package[j];
        }
        uint16_t index = 24+dmx_line->dmx_rdm_package[23];
        //!!! & 优先级小于 ==
        if((((sum >> 8) & 0xFF) != dmx_line->dmx_rdm_package[index]) || ((sum & 0xFF) != dmx_line->dmx_rdm_package[index+1]))
            dmx_line->rdm_package_prase.rdm_package = ERROR_PACKAGE;
    }
    //不需要响应的包
    else
        dmx_line->rdm_package_prase.rdm_package = IRR_PACKAGE;
}

#endif

