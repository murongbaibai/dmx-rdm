#ifndef RDM_H
#define RDM_H 

/**
 * @brief RDM发送包结构体
 */
typedef struct rdm_package_t {
    uint8_t  start;        // 第0位 - 起始码
    uint8_t  sub_start;    // 第1位 - 子起始码
    uint8_t  message_len;  // 第2位 - 包长（不含校验位）
    uint64_t des_uid;      // 第3-8位 - 目标UID
    uint64_t source_uid;   // 第9-14位 - 源UID
    uint64_t other;        // 第15-19位 - 保留字段
    uint8_t  cmd_type;     // 第20位 - 命令类型
    uint16_t cmd;          // 第21-22位 - 具体命令
    uint8_t  data_len;     // 第23位 - 数据长度
    uint8_t  *data;        // 第24位起 - 数据指针
    uint32_t check;        // 校验位
} rdm_package_t;


void rdm_package_set_uid(uint64_t uid);
void rdm_package_set_cmd(uint8_t cmd_type, uint16_t cmd);
void rdm_package_set_data(uint8_t data_len, uint8_t data[]);
void rdm_package_send(dr_line_t *dr_line);


void rdm_send_queue_pop(dr_line_t *dr_line);
void rdm_send_finish(dr_line_t *dr_line, enum rdm_output_errors errors);
void rdm_disc_success(dr_line_t *dr_line);
void rdm_disc_conflict(dr_line_t *dr_line);
void rdm_disc_timeout(dr_line_t *dr_line);


#endif
