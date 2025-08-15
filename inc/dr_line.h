#ifndef DR_LINE_H
#define DR_LINE_H

extern dr_line_t *dr_line_buf[DMX_MAX_LINE];
extern uint8_t dr_line_sum;
extern dr_line_funs_t dr_funs;

void package_recv_ok_handle(dr_line_t *dr_line);
void package_restart_handle(dr_line_t *dr_line);

#endif
