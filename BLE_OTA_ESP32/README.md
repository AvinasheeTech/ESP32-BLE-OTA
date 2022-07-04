##Hardware Required

ESP32 dev board<br />
PC or raspberry pi<br />

##Menuconfig 

idf.py menuconfig

Component config->Bluetooth->Bluedroid Options
 
##Build, Flash and Monitor 

idf.py build

Replace PORT with serial port name:

idf.py -p PORT flash monitor  

(To exit the serial monitor, type Ctrl-])


## Example output  
I (721063) Bluedroid-Peripheral: Received packet 2576  
I (721303) Bluedroid-Peripheral: attribute handle:50, len:253  
I (721303) Bluedroid-Peripheral: Received packet 2577  
I (721483) Bluedroid-Peripheral: attribute handle:50, len:253  
I (721483) Bluedroid-Peripheral: Received packet 2578  
I (721663) Bluedroid-Peripheral: attribute handle:50, len:253  
I (721663) Bluedroid-Peripheral: Received packet 2579  
I (721843) Bluedroid-Peripheral: attribute handle:50, len:253  
I (721843) Bluedroid-Peripheral: Received packet 2580  
I (722023) Bluedroid-Peripheral: attribute handle:50, len:253  
I (722023) Bluedroid-Peripheral: Received packet 2581  
I (722263) Bluedroid-Peripheral: attribute handle:50, len:253  
I (722263) Bluedroid-Peripheral: Received packet 2582  
I (722443) Bluedroid-Peripheral: attribute handle:50, len:253  
I (722443) Bluedroid-Peripheral: Received packet 2583  
I (722623) Bluedroid-Peripheral: attribute handle:50, len:253  
I (722623) Bluedroid-Peripheral: Received packet 2584  
I (722743) Bluedroid-Peripheral: attribute handle:50, len:168  
I (722753) Bluedroid-Peripheral: Received packet 2585  
I (722863) Bluedroid-Peripheral: attribute handle:47, len:1  
I (722863) Bluedroid-Peripheral: 04  
I (722973) esp_image: segment 0: paddr=00210020 vaddr=3f400020 size=1b250h (111184) map  
I (723023) esp_image: segment 1: paddr=0022b278 vaddr=3ffbdb60 size=03bc8h ( 15304)  
I (723033) esp_image: segment 2: paddr=0022ee48 vaddr=40080000 size=011d0h (  4560)  
I (723073) esp_image: segment 3: paddr=00230020 vaddr=400d0020 size=6af7ch (438140) map  
I (723263) esp_image: segment 4: paddr=0029afa4 vaddr=400811d0 size=14a80h ( 84608)  
I (723313) esp_image: segment 5: paddr=002afa2c vaddr=50000000 size=00010h (    16)  
I (723353) esp_image: segment 0: paddr=00210020 vaddr=3f400020 size=1b250h (111184) map  
I (723423) esp_image: segment 1: paddr=0022b278 vaddr=3ffbdb60 size=03bc8h ( 15304)  
I (723443) esp_image: segment 2: paddr=0022ee48 vaddr=40080000 size=011d0h (  4560)  
I (723453) esp_image: segment 3: paddr=00230020 vaddr=400d0020 size=6af7ch (438140) map  
I (723653) esp_image: segment 4: paddr=0029afa4 vaddr=400811d0 size=14a80h ( 84608)  
I (723703) esp_image: segment 5: paddr=002afa2c vaddr=50000000 size=00010h (    16)  
I (723743) Bluedroid-Peripheral: sent data:0  
I (723743) Bluedroid-Peripheral: conn_id:0  
I (723743) Bluedroid-Peripheral: OTA DONE acknowledgement has been sent.  
I (723753) Bluedroid-Peripheral: Preparing to restart!  
I (723823) Bluedroid-Peripheral: attribute handle:48, len:2  
I (723823) Bluedroid-Peripheral: 00 00  
