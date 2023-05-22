
#ifndef UNI_INFRARED_H
#define UNI_INFRARED_H

#include <stdint.h>

#include "uni_type_def.h"
#include "uni_io.h"


int infrared_tx_init(int timer_id, enum tls_io_name pin);
int infrared_tx(unsigned short *out, int length);
void infrared_tx_deinit(void);

int infrared_learn_pwm(unsigned int *pattern, unsigned int out_len, uint32_t timerout_ms);




#endif

