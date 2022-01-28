# Power Profiling
We profiled the power consumption of our application using a Power Profiler Kit (PPK) on top of a nRF52840 DK made by Nordic. 

![](https://infocenter.nordicsemi.com/topic/ug_ppk/UG/ppk/PPK_images/usecase_3_ext_hw_dk_jlink-01.svg)

To measure the power profile of an external device with the PPK, the following steps are required:
1. Set the DUT select switch on: External
2. Set the Power select switch: Reg
3. Set the COM switch: DK 
4. Connect the '+' side of the external DUT to the 3.3V pin of the OCTA
5. Connect the '-' side of the external DUT to the ground pin of the OCTA
6. Open the PPK software in the nrfConnect app on a computer
7. Measure...

## Active power profile: readings + LoRaWAN messages
Below is the energy profile of our Frost Detector device while it has done 2 measurements and has sent 2 LoRaWAN messages. The measurements are represented by the lower peaks and the LoRaWAN messages are represented by the largest peaks.
![active-power-profile](https://github.com/TissieVA/IoT-Project/blob/master/Project/Wiki-Docs/images/active-profile-lorawan+reading.png)

## Active power profile: BLE connect + BLE receive
After being awoken by the bluetooth button on the Frost Detector device and after a BLE connection was established, we measured the following power profile:
![active-2-power-profile](https://github.com/TissieVA/IoT-Project/blob/master/Project/Wiki-Docs/images/sleep-profile.png)

## Sleep mode profile
When our Frost Detector device is in sleep mode, we measured the following power profile:
![sleep-power-profile]()