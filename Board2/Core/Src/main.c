/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c_lcd.h"
#include <string.h>
#include <stdio.h>  // Added for sscanf and string parsing
#include <stdarg.h>
 // Your Adafruit IO credentials (SSID, PASS, IO_USERNAME, IO_KEY)
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim5;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
#define RX_BUFFER_SIZE 128
uint8_t rx_buffer[RX_BUFFER_SIZE];
uint8_t rx_byte;
uint8_t rx_index = 0;
uint8_t data_ready = 0;
uint8_t empty = 4;
uint8_t arr[5]={0,0,0,0,0};
int32_t card_state[6];

#define NUM_RFIDS 6

const char *valid_rfids[NUM_RFIDS] = {
    "C4BC1902", "033FD72C", "7BDE8E02",
    "7673B202", "F393B802", "0AD8B902"
};

// Initialized to -1. A value of -1 means "Not Entered", any other value means "Entered"
int32_t rfid_timestamps[NUM_RFIDS] = {-1, -1, -1, -1, -1, -1};
char fetched_rfid[32] = {0}; // Buffer to hold the incoming ID

uint8_t gate1_in_use = 0;  // 1 if Gate 1 is currently open for entry
uint8_t gate2_in_use = 0;  // 1 if Gate 2 is currently open for exit

uint8_t gate_lock = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM5_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
void rgb_color1(uint8_t red, uint8_t green, uint8_t blue);
//void rgb_color2(uint8_t red, uint8_t green, uint8_t blue);
void rgb_color3(uint8_t red, uint8_t green, uint8_t blue);
//void rgb_color4(uint8_t red, uint8_t green, uint8_t blue);
void Get_Sensor_Update(void);
void Process_Received_Data(char* data);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Function to control hardware based on received data
void debug_print(const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Send to serial
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 100);
    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 100);
	return;
}


void LCD_empty_spots(void) {
    char lcd_buffer[32];
    uint8_t display_col = 7; // Start printing spot numbers after "spots: " (columns 0-6)

    // 1. Safely format and display the total count on Row 0
    snprintf(lcd_buffer, sizeof(lcd_buffer), "Total Empty = %d     ", empty);
    lcd_put_cur(0, 0);
    lcd_send_string(lcd_buffer);

    // 2. Print the static prefix on Row 1
    lcd_put_cur(1, 0);
    lcd_send_string("spots: ");

    // 3. Dynamic loop to print empty spots next to the prefix
    for (int i = 1; i < 5; i++) {
        if (arr[i+1] == 0) { //  means empty based on your Process_Received_Data logic
            snprintf(lcd_buffer, sizeof(lcd_buffer), "%d ", i );
            lcd_put_cur(1, display_col);
            lcd_send_string(lcd_buffer);
            display_col += 2; // Advance position for the next found spot
        }
    }

    // 4. Clear out any trailing character artifacts left over from previous prints
    while (display_col < 16) {
        lcd_put_cur(1, display_col);
        lcd_send_string(" ");
        display_col++;
    }
}

void Get_Sensor_Update(void) {

    char http_request[300];
    char at_cmd[64];
    char clean_payload[32];
    char lcd_msg[32];

    // Static tracking variable to remember if our TCP pipe is open
    static uint8_t tcp_connected = 0;

    // --- HARDWARE FLUSH ---
    volatile uint32_t tmpreg = huart2.Instance->SR;
    volatile uint32_t tmpreg2 = huart2.Instance->DR;
    (void)tmpreg; (void)tmpreg2;

    // 1. Build the HTTP GET request with "keep-alive"
    snprintf(http_request, sizeof(http_request),
             "GET /api/v2/toqahossamx/feeds/sensor/data/last HTTP/1.1\r\n"
             "Host: io.adafruit.com\r\n"
            // "X-AIO-Key: YOUR_ADAFRUIT_KEY\r\n"
             "Connection: keep-alive\r\n\r\n"); // Keep connection open!

    // 2. Connect via TCP ONLY if we aren't already connected
    if (!tcp_connected) {
        //lcd_put_cur(0,0);
        //lcd_send_string("Connecting TCP...");

        char cipstart[] = "AT+CIPSTART=\"TCP\",\"io.adafruit.com\",80\r\n";
        HAL_UART_Transmit(&huart2, (uint8_t*)cipstart, strlen(cipstart), 1000);

        // Dynamic wait for connection response instead of a flat 500ms sleep
        char resp = 0;
        uint32_t start = HAL_GetTick();
        while((HAL_GetTick() - start) < 1000) {
            if(HAL_UART_Receive(&huart2, (uint8_t*)&resp, 1, 10) == HAL_OK) {
                // If we get an OK or ALREADY CONNECTED indicator, we can break early
                if (resp == '\n') break;
            }
        }
        tcp_connected = 1;
    }

    // 3. Send length command
    snprintf(at_cmd, sizeof(at_cmd), "AT+CIPSEND=%d\r\n", (int)strlen(http_request));
    HAL_UART_Transmit(&huart2, (uint8_t*)at_cmd, strlen(at_cmd), 1000);

    // Quick wait for the '>' prompt symbol from ESP instead of a blind 200ms delay
    char prompt = 0;
    uint32_t p_start = HAL_GetTick();
    while((HAL_GetTick() - p_start) < 500) {
        if(HAL_UART_Receive(&huart2, (uint8_t*)&prompt, 1, 5) == HAL_OK) {
            if(prompt == '>') break;
        }
    }

    // 4. Send the HTTP request payload
    HAL_UART_Transmit(&huart2, (uint8_t*)http_request, strlen(http_request), 1000);

    // 5. REAL-TIME STREAM PARSING
    const char *target = "\"value\":\"";
    int match_idx = 0;
    char rx_char;
    uint8_t found_data = 0;
    char type = 0;
    int index = 0, state = 0;

    // Monitor stream for up to 2000ms (Slashed from 3000ms for speed)
    uint32_t start_time = HAL_GetTick();
    while ((HAL_GetTick() - start_time) < 2000) {
        if (HAL_UART_Receive(&huart2, (uint8_t*)&rx_char, 1, 5) == HAL_OK) {
            if (rx_char == target[match_idx]) {
                match_idx++;
                if (target[match_idx] == '\0') {
                    char payload_buf[16];
                    int p_idx = 0;

                    while ((HAL_GetTick() - start_time) < 2000 && p_idx < 15) {
                        if (HAL_UART_Receive(&huart2, (uint8_t*)&rx_char, 1, 5) == HAL_OK) {
                            if (rx_char == '"') break;
                            payload_buf[p_idx++] = rx_char;
                        }
                    }
                    payload_buf[p_idx] = '\0';

                    if (sscanf(payload_buf, "%c,%d,%d", &type, &index, &state) == 3) {
                        found_data = 1;
                    }
                    break;
                }
            } else {
                match_idx = (rx_char == target[0]) ? 1 : 0;
            }
        }
    }

    // 6. Display updates and fallback logic
    if (found_data) {
        snprintf(clean_payload, sizeof(clean_payload), "value=%c,%d,%d", type, index, state);
        snprintf(lcd_msg, sizeof(lcd_msg), "Val: %c | %d | %d   ", type, index, state);

     /*   lcd_put_cur(2,0);
        lcd_send_string(lcd_msg);
        lcd_put_cur(2,0);
        lcd_send_string("Fetch Online    ");*/

        Process_Received_Data(clean_payload);
    } else {
     //   lcd_put_cur(3,);
       // lcd_send_string("Fetch Retry...");
        // If data stream timed out, assume TCP disconnected or broke down.
        // Forcing a fresh connection setup on the next loop.
        tcp_connected = 0;
    }
}
void Fetch_Latest_RFID(void) {
    char http_request[300];
    char at_cmd[64];

    // Clear previous ID
    memset(fetched_rfid, 0, sizeof(fetched_rfid));

    // 1. Build request for the RFID feed
    snprintf(http_request, sizeof(http_request),
             "GET /api/v2/toqahossamx/feeds/rfid/data/last HTTP/1.1\r\n"
             "Host: io.adafruit.com\r\n"
             "X-AIO-Key: YOUR_ADAFRUIT_KEY\r\n"
             "Connection: keep-alive\r\n\r\n");

    // 2. Send length command
    snprintf(at_cmd, sizeof(at_cmd), "AT+CIPSEND=%d\r\n", (int)strlen(http_request));
    HAL_UART_Transmit(&huart2, (uint8_t*)at_cmd, strlen(at_cmd), 1000);

    // Wait for prompt '>'
    char prompt = 0;
    uint32_t p_start = HAL_GetTick();
    while((HAL_GetTick() - p_start) < 500) {
        if(HAL_UART_Receive(&huart2, (uint8_t*)&prompt, 1, 5) == HAL_OK && prompt == '>') break;
    }

    // 3. Send the HTTP request
    HAL_UART_Transmit(&huart2, (uint8_t*)http_request, strlen(http_request), 1000);

    // 4. Parse the String ID from JSON {"value":"C4BC1902"}
    const char *target = "\"value\":\"";
    int match_idx = 0;
    char rx_char;
    uint32_t start_time = HAL_GetTick();

    while ((HAL_GetTick() - start_time) < 2000) {
        if (HAL_UART_Receive(&huart2, (uint8_t*)&rx_char, 1, 5) == HAL_OK) {
            if (rx_char == target[match_idx]) {
                match_idx++;
                if (target[match_idx] == '\0') {
                    int p_idx = 0;
                    while ((HAL_GetTick() - start_time) < 2000 && p_idx < 31) {
                        if (HAL_UART_Receive(&huart2, (uint8_t*)&rx_char, 1, 5) == HAL_OK) {
                            if (rx_char == '"') break; // End of string
                            fetched_rfid[p_idx++] = rx_char;
                        }
                    }
                    fetched_rfid[p_idx] = '\0';
                    break; // Parsing complete
                }
            } else {
                match_idx = (rx_char == target[0]) ? 1 : 0;
            }
        }
    }
}


void Process_Motor_Command(char type, int index, int state)
{
	 if ((gate_lock || empty==0) && state == 1)
	    {
	        lcd_put_cur(2,0);
	        lcd_send_string("ENTRY DENIED       ");
	        HAL_Delay(5000);
	        lcd_clear();



	        return;
	    }

    if (type != 'M') {
        debug_print("[ERROR] Type is not M, ignoring");
        return;
    }

    // =========================================================================
    // M,1,x COMMANDS (Entry/Exit Toggle)
    // =========================================================================
    if (index == 1)
    {

        if (state == 1 && gate_lock==0 && empty!=0)  // M,1,1 - OPEN command
        {
            // Delay for Adafruit to update RFID feed
            HAL_Delay(500);

            // Fetch the RFID
            Fetch_Latest_RFID();

            // Find which card this is
            int card_idx = -1;
            for (int i = 0; i < NUM_RFIDS; i++) {
                if (strcmp(fetched_rfid, valid_rfids[i]) == 0) {
                    card_idx = i;
                    break;
                }
            }

            // ERROR CASE: Unknown card
            if (card_idx == -1) {
                lcd_put_cur(0, 0);
                lcd_send_string("UNKNOWN CARD!   ");
                lcd_put_cur(1, 0);
                lcd_send_string("Access Denied   ");
                return;  // EXIT: Do nothing
            }

            debug_print("[STATE] Card index %d - timestamp = %ld",
                        card_idx, rfid_timestamps[card_idx]);

            if (rfid_timestamps[card_idx] == -1)
            {
                // SAFETY: Prevent both gates from opening
                if (gate1_in_use == 1 || gate2_in_use == 1) {
                    debug_print("[SAFETY] WARNING: A gate is already in use!");
                    return;  // EXIT
                }

                rfid_timestamps[card_idx] = HAL_GetTick();
                gate1_in_use = 1;
                gate2_in_use = 0;

                // OPEN GATE 1
                __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 100);
            }
            else
            {
                // EXIT MODE: Card IS in garage (has timestamp)
                debug_print("[EXIT] Timestamp is %ld (in garage)",
                            rfid_timestamps[card_idx]);

                // SAFETY: Prevent both gates from opening
                if (gate1_in_use == 1 || gate2_in_use == 1) {
                    debug_print("[SAFETY] WARNING: A gate is already in use!");
                    return;  // EXIT
                }

                uint32_t duration = HAL_GetTick() - (uint32_t)rfid_timestamps[card_idx];
                rfid_timestamps[card_idx] = -1; // Clear timestamp

                gate2_in_use = 1;
                gate1_in_use = 0;

                lcd_clear();
                lcd_put_cur(3, 0);
              //lcd_send_string("Time: %lus", duration);
                char msg[16];
                snprintf(msg, sizeof(msg), "Price: %lu LE", duration / 1000);
                lcd_put_cur(3, 0);
                lcd_send_string(msg);

                // OPEN GATE 2
                __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 100);
            }
        }
        else if (state == 0)  // M,1,0 - CLOSE command
        {
            debug_print("[GATE] M,1,0 received - closing active gates");

            // Close Gate 1 if it is open
            if (gate1_in_use == 1)
            {
                lcd_put_cur(0, 0);
                lcd_send_string("Gate 1 Closing...");
                __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, 200);
                gate1_in_use = 0;
            }

            // ADDED CONDITION: Also close Gate 2 if it was opened during exit cycle
            if (gate2_in_use == 1)
            {
                lcd_put_cur(0, 0);
                lcd_send_string("Gate 2 Closing...");
                __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 200);
                gate2_in_use = 0;
            }
        }
    }

    // =========================================================================
    // M,2,x COMMANDS (Force Exit Gate)
    // =========================================================================
    else if (index == 2)
    {

        if (state == 1)  // M,2,1 - OPEN command
        {

            // SAFETY: Prevent both gates from opening simultaneously
            if (gate1_in_use == 1) {
                return;
            }

            // ADDED CONDITION: Force open Gate 2 when explicit M,2,1 instruction arrives
            gate2_in_use = 1;
            gate1_in_use = 0;


            // OPEN GATE 2
            __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 100);
        }
        else if (state == 0)  // M,2,0 - CLOSE command
        {
            if (gate2_in_use == 1)
            {
                __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, 200);
                gate2_in_use = 0;
            }
        }
    }
}



void Process_Received_Data(char* data) {
    char type;
    int index, state;

    // The ESP might send "+IPD,xx:value=B,1,1" or just "value=B,1,1"
    // strstr locates the start of our actual payload
    char *payload_start = strstr(data, "value=");
    if (payload_start != NULL) {
        if (sscanf(payload_start, "value=%c,%d,%d", &type, &index, &state) == 3) {

            switch(type) {
                case 'U': // --- BUZZER CONTROL ---
                    if ((index == 1)&& (state ==1))
                    	{
                    	 HAL_GPIO_WritePin(GPIOA, BUZZ1_Pin, GPIO_PIN_SET);
                    	}
                    else if((index == 1)&& (state ==0))
                	{
                	 HAL_GPIO_WritePin(GPIOA, BUZZ1_Pin, GPIO_PIN_RESET);
                	}
                    else  if ((index == 2)&& (state ==1))
                     	{
                       HAL_GPIO_WritePin(GPIOA, BUZZ2_Pin, GPIO_PIN_SET);
                        }
                   else if((index == 2)&& (state ==0))
                         {
                           HAL_GPIO_WritePin(GPIOA, BUZZ2_Pin, GPIO_PIN_RESET);
                        }
                   else  if ((index == 3)&& (state ==1))
                       {
                          HAL_GPIO_WritePin(GPIOB, BUZZ3_Pin, GPIO_PIN_SET);
                        }
                   else if((index == 3)&& (state ==0))
                     {
                       HAL_GPIO_WritePin(GPIOB, BUZZ3_Pin, GPIO_PIN_RESET);
                     }
                   else if ((index == 4)&& (state ==1))
                    {
                      HAL_GPIO_WritePin(GPIOB, BUZZ4_Pin, GPIO_PIN_SET);
                   	}
                   else if((index == 4)&& (state ==0))
                    {
                      HAL_GPIO_WritePin(GPIOB, BUZZ4_Pin, GPIO_PIN_RESET);
                    }

                    break;


                case 'M':  // MOTOR CONTROL
                                 debug_print("[PROCESS] Routing to motor handler");
                                 Process_Motor_Command(type, index, state);
                                 break;

                case 'G':
                	if ((index ==0)&& (state==1))
                		 HAL_GPIO_WritePin(GPIOB, BUZZ5_Pin, GPIO_PIN_SET);
                	else if ((index ==0)&& (state==0))
                		 HAL_GPIO_WritePin(GPIOB, BUZZ5_Pin, GPIO_PIN_RESET);
                	break;


                case 'I': // --- LED/RGB CONTROL & PARKING LOGIC ---
                	if (index == 1)
                	{
                	    if (state == 1)
                	    {
                	        gate_lock = 1;

                	        lcd_put_cur(2,0);
                	        lcd_send_string("Height not allowed        ");
                	        HAL_Delay(500);
                	    }
                	    else
                	    {
                	        gate_lock = 0;
                	        lcd_put_cur(2,0);
                	        lcd_send_string("Height allowed   ");
                	        HAL_Delay(500);
                	    }

                	    break;
                	}
                                    if (index >= 2 && index <= 5) {
                                        uint8_t slot_idx = index - 1; // Convert 1-4 to array index 0-3

                                        if (state == 1) { // --- Target Spot is OCCUPIED ---
                                            // Only update variables if the spot was previously empty (0)
                                            if (arr[slot_idx+1] == 0) {
                                                arr[slot_idx+1] = 1; // Mark as Occupied
                                                if (empty > 0) empty--;
                                            }

                                            // Update corresponding physical RGB LED to RED
                                            if (index == 2)      rgb_color1(255, 0, 0);
                                            else if (index == 3) {
                                                HAL_GPIO_WritePin(GPIOA, LED2R_Pin, GPIO_PIN_SET);
                                                HAL_GPIO_WritePin(GPIOA, LED2G_Pin, GPIO_PIN_RESET);
                                                HAL_GPIO_WritePin(GPIOA, LED2B_Pin, GPIO_PIN_RESET);
                                            }
                                            else if (index == 4) rgb_color3(255, 0, 0);
                                            else if (index == 5) {
                                                HAL_GPIO_WritePin(GPIOB, LED4R_Pin, GPIO_PIN_SET);
                                                HAL_GPIO_WritePin(GPIOB, LED4G_Pin, GPIO_PIN_RESET);
                                                HAL_GPIO_WritePin(GPIOB, LED4B_Pin, GPIO_PIN_RESET);
                                            }
                                        }
                                        else if (state == 0) { // --- Target Spot is EMPTY ---
                                            // Only update variables if the spot was previously occupied (1)
                                            if (arr[slot_idx+1] == 1) {
                                                arr[slot_idx+1] = 0; // Mark as Empty
                                                if (empty < 4) empty++;
                                            }

                                            // Update corresponding physical RGB LED to GREEN
                                            if (index == 2)      rgb_color1(50, 0, 180);
                                            else if (index == 3) {
                                                HAL_GPIO_WritePin(GPIOA, LED2R_Pin, GPIO_PIN_RESET);
                                                HAL_GPIO_WritePin(GPIOA, LED2G_Pin, GPIO_PIN_SET);
                                                HAL_GPIO_WritePin(GPIOA, LED2B_Pin, GPIO_PIN_RESET);
                                            }
                                            else if (index == 4) rgb_color3(0, 255, 0);
                                            else if (index == 5) {
                                                HAL_GPIO_WritePin(GPIOB, LED4R_Pin, GPIO_PIN_RESET);
                                                HAL_GPIO_WritePin(GPIOB, LED4G_Pin, GPIO_PIN_SET);
                                                HAL_GPIO_WritePin(GPIOB, LED4B_Pin, GPIO_PIN_RESET);
                                            }
                                        }
                                    }
                                    //break;
                    break;
            }
        }
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM5_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

  HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_2);
  HAL_Delay(100);

  // Spot 1 -> GREEN
      rgb_color1(50, 0, 180);

      // Spot 2 -> GREEN
      HAL_GPIO_WritePin(GPIOA, LED2R_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOA, LED2G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOA, LED2B_Pin, GPIO_PIN_RESET);

      // Spot 3 -> GREEN
      rgb_color3(0, 255, 0);

      // Spot 4 -> YELLOW
      HAL_GPIO_WritePin(GPIOB, LED4R_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, LED4G_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, LED4B_Pin, GPIO_PIN_RESET);

  lcd_init();

      for (int i = 0; i < 6; i++) {
          card_state[i] = -1;
      }


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {// 1. Fetch data streamingly from cloud feed
	    Get_Sensor_Update();

	    // 2. Refresh LCD layout showing current spaces available
	    LCD_empty_spots();

	    // 3. Heartbeat visual check
	    HAL_GPIO_TogglePin(led_GPIO_Port, led_Pin);

	    // FIX: Enforce Adafruit IO Rate Limit (Max 30 requests/minute)
	    // A 2.5 second delay prevents the ESP from being temporarily IP-banned.
	    HAL_Delay(2500);
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}
static void MX_I2C1_Init(void)
{
  /* USER CODE BEGIN I2C1_Init 0 */
  /* USER CODE END I2C1_Init 0 */
  /* USER CODE BEGIN I2C1_Init 1 */
  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */
  /* USER CODE END I2C1_Init 2 */
}
static void MX_TIM1_Init(void)
{
  /* USER CODE BEGIN TIM1_Init 0 */
  /* USER CODE END TIM1_Init 0 */
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 84;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 255;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 84;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 255;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);
}
static void MX_TIM4_Init(void)
{
  /* USER CODE BEGIN TIM4_Init 0 */
  /* USER CODE END TIM4_Init 0 */
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 84;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 255;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 839;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 1999;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */
  HAL_TIM_MspPostInit(&htim5);

}

static void MX_USART2_UART_Init(void)
{
  /* USER CODE BEGIN USART2_Init 0 */
  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_MultiProcessor_Init(&huart2, 0, UART_WAKEUPMETHOD_IDLELINE) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_RESET);
  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, BUZZ1_Pin|LED2R_Pin|LED2G_Pin|LED2B_Pin
                          |BUZZ2_Pin, GPIO_PIN_RESET);
  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, BUZZ3_Pin|BUZZ4_Pin|BUZZ5_Pin|LED4R_Pin
                          |LED4G_Pin|LED4B_Pin, GPIO_PIN_RESET);
  /*Configure GPIO pin : led_Pin */
  GPIO_InitStruct.Pin = led_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(led_GPIO_Port, &GPIO_InitStruct);
  /*Configure GPIO pins : BUZZ1_Pin LED2R_Pin LED2G_Pin LED2B_Pin
                           BUZZ2_Pin */
  GPIO_InitStruct.Pin = BUZZ1_Pin|LED2R_Pin|LED2G_Pin|LED2B_Pin
                          |BUZZ2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  /*Configure GPIO pins : BUZZ3_Pin BUZZ4_Pin BUZZ5_Pin LED4R_Pin
                           LED4G_Pin LED4B_Pin */
  GPIO_InitStruct.Pin = BUZZ3_Pin|BUZZ4_Pin|BUZZ5_Pin|LED4R_Pin
                          |LED4G_Pin|LED4B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}
/* USER CODE BEGIN 4 */
void rgb_color1(uint8_t red, uint8_t green, uint8_t blue)
{
	htim1.Instance->CCR1 = red;
	htim1.Instance->CCR2 = green;
	htim1.Instance->CCR3 = blue;
}

/*void rgb_color2(uint8_t red, uint8_t green, uint8_t blue)
{
    // Inverted logic: 0 turns it ON, 1 turns it OFF
    HAL_GPIO_WritePin(GPIOA, LED2R_Pin, red ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, LED2G_Pin, green ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, LED2B_Pin, blue ? GPIO_PIN_RESET : GPIO_PIN_SET);
}*/
void rgb_color3(uint8_t red, uint8_t green, uint8_t blue)
{
	htim3.Instance->CCR1 = red;
	htim3.Instance->CCR2 = green;
	htim3.Instance->CCR3 = blue;
}

/* USER CODE BEGIN 4 */
/*void rgb_color4(uint8_t red, uint8_t green, uint8_t blue)
{
    // Inverted logic: 0 turns it ON, 1 turns it OFF
    HAL_GPIO_WritePin(GPIOB, LED4R_Pin, red ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, LED4G_Pin, green ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, LED4B_Pin, blue ? GPIO_PIN_RESET : GPIO_PIN_SET);
}*/

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  { }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif
