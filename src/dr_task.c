#include "../inc/dr_internal.h"

#if IDLE_TIM_OVER
/**
 * @brief 空闲检测定时器时间到接口
 * @param dr_line DMX+RDM链路指针
 */
static void dmx_idle_time_handle(dr_line_t *dr_line)
{
    // 判断数据包类型
    switch(__DR_RECV_PACKAGE(dr_line))
    {
        case RDM_PACKAGE:
        case DMX_PACKAGE:
            //定长或定点接收，这里不做处理...
        break;
        case DISC_RESPONE_PACKAGE:
            // 输出模式下，接收到设备响应包，则发送查询设备信息包
            if(__DR_LINE_MODE(dr_line) == RDM_OUTPUT)
            {
                package_recv_ok_handle(dr_line);
            }

            //其他时候接收到此包不处理，继续接收
        break;
        case UNKNOWN_PACKAGE:
        case ERROR_PACKAGE:
            // 只做RDM输出时候的冲突错误处理
            if(__DR_LINE_MODE(dr_line) == RDM_OUTPUT)
            {
                package_recv_ok_handle(dr_line);
            }
            //其他时候接收错误不处理，继续接收
            else
            {
                package_restart_handle(dr_line);
            }

        break;
    }
}
#endif

#if SEND_DELAY_TIM_OVER
/**
 * @brief 发送延时时间到接口
 * @param dr_line DMX+RDM链路指针
 */
static void dmx_send_delay_time_handle(dr_line_t *dr_line)
{
    // 如果还是在发送状态就续传数据包
    if(__DR_LINE_MODE(dr_line) != DR_LINE_OFF)
        DR_SEMAPHORE_GIVE(__DR_SEND_SEMAPHORE(dr_line));
}
#endif

#if RDM_RECV_TIM_OVER
/**
 * @brief RDM接收超时接口
 * @param dr_line DMX+RDM链路指针
 */
static void rdm_recv_time_handle(dr_line_t *dr_line)
{
    // 停止接收
    dr_funs.close(dr_line);

    // 如果是搜寻设备超时
    if(__DR_RDM_LAST_PACKAGE(dr_line) == DISC_UNIQUE)
        rdm_disc_timeout(dr_line);
    // 如果是回应超时
    else
        // 处理发送完成
        rdm_send_finish(dr_line, RDM_ERROR_RESPONE_TIMEOUT);

    // 允许下一次发送
    DR_SEMAPHORE_GIVE(__DR_SEND_SEMAPHORE(dr_line));
}
#endif

#if REFRESH_TIM_OVER
/**
 * @brief 断开信号时间到接口(弱定义)
 * @param dr_line DMX+RDM链路指针
 * @weak 默认实现为空函数
 */
__weak void dmx_refreash_tim_handle(dr_line_t *dr_line)
{
    // 默认空实现
}
#endif

#if SEND_TIMEOUT_TIM_OVER
/**
 * @brief 发送超时处理
 * @param dr_line DMX+RDM链路指针
 */
void send_timeout_tim_handle(dr_line_t *dr_line)
{
    //算作发送完成
    dr_uart_txcomplete_handle(dr_line);
}

#endif



/**
 * @brief 软件定时器处理
 * @param dr_line DMX+RDM链路指针
 */
static void time_task_handle(dr_line_t *dr_line)
{
#if IDLE_TIM_OVER
    // 空闲超时判断
    if(__DR_IDLE_TIM(dr_line).count >= __DR_IDLE_TIM(dr_line).over)
    {
        __DR_IDLE_TIM(dr_line).count = 0;
        __DR_IDLE_TIM(dr_line).state = 0;
        dmx_idle_time_handle(dr_line);
    }
#endif

#if SEND_DELAY_TIM_OVER
    // 发送延时判断
    if(__DR_SEND_DELAY_TIM(dr_line).count >= __DR_SEND_DELAY_TIM(dr_line).over)
    {
        __DR_SEND_DELAY_TIM(dr_line).count = 0;
        __DR_SEND_DELAY_TIM(dr_line).state = 0;
        dmx_send_delay_time_handle(dr_line);
    }
#endif

#if REFRESH_TIM_OVER
    // 刷新超时判断
    if(__DR_REFRESH_TIM(dr_line).count >= __DR_REFRESH_TIM(dr_line).over)
    {
        __DR_REFRESH_TIM(dr_line).count = 0;
        __DR_REFRESH_TIM(dr_line).state = 0;
        dmx_refreash_tim_handle(dr_line);
    }
#endif

#if RDM_RECV_TIM_OVER
    // RDM接收超时判断
    if(__DR_RDM_RECV_TIM(dr_line).count >= __DR_RDM_RECV_TIM(dr_line).over)
    {
        __DR_RDM_RECV_TIM(dr_line).count = 0;
        __DR_RDM_RECV_TIM(dr_line).state = 0;
        rdm_recv_time_handle(dr_line);
    }
#endif

#if SEND_TIMEOUT_TIM_OVER
    // 发送超时判断
    if(__DR_SEND_TIMEOUT_TIM(dr_line).count >= __DR_SEND_TIMEOUT_TIM(dr_line).over)
    {
        __DR_SEND_TIMEOUT_TIM(dr_line).count = 0;
        __DR_SEND_TIMEOUT_TIM(dr_line).state = 0;
        send_timeout_tim_handle(dr_line);
    }
#endif
}



/**
 * @brief DMX解包完成回调(弱定义)
 * @param dr_line DMX+RDM链路指针
 * @weak 默认实现为空函数
 */
__weak void dmx_unpack_complete_callback(dr_line_t *dr_line){}

/**
 * @brief RDM设备设置回调(弱定义)
 * @param dr_line DMX+RDM链路指针
 * @param p 设备UID指针
 * @weak 默认实现为空函数
 */
__weak void rdm_device_set_callback(dr_line_t *dr_line){}

/**
 * @brief 解包处理任务
 * @param dr_line DMX+RDM链路指针
 */
static void unpack_task_handle(dr_line_t *dr_line)
{
    // 等待接收完成信号量
    if(DR_SEMAPHORE_TAKE(__DR_RECV_SEMAPHORE(dr_line)) == 0)
        return;

    // 根据数据包类型进行解包
    switch(__DR_RECV_PACKAGE(dr_line))
    {
        case DMX_PACKAGE:
            //非DMX输入模式不处理
            if(__DR_LINE_MODE(dr_line) == DMX_INPUT ||
                __DR_LINE_MODE(dr_line) == DMX_OUTPUT ||
                __DR_LINE_MODE(dr_line) == DMX_RDM_INPUT)
            {
#if USE_DMX_QUICK_UNPACK
                //在中断中完成解包...
#else
                dmx_unpack(dr_line);
#endif
                //调用解包完成接口
                dmx_unpack_complete_callback(dr_line);
            }
        break;

        case RDM_PACKAGE:
            // RDM输出设备处理
            if(__DR_LINE_MODE(dr_line) == RDM_OUTPUT)
            {
                rdm_output_unpack(dr_line);

                if(__DR_RDM_PARSE(dr_line).rdm_package != RDM_PACKAGE_CHECK_ERROR)
                {
                    rdm_auto_set(dr_line);
                    rdm_send_finish(dr_line, RDM_ERROR_NULL);
                }
                else
                {
                    rdm_send_finish(dr_line, RDM_ERROR_UNPACK_FAILD);
                }
            }
            // RDM输入设备处理
            else if(__DR_LINE_MODE(dr_line) == RDM_INPUT || 
                   __DR_LINE_MODE(dr_line) == DMX_RDM_INPUT)
            {
                rdm_input_unpack(dr_line);

                if(__DR_RDM_PARSE(dr_line).rdm_package != RDM_PACKAGE_CHECK_ERROR)
                {
                    rdm_auto_response(dr_line);
                }
            }
            //解包完成

        break;

        case DISC_RESPONE_PACKAGE:
            rdm_output_unpack(dr_line);
            if(__DR_RDM_PARSE(dr_line).rdm_package == DISC_UNIQUE)
                rdm_disc_success(dr_line);
            else if(__DR_RDM_PARSE(dr_line).rdm_package == RDM_PACKAGE_CHECK_ERROR)
                rdm_disc_conflict(dr_line);
        break;

        case UNKNOWN_PACKAGE:
        case ERROR_PACKAGE:
            // RDM输出模式下的错误处理
            if(__DR_LINE_MODE(dr_line) == RDM_OUTPUT)
            {
                // 广播搜寻冲突
                if(__DR_RDM_LAST_PACKAGE(dr_line) == DISC_UNIQUE)
                    rdm_disc_conflict(dr_line);
                // 设备通信错误
                else if(__DR_RDM_LAST_PACKAGE(dr_line) != DISC_UNIQUE)
                    rdm_send_finish(dr_line, RDM_ERROR_UNPACK_FAILD);
            }
            // DMX输入模式下的错误处理
            // 不处理，跳过解包...

            // RDM输入下的错误处理
            // 不处理，跳过解包...
            // 设备搜索导致的错误包
        break;
    }

    // 输出模式下允许下一次发送
    if(__DR_LINE_MODE(dr_line) == RDM_OUTPUT || 
       __DR_LINE_MODE(dr_line) == DMX_OUTPUT)
    {
        DR_SEMAPHORE_GIVE(__DR_SEND_SEMAPHORE(dr_line));
    }
    // 输入模式重启接收
    else if(__DR_SEND_STATUS(dr_line) == 0)
    {
        package_restart_handle(dr_line);
    }
}

/**
 * @brief DMX任务处理
 * @param dr_line DMX+RDM链路指针
 */
static void dmx_task_handle(dr_line_t *dr_line)
{
    // 非发送模式直接返回
    if(__DR_LINE_MODE(dr_line) != DMX_OUTPUT)
        return;
        
    // 等待发送信号量
    if(DR_SEMAPHORE_TAKE(__DR_SEND_SEMAPHORE(dr_line)) == 0)
        return;

    dmx_send(dr_line);

    //开启发送超时检测
    __DR_SEND_TIMEOUT_TIM(dr_line).count = 0;
    __DR_SEND_TIMEOUT_TIM(dr_line).state = 1;
}

/**
 * @brief RDM任务处理
 * @param dr_line DMX+RDM链路指针
 */
static void rdm_task_handle(dr_line_t *dr_line)
{
    // 非发送模式直接返回
    if(__DR_LINE_MODE(dr_line) != RDM_OUTPUT)
        return;
        
    // 等待发送信号量
    if(DR_SEMAPHORE_TAKE(__DR_SEND_SEMAPHORE(dr_line)) == 0)
        return;

    // 队列为空时的处理
    if(!__DR_RDM_QUEUE_FULL(dr_line) && (__DR_RDM_QUEUE_HEAD(dr_line) == __DR_RDM_QUEUE_TAIL(dr_line)))
    {
        // 二分搜索处理
        if(__DR_RDM_STACK_DEPTH(dr_line) == 0)
        {   
            for(int i = 0; i < 5; i++)
            {
                // 发送解除哑音广播命令*5
                rdm_send(dr_line, DISC_UN_MUTE, __DR_RDM_UID_LIST(dr_line));
                // 设置广播节点*5
                __DR_RDM_STACK(dr_line)[i].low_uid = 0;
                __DR_RDM_STACK(dr_line)[i].high_uid = RDM_BROADCAST_ADDR;
            }


            //设置快速添加节点
            uint8_t device_sum = rdm_uid_get_sum(dr_line);
            device_sum = (device_sum > 49) ? 49 : device_sum;
            for(int i = 0; i < device_sum; i++)
            {
                uid_t *p = rdm_uid_get(dr_line, i+1);

                //第一个节点为广播节点
                __DR_RDM_STACK(dr_line)[i+5].low_uid = p->uid;
                __DR_RDM_STACK(dr_line)[i+5].high_uid = p->uid;
            }

            // 设置栈深度
            __DR_RDM_STACK_DEPTH(dr_line) = device_sum+5;
            
            //发送一次
            rdm_send_queue_pop(dr_line);
        }
        else
        {
            rdm_disc_driver(dr_line, 
                __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].low_uid,
                __DR_RDM_STACK(dr_line)[__DR_RDM_STACK_DEPTH(dr_line)-1].high_uid);
            __DR_RDM_LAST_PACKAGE(dr_line) = DISC_UNIQUE;
        }

    }
    // 队列非空处理
    else
    {
        rdm_send_queue_pop(dr_line);
    }

    // 开启接收超时定时器
    __DR_RDM_RECV_TIM(dr_line).count = 0;
    __DR_RDM_RECV_TIM(dr_line).state = 1;
}

/**
 * @brief DMX-RDM心跳时钟接口
 * @param nms 时间增量(毫秒)
 * @note nms 必须比所有软件定时器的最短的溢出时间短
 *          否则会导致处理失败，建议心跳频率为1ms
 */
void dr_tick_inc(uint8_t nms)
{
    for(int i = 0; i < DMX_MAX_LINE; i++)
    {
        if(dr_line_buf[i] == NULL)
            continue;

        // 定时器时间累加
        if(dr_line_buf[i]->time.idle_tim.state == 1)
            dr_line_buf[i]->time.idle_tim.count += nms;
        if(dr_line_buf[i]->time.send_delay_tim.state == 1)
            dr_line_buf[i]->time.send_delay_tim.count += nms;
        if(dr_line_buf[i]->time.refresh_tim.state == 1)
            dr_line_buf[i]->time.refresh_tim.count += nms;
        if(dr_line_buf[i]->time.rdm_recv_tim.state == 1)
            dr_line_buf[i]->time.rdm_recv_tim.count += nms;
        if(dr_line_buf[i]->time.send_timeout_tim.state == 1)
            dr_line_buf[i]->time.send_timeout_tim.count += nms;
    }
}

/**
 * @brief DMX-RDM任务处理接口
 * @note 建议触发频率为5ms，频率越高，处理数据速度越快
 */
void dr_task_handle(void)
{
    for(int i = 0; i < DMX_MAX_LINE; i++)
    {
        if(dr_line_buf[i] == NULL)
            continue;

        // 定时器任务处理
        time_task_handle(dr_line_buf[i]);
        // 解包任务处理
        unpack_task_handle(dr_line_buf[i]);
        // DMX发送任务处理
        dmx_task_handle(dr_line_buf[i]);
        // RDM发送任务处理
        rdm_task_handle(dr_line_buf[i]);
    }
}

