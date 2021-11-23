#include <limits.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32l4xx_hal.h"
#include "platform.h"

/*
 * In this example the tick interrupt is generated by the TIM6 peripheral.
 * The TIM6 configuration and handling functions are defined in this file.
 *
 * When configCREATE_LOW_POWER_DEMO is set to 0 the tick interrupt is
 * generated by the standard FreeRTOS Cortex-M port layer, which uses the
 * SysTick timer.
 */
/* The frequency at which TIM6 will run. */
#define lpCLOCK_INPUT_FREQUENCY 	( 1000UL )

/* This is used to print message on the serial console. */
//#define TICKLESS_DEBUG              DEBUG

/*-----------------------------------------------------------*/

/* Calculate how many clock increments make up a single tick period.
 Since we are using a prescaler equal to 23999, and assuming a clock
 speed of 48MHz, according the equation [1] in Chapter 11 this
 period value ensure a timer overflow ever 1ms. 
 CLOCKFREQ = 8MHz
 updateevent = 1ms -> 1kHz

 updateevent(freq) = Timerfreq/((prescaler+1)(period+1)
 1k = 8M/((4000)(2)) 
*/
static const uint32_t ulMaximumPrescalerValue = 3999;
static const uint32_t ulPeriodValueForOneTick = 1;

/* Holds the maximum number of ticks that can be suppressed - which is
 basically how far into the future an interrupt can be generated without
 loosing the overflow event at all. It is set during initialization. */
static TickType_t xMaximumPossibleSuppressedTicks = 0;

/* Flag set from the tick interrupt to allow the sleep processing to know if
 sleep mode was exited because of an tick interrupt or a different interrupt. */
static volatile uint8_t ucTickFlag = pdFALSE;

/* The HAL handler of the TIM6 timer */
//TODO: LPTIM instead of htim6?
TIM_HandleTypeDef htim6;
//LPTIM_HandleTypeDef hlptim1;

#if defined(TICKLESS_DEBUG) && TICKLESS_DEBUG == 1

#endif /* #if defined(TICKLESS_DEBUG) && TICKLESS_DEBUG == 1 */

/*-----------------------------------------------------------*/

void xPortSysTickHandler( void );

/* The callback function called by the HAL when TIM6 overflows. */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM6) {
    xPortSysTickHandler();

    /* In case this is the first tick since the MCU left a low power mode.
     The period is so configured by vPortSuppressTicksAndSleep(). Here
     the reload value is reset to its default. */
    __HAL_TIM_SET_AUTORELOAD(htim, ulPeriodValueForOneTick);

    /* The CPU woke because of a tick. */
    ucTickFlag = pdTRUE;
  }
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
}
/*-----------------------------------------------------------*/

/* Override the default definition of vPortSetupTimerInterrupt() that is weakly
 defined in the FreeRTOS Cortex-M3 port layer with a version that configures TIM6
 to generate the tick interrupt. */
void vPortSetupTimerInterrupt(void) {
  uint32_t ulPrescalerValue;

  /* Enable the TIM6 clock. */
  __HAL_RCC_TIM6_CLK_ENABLE();

  /* Ensure clock stops in debug mode. */
  __HAL_DBGMCU_FREEZE_TIM6();

  // Disable DBG during stop, sleep and standby mode to reduce power consumption
  HAL_DBGMCU_DisableDBGStandbyMode();
  HAL_DBGMCU_DisableDBGStopMode();
  HAL_DBGMCU_DisableDBGSleepMode();
  /* Scale the clock so longer tickless periods can be achieved.  The	SysTick
   is not used as even when its frequency is divided by 8 the maximum tickless
   period with a system clock of 16MHz is only 8.3 seconds.  Using	a prescaled
   clock on the 16-bit TIM6 allows a tickless period of nearly	66 seconds,
   albeit at low resolution. */
#if defined(TICKLESS_DEBUG) && TICKLESS_DEBUG == 1
  uint32_t clockfreq = HAL_RCC_GetSysClockFreq();
  uint32_t hclkfreq =  HAL_RCC_GetHCLKFreq();
  uint32_t configtickrate = configTICK_RATE_HZ;
  printDBG("CLOCKS:\r\n");
  printDBG("sysclockfreq: %d\r\n", clockfreq);
  printDBG("hclkfreq: %d\r\n", hclkfreq);
  printDBG("configtickrate: %d\r\n", configtickrate);
  printDBG("ulMaximumPrescalerValue: %d\r\n", ulMaximumPrescalerValue);
  printDBG("ulPeriodValueForOneTick: %d\r\n", ulPeriodValueForOneTick);

#endif  

  ulPrescalerValue = (HAL_RCC_GetHCLKFreq() / configTICK_RATE_HZ * 2);
  while (ulPrescalerValue > 0xFFFF)
    ulPrescalerValue /= 2;

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = (uint16_t) ulMaximumPrescalerValue;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = ulPeriodValueForOneTick;
  HAL_TIM_Base_Init(&htim6);

  /* Enable the TIM6 interrupt. This must execute at the lowest interrupt priority. */
  HAL_NVIC_SetPriority(TIM6_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY, 0);
  HAL_NVIC_EnableIRQ(TIM6_IRQn);

  HAL_TIM_Base_Start_IT(&htim6);
  /* See the comments where xMaximumPossibleSuppressedTicks is declared. */
  xMaximumPossibleSuppressedTicks = ((unsigned long) USHRT_MAX)
      / ulPeriodValueForOneTick;
}
/*-----------------------------------------------------------*/

/* Override the default definition of vPortSuppressTicksAndSleep() that is
 weakly defined in the FreeRTOS Cortex-M3 port layer with a version that manages
 the TIM6 interrupt, as the tick is generated from TIM6 compare matches events.

         THIS FUNCTION IS CALLED WITH THE SCHEDULER SUSPENDED.
 */
void vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime) {
  uint32_t ulCounterValue, ulCompleteTickPeriods;
  eSleepModeStatus eSleepAction;
  TickType_t xModifiableIdleTime;
  const TickType_t xRegulatorOffIdleTime = 50;

  /* Make sure the TIM6 reload value does not overflow the counter. */
  if (xExpectedIdleTime > xMaximumPossibleSuppressedTicks) {
    xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
  }

  /* Calculate the reload value required to wait xExpectedIdleTime tick
   periods. */
  ulCounterValue = ulPeriodValueForOneTick * xExpectedIdleTime;

  /* Stop TIM6 momentarily.  The time TIM6 is stopped for is not accounted for
   in this implementation (as it is in the generic implementation) because the
   clock is so slow it is unlikely to be stopped for a complete count period
   anyway. */
  HAL_TIM_Base_Stop_IT(&htim6);

  /* To avoid race conditions, enter a critical section.  */
  __disable_irq();

  /* The tick flag is set to false before sleeping.  If it is true when sleep
   mode is exited then sleep mode was probably exited because the tick was
   suppressed for the entire xExpectedIdleTime period. */
  ucTickFlag = pdFALSE;

  /* If a context switch is pending then abandon the low power entry as
   the context switch might have been pended by an external interrupt that
   requires processing. */
  eSleepAction = eTaskConfirmSleepModeStatus();
  if (eSleepAction == eAbortSleep) {
    /* Restart tick. */
    HAL_TIM_Base_Start_IT(&htim6);

    /* Re-enable interrupts. */
    __enable_irq();
  } else if (eSleepAction == eNoTasksWaitingTimeout) {

#if defined(TICKLESS_DEBUG) && TICKLESS_DEBUG == 1
      printDBG("No more running tasks and delays: we can enter STOP mode\r\n");

#endif
    /* A user definable macro that allows application code to be inserted
     here.  Such application code can be used to minimize power consumption
     further by turning off IO, peripheral clocks, the Flash, etc. */
    configPRE_STOP_PROCESSING();

    /* Enter in Low power Stop 2 */
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

    /* There are no running state tasks and no tasks that are blocked with a
     time out.  Assuming the application does not care if the tick time slips
     with respect to calendar time then enter a deep sleep that can only be
     woken by (in this demo case) the user button being pushed on the
     STM32L discovery board.  If the application does require the tick time
     to keep better track of the calendar time then the RTC peripheral can be
     used to make rough adjustments. */
//    HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);

    /* A user definable macro that allows application code to be inserted
     here.  Such application code can be used to reverse any actions taken
     by the configPRE_STOP_PROCESSING().  In this demo
     configPOST_STOP_PROCESSING() is used to re-initialize the clocks that
     were turned off when STOP mode was entered. */
    configPOST_STOP_PROCESSING();

#if defined(TICKLESS_DEBUG) && TICKLESS_DEBUG == 1
      printDBG("Exiting from STOP mode\r\n");
#endif

    /* Restart tick. */
    HAL_TIM_Base_Start_IT(&htim6);

    /* Re-enable interrupts. */
    __enable_irq();
  } else {

    /* Trap underflow before the next calculation. */
    configASSERT(ulCounterValue >= __HAL_TIM_GET_COUNTER(&htim6));

    /* Adjust the TIM6 value to take into account that the current time
     slice is already partially complete. */
    ulCounterValue -= (uint32_t) __HAL_TIM_GET_COUNTER(&htim6);

    /* Trap overflow/underflow before the calculated value is written to TIM6. */
    configASSERT(ulCounterValue < ( uint32_t ) USHRT_MAX);
    configASSERT(ulCounterValue != 0);

    /* Update to use the calculated overflow value. */
    __HAL_TIM_SET_AUTORELOAD(&htim6, ulCounterValue);
    __HAL_TIM_SET_COUNTER(&htim6, 0);

#if defined(TICKLESS_DEBUG) && TICKLESS_DEBUG == 1
      printDBG("eTaskConfirmSleepModeStatus returned eStandardSleep\r\n");
      printDBG("Expected idle time in ticks: %lu\r\n", xExpectedIdleTime);
      printDBG("MCU will sleep for %lu ticks\r\n", ulCounterValue/ulPeriodValueForOneTick);
#endif

    /* Restart the TIM6. */
    HAL_TIM_Base_Start_IT(&htim6);

    /* Allow the application to define some pre-sleep processing.  This is
     the standard configPRE_SLEEP_PROCESSING() macro as described on the
     FreeRTOS.org website. */
    xModifiableIdleTime = xExpectedIdleTime;
    configPRE_SLEEP_PROCESSING( xModifiableIdleTime );

    /* xExpectedIdleTime being set to 0 by configPRE_SLEEP_PROCESSING()
     means the application defined code has already executed the wait/sleep
     instruction. */
    if (xModifiableIdleTime > 0) {
      /* The sleep mode used is dependent on the expected idle time
       as the deeper the sleep the longer the wake up time.  See the
       comments at the top of main_low_power.c.  Note xRegulatorOffIdleTime
       is set purely for convenience of demonstration and is not intended
       to be an optimized value. */
      if (xModifiableIdleTime > xRegulatorOffIdleTime) {
#if defined(TICKLESS_DEBUG) && TICKLESS_DEBUG == 1
      printDBG("There is sufficient time to enter a deeper SLEEP mode\r\n");
#endif
        /* A slightly lower power sleep mode with a longer wake up time. */
        HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
      } else {
#if defined(TICKLESS_DEBUG) && TICKLESS_DEBUG == 1
      printDBG("There is no sufficient time to enter a deeper SLEEP mode\r\n");
#endif
        /* A slightly higher power sleep mode with a faster wake up time. */
        HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
      }
    }

    /* Allow the application to define some post sleep processing.  This is
     the standard configPOST_SLEEP_PROCESSING() macro, as described on the
     FreeRTOS.org website. */
    configPOST_SLEEP_PROCESSING( xModifiableIdleTime );

    /* Re-enable interrupts. If the timer has overflowed during this period
     then this will cause that the TIM6_IRQHandler() is called. So the
     global tick counter is incremented by 1 and the ulTickFlag variable
     is set to pdTRUE.
     Take note that in the STM32L example in the official FreeRTOS
     distribution interrupts are re-enabled after the TIM6 is stopped.
     This is wrong, because it causes that the IRQ is leaved pending,
     even if has been set. So we must first re-enable interrupts - this
     causes that a pending TIM6 IRQ fires - and then stop the timer. */
    __enable_irq();

    /* Stop TIM6.  Again, the time the clock is stopped for in not accounted
     for here (as it would normally be) because the clock is so slow it is
     unlikely it will be stopped for a complete count period anyway. */
    HAL_TIM_Base_Stop_IT(&htim6);

    if (ucTickFlag != pdFALSE) {

#if defined(TICKLESS_DEBUG) && TICKLESS_DEBUG == 1
        printDBG("TIM6 woken up the MCU and we can increase the tick count with %lu ticks\r\n", xExpectedIdleTime -1UL);
#endif
      /* The MCU has been woken up by the TIM6. So we trap overflows
       before the next calculation. */
      configASSERT(ulPeriodValueForOneTick >= (uint32_t ) __HAL_TIM_GET_COUNTER(&htim6));

      /* The tick interrupt has already executed, although because this
       function is called with the scheduler suspended the actual tick
       processing will not occur until after this function has exited.
       Reset the reload value with whatever remains of this tick period. */
      ulCounterValue = ulPeriodValueForOneTick - (uint32_t) __HAL_TIM_GET_COUNTER(&htim6);

      /* Trap under/overflows before the calculated value is used. */
      configASSERT(ulCounterValue <= ( uint32_t ) USHRT_MAX);
      configASSERT(ulCounterValue != 0);

      /* Use the calculated reload value. */
      __HAL_TIM_SET_AUTORELOAD(&htim6, ulCounterValue);
      __HAL_TIM_SET_COUNTER(&htim6, 0);

      /* The tick interrupt handler will already have pended the tick
       processing in the kernel.  As the pending tick will be processed as
       soon as this function exits, the tick value	maintained by the tick
       is stepped forward by one less than the	time spent sleeping.  The
       actual stepping of the tick appears later in this function. */
      ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
    } else {
      /* Something other than the tick interrupt ended the sleep.  How
       many complete tick periods passed while the processor was
       sleeping? */
      ulCompleteTickPeriods = ((uint32_t) __HAL_TIM_GET_COUNTER(&htim6)) / ulPeriodValueForOneTick;

      /* Check for over/under flows before the following calculation. */
      configASSERT(((uint32_t ) __HAL_TIM_GET_COUNTER(&htim6)) >= (ulCompleteTickPeriods * ulPeriodValueForOneTick));

      /* The reload value is set to whatever fraction of a single tick
       period remains. */
      ulCounterValue = ((uint32_t) __HAL_TIM_GET_COUNTER(&htim6))- (ulCompleteTickPeriods * ulPeriodValueForOneTick);
      configASSERT(ulCounterValue <= ( uint32_t ) USHRT_MAX);
      if (ulCounterValue == 0) {
        /* There is no fraction remaining. */
        ulCounterValue = ulPeriodValueForOneTick;
        ulCompleteTickPeriods++;
      }
      __HAL_TIM_SET_AUTORELOAD(&htim6, ulCounterValue);
      __HAL_TIM_SET_COUNTER(&htim6, 0);

#if defined(TICKLESS_DEBUG) && TICKLESS_DEBUG == 1
        printDBG("Another IRQ woken up the MCU and we can increase the tick count with %lu ticks\r\n", ulCompleteTickPeriods);
#endif
    }

    /* Restart TIM6 so it runs up to the reload value.  The reload value
     will get set to the value required to generate exactly one tick period
     the next time the TIM6 interrupt executes. */
    HAL_TIM_Base_Start_IT(&htim6);

    /* Wind the tick forward by the number of tick periods that the CPU
     remained in a low power state. */
    vTaskStepTick(ulCompleteTickPeriods);
  }
}


void preSLEEP(TickType_t xModifiableIdleTime) {
    UNUSED(xModifiableIdleTime);

    HAL_SuspendTick();
    __enable_irq();
    __disable_irq();
}

void postSLEEP(TickType_t xModifiableIdleTime) {
    UNUSED(xModifiableIdleTime);

    HAL_ResumeTick();
}

void preSTOP() 
{ 
  /* enter critical section */
  __disable_irq();

  HAL_RCCEx_WakeUpStopCLKConfig(RCC_STOP_WAKEUPCLOCK_MSI);

  /* Enable PWR clock enable */
  __HAL_RCC_PWR_CLK_ENABLE();

  HAL_PWREx_DisableLowPowerRunMode();

  /* Suspend tick interrupt */
  HAL_SuspendTick();

  //clear systick pending bit
  SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;

  DeInitialize_Platform();

  /* exit critical section */
  __enable_irq();
}

void postSTOP() 
{
  HAL_ResumeTick();
  ReInitialize_Platform();
  HAL_SuspendTick();
}