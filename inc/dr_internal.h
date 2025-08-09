#ifndef DR_INTERNAL_H
#define DR_INTERNAL_H

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
