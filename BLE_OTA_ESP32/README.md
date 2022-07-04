##Hardware Required

ESP32 dev board<br />
LIS2DW12 module

##Menuconfig 

idf.py menuconfig

Component config->Bluetooth->Bluedroid Options
 
##Build, Flash and Monitor 

idf.py build

Replace PORT with serial port name:

idf.py -p PORT flash 

idf.py -p PORT monitor

(To exit the serial monitor, type Ctrl-])


## Example output
I (904) Bluedroid-Peripheral: app_main init bluetooth<br />
I (994) Bluedroid-Peripheral: Mac address<br />
I (994) Bluedroid-Peripheral: 7c:<br />
I (994) Bluedroid-Peripheral: 9e:<br />
I (994) Bluedroid-Peripheral: bd:<br />
I (994) Bluedroid-Peripheral: 45:<br />
I (1004) Bluedroid-Peripheral: 9a:<br />
I (1004) Bluedroid-Peripheral: fc:<br />
I (1024) Bluedroid-Peripheral: The number handle = a<br />
I (1024) Bluedroid-Peripheral: attribute table:40<br />
I (1024) Bluedroid-Peripheral: attribute table:41<br />
I (1024) Bluedroid-Peripheral: attribute table:42<br />
I (1034) Bluedroid-Peripheral: attribute table:43<br />
I (1034) Bluedroid-Peripheral: attribute table:44<br />
I (1044) Bluedroid-Peripheral: attribute table:45<br />
I (1044) Bluedroid-Peripheral: attribute table:46<br />
I (1054) Bluedroid-Peripheral: attribute table:47<br />
I (1054) Bluedroid-Peripheral: attribute table:48<br />
I (1064) Bluedroid-Peripheral: attribute table:49<br />
I (1074) Bluedroid-Peripheral: Mac address<br />
I (1074) Bluedroid-Peripheral: 7c:<br />
I (1074) Bluedroid-Peripheral: 9e:<br />
I (1084) Bluedroid-Peripheral: bd:<br />
I (1084) Bluedroid-Peripheral: 45:<br />
I (1094) Bluedroid-Peripheral: 9a:<br />
I (1094) Bluedroid-Peripheral: fc:<br />
I (1114) Bluedroid-Peripheral: I2C initialized...................<br />
I (1124) Bluedroid-Peripheral: advertising start success<br />
I (1144) Bluedroid-Peripheral: advertising start success<br />
I (1154) Bluedroid-Peripheral: advertising start success<br />
I (1164) Bluedroid-Peripheral: LIS2DW12 validated................<br />
I (1164) Bluedroid-Peripheral: WHO_AM_I:44<br />
I (1164) Bluedroid-Peripheral: CTRL2:8<br />
I (1164) Bluedroid-Peripheral: FIFO_CTRL:c0<br />
I (1174) Bluedroid-Peripheral: CTRL1:20<br />
I (1174) Bluedroid-Peripheral: CTRL3:1<br />
I (68354) Bluedroid-Peripheral: ESP_GATTS_CONNECT_EVT<br />
I (68834) Bluedroid-Peripheral: attribute handle:43, len:2<br />
I (68834) Bluedroid-Peripheral: 01 00<br />
I (68834) Bluedroid-Peripheral: notify enabled, handle:43<br />
I (68844) Bluedroid-Peripheral: sent data:144.000000 for x-axes<br />
I (68844) Bluedroid-Peripheral: conn_id:0<br />
I (68924) Bluedroid-Peripheral: attribute handle:46, len:2<br />
I (68924) Bluedroid-Peripheral: 01 00<br />
I (68924) Bluedroid-Peripheral: notify enabled, handle:46<br />
I (68934) Bluedroid-Peripheral: sent data:64.000000 for y-axes<br />
I (68934) Bluedroid-Peripheral: conn_id:0<br />
I (68984) Bluedroid-Peripheral: attribute handle:49, len:2<br />
I (68984) Bluedroid-Peripheral: 01 00<br />
I (68984) Bluedroid-Peripheral: notify enabled, handle:49<br />
I (68994) Bluedroid-Peripheral: sent data:32.000000 for z-axes<br />
I (68994) Bluedroid-Peripheral: conn_id:0<br />
I (69174) Bluedroid-Peripheral: notify enabled<br />
I (69184) Bluedroid-Peripheral: x_axes:128.000000<br />
I (69184) Bluedroid-Peripheral: sent data:128.000000 for x-axes<br />
I (69184) Bluedroid-Peripheral: conn_id:0<br />
I (69194) Bluedroid-Peripheral: y_axes:16.000000<br />
I (69194) Bluedroid-Peripheral: sent data:16.000000 for y-axes<br />
I (69204) Bluedroid-Peripheral: conn_id:0<br />
I (69204) Bluedroid-Peripheral: z_axes:128.000000<br />
I (69214) Bluedroid-Peripheral: sent data:128.000000 for z-axes<br />
I (69214) Bluedroid-Peripheral: conn_id:0<br />
I (70234) Bluedroid-Peripheral: notify enabled<br />
I (70234) Bluedroid-Peripheral: x_axes:208.000000<br />
I (70244) Bluedroid-Peripheral: sent data:208.000000 for x-axes<br />
I (70244) Bluedroid-Peripheral: conn_id:0<br />
I (70244) Bluedroid-Peripheral: y_axes:64.000000<br />
I (70254) Bluedroid-Peripheral: sent data:64.000000 for y-axes<br />
I (70254) Bluedroid-Peripheral: conn_id:0<br />
I (70264) Bluedroid-Peripheral: z_axes:32.000000<br />
I (70274) Bluedroid-Peripheral: sent data:32.000000 for z-axes<br />
I (70274) Bluedroid-Peripheral: conn_id:0<br />
I (71284) Bluedroid-Peripheral: notify enabled<br />
I (71284) Bluedroid-Peripheral: x_axes:0.000000<br />
I (71284) Bluedroid-Peripheral: sent data:0.000000 for x-axes<br />
I (71284) Bluedroid-Peripheral: conn_id:0<br />
I (71294) Bluedroid-Peripheral: y_axes:64.000000<br />
I (71294) Bluedroid-Peripheral: sent data:64.000000 for y-axes<br />
I (71314) Bluedroid-Peripheral: conn_id:0<br />
I (71314) Bluedroid-Peripheral: z_axes:80.000000<br />
I (71314) Bluedroid-Peripheral: sent data:80.000000 for z-axes<br />
I (71334) Bluedroid-Peripheral: conn_id:0<br />
I (72334) Bluedroid-Peripheral: notify enabled<br />
I (72334) Bluedroid-Peripheral: x_axes:160.000000<br />
I (72344) Bluedroid-Peripheral: sent data:160.000000 for x-axes<br />
I (72344) Bluedroid-Peripheral: conn_id:0<br />
I (72344) Bluedroid-Peripheral: y_axes:64.000000<br />
I (72354) Bluedroid-Peripheral: sent data:64.000000 for y-axes<br />
I (72354) Bluedroid-Peripheral: conn_id:0<br />
I (72364) Bluedroid-Peripheral: z_axes:0.000000<br />
I (72364) Bluedroid-Peripheral: sent data:0.000000 for z-axes<br />
I (72374) Bluedroid-Peripheral: conn_id:0<br />
I (73384) Bluedroid-Peripheral: notify enabled<br />
I (73384) Bluedroid-Peripheral: x_axes:144.000000<br />
I (73384) Bluedroid-Peripheral: sent data:144.000000 for x-axes<br />
I (73384) Bluedroid-Peripheral: conn_id:0<br />
I (73394) Bluedroid-Peripheral: y_axes:0.000000<br />
I (73394) Bluedroid-Peripheral: sent data:0.000000 for y-axes<br />
I (73404) Bluedroid-Peripheral: conn_id:0<br />
I (73404) Bluedroid-Peripheral: z_axes:32.000000<br />
I (73414) Bluedroid-Peripheral: sent data:32.000000 for z-axes<br />
I (73424) Bluedroid-Peripheral: conn_id:0<br />
I (74434) Bluedroid-Peripheral: notify enabled<br />
I (74434) Bluedroid-Peripheral: x_axes:16.000000<br />
I (74434) Bluedroid-Peripheral: sent data:16.000000 for x-axes<br />
I (74434) Bluedroid-Peripheral: conn_id:0<br />
I (74444) Bluedroid-Peripheral: y_axes:240.000000<br />
I (74444) Bluedroid-Peripheral: sent data:240.000000 for y-axes<br />
I (74464) Bluedroid-Peripheral: conn_id:0<br />
I (74464) Bluedroid-Peripheral: z_axes:48.000000<br />
I (74474) Bluedroid-Peripheral: sent data:48.000000 for z-axes<br />
I (74474) Bluedroid-Peripheral: conn_id:0<br />
I (75484) Bluedroid-Peripheral: notify enabled<br />
I (75484) Bluedroid-Peripheral: x_axes:160.000000<br />
I (75484) Bluedroid-Peripheral: sent data:160.000000 for x-axes<br />
I (75484) Bluedroid-Peripheral: conn_id:0<br />
I (75494) Bluedroid-Peripheral: y_axes:240.000000<br />
I (75494) Bluedroid-Peripheral: sent data:240.000000 for y-axes<br />
I (75514) Bluedroid-Peripheral: conn_id:0<br />
I (75514) Bluedroid-Peripheral: z_axes:0.000000<br />
I (75514) Bluedroid-Peripheral: sent data:0.000000 for z-axes<br />
I (75534) Bluedroid-Peripheral: conn_id:0<br />
I (76534) Bluedroid-Peripheral: notify enabled<br />
I (76534) Bluedroid-Peripheral: x_axes:192.000000<br />
I (76544) Bluedroid-Peripheral: sent data:192.000000 for x-axes<br />
I (76544) Bluedroid-Peripheral: conn_id:0<br />
I (76544) Bluedroid-Peripheral: y_axes:160.000000<br />
I (76554) Bluedroid-Peripheral: sent data:160.000000 for y-axes<br />
I (76554) Bluedroid-Peripheral: conn_id:0<br />
I (76564) Bluedroid-Peripheral: z_axes:64.000000<br />
I (76564) Bluedroid-Peripheral: sent data:64.000000 for z-axes<br />
I (76574) Bluedroid-Peripheral: conn_id:0<br />
I (77584) Bluedroid-Peripheral: notify enabled<br />
I (77584) Bluedroid-Peripheral: x_axes:64.000000<br />
I (77584) Bluedroid-Peripheral: sent data:64.000000 for x-axes<br />
I (77584) Bluedroid-Peripheral: conn_id:0<br />
I (77594) Bluedroid-Peripheral: y_axes:16.000000<br />
I (77594) Bluedroid-Peripheral: sent data:16.000000 for y-axes<br />
I (77614) Bluedroid-Peripheral: conn_id:0<br />
I (77614) Bluedroid-Peripheral: z_axes:208.000000<br />
I (77614) Bluedroid-Peripheral: sent data:208.000000 for z-axes<br />
I (77634) Bluedroid-Peripheral: conn_id:0<br />
I (78634) Bluedroid-Peripheral: notify enabled<br />
I (78634) Bluedroid-Peripheral: x_axes:0.000000<br />
I (78644) Bluedroid-Peripheral: sent data:0.000000 for x-axes<br />
I (78644) Bluedroid-Peripheral: conn_id:0<br />
I (78644) Bluedroid-Peripheral: y_axes:224.000000<br />
I (78654) Bluedroid-Peripheral: sent data:224.000000 for y-axes<br />
I (78654) Bluedroid-Peripheral: conn_id:0<br />
I (78664) Bluedroid-Peripheral: z_axes:96.000000<br />
I (78674) Bluedroid-Peripheral: sent data:96.000000 for z-axes<br />
I (78674) Bluedroid-Peripheral: conn_id:0<br />
