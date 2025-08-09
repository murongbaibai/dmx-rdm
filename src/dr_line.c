#include "../inc/dr_internal.h"

/************************ DMX/RDM链路管理 ************************/

// 全局DMX/RDM链路数组和计数器
dr_line_t *dr_line_buf[DMX_MAX_LINE] = {NULL};
uint8_t dr_line_sum = 0;

// DMX/RDM操作函数集
dr_line_funs_t dr_funs;

/************************ 初始化相关函数 ************************/
/**
 * @brief 创建并初始化DMX/RDM链路结构体
 * @return 成功返回初始化后的dr_line_t指针，失败返回NULL
 * @note 内部使用，分配内存并初始化所有字段
 */
static dr_line_t *dr_line_handle_create(void)
{
    dr_line_t *dr_line = (dr_line_t *)DR_MALLOC(sizeof(dr_line_t));
    if(dr_line == NULL) return NULL;

    /* RDM UID链表初始化 */
    __DR_RDM_UID_LIST(dr_line).uid = RDM_BROADCAST_ADDR;
    __DR_RDM_UID_LIST(dr_line).next = &__DR_RDM_UID_LIST(dr_line);
    __DR_RDM_UID_LIST(dr_line).prev = &__DR_RDM_UID_LIST(dr_line);
    __DR_RDM_QUICK_ADD(dr_line) = &__DR_RDM_UID_LIST(dr_line);

    /* RDM搜索栈初始化 */
    __DR_RDM_STACK_DEPTH(dr_line) = 0;

    /* RDM发送队列初始化 */
    __DR_RDM_QUEUE_HEAD(dr_line) = 0;
    __DR_RDM_QUEUE_TAIL(dr_line) = 0;
    __DR_RDM_QUEUE_FULL(dr_line) = 0;

    /* 系统信号量初始化 */
    __DR_RECV_SEMAPHORE(dr_line) = 0;
    __DR_SEND_SEMAPHORE(dr_line) = 0;

    /* 设备信息初始化 */
    __DR_RDM_DEVICE_INFO(dr_line).mute = 0;                     // 哑音状态关闭
    __DR_RDM_DEVICE_INFO(dr_line).uid  = RDM_DEFAULT_UID;       // 默认UID
    __DR_RDM_DEVICE_INFO(dr_line).flag = 0;                     // 状态标志清零
    __DR_RDM_DEVICE_INFO(dr_line).info.device_id = 1;           // 设备类型ID
    __DR_RDM_DEVICE_INFO(dr_line).info.product = 1;             // 产品类别
    __DR_RDM_DEVICE_INFO(dr_line).info.rdm_version = 0;         // RDM协议版本
    __DR_RDM_DEVICE_INFO(dr_line).info.sensor = 0;              // 传感器数量
    __DR_RDM_DEVICE_INFO(dr_line).info.soft_version = 0x0100;   // 软件版本v1.0
    __DR_RDM_DEVICE_INFO(dr_line).info.sub_device = 0;          // 子设备数量

    /* 定时器初始化 */
    __DR_IDLE_TIM(dr_line).over = IDLE_TIM_OVER;                 // 空闲超时阈值
    __DR_SEND_DELAY_TIM(dr_line).over = SEND_DELAY_TIM_OVER;     // 发送延迟阈值
    __DR_REFRESH_TIM(dr_line).over = REFRESH_TIM_OVER;           // 刷新间隔阈值
    __DR_RDM_RECV_TIM(dr_line).over = RDM_RECV_TIM_OVER;         // RDM接收超时阈值
    __DR_SEND_TIMEOUT_TIM(dr_line).over = SEND_TIMEOUT_TIM_OVER; // 发送超时阈值

    /* 缓冲区初始化 */
    memset(__DR_TX_BUF(dr_line), 0, 513);                     // 清空发送缓冲区
    __DR_RX_BUF(dr_line)[0] = 0x00;                           // 接收缓冲区首字节清零
    __DR_RX_CNT(dr_line) = 0;                                 // 接收计数复位

    /* DMX配置初始化 */
    __DR_DMX_ADDR(dr_line) = 1;                                 // 默认起始地址1
    __DR_DMX_CHANNEL(dr_line) = DMX_MODE1_CHANNEL;              // 默认通道数
    __DR_DMX_MODE(dr_line) = 1;                                 // 默认模式1

    return dr_line;
}

/**
 * @brief 创建新的DMX链路
 * @return 成功返回链路指针，失败返回NULL
 * @note 全局链路数量不超过DMX_MAX_LINE
 */
dr_line_t* dr_line_create(void)
{
    if(dr_line_sum >= DMX_MAX_LINE) {
        return NULL;  // 链路数量已达上限
    }

    dr_line_buf[dr_line_sum] = dr_line_handle_create();
    if(dr_line_buf[dr_line_sum] == NULL) {
        return NULL;  // 内存分配失败
    }

    dr_line_sum++;  // 全局链路计数器递增
    return dr_line_buf[dr_line_sum - 1];
}

/**
 * @brief 初始化DMX/RDM操作函数集
 * @param funs 包含具体实现的操作函数集
 */
void dr_line_funs_create(dr_line_funs_t *funs)
{
    // 绑定操作集
    dr_funs.change_mode = funs->change_mode;
    dr_funs.send_reset = funs->send_reset;
    dr_funs.close = funs->close;
    dr_funs.receive = funs->receive;
    dr_funs.transmit = funs->transmit;
}

/**
 * @brief 重置链路状态
 * @param dr_line DMX链路指针
 * @note 用于将链路恢复到初始状态
 */
static void dr_line_reset(dr_line_t *dr_line)
{
    /* 定时器重置 */
    __DR_IDLE_TIM(dr_line).state = 0;
    __DR_IDLE_TIM(dr_line).count = 0;
    __DR_SEND_DELAY_TIM(dr_line).state = 0;
    __DR_SEND_DELAY_TIM(dr_line).count = 0;
    __DR_REFRESH_TIM(dr_line).state = 0;
    __DR_REFRESH_TIM(dr_line).count = 0;
    __DR_RDM_RECV_TIM(dr_line).state = 0;
    __DR_RDM_RECV_TIM(dr_line).count = 0;
    __DR_SEND_TIMEOUT_TIM(dr_line).count = 0;
    __DR_SEND_TIMEOUT_TIM(dr_line).count = 0;
    
    /* 缓冲区重置 */
    memset(__DR_TX_BUF(dr_line), 0, sizeof(__DR_TX_BUF(dr_line)));
    __DR_RX_CNT(dr_line) = 0;
    __DR_SEND_STATUS(dr_line) = 0;
    __DR_SEND_STATUS(dr_line) = 0;

    /* RDM队列重置 */
    __DR_RDM_QUEUE_HEAD(dr_line) = 0;
    __DR_RDM_QUEUE_TAIL(dr_line) = 0;
    __DR_RDM_QUEUE_FULL(dr_line) = 0;
    __DR_RDM_STACK_DEPTH(dr_line) = 0;

    /* UID链表处理 */
    uid_t *p = __DR_RDM_UID_LIST(dr_line).next;
    while(p != &__DR_RDM_UID_LIST(dr_line))
    {
        p->online_flag = 0;  // 标记所有设备为离线
        p = p->next;
    }

    __DR_RDM_QUICK_ADD(dr_line) = &__DR_RDM_UID_LIST(dr_line);  // 关闭快速添加
}

/**
 * @brief 设置DMX/RDM链路工作模式
 * @param dr_line DMX/RDM链路指针
 * @param line_mode 要设置的工作模式
 * @note 支持以下工作模式:
 *       LINE_OFF - 关闭发送接收
 *       DMX_OUTPUT - DMX信号输出模式
 *       DMX_OUTPUT_BLACK - DMX信号全零输出模式  
 *       DMX_INPUT - DMX信号输入模式
 *       RDM_OUTPUT - RDM信号输出模式
 *       RDM_INPUT - RDM信号输入模式
 *       DMX_RDM_INPUT - DMX/RDM混合输入模式
 */
void dr_line_mode_set(dr_line_t *dr_line, uint8_t line_mode)
{
    // 安全关闭当前链路
    dr_funs.close(dr_line);
    
    // 重置链路状态
    dr_line_reset(dr_line);
    
    // 设置新模式
    __DR_LINE_MODE(dr_line) = line_mode;
    
    // 根据模式执行不同初始化
    switch(line_mode)
    {
        case LINE_OFF:
            // 设置为接收模式后关闭
            dr_funs.change_mode(dr_line, DR_RECV_DATA);
            dr_funs.close(dr_line);
            break;
            
        case DMX_INPUT:
        case RDM_INPUT: 
        case DMX_RDM_INPUT:
            // 输入模式初始化
            __DR_SEND_STATUS(dr_line) = 0;
            dr_funs.change_mode(dr_line, DR_RECV_RESET);
            dr_funs.receive(dr_line, __DR_RX_BUF(dr_line), 1);
            break;
            
        case DMX_OUTPUT:
            // 允许DMX发送
            DR_SEMAPHORE_GIVE(__DR_SEND_SEMAPHORE(dr_line));
            break;
            
        case RDM_OUTPUT:
            // 设置快速添加
            __DR_RDM_QUICK_ADD(dr_line) = __DR_RDM_UID_LIST(dr_line).next;
            
            // 发送解除哑音广播命令
            rdm_send(dr_line, DISC_UN_MUTE, &__DR_RDM_UID_LIST(dr_line));
            
            // 允许RDM发送
            DR_SEMAPHORE_GIVE(__DR_SEND_SEMAPHORE(dr_line));
            break;
            
        default:
            // 无效模式处理
            break;
    }
}

/************************ 串口事件处理接口 ************************/

/**
 * @brief 串口接收完成中断处理
 * @param dr_line DMX/RDM链路指针
 * @note 处理接收到的每个字节，管理接收缓冲区和相关定时器
 */
void dr_uart_rxcomplete_handle(dr_line_t *dr_line)
{
    // 更新接收计数器
    __DR_RX_CNT(dr_line)++;
    if(__DR_RX_CNT(dr_line) >= 520) {
        __DR_RX_CNT(dr_line) = 0;
    }

    // 检查链路状态决定是否继续接收
    if(__DR_LINE_MODE(dr_line) != LINE_OFF) {
        dr_funs.receive(dr_line, &__DR_RX_BUF(dr_line)[__DR_RX_CNT(dr_line)], 1);
    } else {
        __DR_RX_CNT(dr_line) = 0;
    }

#if IDLE_TIM_OVER
    // 重置空闲检测定时器
    __DR_IDLE_TIM(dr_line).count = 0;
    __DR_IDLE_TIM(dr_line).state = 1;
#endif

#if REFRESH_TIM_OVER
    // 重置刷新率检测定时器
    __DR_REFRESH_TIM(dr_line).count = 0;
    __DR_REFRESH_TIM(dr_line).state = 1;
#endif
}

/**
 * @brief 串口断开信号(BREAK)中断处理
 * @param dr_line DMX/RDM链路指针
 * @note 处理DMX/RDM协议的BREAK信号，用于帧同步和冲突检测
 */
void dr_break_handle(dr_line_t *dr_line)
{
    /* 
     * 处理DMX连续数据包情况：
     * 如果空闲检测定时器未超时就收到BREAK信号，
     * 表示DMX数据包连续发送，间隔时间过短，
     * 此时以BREAK信号作为包间隔，提前结束当前包
     */
    if(__DR_RX_CNT(dr_line) > 1) {
        // 停止空闲检测定时器
        __DR_IDLE_TIM(dr_line).count = 0;
        __DR_IDLE_TIM(dr_line).state = 0;

        //关闭RDM接收超时定时器
        __DR_RDM_RECV_TIM(dr_line).count = 0;
        __DR_RDM_RECV_TIM(dr_line).state = 0;

        // 重置接收计数器
        __DR_RX_CNT(dr_line) = 0;

        // 触发解包任务
        DR_SEMAPHORE_GIVE(__DR_RECV_SEMAPHORE(dr_line));
        return;
    }

    /* 
     * 设置BREAK接收标志位：
     * 1. 用于RDM协议冲突检测
     * 2. 标记新数据包开始
     */
    __DR_SEND_STATUS(dr_line) |= DR_LINE_RECV_BREAK;

    /* 
     * 注意事项：
     * 1. HAL库中BREAK信号可能触发接收完成中断
     * 2. 接收顺序可能是: 数据中断 -> BREAK中断
     * 3. DMA接收时第一个字节可能丢失
     */
    __DR_RX_CNT(dr_line) = 0;
    dr_funs.close(dr_line);
    dr_funs.receive(dr_line, __DR_RX_BUF(dr_line), 1);
}

/**
 * @brief 串口发送完成中断处理
 * @param dr_line DMX/RDM链路指针
 * @note 根据当前工作模式处理发送完成事件
 */
void dr_uart_txcomplete_handle(dr_line_t *dr_line)
{
    switch(__DR_LINE_MODE(dr_line)) {
        case DMX_OUTPUT:
            //关闭发送超时检测定时器
            __DR_SEND_TIMEOUT_TIM(dr_line).state = 0;
            // DMX输出模式处理
#if DMX_PACKET_DELAY
            // 启用包间隔延迟
            __DR_SEND_STATUS(dr_line) = 0;
            dr_funs.change_mode(dr_line, DR_RECV_RESET);
            dr_funs.receive(dr_line, __DR_RX_BUF(dr_line), 1);
            
            // 启动发送延迟定时器
            __DR_SEND_DELAY_TIM(dr_line).count = 0;
            __DR_SEND_DELAY_TIM(dr_line).state = 1;
#else
            // 立即发送下一包
            DR_SEMAPHORE_GIVE(__DR_SEND_SEMAPHORE(dr_line));
#endif
            break;

        case RDM_OUTPUT:            
            // 准备接收响应
            __DR_SEND_STATUS(dr_line) = 0;
            dr_funs.change_mode(dr_line, DR_RECV_RESET);
            dr_funs.receive(dr_line, __DR_RX_BUF(dr_line), 1);
            break;

        case LINE_OFF:
            // 链路关闭状态不处理
            break;
    }
}

/**
 * @brief 串口错误处理
 * @param dr_line DMX/RDM链路指针
 * @note 处理串口通信错误，根据当前状态恢复接收或发送
 */
void dr_uart_error_handle(dr_line_t *dr_line)
{
    if(__DR_SEND_STATUS(dr_line) == 0) {
        // 接收错误处理
        __DR_IDLE_TIM(dr_line).count = 0;
        __DR_IDLE_TIM(dr_line).state = 1;
        dr_funs.receive(dr_line, __DR_RX_BUF(dr_line), 1);
        __DR_SEND_STATUS(dr_line) |= DR_LINE_RECV_ERROR;
    } else {
        // 发送错误处理
        // 算作完成发送
        dr_uart_txcomplete_handle(dr_line);
    }
}
