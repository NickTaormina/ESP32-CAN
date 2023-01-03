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
    g_config.rx_queue_len = 500;
    g_config.intr_flags = ESP_INTR_FLAG_IRAM;
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
// Lookup table to map ASCII characters to their corresponding hexadecimal values
const uint8_t ascii_hex_lookup[256] =
{
    ['0'] = 0,
    ['1'] = 1,
    ['2'] = 2,
    ['3'] = 3,
    ['4'] = 4,
    ['5'] = 5,
    ['6'] = 6,
    ['7'] = 7,
    ['8'] = 8,
    ['9'] = 9,
    ['A'] = 10,
    ['B'] = 11,
    ['C'] = 12,
    ['D'] = 13,
    ['E'] = 14,
    ['F'] = 15,
};

int asciiToHex(uint8_t ascii_hex_digit)
{
    // Look up the hexadecimal value in the lookup table
    return ascii_hex_lookup[ascii_hex_digit];
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
// Create a buffer to hold the string representation of the CAN frame data
#define CAN_FRAME_BUFFER_SIZE 32
char can_frame_buffer[CAN_FRAME_BUFFER_SIZE];
void slcan_receiveFrame(twai_message_t message)
{
    // Create a string representation of the CAN frame data using snprintf
    int len = snprintf(can_frame_buffer, CAN_FRAME_BUFFER_SIZE, "t%03X%01X", message.identifier, message.data_length_code);
    for (int i = 0; i < message.data_length_code; i++)
    {
        len += snprintf(can_frame_buffer + len, CAN_FRAME_BUFFER_SIZE - len, "%02X", message.data[i]);
    }

    // Print the CAN frame data to the log
    printf("%s\r", can_frame_buffer);

   // slcan_ack();
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

