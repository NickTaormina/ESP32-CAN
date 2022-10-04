# ESP32-CAN
ESP32 program to read/write canbus messages. communicates over usb/uart

For use with: https://github.com/NickTaormina/Gauge-Cluster

uses ESP32 and TI sn65hvd230 can-bus transceiver module

outputs frames with message "READ: [frame id] [byte] [byte] [byte] [byte] [byte] [byte] [byte] [byte]\\"

write can message with command: "WRITE: [frame id] [byte] [byte] [byte] [byte] [byte] [byte] [byte] [byte] /"

uses base 10 values
