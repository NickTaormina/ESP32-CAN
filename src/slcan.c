#include "slcan.h"



void slcan_ack(){
    printf("\r");
    fflush(stdout);
}
void slcan_nack(){
    printf("\a");
    fflush(stdout);
}

void slcan_init(void)
{
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(SL_CAN_TX_GPIO, SL_CAN_RX_GPIO, TWAI_MODE_NO_ACK);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    

    //Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver\n");
        return;
    }

    //Start TWAI driver
    if (twai_start() == ESP_OK) {
        printf("Driver started\n");
        busIsRunning = true;
    } else {
        printf("Failed to start driver\n");
        return;
    }

}

void slcan_close(void){
    printf("close driver");
    busIsRunning = false;
    vTaskSuspend(readHandle);
    twai_stop();
    twai_driver_uninstall();
}
int asciiToHex(uint8_t ascii_hex_digit){
    int number = 0;
    if (ascii_hex_digit >= '0' && ascii_hex_digit <= '9') {
        number = ascii_hex_digit - '0';
    } else if (ascii_hex_digit >= 'A' && ascii_hex_digit <= 'F') {
        number = ascii_hex_digit - 'A' + 10;
    }
  return number;
}

//blinks led
void blink_task1()
{
    gpio_pad_select_gpio(2);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
    gpio_set_level(2, 0);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    gpio_set_level(2, 1);
    
}
//parses and sends a frame given message from slcan
void send_can(uint8_t* bytes){
    twai_message_t msg;
    uint32_t frameID = 0;
    //printf("sendCan\n");
    //gets the frame id from the ascii hex codes
    for(int i = 0; i<3; i++){
        //printf("id: %02X", bytes[i]);
        frameID = (frameID<< 4) | (asciiToHex(bytes[i+1]));
    }
    if(frameID != 0){
        msg.identifier = frameID;
        msg.flags = 0;
        
        //gets message 
        int frameLen = asciiToHex(bytes[SLCAN_FRAME_LEN_OFFSET]);

        //printf("frame id: %03X \n", msg.identifier);
       // printf("frame len: %02X \n", frameLen);
        //checks if the frame is a standard frame
        if(frameLen <= 8){
            msg.data_length_code = frameLen;
            //gets the data from the ascii hex codes
            for(int i = 0; i<frameLen; i++){
                msg.data[i] = (asciiToHex(bytes[SLCAN_FRAME_DATA_OFFSET + i*2]) << 4 | asciiToHex(bytes[SLCAN_FRAME_DATA_OFFSET + i*2 + 1]));
                //printf("byte: %02X \n", msg.data[i]);
            }

            //printf("sending: %03X : ", msg.identifier);
            
            //for(int i = 0; i<frameLen; i++){
              //  printf("%02X ",msg.data[i]);
           // }
            //printf("\n");
            //sends the frame
            if(twai_transmit(&msg, 20/portTICK_PERIOD_MS) == ESP_OK){
               // printf("transmitted\n");
            };
        } else {
            //printf("too long\n");
        }
    }
}

//processes received frame and sends it to slcan
void slcan_receiveFrame(twai_message_t message){

    printf("t");
    //prints the id as hex value
    if(message.identifier){
        printf("%03X", message.identifier);
    }
    //prints the data length code
    printf("%01X", message.data_length_code);
    

    //writes the data as hex values
    for (int i = 0; i < message.data_length_code; i++) {
        printf("%02X", message.data[i]);
    }
    slcan_ack();            
}

void setFilter(uint8_t* bytes){
    uint32_t frameID = 0;
    //gets the frame id from the ascii hex codes
    for(int i = 0; i<3; i++){
        //printf("id: %02X", bytes[i]);
        frameID = (frameID<< 4) | (asciiToHex(bytes[i+1]));
    }
}

void processSlCommand(uint8_t* bytes){
    //printf("process sl command: %02X", bytes[0]); 
    switch((char)bytes[0]){
        case 'O':
            slcan_init();
            vTaskResume(readHandle);
            break;
        case 'C':
            slcan_close();
            slcan_ack();
            break;
        case 't':
            send_can(bytes);
            slcan_ack();
            break;
        case 'T':
            send_can(bytes);
            slcan_ack();
            break;
        /*
        case 'W':
        break;
        case 'm':
        break;
        case 'M':
        break;
        case 'r':
        break;
        case 'R':
        break;
        case 'V':
        break;
        case 'N':
        break;
        case 'U':
        break;
        case 'Z':
        break;
        case 'S':
        break;
        case 's':
        break;
        case 'F':
        break;
        case 'L':
        break;
        case 'A':
            break;*/



    }
    
        
        
     
    

}

