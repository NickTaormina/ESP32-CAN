#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "driver/uart.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "string.h"
#include "stdlib.h"
#include "slcan.h"


#define TWAI_MAX_DATA_LEN 128
TaskHandle_t readHandle = NULL;

static QueueHandle_t can_frame_queue;

//blinks led
void blink_task()
{
    gpio_pad_select_gpio(2);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
        gpio_set_level(2, 0);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        gpio_set_level(2, 1);

    
}




//looks for uart commands and sends them to slcan
void echo_task(){
    // Configure a temporary buffer for the incoming data
    uint8_t *data =  malloc(2048);
    uint8_t *tmp =  malloc(2048);
    memset(tmp, 0, 2048);

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_0, data, (2048 - 1), 5 / portTICK_PERIOD_MS);

        if (len) {
            //printf("read");
            memcpy(tmp+strlen((char*)tmp), data, len);
            int tmplen = strlen((char*)tmp);
            for(int i = 0; i<tmplen; i++){
                if(strcmp((char*)&tmp[i], "\r") == 0){
                        uart_flush_input(UART_NUM_0);
                        processSlCommand(tmp);
                        memset(tmp, 0, 2048);
                        memset(data,0,2048);
                        
                        break;

                }
            }

            memset(data, 0, 2048);
            
        }
    }
}
//reads frames from the twai receiver
void readFrames(){
    while(1){
        twai_status_info_t test;
        twai_get_status_info(&test);
        if(test.state == TWAI_STATE_RUNNING){
            break; 
        } else {
            printf("bus not open");
        }
    }
    twai_message_t message;
    //waits for message to be received
    while(1){
        if(busIsRunning == true){
            if (twai_receive(&message, pdMS_TO_TICKS(99000)) == ESP_OK) {             
                slcan_receiveFrame(message); 
                //printf("missed: %d : %d : %d \n", test.rx_error_counter, test.rx_missed_count, test.rx_overrun_count);
            } else {
                slcan_nack();
            }
        }
    }
}
void app_main() {
    
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 921600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    int intr_alloc_flags = 0;

    //initialize usb uart connection
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 4096 * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, GPIO_NUM_1, GPIO_NUM_3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    uart_set_baudrate(UART_NUM_0, 921600);
    //Initialize configuration structures using macro initializers
    

    //onboard led
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

    //remove printf buffer
    setbuf(stdout, NULL);

    //xQueueCreate(20, sizeof(twai_message_t));
    slcan_init();
    xTaskCreatePinnedToCore(echo_task, "echo task", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(readFrames, "read frames", 8192, NULL, configMAX_PRIORITIES, &readHandle, 0);
}