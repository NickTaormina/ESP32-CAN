# ESP32-CAN
ESP32 program to read/write canbus messages. communicates over usb/uart with slcan/socketcan protocol

For use with: https://github.com/NickTaormina/Gauge-Cluster

uses ESP32 and TI sn65hvd230 can-bus transceiver module

outputs frames with message "tiiildddddddd\r" 
-> t (3 digit hex id) ( 1 digit hex length) (data) \r
send same format to device to send a frame
