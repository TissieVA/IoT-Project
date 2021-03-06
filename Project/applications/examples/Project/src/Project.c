#include "Project.h"
#include "sht31.h"
#include "stc3115.h"
#include "murata.h"

#define temp_hum_timer 30

#define IWDG_INTERVAL 5          // seconds
#define BATTERYLEVEL_INTERVAL 30 // seconds

#define LORAWAN_INTERVAL 30        // seconds
#define MODULE_CHECK_INTERVAL 3600 // seconds
#define LORA_MESSAGE_SIZE 10       // bytes
#define UART_INTERVAL 1            // seconds

// Murata Code
uint16_t LoRaWAN_Counter = 0;
uint8_t murata_init = 0;
uint64_t short_UID;
uint8_t murata_data_ready = 0;

// Battery Level Code
STC3115_ConfigData_TypeDef STC3115_ConfigData;
STC3115_BatteryData_TypeDef STC3115_BatteryData;
int batteryPercentage, batteryVoltage;

// Temp/Hum Code
osTimerId temp_hum_timer_id, UART_TimId;
float SHTData[2];

// UART Code
uint8_t response[50] = {0};
uint8_t TimeAlarm[4];

int main(void)
{
  Initialize_Platform();

  // Battery monitoring
  GasGauge_Initialization(&common_I2C, &STC3115_ConfigData, &STC3115_BatteryData);

  // SHT (van temp/hum)
  setI2CInterface_SHT31(&common_I2C);
  SHT31_begin();

  osThreadDef(defaultTask, StartDefaultTask, osPriorityLow, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  // Lorawan Code
  murata_init = Murata_Initialize(short_UID); // short_UID = devEUI
  UART_SetApplicationCallback(&Dualstack_ApplicationCallback, (uint8_t)MURATA_CONNECTOR);

  if (murata_init)
  {
    printINF("Murata dualstack module init OK\r\n\r\n");
  }

  // TX MUTEX ensuring no transmits are happening at the same time
  osMutexDef(txMutex);
  txMutexId = osMutexCreate(osMutex(txMutex));

  // Thread made for murata actions
  osThreadDef(murata_rx_processing, murata_process_rx_response, osPriorityNormal, 0, 512);
  murata_rx_processing_handle = osThreadCreate(osThread(murata_rx_processing), NULL);

  // pass processing thread handle to murata driver
  Murata_SetProcessingThread(murata_rx_processing_handle);
  //

  // feed IWDG every 5 seconds
  IWDG_feed(NULL);
  osTimerDef(iwdgTim, IWDG_feed);
  iwdgTimId = osTimerCreate(osTimer(iwdgTim), osTimerPeriodic, NULL);
  osTimerStart(iwdgTimId, IWDG_INTERVAL * 1000);

  osTimerDef(temp_hum_Tim, temp_hum_measurement);
  temp_hum_timer_id = osTimerCreate(osTimer(temp_hum_Tim), osTimerPeriodic, NULL);
  osTimerStart(temp_hum_timer_id, temp_hum_timer * 1000);

  osTimerDef(batteryLevel_Tim, batteryLevel_measurement);
  batteryTimId = osTimerCreate(osTimer(batteryLevel_Tim), osTimerPeriodic, NULL);
  osTimerStart(batteryTimId, BATTERYLEVEL_INTERVAL * 1000);

  osTimerDef(loraWANTim, LoRaWAN_send);
  loraWANTimId = osTimerCreate(osTimer(loraWANTim), osTimerPeriodic, NULL);
  osTimerStart(loraWANTimId, LORAWAN_INTERVAL * 1000);

  osTimerDef(moduleCheckTim, check_modules);
  moduleCheckTimId = osTimerCreate(osTimer(moduleCheckTim), osTimerPeriodic, NULL);
  osTimerStart(moduleCheckTimId, MODULE_CHECK_INTERVAL * 1000);

  // Join before starting the kernel
  Murata_LoRaWAN_Join();

  osKernelStart();

  while (1)
  {
  }
}

// Listen on the UART if data is being sent from the BLE chip
void UART_Receive(void)
{
  HAL_UART_Abort(&BLE_UART);
  int result = HAL_UART_Receive(&BLE_UART, response, 50, 1000);
  if (result == HAL_OK)
  {
    if(response[7] == 0x63)                                                     //Connect case
    {
      printINF("The Response is: %s\n\r", response);
    }
    else if (response[0] == 0x30 || response[0] == 0x31 || response[0] == 0x32) //Alarm time case
    {
      for (int i = 0; i <= 3; i++)
      {
        TimeAlarm[i] = response[i];
        //  setalarm(TimeAlarm);
      }
      printINF("Alarm time is: %s\r\n", TimeAlarm);
    }
    else if (response[7] == 0x64)                                               //Disconnect case
    {
      printINF("The Response is: %s\n\r", response);
      osTimerStop(UART_TimId);
      SleepMode();
    }
  }
}

// Battery Level measurement
void batteryLevel_measurement(void const *argument)
{
  GasGauge_Task(&STC3115_ConfigData, &STC3115_BatteryData);
  batteryVoltage = STC3115_BatteryData.Voltage;
  printf("Battery Voltage = %i mV\r\n", // voltage drops from 4.2 to 3.7 if empty
         batteryVoltage);
}

// Temperature and Humidity measurement
void temp_hum_measurement(void)
{
  SHT31_get_temp_hum(SHTData);
  print_temp_hum();
}

void print_temp_hum(void)
{
  printf("Temperature: %.2f degC \r\n", SHTData[0]);
  printf("Humidity: %.2f %% \r\n", SHTData[1]);
}

void StartDefaultTask(void const *argument)
{
  for (;;)
  {
    osDelay(1);
  }
}

// Lorawan
void LoRaWAN_send(void const *argument)
{
  if (murata_init)
  {
    uint8_t loraMessage[LORA_MESSAGE_SIZE];
    // float_union changes datatype from floats to bytes (see core/platform/common/inc/datatypes.h)
    // 1 float needs 4 bytes so we need loraMessage to be 8 bytes
    // Found in core/platform/common/inc/datatypes.h
    // Check converter at UINT16 Little Endion
    float_union.fl = SHTData[0]; // temperature
    loraMessage[0] = float_union.bytes.b1;
    loraMessage[1] = float_union.bytes.b2;
    loraMessage[2] = float_union.bytes.b3;
    loraMessage[3] = float_union.bytes.b4;
    float_union.fl = SHTData[1]; // humidity
    loraMessage[4] = float_union.bytes.b1;
    loraMessage[5] = float_union.bytes.b2;
    loraMessage[6] = float_union.bytes.b3;
    loraMessage[7] = float_union.bytes.b4;
    int32LittleEndian.integer = batteryVoltage; // battery voltage
    loraMessage[8] = int32LittleEndian.byte[0];
    loraMessage[9] = int32LittleEndian.byte[1];

    osMutexWait(txMutexId, osWaitForever);
    if (!Murata_LoRaWAN_Send((uint8_t *)loraMessage, LORA_MESSAGE_SIZE))
      printINF("Message sent!");

    // BLOCK TX MUTEX FOR 3s
    osDelay(3000);
    osMutexRelease(txMutexId);
    LoRaWAN_Counter++;

    printINF("Listening on UART\n");
    osTimerDef(UART_Tim, UART_Receive);
    UART_TimId = osTimerCreate(osTimer(UART_Tim), osTimerPeriodic, NULL);
    osTimerStart(UART_TimId, UART_INTERVAL * 100);
  }
  else
  {
    printINF("murata not initialized, not sending\r\n");
  }
}

void check_modules(void const *argument)
{
  printINF("checking the status of the modules\r\n");
  if (!murata_init)
  {
    // LORAWAN
    murata_init = Murata_Initialize(short_UID);
    Murata_toggleResetPin();
  }
}

void murata_process_rx_response(void const *argument)
{
  uint32_t startProcessing;
  while (1)
  {
    // Wait to be notified that the transmission is complete.  Note the first
    // parameter is pdTRUE, which has the effect of clearing the task's notification
    // value back to 0, making the notification value act like a binary (rather than
    // a counting) semaphore.
    startProcessing = ulTaskNotifyTake(pdTRUE, osWaitForever);
    if (startProcessing == 1)
    {
      // The transmission ended as expected.
      while (murata_data_ready)
      {
        //printINF("processing murata fifo\r\n");
        murata_data_ready = !Murata_process_fifo();
        osDelay(50);
      }
    }
    else
    {
    }
    osDelay(1);
  }
  osThreadTerminate(NULL);
}

void wakeUp(void)
{
  printf("WAKE UP");
}

void SleepMode(void)
{
  printINF("Going to sleep\r\n");
  osTimerStop(moduleCheckTimId);
  osTimerStop(temp_hum_timer_id);
  osTimerStop(loraWANTimId);
  osTimerStop(batteryTimId);
  HAL_SuspendTick();
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
  HAL_ResumeTick();
}

void Dualstack_ApplicationCallback(void)
{
  murata_data_ready = 1;
}