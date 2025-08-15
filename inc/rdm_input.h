#ifndef RDM_INPUT_H
#define RDM_INPUT_H

//如果UID是本设备UID 或者 UID是0xFFFFFFFFFFFF
#define RDM_UID_TRUE (__DR_RX_BUF(dr_line)[3] == ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 5*8) & 0xFF) &&   \
                      __DR_RX_BUF(dr_line)[4] == ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 4*8) & 0xFF) &&   \
                      __DR_RX_BUF(dr_line)[5] == ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 3*8) & 0xFF) &&   \
                      __DR_RX_BUF(dr_line)[6] == ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 2*8) & 0xFF) &&   \
                      __DR_RX_BUF(dr_line)[7] == ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 1*8) & 0xFF) &&   \
                      __DR_RX_BUF(dr_line)[8] == ((__DR_RDM_DEVICE_INFO(dr_line).uid >> 0*8) & 0xFF)) ||  \
                     (__DR_RX_BUF(dr_line)[3] == 0xFF &&                                    \
                      __DR_RX_BUF(dr_line)[4] == __DR_RX_BUF(dr_line)[3] &&            \
                      __DR_RX_BUF(dr_line)[5] == __DR_RX_BUF(dr_line)[4] &&            \
                      __DR_RX_BUF(dr_line)[6] == __DR_RX_BUF(dr_line)[5] &&            \
                      __DR_RX_BUF(dr_line)[7] == __DR_RX_BUF(dr_line)[6] &&            \
                      __DR_RX_BUF(dr_line)[8] == __DR_RX_BUF(dr_line)[7])   


void rdm_disc_driver_respone(dr_line_t *dr_line);
void rdm_disc_mute_respone(dr_line_t *dr_line, uint64_t uid);
void rdm_disc_un_mute_respone(dr_line_t *dr_line, uint64_t uid);
void rdm_get_version_respone(dr_line_t *dr_line, uint64_t uid);
void rdm_get_info_respone(dr_line_t *dr_line, uint64_t uid);
void rdm_get_flag_respone(dr_line_t *dr_line, uint64_t uid);
void rdm_set_flag_respone(dr_line_t *dr_line, uint64_t uid);
void rdm_get_dmx_addr_respone(dr_line_t *dr_line, uint64_t uid);
void rdm_set_dmx_addr_respone(dr_line_t *dr_line, uint64_t uid);
void rdm_get_pid_respone(dr_line_t *dr_line, uint64_t uid);
void rdm_get_cust_pid_respone(dr_line_t *dr_line, uint64_t uid);


void rdm_get_mode_respone(dr_line_t *dr_line, uint64_t uid);
void rdm_set_mode_respone(dr_line_t *dr_line, uint64_t uid);
void rdm_get_mode_info_respone(dr_line_t *dr_line, uint64_t uid);


void rdm_input_unpack(dr_line_t *dr_line);
void rdm_auto_response(dr_line_t *dr_line);

#endif
