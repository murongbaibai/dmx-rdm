#ifndef RDM_H
#define RDM_H 

extern uint64_t rdm_uid;

void rdm_package_set_uid(dr_line_t *dr_line, uint64_t uid);
void rdm_package_set_cmd(dr_line_t *dr_line, uint8_t cmd_type, uint16_t cmd);
void rdm_package_set_data(dr_line_t *dr_line, uint8_t data_len, uint8_t data[]);
void rdm_package_send(dr_line_t *dr_line);


void rdm_send_queue_pop(dr_line_t *dr_line);
void rdm_send_finish(dr_line_t *dr_line, enum rdm_output_errors errors);
void rdm_disc_success(dr_line_t *dr_line);
void rdm_disc_conflict(dr_line_t *dr_line);
void rdm_disc_timeout(dr_line_t *dr_line);


#endif
