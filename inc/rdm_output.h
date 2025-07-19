#ifndef RDM_OUTPUT_H
#define RDM_OUTPUT_H



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

//UID链表定义(单向循环链表)
typedef struct uid_t
{
    //设备哑音状态
    uint8_t mute;    
    //设备类型
    uint8_t device_type;
    //设备状态
    uint8_t device_flag;
    //当前模式，总模式数
    uint8_t device_cur_mode;
    uint8_t device_mode_max;
    //当前通道数
    uint16_t device_cur_channel;
    //设备DMX地址
    uint16_t device_dmx;
    //设备版本
    uint16_t device_version;
    //设备RDM地址
    uint64_t uid;
    struct uid_t *next;
    struct uid_t *prev;
}uid_t;
//黑场数组定义
extern uint8_t dmx_black_buf[513];



/**************************************** 变量外部声明 ****************************************/
extern uid_t *uid_list;
extern uid_t *cur_uid;
extern rdm_package_t rdm_package;


/**************************************** 函数声明 ****************************************/

__weak void RDM_Device_Offline_Callback(dmx_line_t *dmx_line, uid_t *p);
__weak void RDM_Device_Online_Callback(dmx_line_t *dmx_line, uid_t *p);

/// @brief 初始化UID链表
void    RDM_Uid_Init     (void);

/// @brief 销毁UID链表
void    RDM_Uid_Distory  (void);

/// @brief 头插法加入节点
/// @param uid      节点uid
/// @return         加入成功返回1 加入失败返回0
uint8_t RDM_Uid_Add(uint64_t uid);

/// @brief 指定UID搜寻节点
/// @param uid      节点uid
/// @return         搜寻到节点返回节点指针 没有搜寻到返回NULL
uid_t *RDM_Uid_Search(uint64_t uid);

/// @brief 删除指定UID的节点
/// @param uid 指定的UID
void    RDM_Uid_Delete   (uint64_t uid);

/// @brief 删除指定的节点
/// @param p 指定的节点
void    RDM_Device_Delete(uid_t *p);

/// @brief 获取UID链表中设备的总数
/// @return 设备总数（头节点不计入）
uint8_t RDM_Uid_Get_Count(void);



/// @brief 哑音目标设备
/// @param dmx_line DMX数据链路
/// @param p        目标设备指针
/// @return         目标设备指针（如果目标设备未回应会删除设备，并返回NULL）
uid_t *RDM_Disc_Mute(dmx_line_t *dmx_line,uid_t *p);

/// @brief 解除目标设备哑音
/// @param dmx_line DMX数据链路
/// @param p        目标设备指针
/// @return         目标设备指针（如果目标设备未回应会删除设备，并返回NULL）
uid_t *RDM_Disc_Un_Mute(dmx_line_t *dmx_line,uid_t *p);

/// @brief 解除所有设备哑音
/// @param dmx_line DMX数据链路
void RDM_Disc_Un_Mute_All(dmx_line_t *dmx_line);

/// @brief 获取目标设备版本信息
/// @param dmx_line DMX数据链路
/// @param p        目标设备指针
/// @return         目标设备指针（如果目标设备未回应会删除设备，并返回NULL）
uid_t * RDM_Get_Version(dmx_line_t *dmx_line,uid_t *p);

/// @brief 获取目标设备信息
/// @param dmx_line DMX数据链路
/// @param p        目标设备指针
/// @return         目标设备指针（如果目标设备未回应会删除设备，并返回NULL）
uid_t * RDM_Get_Info(dmx_line_t *dmx_line,uid_t *p);

/// @brief 获取目标设备状态
/// @param dmx_line DMX数据链路
/// @param p        目标设备指针
/// @return         目标设备指针（如果目标设备未回应会删除设备，并返回NULL）
uid_t * RDM_Get_Flag(dmx_line_t *dmx_line,uid_t *p);

/// @brief 设置目标设备状态
/// @param dmx_line DMX数据链路
/// @param p        目标设备指针
/// @return         目标设备指针（如果目标设备未回应会删除设备，并返回NULL）
uid_t * RDM_Set_Flag(dmx_line_t *dmx_line,uid_t *p);

/// @brief 获取目标设备DMX地址
/// @param dmx_line DMX数据链路
/// @param p        目标设备指针
/// @return         目标设备指针（如果目标设备未回应会删除设备，并返回NULL）
uid_t * RDM_Get_DMX_Addr(dmx_line_t *dmx_line,uid_t *p);

/// @brief 设置目标设备DMX地址
/// @param dmx_line DMX数据链路
/// @param p        目标设备指针
/// @return         目标设备指针（如果目标设备未回应会删除设备，并返回NULL）
uid_t * RDM_Set_DMX_Addr(dmx_line_t *dmx_line,uid_t *p);

/// @brief 获取目标设备支持的命令
/// @param dmx_line DMX数据链路
/// @param p        目标设备指针
/// @return         目标设备指针（如果目标设备未回应会删除设备，并返回NULL）
uid_t * RDM_Get_PID(dmx_line_t *dmx_line,uid_t *p);

/// @brief 获取目标设备自定义的命令
/// @param dmx_line DMX数据链路
/// @param p        目标设备指针
/// @return         目标设备指针（如果目标设备未回应会删除设备，并返回NULL）
uid_t * RDM_Get_Cust_PID(dmx_line_t *dmx_line,uid_t *p);

/// @brief 获取目标设备模式
/// @param dmx_line DMX数据链路
/// @param p        目标设备指针
/// @return         目标设备指针（如果目标设备未回应会删除设备，并返回NULL）
uid_t * RDM_Get_Mode(dmx_line_t *dmx_line,uid_t *p);

/// @brief 设置目标设备模式
/// @param dmx_line DMX数据链路
/// @param p        目标设备指针
/// @return         目标设备指针（如果目标设备未回应会删除设备，并返回NULL）
uid_t * RDM_Set_Mode(dmx_line_t *dmx_line,uid_t *p);

/// @brief 获取目标设备模式信息
/// @param dmx_line DMX数据链路
/// @param p        目标设备指针
/// @return         目标设备指针（如果目标设备未回应会删除设备，并返回NULL）
uid_t * RDM_Get_Mode_Info(dmx_line_t *dmx_line,uid_t *p);

/// @brief 执行二分搜寻算法
/// @param dmx_line DMX数据链路
/// @param low_uid  搜寻设备的最小UID 默认值：0x0000000000000000
/// @param high_uid 搜寻设备的最大UID 默认值：0x0000FFFFFFFFFFFF
void RDM_Binary_Search(dmx_line_t *dmx_line, uint64_t low_uid, uint64_t high_uid);

/// @brief RDM解包
/// @param dmx_line DMX数据链路
void RDM_Unpack(dmx_line_t *dmx_line);

#endif
