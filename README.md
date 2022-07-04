# ESP32-BLE-OTA
application to cover OTA firmware update over BLE using bluedroid stack of ESP32.  
ESP32 acts as a peripheral and gatt server while our pc acts as central device and gatt client.  
a python script is to be run on the client side(use python 3.8 or above) device after activating bluetooth connectivity  
and selecting binary file path in the script.  
a custom partition table for OTA update is used based on default options available.  
build and flash the given code using ESP-IDF platform.  
  
reference - https://michaelangerer.dev/esp32/ble/ota/2021/06/01/esp32-ota-part-1.html
