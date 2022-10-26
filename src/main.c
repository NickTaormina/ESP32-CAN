#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/can.h"
#include "driver/uart.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "string.h"
#include "stdlib.h"


#define CAN_MAX_DATA_LEN 128

uint32_t filter;
uint8_t filterOn = 0;


//reads frames from the can receiver
void readFrames(){
    can_message_t message;
    int x = 0;
    //waits for message to be received
    while(1){
              
        
        if (can_receive(&message, pdMS_TO_TICKS(99000)) == ESP_OK) {
        } else {
            printf("Failed to receive message\n");
        }

        
        if (!(message.flags & CAN_MSG_FLAG_RTR) && filterOn == 0) {
            printf(" R:");
            printf("[%d]", message.identifier);
            for (int i = 0; i < message.data_length_code; i++) {
                printf("[%d]", message.data[i]);
            }
        } else if (!(message.flags & CAN_MSG_FLAG_RTR) && filterOn == 1){
            if(message.identifier == filter){
                printf("R:");
                printf("[%d] | ", message.identifier);
                for (int i = 0; i < message.data_length_code; i++) {
                printf(" [%d] ", message.data[i]);
            }
            }
        }
        printf("\\");
        
    }
    
}

void processCommand(uint8_t* bytes){
    can_message_t msg;
    int tmplen = strlen((char*) bytes);
    uint8_t* id = malloc(4);
    uint8_t* frame = malloc(8);
    memset(id, 0, 4);
    memset(frame, 0, 8);
    int framePos = 0;
    if(strstr((char*)bytes, "W:") != 0){
        uint8_t foundID = 0;
        int i = 0;
        while(i < tmplen-1){
            //finds the id
            
            //use strNcmp when finding i single byte
            if(strncmp((char*)&bytes[i], "[", 1) == 0 && foundID == 0){
                int t = 1;  
                for(int x = 1; x<12; x++){
                    if(strncmp((char*)&bytes[i+x], "]",1) != 0){
                        t = t + 1;
                    } else {  
                        memcpy(id, &bytes[i+1], t-1);
                        uint32_t idByte = atoi((char*)id); //converts our string id array to int for frame
                        msg.identifier = idByte;
                        i = i + t;
                        foundID = 1;
                        break;
                    }
                }
                
            } 
            //find the data
            if(strncmp((char*)&bytes[i], "[", 1) == 0 && foundID == 1){
                uint8_t* tmp = malloc(3);
                for(int x = 1; x< 5; x++){
                    if(strncmp((char*)&bytes[i+x], "]",1) != 0){
                        if(x<4){
                        memcpy(&tmp[x-1], &bytes[i+x], 1);
                        }
                    } else{
                        uint8_t byte = atoi((char*) tmp);
                        msg.data[framePos] = byte;
                        memcpy(&frame[framePos], &byte, 1);
                        i = i+x;
                        framePos = framePos + 1;
                        break;
                    }
                }        
                
            }

            i = i+1;
        }

        int frameLength = 8;
        msg.data_length_code = frameLength;
        can_transmit(&msg, 50);
     
    }

}

//echos on the usb serial connection to pc
void echo_task(){
    // Configure a temporary buffer for the incoming data
    uint8_t *data =  malloc(2048);
    uint8_t *tmp =  malloc(1024);
    memset(tmp, 0, 1024);

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_0, data, (2048 - 1), 5 / portTICK_PERIOD_MS);

        //Write data back to the UART
        if (len) {
            
            memcpy(tmp+strlen((char*)tmp), data, len);
            int tmplen = strlen((char*)tmp);
            for(int i = 0; i<tmplen; i++){
                if(strcmp((char*)&tmp[i], "/") == 0){
                        uint8_t* tmp2 = malloc(1024);
                        memcpy(tmp2, tmp, strlen((char*)tmp)-1);
                        processCommand(tmp2);
                        memset(tmp, 0, 1024);
                        uart_flush_input(UART_NUM_0);
                        break;

                }
            }
            data[len] = '\0';
            if(strlen((char*)data) > 1900){
                memset(data, 0, 2048);
            }
            
        }
    }
}

void app_main() {
    //Initialize configuration structures using macro initializers
    can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, CAN_MODE_NO_ACK);
    can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
    can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

    //Install CAN driver
    if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver\n");
        return;
    }

    //Start CAN driver
    if (can_start() == ESP_OK) {
        printf("Driver started\n");
    } else {
        printf("Failed to start driver\n");
        return;
    }

    //onboard led
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    int intr_alloc_flags = 0;

    //initialize usb uart connection
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 2048 * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, GPIO_NUM_1, GPIO_NUM_3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    uart_set_baudrate(UART_NUM_0, 921600);

    xTaskCreatePinnedToCore(echo_task, "echo task", 8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(readFrames, "read frames", 8192, NULL, 2, NULL, 1);
}