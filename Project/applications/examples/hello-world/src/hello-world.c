#include "hello-world.h"

int main(void)
{
  Initialize_Platform();

  HAL_Delay(100);

  while (1)
  {
    uint8_t response[50] = {0};
    HAL_UART_Abort(&BLE_UART);
    int result = HAL_UART_Receive(&BLE_UART, response, 50, 1000);
    if(result == HAL_OK){
      printINF("%s\n\r", response);
    }
    IWDG_feed(NULL);
  }
}

void StartDefaultTask(void const *argument)
{
  for (;;)
  {
    osDelay(1);
  }
}