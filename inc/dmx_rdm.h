#ifndef DMX_RDM_H
#define DMX_RDM_H

/*
  ******************************************************************************
  * @author  MURONGBAI 3216147898@qq.com
  * @brief   本代码由慕容白开发、遵循MIT许可协议、转载需标明本词条
  * @date    2025.6.19
  ******************************************************************************
*/
#if USE_HAL_DRIVER
#include "py32f0xx_hal.h"
#endif

//前置声明
typedef struct dmx_line_t dmx_line_t;

#include "stdint.h"
#include "stdlib.h"
#include "string.h"

#include "dmx_rdm_config.h"
#include "dmx.h"
#if (DEVICE_TYPE_SWITCH == DEVICE_TYPE_OUTPUT)
#include "rdm_output.h"
#else
#include "rdm_input.h"
#endif
#include "rs_interfaces.h"
#include "uart_init_template.h"



#endif
