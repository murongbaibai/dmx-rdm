#ifndef DMX_RDM_H
#define DMX_RDM_H 

/*
  ******************************************************************************
  * @author  MURONGBAI 3216147898@qq.com
  * @brief   本代码由慕容白开发、遵循MIT许可协议、转载需标明本词条
  * @date    2025.6.19
  ******************************************************************************
*/
#include "./inc/dr_config.h"
#include "./inc/dr_types.h"


// 任务处理函数
void dr_tick_inc(uint8_t nms);
void dr_task_handle(void);


// 链路操作函数
dr_line_t* dr_line_create(void);
void dr_line_funs_create(dr_line_funs_t *funs);
void dr_line_mode_set(dr_line_t *dr_line, uint8_t status);



// 中断处理接口
void dr_uart_rxcomplete_handle(dr_line_t *dr_line);
void dr_break_handle(dr_line_t *dr_line);
void dr_uart_txcomplete_handle(dr_line_t *dr_line);
void dr_uart_error_handle(dr_line_t *dr_line);



// 回调函数
__weak void dmx_unpack_complete_callback(dr_line_t *dr_line);
__weak void dmx_before_send_callback(dr_line_t *dr_line);
__weak void rdm_device_offline_callback(dr_line_t *dr_line, uid_t *p);
__weak void rdm_device_add_callback(dr_line_t *dr_line, uid_t *p);
__weak void rdm_device_online_callback(dr_line_t *dr_line, uid_t *p);
__weak void rdm_device_respone_callback(dr_line_t *dr_line, uid_t *p, uint16_t package);
__weak void rdm_device_set_callback(dr_line_t *dr_line);



// RDM发送函数
uid_t* rdm_uid_get(dr_line_t *dr_line, uint16_t index);
uint8_t rdm_uid_get_sum(dr_line_t *dr_line);
void rdm_send(dr_line_t *dr_line, enum rdm_package package, uid_t *target_uid);



#endif

