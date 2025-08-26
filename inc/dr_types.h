#ifndef DR_TYPES_H
#define DR_TYPES_H

/***************************************************************DMX链路配置开始***************************************************************/
/**
 * @brief DMX512链路
 */
typedef struct dmx_line_t
{
    uint8_t package_parse[DMX_CHANNEL_MAX];  ///< DMX解包结果缓冲区

    uint8_t output_black;   // DMX黑场输出标志位，置1输出为全0

    uint16_t addr;      //DMX地址
    uint16_t channel;   //DMX通道数
    uint8_t mode;       //DMX模式
} dmx_line_t;




/***************************************************************RDM协议配置开始***************************************************************/
/**
 * @brief RDM包类型枚举定义
 */
enum rdm_package {
    DISC_UNIQUE = 0,        // 搜寻设备
    DISC_MUTE,              // 哑音
    DISC_UN_MUTE,           // 解除哑音
    GET_DRIVER_FLAG,        // 获取设备状态
    GET_DRIVER_DMX_ADDR,    // 获取DMX地址
    GET_DRIVER_VERSION,     // 获取版本
    GET_DRIVER_INFO,        // 获取设备信息
    GET_DRIVER_MODE,        // 获取设备模式
    GET_DRIVER_MODE_INFO,   // 获取设备模式信息
    SET_DRIVER_MODE,        // 设置设备模式
    SET_DRIVER_DMX_ADDR,    // 设置DMX地址
    SET_DRIVER_FLAG,        // 设置设备状态
    RDM_PACKAGE_CHECK_ERROR,// 校验错误包或者非本设备数据包
};

/**
 * @brief RDM接收包结构体
 */
typedef struct rdm_package_parse_t {
    enum rdm_package rdm_package;  // RDM包类型 @see rdm_package
    uint8_t data[231];             // 最大允许填入数据（231字节）
    uint64_t source_uid;           // 发送设备的UID
} rdm_package_parse_t;


/**
 * @brief RDM扩展结构体
 */
typedef struct uid_port_t {
    uint8_t user;          // 用于扩展RDM协议
} uid_port_t;

/**
 * @brief UID链表节点（输出设备用）
 */
typedef struct uid_t {
    uint8_t online_flag;        // 设备在线状态(1-在线，0离线)
    uint8_t sync_flag;          // 设备同步状态(1-同步，0-不同步)
    uint8_t mute;               // 设备哑音状态
    uint8_t device_type;        // 设备类型
    uint8_t device_flag;        // 设备状态标志
    uint8_t device_cur_mode;    // 当前工作模式
    uint8_t device_mode_max;    // 支持的最大模式数
    uint16_t device_cur_channel;// 当前通道数
    uint16_t device_dmx;        // DMX起始地址
    uint16_t device_version;    // 设备版本号
    uint64_t uid;               // 设备UID
    uid_port_t port;            // 待扩展
    struct uid_t *next;         // 后向指针
    struct uid_t *prev;         // 前向指针
} uid_t;

/**
 * @brief RDM设备地址搜索栈结构体
 * @note RDM UID地址空间规范：
 *       - 范围：0x000000000000 ~ 0xFFFFFFFFFFFF (48位)
 *       - 搜索算法：二分查找（时间复杂度O(log₂n)) 
 *       - 最大搜索深度：48次（2⁴⁸地址空间）
 *       - 栈深度设计：
 *         1. 基础深度：48+1（理论最大值）
 *         2. +5 全范围节点
 *         总计：54级深度
 */
typedef struct rdm_search_stack_t {
    uint64_t low_uid : 48;     // 搜索范围低地址
    uint64_t high_uid : 48;    // 搜索范围高地址
} rdm_search_stack_t[54];

#define RDM_BROADCAST_ADDR 0xFFFFFFFFFFFF  // RDM广播地址

/**
 * @brief RDM发送时可能会遇见的错误
 */
enum rdm_output_errors { 
    RDM_ERROR_NULL = 0x00,              // 没有错误
    RDM_ERROR_QUEUE_FULL = 0x01,        // 队列已满
    RDM_ERROR_UNNOWM_PACKET = 0x02,     // 未知的包
    RDM_ERROR_RESPONE_TIMEOUT = 0x04,   // 响应超时
    RDM_ERROR_UNPACK_FAILD = 0x08,      // 解包失败
};

/**
 * @brief RDM数据包发送队列结构体
 * @note 实现循环队列存储管理，用于异步发送RDM命令包
 */
typedef struct rdm_send_queue_t {
    uint16_t package;                   // RDM包类型 @see enum rdm_package
    uid_t *target_uid;                  // 目标设备节点指针
} rdm_send_queue_t[RDM_SEND_QUEUE_LENGTH];

/**
 * @brief 设备详细信息结构体
 */
typedef struct info_t {
    uint16_t rdm_version;     // RDM协议版本（0x0100）
    uint16_t device_id;       // 设备类型ID
    uint16_t product;         // 产品类别
    uint32_t soft_version;    // 软件版本号
    uint16_t sub_device;      // 子设备数量
    uint8_t sensor;           // 传感器数量
} info_t;

/**
 * @brief 设备信息结构体
 */
typedef struct device_info_t {
    uint8_t  mute;          // 哑音状态
    uint64_t uid;           // 设备UID
    uint8_t  flag;          // 状态标志位
    info_t   info;          // 详细信息
} device_info_t;

/**
 * @brief RDM链路结构体
 */
typedef struct rdm_line_t {
    rdm_package_parse_t package_parse;     // RDM包解析结构体

    uint8_t last_package;                  // 上次发送的RDM包（输出设备专用）
    rdm_search_stack_t stack;              // RDM搜索栈指针（输出设备专用,输入设备置空）
    uint8_t stack_depth;                   // RDM搜索栈深度（输出设备专用）
    rdm_send_queue_t queue;                // RDM发送队列指针（输出设备专用,输入设备置空）
    uint8_t queue_head;                    // RDM发送队列头节点（输出设备专用）
    uint8_t queue_tail;                    // RDM发送队列尾节点（输出设备专用）
    uint8_t queue_full;                    // RDM发送队列满标志（输出设备专用）
    uid_t uid_list;                        // UID链表头节点（输出设备专用,输入设备置空）

    device_info_t device_info;             // 设备信息指针（输入设备专用,输出设备置空）
} rdm_line_t;




/***************************************************************通用链路配置开始***************************************************************/
/**
 * @brief 软件定时器结构体
 */
typedef struct dr_tim_t
{
    uint8_t state;        // 定时器状态 0->停止 1->运行
    uint16_t count;       // 当前计数值
    uint16_t over;        // 溢出值（触发阈值）
} dr_tim_t;

/**
 * @brief 发送/接收状态定义
 */
enum dr_mode {
    DR_SEND_RESET,       // 发送起始信号
    DR_SEND_DATA,        // 发送数据
    DR_RECV_DATA,        // 接收数据
    DR_RECV_RESET        // 接收起始信号
};


/**
 * @brief 链路模式定义
 */
enum line_mode {
    LINE_OFF = 0,       // 链路关闭
    DMX_INPUT,          // DMX协议输入
    DMX_OUTPUT,         // DMX协议输出
    RDM_INPUT,          // RDM协议输入
    RDM_OUTPUT,         // RDM协议输出
    DMX_RDM_INPUT,      // DMX+RDM协议输入
};

/**
 * @brief 接收数据包类型定义
 */
enum recv_package_type {
    NULL_PACKAGE = 0,       // 空包
    ERROR_PACKAGE,          // 接受错误
    UNKNOWN_PACKAGE,        // 未知数据包
    DMX_PACKAGE,            // DMX数据包
    RDM_PACKAGE,            // RDM数据包
    DISC_RESPONE_PACKAGE,   // 搜寻设备响应包
};

/**
 * @brief dmx+rdm协议链路
 */
typedef struct dmx_rdm_line_t
{
    /* 系统标志位 */
    struct system_flag_t
    {
        uint8_t line_mode;        // 设备运行模式标志位
        uint8_t recv_package;     // 正在接收数据包类型
        uint8_t send_status;      // 发送数据标志位
        uint8_t recv_break;       // 接收BREAK信号标志位
    } system_flag;

    /* 信号量 */
    struct system_semaphore_t
    {
        uint8_t send_semaphore;
        uint8_t recv_semaphore;
    }system_semaphore;

    /* 缓冲区 */
    struct buffer_t {
        uint8_t rx_buf[520];      // 接收缓存
        uint16_t rx_cnt;          // 接收计数
        uint8_t tx_buf[520];      // 发送缓存
    } buffer;

    /* 软件定时器 */
    struct time_t
    {
        dr_tim_t idle_tim;          // 空闲检测定时器
        dr_tim_t send_delay_tim;    // 发送延时定时器
        dr_tim_t refresh_tim;       // 刷新检测定时器
        dr_tim_t rdm_recv_tim;      // RDM接收超时定时器
        dr_tim_t send_timeout_tim;  // 发送超时检测定时器
    } time;

    dmx_line_t dmx_line;    // DMX链路
    rdm_line_t rdm_line;    // RDM链路

} dr_line_t;




/**************************************************************链路操作集配置开始**************************************************************/
/**
 * @brief DMX+RDM链路操作函数集
 */
typedef struct dmx_rdm_line_funs_t {

    /** 
     * @brief 模式切换函数指针类型
     * @note 自动收发485并且串口支持LIN模式链路不需要实现这个函数
     *       使用RDE切换发送接收，需要通过这个函数实现RDE（切换发送接收的引脚）的翻转
     *       无LIN模式的串口，需要在切换发送起始信号时将串口改为普通GPIO，随后在send_reset中实现起始信号的发送
     *       需要在接收起始信号时将GPIO改为上升沿中断检测，并在GPIO中断中实现 dr_break_handle
     * 
     *       系统接收数据调用流程：
     *       change_mode(DR_RECV_RESET);
     *       dr_break_handle();
     *       change_mode(DR_RECV_DATA);
     *       receive(...);
     * 
     *       系统发送数据调用流程：
     *       change_mode(DR_SEND_RESET);
     *       send_reset();
     *       change_mode(DR_SEND_DATA);
     *       transmit(...);
     */
    void (*change_mode)(dr_line_t *dmx_line, enum dr_mode mode);

    /**
     * @brief 起始信号发送函数指针类型
     * @note  至少需要拉低92us 然后再拉高12us
     */
    void (*send_reset)(dr_line_t *dmx_line);

    /**
     * @brief 关闭DMX+RDM链路
     * @param dmx_line DMX链路指针
     */
    void (*close)(dr_line_t *dmx_line);
    
    /**
     * @brief 接收数据
     * @param dmx_line DMX+RDM链路指针
     * @param rx_buf 接收缓冲区
     * @param data_len 数据长度
     */
    void (*receive)(dr_line_t *dmx_line, uint8_t *rx_buf, uint32_t data_len);
    
    /**
     * @brief 发送数据
     * @param dmx_line DMX+RDM链路指针
     * @param tx_buf 发送缓冲区
     * @param data_len 数据长度
     */
    void (*transmit)(dr_line_t *dmx_line, uint8_t *tx_buf, uint32_t data_len);

} dr_line_funs_t;




/*************************************************************DR链路选项快捷访问宏*************************************************************/
/* 系统标志位访问宏 */
#define __DR_LINE_MODE(dr_line)            (dr_line->system_flag.line_mode)
#define __DR_RECV_PACKAGE(dr_line)         (dr_line->system_flag.recv_package)
#define __DR_SEND_STATUS(dr_line)          (dr_line->system_flag.send_status)
#define __DR_RECV_BREAK(dr_line)           (dr_line->system_flag.recv_break)

/* 信号量访问宏 */
#define __DR_SEND_SEMAPHORE(dr_line)       (dr_line->system_semaphore.send_semaphore)
#define __DR_RECV_SEMAPHORE(dr_line)       (dr_line->system_semaphore.recv_semaphore)

/* 缓冲区访问宏 */
#define __DR_RX_BUF(dr_line)               (dr_line->buffer.rx_buf)
#define __DR_RX_CNT(dr_line)               (dr_line->buffer.rx_cnt)
#define __DR_TX_BUF(dr_line)               (dr_line->buffer.tx_buf)

/* 定时器访问宏 */
#define __DR_IDLE_TIM(dr_line)             (dr_line->time.idle_tim)
#define __DR_SEND_DELAY_TIM(dr_line)       (dr_line->time.send_delay_tim)
#define __DR_REFRESH_TIM(dr_line)          (dr_line->time.refresh_tim)
#define __DR_RDM_RECV_TIM(dr_line)         (dr_line->time.rdm_recv_tim)
#define __DR_SEND_TIMEOUT_TIM(dr_line)     (dr_line->time.send_timeout_tim)
/* DMX链路访问宏 */
#define __DR_DMX_PARSE(dr_line)            (dr_line->dmx_line.package_parse)
#define __DR_DMX_OUTPUT_BLACK(dr_line)     (dr_line->dmx_line.output_black)
#define __DR_DMX_ADDR(dr_line)             (dr_line->dmx_line.addr)
#define __DR_DMX_CHANNEL(dr_line)          (dr_line->dmx_line.channel)
#define __DR_DMX_MODE(dr_line)             (dr_line->dmx_line.mode)

/* RDM链路访问宏 */
#define __DR_RDM_PARSE(dr_line)            (dr_line->rdm_line.package_parse)
#define __DR_RDM_LAST_PACKAGE(dr_line)     (dr_line->rdm_line.last_package)
#define __DR_RDM_STACK(dr_line)            (dr_line->rdm_line.stack)
#define __DR_RDM_STACK_DEPTH(dr_line)      (dr_line->rdm_line.stack_depth)
#define __DR_RDM_QUEUE(dr_line)            (dr_line->rdm_line.queue)
#define __DR_RDM_QUEUE_HEAD(dr_line)       (dr_line->rdm_line.queue_head)
#define __DR_RDM_QUEUE_TAIL(dr_line)       (dr_line->rdm_line.queue_tail)
#define __DR_RDM_QUEUE_FULL(dr_line)       (dr_line->rdm_line.queue_full)
#define __DR_RDM_UID_LIST(dr_line)         (&dr_line->rdm_line.uid_list)
#define __DR_RDM_DEVICE_INFO(dr_line)      (dr_line->rdm_line.device_info)

#endif
