#include "dmx_rdm.h"

#if (DEVICE_TYPE_SWITCH == DEVICE_TYPE_OUTPUT) 
uid_t *uid_list;
uid_t *cur_uid;

__weak void RDM_Device_Offline_Callback(dmx_line_t *dmx_line, uid_t *p)
{
}
__weak void RDM_Device_Online_Callback(dmx_line_t *dmx_line, uid_t *p)
{
}

/******************UID链表配置函数******************/
//头插法加UID
uint8_t RDM_Uid_Add(uint64_t uid)
{
    //如果是已经添加过的设备就退出
    if(RDM_Uid_Search(uid) != NULL)
        return 0;
    uid_t* new_uid = (uid_t *)malloc(sizeof(uid_t));
    if(new_uid == NULL)
        return 0;
    new_uid->uid = uid;
    new_uid->next = uid_list->next;
    uid_list->next = new_uid;

    new_uid->prev = uid_list;
    new_uid->next->prev = new_uid;
    return 1;
}
//初始化UID链表
void RDM_Uid_Init(void)
{
    if(uid_list == NULL)
        uid_list = (uid_t *)malloc(sizeof(uid_t));
    //头节点存厂商ID
    uid_list->uid = RDM_DEFAULT_UID;
    uid_list->next = uid_list;
    uid_list->prev = uid_list;
}
//搜寻UID返回操作指针
uid_t *RDM_Uid_Search(uint64_t uid)
{
    uid_t *p = uid_list->next;
    while(p != uid_list)
    {
        if(p->uid == uid)
            return p;
        p=p->next;
    }
    return NULL;
}
//销毁链表
void RDM_Uid_Distory(void)
{
    uid_t *p = uid_list->next;

    while(p != uid_list)
    {
        uid_list->next = p->next;
        free(p);
        p = uid_list->next;
    }
    uid_list->next = uid_list;
    uid_list->prev = uid_list;
}
//根据UID删除设备
void RDM_Uid_Delete(uint64_t uid)
{
    uid_t *p = uid_list->next;
    while(p != uid_list)
    {
        if(p->uid == uid)
        {
            p->prev->next = p->next;
            p->next->prev = p->prev;
            free(p);
            return;
        }
        p=p->next;
    }
}
//直接删除设备
void RDM_Device_Delete(uid_t *p)
{
    p->prev->next = p->next;
    p->next->prev = p->prev;
    free(p);
}
//获取设备总数
uint8_t RDM_Uid_Get_Count(void)
{
    uint8_t device_max = 0;

    uid_t *p = uid_list->next;
    while(p != uid_list)
    {
        device_max++;         
        p = p->next;
    }
    return device_max;
}

/******************发送RDM包******************/
//RDM包初始化
rdm_package_t rdm_package = {
    .start = 0xCC,
    .sub_start = 0x01,
    .source_uid = RDM_DEFAULT_UID,
    .other = 0x0000000001000000,
    .check = 0,
};
/****** 标准最小RDM协议 ******/
//广播包
static void RDM_Disc_Driver(dmx_line_t *dmx_line, uint64_t low_uid,uint64_t high_uid)
{
    //广播地址
    RDM_Package_Set_Uid(0x0000FFFFFFFFFFFF);
    
    //寻找设备
    RDM_Package_Set_Cmd(0x10,0x0001);

    //设置数据
    uint8_t data[12];
    data[0] = (low_uid >> 40) & 0xFF;
    data[1] = (low_uid >> 32) & 0xFF;
    data[2] = (low_uid >> 24) & 0xFF;
    data[3] = (low_uid >> 16) & 0xFF;
    data[4] = (low_uid >> 8) & 0xFF;
    data[5] = low_uid & 0xFF;

    data[6]  = (high_uid >> 40) & 0xFF;
    data[7]  = (high_uid >> 32) & 0xFF;
    data[8]  = (high_uid >> 24) & 0xFF;
    data[9]  = (high_uid >> 16) & 0xFF;
    data[10] = (high_uid >> 8) & 0xFF;
    data[11] = high_uid & 0xFF;
    RDM_Package_Set_Data(0x0C,data);

    RDM_Package_Send(dmx_line);
}
//哑音包
uid_t *RDM_Disc_Mute(dmx_line_t *dmx_line,uid_t *p)
{
    if(p == NULL)
        return NULL;
    RDM_Package_Set_Uid(p->uid);

    RDM_Package_Set_Cmd(0x10,0x0002);

    RDM_Package_Set_Data(0,NULL);

    //三次冗余，三次都回应错误删除设备
    for(uint8_t i = 0; i < 3; i++)
    {
        RDM_Package_Send(dmx_line);
        HAL_Delay(100);
        {
            if(dmx_line->rdm_package_prase.rdm_package == DISC_MUTE)
            {
                p->mute = 1;
                //正确回应，退出
                return p;
            }
        }
    }
    //设备未回应回调函数
    RDM_Device_Offline_Callback(dmx_line, p);
    //删除设备
    RDM_Device_Delete(p);


    return NULL;
}
//解除哑音包
uid_t *RDM_Disc_Un_Mute(dmx_line_t *dmx_line,uid_t *p)
{
    if(p == NULL)
        return NULL;
    RDM_Package_Set_Uid(p->uid);

    RDM_Package_Set_Cmd(0x10,0x0003);

    RDM_Package_Set_Data(0,NULL);

    //三次冗余，三次都回应错误删除设备
    for(uint8_t i = 0; i < 3; i++)
    {
        RDM_Package_Send(dmx_line);
        HAL_Delay(100);
        {
            if(dmx_line->rdm_package_prase.rdm_package == DISC_UN_MUTE)
            {
                p->mute = 0;
                //正确回应，退出
                return p;
            }
        }
    }
    //设备未回应回调函数
    RDM_Device_Offline_Callback(dmx_line, p);
    //删除设备
    RDM_Device_Delete(p);


    return NULL;
}
//解除所有哑音
void RDM_Disc_Un_Mute_All(dmx_line_t *dmx_line)
{
    RDM_Package_Set_Uid(0x0000FFFFFFFFFFFF);

    RDM_Package_Set_Cmd(0x10,0x0003);

    RDM_Package_Set_Data(0,NULL);

    RDM_Package_Send(dmx_line);
	
	HAL_Delay(100);
}
//查设备软件版本
uid_t * RDM_Get_Version(dmx_line_t *dmx_line,uid_t *p)
{
    if(p == NULL)
        return NULL;
    RDM_Package_Set_Uid(p->uid);

    RDM_Package_Set_Cmd(0x20,0x00C0);

    RDM_Package_Set_Data(0,NULL);

    //三次冗余，三次都回应错误删除设备
    for(uint8_t i = 0; i < 3; i++)
    {
        RDM_Package_Send(dmx_line);
        HAL_Delay(100);
        {
            if(dmx_line->rdm_package_prase.rdm_package == GET_DRIVER_VERSION)
            {
                p->device_version = dmx_line->rdm_package_prase.data[0];
                p->device_version <<= 8;
                p->device_version |= dmx_line->rdm_package_prase.data[1];
                //正确回应，退出
                return p;
            }
        }
    }
    //设备未回应回调函数
    RDM_Device_Offline_Callback(dmx_line, p);
    //删除设备
    RDM_Device_Delete(p);


    return NULL;
}
//查设备信息
uid_t * RDM_Get_Info(dmx_line_t *dmx_line,uid_t *p)
{
    if(p == NULL)
        return NULL;
    RDM_Package_Set_Uid(p->uid);

    RDM_Package_Set_Cmd(0x20,0x0060);

    RDM_Package_Set_Data(0,NULL);

    //三次冗余，三次都回应错误删除设备
    for(uint8_t i = 0; i < 3; i++)
    {
        RDM_Package_Send(dmx_line);
        HAL_Delay(100);
        {
            if(dmx_line->rdm_package_prase.rdm_package == GET_DRIVER_INFO)
            {
                //解析DMX地址
                p->device_dmx = dmx_line->rdm_package_prase.data[14];
                p->device_dmx <<= 8;
                p->device_dmx |= dmx_line->rdm_package_prase.data[15];
                //解析通道数
                p->device_cur_channel = dmx_line->rdm_package_prase.data[10];
                p->device_cur_channel <<= 8;
                p->device_cur_channel = dmx_line->rdm_package_prase.data[11];
                //解析模式
                p->device_cur_mode = dmx_line->rdm_package_prase.data[12];
                p->device_mode_max = dmx_line->rdm_package_prase.data[13];
                //错误检测
                if(p->device_cur_mode > p->device_mode_max || p->device_mode_max==0 || p->device_cur_mode==0)
                {
                    p->device_cur_mode = 1;
                    p->device_mode_max = 1;
                }
                if(p->device_cur_channel > 512)
                {
                    p->device_cur_channel = 512;
                }
                if(p->device_dmx > 512 || p->device_dmx == 0)
                {
                    p->device_dmx = 1;
                }
                //正确回应，退出
                return p;
            }
        }
    }
    //设备未回应回调函数
    RDM_Device_Offline_Callback(dmx_line, p);
    //删除设备
    RDM_Device_Delete(p);


    return NULL;
}
//查设备状态
uid_t * RDM_Get_Flag(dmx_line_t *dmx_line,uid_t *p)
{
    if(p == NULL)
        return NULL;
    RDM_Package_Set_Uid(p->uid);

    RDM_Package_Set_Cmd(0x20,0x1000);

    RDM_Package_Set_Data(0,NULL);

    //三次冗余，三次都回应错误删除设备
    for(uint8_t i = 0; i < 3; i++)
    {
        RDM_Package_Send(dmx_line);
        HAL_Delay(100);
        {
            if(dmx_line->rdm_package_prase.rdm_package == GET_DRIVER_FLAG)
            {
                p->device_flag = dmx_line->rdm_package_prase.data[0];
                //正确回应，退出
                return p;
            }
        }
    }
    //设备未回应回调函数
    RDM_Device_Offline_Callback(dmx_line, p);
    //删除设备
    RDM_Device_Delete(p);


    return NULL;
}
//设置设备状态
uid_t * RDM_Set_Flag(dmx_line_t *dmx_line,uid_t *p)
{
    if(p == NULL)
        return NULL;
    RDM_Package_Set_Uid(p->uid);

    RDM_Package_Set_Cmd(0x30,0x1000);

    RDM_Package_Set_Data(1,&p->device_flag);

    //三次冗余，三次都回应错误删除设备
    for(uint8_t i = 0; i < 3; i++)
    {
        RDM_Package_Send(dmx_line);
        HAL_Delay(100);
        {
            if(dmx_line->rdm_package_prase.rdm_package == SET_DRIVER_FLAG)
            {
                //正确回应，退出
                return p;
            }
        }
    }
    //设备未回应回调函数
    RDM_Device_Offline_Callback(dmx_line, p);
    //删除设备
    RDM_Device_Delete(p);

    return NULL;
}
//查设备DMX地址
uid_t * RDM_Get_DMX_Addr(dmx_line_t *dmx_line,uid_t *p)
{
    if(p == NULL)
        return NULL;
    RDM_Package_Set_Uid(p->uid);

    RDM_Package_Set_Cmd(0x20,0x00F0);

    RDM_Package_Set_Data(0,NULL);

    //三次冗余，三次都回应错误删除设备
    for(uint8_t i = 0; i < 3; i++)
    {
        RDM_Package_Send(dmx_line);
        HAL_Delay(100);
        {
            if(dmx_line->rdm_package_prase.rdm_package == GET_DRIVER_DMX_ADDR)
            {
                p->device_dmx = dmx_line->rdm_package_prase.data[0];
                p->device_dmx <<= 8;
                p->device_dmx |= dmx_line->rdm_package_prase.data[1];
                //正确回应，退出
                return p;
            }
        }
    }
    //设备未回应回调函数
    RDM_Device_Offline_Callback(dmx_line, p);
    //删除设备
    RDM_Device_Delete(p);


    return NULL;
}
//设置设备DMX地址
uid_t * RDM_Set_DMX_Addr(dmx_line_t *dmx_line,uid_t *p)
{
    if(p == NULL)
        return NULL;
    RDM_Package_Set_Uid(p->uid);

    RDM_Package_Set_Cmd(0x30,0x00F0);

    uint8_t data[2] = {0};
    data[0] = (p->device_dmx >> 8) & 0xFF;
    data[1] = p->device_dmx & 0xFF;
    RDM_Package_Set_Data(2,data);

    //三次冗余，三次都回应错误删除设备
    for(uint8_t i = 0; i < 3; i++)
    {
        RDM_Package_Send(dmx_line);
        HAL_Delay(100);
        {
            if(dmx_line->rdm_package_prase.rdm_package == SET_DRIVER_DMX_ADDR)
            {
                //正确回应，退出
                return p;
            }
        }
    }
    //设备未回应回调函数
    RDM_Device_Offline_Callback(dmx_line, p);
    //删除设备
    RDM_Device_Delete(p);

    return NULL;
}
//查设备支持的命令
uid_t * RDM_Get_PID(dmx_line_t *dmx_line,uid_t *p)
{
    if(p == NULL)
        return NULL;
    RDM_Package_Set_Uid(p->uid);

    RDM_Package_Set_Cmd(0x20,0x0050);

    RDM_Package_Set_Data(0,NULL);

    //三次冗余，三次都回应错误删除设备
    for(uint8_t i = 0; i < 3; i++)
    {
        RDM_Package_Send(dmx_line);
        HAL_Delay(100);
        {
            if(dmx_line->rdm_package_prase.rdm_package == IRR_PACKAGE)
            {
                //正确回应，退出
                return p;
            }
        }
    }
    //设备未回应回调函数
    RDM_Device_Offline_Callback(dmx_line, p);
    //删除设备
    RDM_Device_Delete(p);


    return NULL;
}
//查设备自定义命令
uid_t * RDM_Get_Cust_PID(dmx_line_t *dmx_line,uid_t *p)
{
    if(p == NULL)
        return NULL;
    RDM_Package_Set_Uid(p->uid);

    RDM_Package_Set_Cmd(0x20,0x0051);

    RDM_Package_Set_Data(0,NULL);

    //三次冗余，三次都回应错误删除设备
    for(uint8_t i = 0; i < 3; i++)
    {
        RDM_Package_Send(dmx_line);
        HAL_Delay(100);
        {
            if(dmx_line->rdm_package_prase.rdm_package == IRR_PACKAGE)
            {
                //正确回应，退出
                return p;
            }
        }
    }
    //设备未回应回调函数
    RDM_Device_Offline_Callback(dmx_line, p);
    //删除设备
    RDM_Device_Delete(p);


    return NULL;
}
/****** 标准扩展RDM协议 ******/
//查设备模式
uid_t * RDM_Get_Mode(dmx_line_t *dmx_line,uid_t *p)
{
    if(p == NULL)
        return NULL;
    RDM_Package_Set_Uid(p->uid);

    RDM_Package_Set_Cmd(0x20,0x00E0);

    RDM_Package_Set_Data(0,NULL);

    //三次冗余，三次都回应错误删除设备
    for(uint8_t i = 0; i < 3; i++)
    {
        RDM_Package_Send(dmx_line);
        HAL_Delay(100);
        {
            if(dmx_line->rdm_package_prase.rdm_package == GET_DRIVER_MODE)
            {
                p->device_cur_mode = dmx_line->rdm_package_prase.data[0];
                p->device_mode_max = dmx_line->rdm_package_prase.data[1];
                //正确回应，退出
                return p;
            }
        }
    }
    //设备未回应回调函数
    RDM_Device_Offline_Callback(dmx_line, p);
    //删除设备
    RDM_Device_Delete(p);


    return NULL;
}
//设置设备模式
uid_t * RDM_Set_Mode(dmx_line_t *dmx_line,uid_t *p)
{
    if(p == NULL)
        return NULL;
    RDM_Package_Set_Uid(p->uid);

    RDM_Package_Set_Cmd(0x30,0x00E0);

    RDM_Package_Set_Data(1,&p->device_cur_mode);

    //三次冗余，三次都回应错误删除设备
    for(uint8_t i = 0; i < 3; i++)
    {
        RDM_Package_Send(dmx_line);
        HAL_Delay(100);
        {
            if(dmx_line->rdm_package_prase.rdm_package == SET_DRIVER_MODE)
            {
                //正确回应，退出
                return p;
            }
        }
    }
    //设备未回应回调函数
    RDM_Device_Offline_Callback(dmx_line, p);
    //删除设备
    RDM_Device_Delete(p);

    return NULL;
}
//查设备模式描述(当前模式通道数)
uid_t * RDM_Get_Mode_Info(dmx_line_t *dmx_line,uid_t *p)
{
    if(p == NULL)
        return NULL;
    RDM_Package_Set_Uid(p->uid);

    RDM_Package_Set_Cmd(0x20,0x00E1);

    RDM_Package_Set_Data(1,&p->device_cur_mode);

    //三次冗余，三次都回应错误删除设备
    for(uint8_t i = 0; i < 3; i++)
    {
        RDM_Package_Send(dmx_line);
        HAL_Delay(100);
        {
            if(dmx_line->rdm_package_prase.rdm_package == GET_DRIVER_MODE_INFO)
            {
                p->device_cur_channel = dmx_line->rdm_package_prase.data[0];
                p->device_cur_channel <<= 8;
                p->device_cur_channel |= dmx_line->rdm_package_prase.data[1];
                //正确回应，退出
                return p;
            }
        }
    }
    //设备未回应回调函数
    RDM_Device_Offline_Callback(dmx_line, p);
    //删除设备
    RDM_Device_Delete(p);
    return NULL;
}


/******************二分搜寻设备算法******************/
void RDM_Binary_Search(dmx_line_t *dmx_line, uint64_t low_uid, uint64_t high_uid)
{
    RDM_Disc_Driver(dmx_line, low_uid, high_uid); 
    //等待解包成功
    HAL_Delay(100);
    {
        //确定唯一设备
        if(dmx_line->rdm_package_prase.rdm_package == DISC_UNIQUE)
        {
            uint64_t uid = 0;
            //解析UID
            for(int j = 0; j < 5; j++)
            {
                uid |= dmx_line->rdm_package_prase.data[j];
                uid <<= 8;
            }
            uid |= dmx_line->rdm_package_prase.data[5];
            
            //如果未添加过该设备
            if(RDM_Uid_Add(uid))
            {
                //获取操作指针
                uid_t *p = RDM_Uid_Search(uid);
                //哑音
                p = RDM_Disc_Mute(dmx_line, p);
                //获取设备信息
                p = RDM_Get_Info(dmx_line, p);
                p = RDM_Get_Flag(dmx_line1, p);
                //成功添加设备、通知更新显示
                if(p != NULL)
                    RDM_Device_Online_Callback(dmx_line, p);
            }
            //已添加过该设备
            else
            {
                //获取操作指针
                uid_t *p = RDM_Uid_Search(uid);
                //哑音
                p = RDM_Disc_Mute(dmx_line, p);
            }
            //再次搜索一次该分支
            RDM_Binary_Search(dmx_line, low_uid, high_uid);
            return;
        }
        //可能有两个UID相同的设备
        else if(low_uid == high_uid)
        {
            //如果未添加过该设备
            if(RDM_Uid_Add(low_uid))
            {
                //获取操作指针
                uid_t *p = RDM_Uid_Search(low_uid);
                //哑音
                p = RDM_Disc_Mute(dmx_line, p);
                //获取设备信息
                p = RDM_Get_Info(dmx_line, p);
                p = RDM_Get_Flag(dmx_line1, p);
                //成功添加设备、通知更新显示
                if(p != NULL)
                    RDM_Device_Online_Callback(dmx_line, p);
            }
            //已添加过该设备
            else
            {
                //获取操作指针
                uid_t *p = RDM_Uid_Search(low_uid);
                //哑音
                p = RDM_Disc_Mute(dmx_line, p);
            }
            return;
        }
        //冲突
        else
        {
            uint64_t mid_uid = (high_uid + low_uid)/2;
            RDM_Binary_Search(dmx_line, low_uid,mid_uid);
            RDM_Binary_Search(dmx_line, mid_uid+1,high_uid);
            return;
        }
    }
//    // 超时未获取到信号量
//    return;
}


/******************发送端解包函数******************/
void RDM_Unpack(dmx_line_t *dmx_line)
{
    //定位包头
    uint8_t disc_unique = 0;
    //用于确定包长
    int i = 0;
    for(i = 0; i < 255; i++)
    {
        if(dmx_line->dmx_rdm_package[i] == 0xCC && dmx_line->dmx_rdm_package[i+1] == 0x01)
            break;
        if(dmx_line->dmx_rdm_package[i] == 0xFE)
        {
            disc_unique++;
            if(disc_unique == 5)
                break;
        }
    }
    if(i == 255)
    {
        dmx_line->rdm_package_prase.rdm_package = IRR_PACKAGE;
        return;
    }
    //广播包
    if(disc_unique == 5)
    {
        //跳过0xFE
        while(dmx_line->dmx_rdm_package[++i] == 0xFE);
        //再次确认
        if(dmx_line->dmx_rdm_package[i++] == 0xAA)
        {
            for(int j = 0; j < 6; j++)
            {
                // | 0XAA
                dmx_line->rdm_package_prase.data[j] = dmx_line->dmx_rdm_package[i]&0x55; 
                // | 0x55
                dmx_line->rdm_package_prase.data[j] |= dmx_line->dmx_rdm_package[i+1]&0xAA; 
                i+=2;
            }

            uint32_t sum = 0;
            for(int j = 0; j < 6; j++)
            {
                sum += dmx_line->rdm_package_prase.data[j]|0xAA;
                sum += dmx_line->rdm_package_prase.data[j]|0x55;
            }
            uint8_t checksum[4] = {0};
            checksum[0] = (sum >> 8)|0xAA;
            checksum[1] = (sum >> 8)|0x55;
            checksum[2] = sum|0xAA;
            checksum[3] = sum|0x55;
            if(dmx_line->dmx_rdm_package[i] == checksum[0] && dmx_line->dmx_rdm_package[i+1] == checksum[1] && dmx_line->dmx_rdm_package[i+2] == checksum[2] && dmx_line->dmx_rdm_package[i+3] == checksum[3])
            {
                dmx_line->rdm_package_prase.rdm_package = DISC_UNIQUE;
            }
            else
                dmx_line->rdm_package_prase.rdm_package = ERROR_PACKAGE;
            i+=4;
        }
    }
    //其他包
    else
    {
        switch(dmx_line->dmx_rdm_package[i+20])
        {
            case 0x11:
                if(dmx_line->dmx_rdm_package[i+21] == 0x00 && dmx_line->dmx_rdm_package[i+22] == 0x02)
                {
                    dmx_line->rdm_package_prase.rdm_package = DISC_MUTE;
                }
                else if(dmx_line->dmx_rdm_package[i+21] == 0x00 && dmx_line->dmx_rdm_package[i+22] == 0x03)
                {
                    dmx_line->rdm_package_prase.rdm_package = DISC_UN_MUTE;
                }
            break;
            case 0x21:
                if(dmx_line->dmx_rdm_package[i+21] == 0x10 && dmx_line->dmx_rdm_package[i+22] == 0x00)
                {
                    dmx_line->rdm_package_prase.rdm_package = GET_DRIVER_FLAG;
                }
                else if(dmx_line->dmx_rdm_package[i+21] == 0x00 && dmx_line->dmx_rdm_package[i+22] == 0xF0)
                {
                    dmx_line->rdm_package_prase.rdm_package = GET_DRIVER_DMX_ADDR;
                }
                else if(dmx_line->dmx_rdm_package[i+21] == 0x00 && dmx_line->dmx_rdm_package[i+22] == 0xC0)
                {
                    dmx_line->rdm_package_prase.rdm_package = GET_DRIVER_VERSION;
                }
                else if(dmx_line->dmx_rdm_package[i+21] == 0x00 && dmx_line->dmx_rdm_package[i+22] == 0x60)
                {
                    dmx_line->rdm_package_prase.rdm_package = GET_DRIVER_INFO;
                }
                else if(dmx_line->dmx_rdm_package[i+21] == 0x00 && dmx_line->dmx_rdm_package[i+22] == 0xE0)
                {
                    dmx_line->rdm_package_prase.rdm_package = GET_DRIVER_MODE;
                }
                else if(dmx_line->dmx_rdm_package[i+21] == 0x00 && dmx_line->dmx_rdm_package[i+22] == 0xE1)
                {
                    dmx_line->rdm_package_prase.rdm_package = GET_DRIVER_MODE_INFO;
                }
            break;
            case 0x31:
                if(dmx_line->dmx_rdm_package[i+21] == 0x10 && dmx_line->dmx_rdm_package[i+22] == 0x00)
                {
                    dmx_line->rdm_package_prase.rdm_package = SET_DRIVER_FLAG;
                }
                else if(dmx_line->dmx_rdm_package[i+21] == 0x00 && dmx_line->dmx_rdm_package[i+22] == 0xF0)
                {
                    dmx_line->rdm_package_prase.rdm_package = SET_DRIVER_DMX_ADDR;
                }
                else if(dmx_line->dmx_rdm_package[i+21] == 0x00 && dmx_line->dmx_rdm_package[i+22] == 0xE0)
                {
                    dmx_line->rdm_package_prase.rdm_package = SET_DRIVER_MODE;
                }
            break;
            default:
                dmx_line->rdm_package_prase.rdm_package = IRR_PACKAGE;
            break;
        }
        //记录数据
        for(int j = 0; j < dmx_line->dmx_rdm_package[i+23]; j++)
        {
            dmx_line->rdm_package_prase.data[j] = dmx_line->dmx_rdm_package[i+24+j];
        }
        //记录源UID
        dmx_line->rdm_package_prase.source_uid = dmx_line->dmx_rdm_package[i+9];
        dmx_line->rdm_package_prase.source_uid <<= 8;
        dmx_line->rdm_package_prase.source_uid |= dmx_line->dmx_rdm_package[i+10];
        dmx_line->rdm_package_prase.source_uid <<= 8;
        dmx_line->rdm_package_prase.source_uid |= dmx_line->dmx_rdm_package[i+11];
        dmx_line->rdm_package_prase.source_uid <<= 8;
        dmx_line->rdm_package_prase.source_uid |= dmx_line->dmx_rdm_package[i+12];
        dmx_line->rdm_package_prase.source_uid <<= 8;
        dmx_line->rdm_package_prase.source_uid |= dmx_line->dmx_rdm_package[i+13];
        dmx_line->rdm_package_prase.source_uid <<= 8;
        dmx_line->rdm_package_prase.source_uid |= dmx_line->dmx_rdm_package[i+14];


        //计算校验位
        uint32_t sum = 0;
        for(int j = i; j < i+24+dmx_line->dmx_rdm_package[i+23]; j++)
        {
            sum += dmx_line->dmx_rdm_package[j];
        }
        i += 24+dmx_line->dmx_rdm_package[i+23];
        //检测校验位 !!! & 优先级小于 ==
        if((((sum >> 8) & 0xFF) == dmx_line->dmx_rdm_package[i]) && ((sum & 0xFF) == dmx_line->dmx_rdm_package[i+1]));
        else
            dmx_line->rdm_package_prase.rdm_package = ERROR_PACKAGE;
    }
}
#endif


