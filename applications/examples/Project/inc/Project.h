#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h"

osTimerId iwdgTimId;
osTimerId batteryTimId;
osThreadId defaultTaskHandle;

void batteryLevel_measurement(void const *argument);
void StartDefaultTask(void const *argument);
void temp_hum_measurement(void);
void print_temp_hum(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */