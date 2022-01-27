# OCTA

## Initialization
We combined some examples called batterylevel, murata-dualstack and temperature-humidity to create our base for the project. 

To start off our own LoRaWAN project we defined some variables which we will be using throughout the project. 

![defines_variables](https://github.com/TissieVA/IoT-Project/blob/master/Project/Wiki-Docs/images/defines_variables.jpg)

## Main
To start off the main function we call Initialize_platform which initializes the peripherals such as the UART, SPIO & GPIO handles. Next we initialize our battery monitoring and temperature and humidity chips. After this we initialize our Murata, threads and timers such as watchdog timer, LoRaWAN timer and other timers. Lastly we try to join the LoRaWAN network and start our kernel. 

![main](https://github.com/TissieVA/IoT-Project/blob/master/Project/Wiki-Docs/images/main.jpg)

## Functions

### Receiving UART
UART_Receive receives the messages that were send by the BLE chip to the octa. Here we check the response if it's "device disconnected", "device connected" or a clock number in the form of "1230". Depending on this print we will print that the device has connected, set an alarm or go into sleepmode because the device has disconnected. 

![UART_Receive](https://github.com/TissieVA/IoT-Project/blob/master/Project/Wiki-Docs/images/UART_Receive.jpg)

### Battery measurement
batteryLevel_measurement reads out the battery level, prints it out and saves it in a variable to send it through LoRaWAN later. 


![batteryLevel_measurement](https://github.com/TissieVA/IoT-Project/blob/master/Project/Wiki-Docs/images/batteryLevel_measurement.jpg)

### Temperature and Humidity measurement
temp_hum_measurement reads out the temperature and humidity and puts it in a variable to send it through LoRaWAN later. 

![temp_hum_measurement](https://github.com/TissieVA/IoT-Project/blob/master/Project/Wiki-Docs/images/temp_hum_measurement.jpg)

### Sending LoRaWAN messages
LoRaWAN_send creates a LoRaWAN message with the measured data and sends it through LoRaWAN to the gateway. After this it listens to the UART for possibly a new time to set the alarm. 

![LoRaWAN_send](https://github.com/TissieVA/IoT-Project/blob/master/Project/Wiki-Docs/images/LoRaWAN_send.jpg)

### Module check 
check_modules checks to see if the murata has ben initialized correctly. 

![check_modules](https://github.com/TissieVA/IoT-Project/blob/master/Project/Wiki-Docs/images/check_modules.jpg)

### Power saving
SleepMode prints a string that it is going to sleep and stops every timer whereafter it goes to sleep using HAL commands. 

![SleepMode](https://github.com/TissieVA/IoT-Project/blob/master/Project/Wiki-Docs/images/SleepMode.jpg)


## Turning BLE on 
To turn the BLE chip of the OCTA on, some files needed to be edited. First of all we need to create a define called `USE_BLE` in the Makefile.core file.

![core](https://github.com/TissieVA/IoT-Project/blob/master/Project/Wiki-Docs/images/core.png)


Also in the stml4xx_hal_msp.c file all the `#if USE_BOOTLOADER` had to be changed to `#if USE_BOOTLOADER || USE_BLE`.



