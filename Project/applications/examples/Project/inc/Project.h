#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h"

osTimerId iwdgTimId;
osTimerId batteryTimId;
osTimerId UART_TimId;
osThreadId defaultTaskHandle;

osThreadId defaultTaskHandle;
osThreadId murata_rx_processing_handle;
osTimerId iwdgTimId;
osTimerId loraWANTimId;
osTimerId moduleCheckTimId;
osMutexId txMutexId;
osMutexId murata_rx_process_mutex_id;

void batteryLevel_measurement(void const *argument);
void StartDefaultTask(void const *argument);
void temp_hum_measurement(void);
void print_temp_hum(void);

void IWDG_feed(void const *argument);
void LoRaWAN_send(void const *argument);
void check_modules(void const *argument);
void murata_process_rx_response(void const *argument);
void Dualstack_ApplicationCallback(void);
void UART_Receive(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */