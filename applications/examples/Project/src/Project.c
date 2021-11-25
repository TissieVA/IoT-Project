#include "Project.h"
#include "sht31.h"
#include "stc3115.h"
#include "murata.h"

#define temp_hum_timer    60

#define IWDG_INTERVAL                   5         //seconds
#define BATTERYLEVEL_INTERVAL           60        //seconds

#define LORAWAN_INTERVAL        60   //seconds
#define MODULE_CHECK_INTERVAL   3600 //seconds

//Murata Code
uint16_t LoRaWAN_Counter = 0;
uint8_t murata_init = 0;
uint64_t short_UID;
uint8_t murata_data_ready = 0;

//Battery Level Code
STC3115_ConfigData_TypeDef STC3115_ConfigData;
STC3115_BatteryData_TypeDef STC3115_BatteryData;

//Temp/Hum Code
osTimerId temp_hum_timer_id;
float SHTData[2];

int main(void)
{
  Initialize_Platform();

// Battery monitoring
  GasGauge_Initialization(&common_I2C, &STC3115_ConfigData, &STC3115_BatteryData);
//

//SHT (van temp/hum)
  setI2CInterface_SHT31(&common_I2C);
  SHT31_begin();

  osThreadDef(defaultTask, StartDefaultTask, osPriorityLow, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);
//

//Lorawan Code
murata_init = Murata_Initialize(short_UID); //short_UID = devEUI
UART_SetApplicationCallback(&Dualstack_ApplicationCallback, (uint8_t)MURATA_CONNECTOR);

if (murata_init)
{
  printINF("Murata dualstack module init OK\r\n\r\n");
}

// TX MUTEX ensuring no transmits are happening at the same time
osMutexDef(txMutex);
txMutexId = osMutexCreate(osMutex(txMutex));
// Thread gemaakt voor murata acties
osThreadDef(murata_rx_processing, murata_process_rx_response, osPriorityNormal, 0, 512);
murata_rx_processing_handle = osThreadCreate(osThread(murata_rx_processing), NULL);

// pass processing thread handle to murata driver
Murata_SetProcessingThread(murata_rx_processing_handle);
//


  //feed IWDG every 5 seconds
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


//Join before starting the kernel
  Murata_LoRaWAN_Join();

  osKernelStart();

  while (1)
  {
  }

}

// Battery Level measurement
void batteryLevel_measurement(void const *argument)
{
  GasGauge_Task(&STC3115_ConfigData, &STC3115_BatteryData);
  printf("Vbat: %i mV, I=%i mA SoC=%i, C=%i, P=%i A=%i , CC=%d\r\n",
         STC3115_BatteryData.Voltage, //hieruit nog selecteren welke te verwijderen
         STC3115_BatteryData.Current,
         STC3115_BatteryData.SOC,
         STC3115_BatteryData.ChargeValue,
         STC3115_BatteryData.Presence,
         STC3115_BatteryData.StatusWord >> 13,
         STC3115_BatteryData.ConvCounter);
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
    uint8_t loraMessage[8];
    //float_union changes datatype from floats to bytes (see core/platform/common/inc/datatypes.h)
    //1 float needs 4 bytes so we need loraMessage to be 8 bytes
    float_union.fl = SHTData[0];             
    loraMessage[0] = float_union.bytes.b1;
    loraMessage[1] = float_union.bytes.b2;
    loraMessage[2] = float_union.bytes.b3;
    loraMessage[3] = float_union.bytes.b4;
    float_union.fl = SHTData[1];
    loraMessage[4] = float_union.bytes.b1;
    loraMessage[5] = float_union.bytes.b2;
    loraMessage[6] = float_union.bytes.b3;
    loraMessage[7] = float_union.bytes.b4;
    
    osMutexWait(txMutexId, osWaitForever);
    if(!Murata_LoRaWAN_Send((uint8_t *)loraMessage, 8)) 
      printINF("Message sent!");
    
    //BLOCK TX MUTEX FOR 3s
    osDelay(3000);
    osMutexRelease(txMutexId);
    LoRaWAN_Counter++;
  }
  else{
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
    //parameter is pdTRUE, which has the effect of clearing the task's notification
    //value back to 0, making the notification value act like a binary (rather than
    //a counting) semaphore.
    startProcessing = ulTaskNotifyTake(pdTRUE, osWaitForever);
    if (startProcessing == 1)
    {
      // The transmission ended as expected.
      while(murata_data_ready)
      {
        printINF("processing murata fifo\r\n");
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

void Dualstack_ApplicationCallback(void)
{
  murata_data_ready = 1;
}


