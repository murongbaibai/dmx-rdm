#ifndef RDM_OUTPUT_H
#define RDM_OUTPUT_H

void rdm_uid_distory(dr_line_t *dr_line);
uint8_t rdm_uid_add(dr_line_t *dr_line, uint64_t uid);
uid_t* rdm_uid_search(dr_line_t *dr_line, uint64_t uid);
void rdm_uid_delete(uid_t* p);



void rdm_disc_driver(dr_line_t *dr_line, uint64_t low_uid,uint64_t high_uid);
void rdm_disc_mute(dr_line_t *dr_line, uid_t *p);
void rdm_disc_un_mute(dr_line_t *dr_line, uid_t *p);
void rdm_get_version(dr_line_t *dr_line, uid_t *p);
void rdm_get_info(dr_line_t *dr_line, uid_t *p);
void rdm_get_flag(dr_line_t *dr_line, uid_t *p);
void rdm_set_flag(dr_line_t *dr_line, uid_t *p);
void rdm_get_dmx_addr(dr_line_t *dr_line, uid_t *p);
void rdm_set_dmx_addr(dr_line_t *dr_line, uid_t *p);
void rdm_get_pid(dr_line_t *dr_line, uid_t *p);
void rdm_get_cust_pid(dr_line_t *dr_line, uid_t *p);


void rdm_get_mode(dr_line_t *dr_line, uid_t *p);
void rdm_set_mode(dr_line_t *dr_line, uid_t *p);
void rdm_get_mode_info(dr_line_t *dr_line, uid_t *p);


void rdm_output_unpack(dr_line_t *dr_line);
void rdm_auto_set(dr_line_t *dr_line);
#endif
