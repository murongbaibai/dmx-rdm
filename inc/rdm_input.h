#ifndef RDM_INPUT_H
#define RDM_INPUT_H

/**************************************** 结构体定义 ****************************************/
//RDM解析包定义    搜寻设备回应      哑音回应    解除哑音回应   获取设备状态回应  获取DMX地址回应      获取版本回应        获取设备信息回应 
enum rdm_package {DISC_UNIQUE = 0, DISC_MUTE, DISC_UN_MUTE, GET_DRIVER_FLAG, GET_DRIVER_DMX_ADDR,GET_DRIVER_VERSION,GET_DRIVER_INFO,
//                获取设备模式回应  获取设备模式信息回应
                  GET_DRIVER_MODE, GET_DRIVER_MODE_INFO,
//                设置设备模式回应  设置DMX地址回应       设置设备状态回应   校验错误包      无回应包      不需要本设备处理的包
                  SET_DRIVER_MODE, SET_DRIVER_DMX_ADDR, SET_DRIVER_FLAG, ERROR_PACKAGE, NULL_PACKAGE, IRR_PACKAGE};
//RDM解包返回数据
typedef struct rdm_package_prase_t
{
    //RDM包类型
    enum rdm_package rdm_package;
    //最大允许填入数据
    uint8_t data[231];
    //发送这个包的设备的UID
    uint64_t source_uid; 
}rdm_package_prase_t;

//RDM发送包结构定义
typedef struct rdm_package_t
{
    uint8_t  start;         //第0位         起始码
    uint8_t  sub_start;     //第1位         起始码
    uint8_t  message_len;   //第2位         包长（不含校验位）
    uint64_t des_uid;       //第3到8位      目的地UID
    uint64_t source_uid;    //第9到14位     源UID
    uint64_t other;         //第15到19位    无关紧要的数据
    uint8_t  cmd_type;      //第20位        命令类型
    uint16_t cmd;           //第21到22位    命令
    uint8_t  data_len;      //第23位        数据长度
    uint8_t  *data;         //23位          往后数据
    uint32_t check;         //两位校验位    （之所以是uint32_t 是为了防止计算溢出）

}rdm_package_t;

typedef struct info_t
{
    //RDM版本 0x01,0x00
    uint16_t rdm_version;
    //设备类型
    uint16_t device_id;
    //产品类别
    uint16_t product;
    //软件版本
    uint32_t soft_version;
    //DMX通道
    uint16_t dmx_channel;
    //DMX当前模式
    uint8_t dmx_cur_mode;
    //DMX总模式
    uint8_t dmx_mode_max;
    //DMX地址
    uint16_t dmx_start_addr;
    //子设备数
    uint16_t sub_device;
    //传感器个数
    uint8_t sensor;
}info_t;
typedef struct device_info_t
{
    //是否哑音
    uint8_t  mute;
    uint64_t uid;
    uint8_t  flag;
    //info
    info_t   info;
}device_info_t;

//如果UID是本设备UID 或者 UID是0xFFFFFFFFFFFF
#define RDM_UID_TRUE (dmx_line->dmx_rdm_package[3] == ((device_info.uid >> 5*8) & 0xFF) &&   \
                      dmx_line->dmx_rdm_package[4] == ((device_info.uid >> 4*8) & 0xFF) &&   \
                      dmx_line->dmx_rdm_package[5] == ((device_info.uid >> 3*8) & 0xFF) &&   \
                      dmx_line->dmx_rdm_package[6] == ((device_info.uid >> 2*8) & 0xFF) &&   \
                      dmx_line->dmx_rdm_package[7] == ((device_info.uid >> 1*8) & 0xFF) &&   \
                      dmx_line->dmx_rdm_package[8] == ((device_info.uid >> 0*8) & 0xFF)) ||  \
                     (dmx_line->dmx_rdm_package[3] == 0xFF &&                                \
                      dmx_line->dmx_rdm_package[4] == dmx_line->dmx_rdm_package[3] &&                \
                      dmx_line->dmx_rdm_package[5] == dmx_line->dmx_rdm_package[4] &&                \
                      dmx_line->dmx_rdm_package[6] == dmx_line->dmx_rdm_package[5] &&                \
                      dmx_line->dmx_rdm_package[7] == dmx_line->dmx_rdm_package[6] &&                \
                      dmx_line->dmx_rdm_package[8] == dmx_line->dmx_rdm_package[7])   

/**************************************** 变量外部声明 ****************************************/
extern device_info_t device_info;
extern rdm_package_t rdm_package;

/**************************************** 函数声明 ****************************************/
/// @brief  获取设备DMX地址
/// @return 设备DMX地址
uint16_t Device_Get_DMX_Addr(void);

/// @brief  获取设备模式
/// @return 设备模式
uint8_t Device_Get_Mode(void);

/// @brief  获取设备当前通道数
/// @return 设备当前通道数
uint16_t Device_Get_Channel(void);

/// @brief  获取设备状态信息
/// @return 设备状态信息
uint8_t Device_Get_Flag(void);

/// @brief  获取设备哑音信息
/// @return 设备哑音信息
uint8_t Device_Get_Mute(void);

/// @brief 设置设备DMX地址
/// @param dmx_addr 设备DMX地址
void Device_Set_DMX_Addr(uint16_t dmx_addr);

/// @brief 设置设备模式（同时会根据当前模式修改通道数）
/// @param mode 设备模式
void Device_Set_Mode(uint8_t mode);

/// @brief 设置设备状态信息
/// @param flag 设备状态信息（零关、非零开）
void Device_Set_Flag(uint8_t flag);

/// @brief 设置设备哑音信息
/// @param mute 设备哑音信息（零无哑音、非零哑音）
void Device_Set_Mute(uint8_t mute);

/// @brief 搜寻设备回应
/// @param dmx_line DMX数据链路
void RDM_Disc_Driver_Respone(dmx_line_t *dmx_line);

/// @brief 哑音目标设备回应
/// @param dmx_line DMX数据链路
/// @param uid      发送设备的UID
void RDM_Disc_Mute_Respone(dmx_line_t *dmx_line,uint64_t uid);

/// @brief 解除哑音目标设备回应
/// @param dmx_line DMX数据链路
/// @param uid      发送设备的UID
void RDM_Disc_Un_Mute_Respone(dmx_line_t *dmx_line,uint64_t uid);

/// @brief 获取目标设备版本信息回应
/// @param dmx_line DMX数据链路
/// @param uid      发送设备的UID
void RDM_Get_Version_Respone(dmx_line_t *dmx_line, uint64_t uid);

/// @brief 获取目标设备信息回应
/// @param dmx_line DMX数据链路
/// @param uid      发送设备的UID
void RDM_Get_Info_Respone(dmx_line_t *dmx_line, uint64_t uid);

/// @brief 获取目标设备状态回应
/// @param dmx_line DMX数据链路
/// @param uid      发送设备的UID
void RDM_Get_Flag_Respone(dmx_line_t *dmx_line, uint64_t uid);

/// @brief 设置目标设备状态回应
/// @param dmx_line DMX数据链路
/// @param uid      发送设备的UID
void RDM_Set_Flag_Respone(dmx_line_t *dmx_line, uint64_t uid);

/// @brief 获取目标设备DMX地址回应
/// @param dmx_line DMX数据链路
/// @param uid      发送设备的UID
void RDM_Get_DMX_Addr_Respone(dmx_line_t *dmx_line, uint64_t uid);

/// @brief 设置目标设备DMX地址回应
/// @param dmx_line DMX数据链路
/// @param uid      发送设备的UID
void RDM_Set_DMX_Addr_Respone(dmx_line_t *dmx_line, uint64_t uid);

/// @brief 获取目标设备支持的命令回应
/// @param dmx_line DMX数据链路
/// @param uid      发送设备的UID
void RDM_Get_PID_Respone(dmx_line_t *dmx_line, uint64_t uid);

/// @brief 获取目标设备自定义的命令回应
/// @param dmx_line DMX数据链路
/// @param uid      发送设备的UID
void RDM_Get_Cust_PID_Respone(dmx_line_t *dmx_line, uint64_t uid);

/// @brief 获取目标设备模式回应
/// @param dmx_line DMX数据链路
/// @param uid      发送设备的UID
void RDM_Get_Mode_Respone(dmx_line_t *dmx_line, uint64_t uid);

/// @brief 设置目标设备模式回应
/// @param dmx_line DMX数据链路
/// @param uid      发送设备的UID
void RDM_Set_Mode_Respone(dmx_line_t *dmx_line, uint64_t uid);

/// @brief 获取目标设备模式信息回应
/// @param dmx_line DMX数据链路
/// @param uid      发送设备的UID
void RDM_Get_Mode_Info_Respone(dmx_line_t *dmx_line, uint64_t uid);

/// @brief RDM解包
/// @param dmx_line DMX数据链路
void RDM_Unpack(dmx_line_t *dmx_line);

#endif
