#ifndef DR_INTERNAL_H
#define DR_INTERNAL_H

#define RDM_UID ((uint64_t)RDM_VENDOR_ID << 32) | (uint64_t)RDM_DEVICE_ID

#define DMX_START_CODE 0x00
#define RDM_START_CODE 0xCC
#define DISC_RESPONE_START_CODE 0xFE
#define DISC_RESPONE_BREAK_CODE 0xAA


#include <stdint.h>         // 标准整型类型定义
#include <string.h>         // 字符串和内存操作函数

#include "../dmx_rdm.h"     // DMX/RDM公共API定义

#include DR_MEM_INCLUDE

#include "dmx.h"            // DMX512协议核心实现
#include "rdm.h"            // RDM协议核心实现
#include "rdm_output.h"     // RDM输出设备逻辑实现
#include "rdm_input.h"      // RDM输入设备逻辑实现

#include "dr_line.h"        // 硬件线路控制抽象接口
#include "dr_task.h"        // 系统任务调度接口

#endif
