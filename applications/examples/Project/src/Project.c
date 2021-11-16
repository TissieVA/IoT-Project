#include "Project.h"
#include "sht31.h"
#include "stc3115.h"

#define temp_hum_timer    3

#define IWDG_INTERVAL                   5         //seconds
#define BATTERYLEVEL_INTERVAL           60        //seconds

#define LORAWAN_INTERVAL        60   //seconds
#define MODULE_CHECK_INTERVAL   3600 //seconds

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
  // GasGauge_Initialization(&common_I2C, &STC3115_ConfigData, &STC3115_BatteryData);

  //SHT (van temp/hum)
  setI2CInterface_SHT31(&common_I2C);
  SHT31_begin();

  osThreadDef(defaultTask, StartDefaultTask, osPriorityLow, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);


  //feed IWDG every 5 seconds
  IWDG_feed(NULL);
  osTimerDef(iwdgTim, IWDG_feed);
  iwdgTimId = osTimerCreate(osTimer(iwdgTim), osTimerPeriodic, NULL);
  osTimerStart(iwdgTimId, IWDG_INTERVAL * 1000);

  // osTimerDef(batteryLevel_Tim, batteryLevel_measurement);
  // batteryTimId = osTimerCreate(osTimer(batteryLevel_Tim), osTimerPeriodic, NULL);
  // osTimerStart(batteryTimId, BATTERYLEVEL_INTERVAL * 1000);

  osTimerDef(temp_hum_Tim, temp_hum_measurement);
  temp_hum_timer_id = osTimerCreate(osTimer(temp_hum_Tim), osTimerPeriodic, NULL);
  osTimerStart(temp_hum_timer_id, temp_hum_timer * 1000);


  osKernelStart();

  while (1)
  {
  }

}

// Battery Level measurement
// void batteryLevel_measurement(void const *argument)
// {
//   GasGauge_Task(&STC3115_ConfigData, &STC3115_BatteryData);
//   printINF("Vbat: %i mV, I=%i mA SoC=%i, C=%i, P=%i A=%i , CC=%d\r\n",
//          STC3115_BatteryData.Voltage, //hieruit nog selecteren welke te verwijderen
//          STC3115_BatteryData.Current,
//          STC3115_BatteryData.SOC,
//          STC3115_BatteryData.ChargeValue,
//          STC3115_BatteryData.Presence,
//          STC3115_BatteryData.StatusWord >> 13,
//          STC3115_BatteryData.ConvCounter);
// }


// Temperature and Humidity measurement
void temp_hum_measurement(void)
{
  SHT31_get_temp_hum(SHTData);
  print_temp_hum();
}

void print_temp_hum(void)
{
  printINF("Temperature: %.2f degC \r\n", SHTData[0]);
  printINF("Humidity: %.2f %% \r\n", SHTData[1]);
}

void StartDefaultTask(void const *argument)
{
  for (;;)
  {
    osDelay(1);
  }
}

// Murata Code

//

