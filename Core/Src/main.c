/* USER CODE BEGIN Header */
/**
 *******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 *******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 *******************************************************************************
 * @author : Piotr Pokornowski
 * @version : 0.1
 * @date : 2024-09-13
 * @brief : Main program body
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <string.h>
#include "display.h"
#include "bmp280.h"
#include "printer.h"
#include "LCD_HD44780.h"
#include "delays.h"
#include "encoder.h"

#include "socket.h"
#include "httpServer.h"
#include "dhcp.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef void (*EXTI_Callback_t)(void);

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define DEBUG_PRINT

#define MAX_EXTI_PINS 16
#define TASK_INTERVAL 60000 // [ms]

#define DHCP_SOCKET 0
#define DNS_SOCKET 1
#define HTTP_SOCKET 2
#define SOCK_TCPS 0
#define SOCK_UDPS 1
#define PORT_TCPS 5000
#define PORT_UDPS 3000
#define MAX_HTTPSOCK 6

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

BMP280_HandleTypedef bmp280;
PRINTER_HandleTypedef printer;

EXTI_Callback_t EXTI_Callbacks[MAX_EXTI_PINS] = {NULL};

void printData(void);

struct weatherData
{
  float temperature;
  float pressure;
  float humidity;
  // float dewPoint;
};

uint8_t value;

char buffer[64]; // UART
volatile uint32_t lastTaskTime = 0;

extern menu_t menu1, menu2, menu3, menu4, menu5;
extern menu_t submenu1_1, submenu1_2, submenu1_3, submenu1_4;
extern menu_t submenu3_1;

//{*name; *next; *prev; *child; *parent; (*menuFunction)(void)}
menu_t menu1 = {"Sensor data", &menu2, NULL, &submenu1_1, NULL, NULL};
menu_t submenu1_1 = {"Temperature", &submenu1_2, NULL, NULL, &menu1, NULL};
menu_t submenu1_2 = {"Pressure", &submenu1_3, &submenu1_1, NULL, &menu1, NULL};
menu_t submenu1_3 = {"Humidity", &submenu1_4, &submenu1_2, NULL, &menu1, NULL};
menu_t submenu1_4 = {"Dewpoint", NULL, &submenu1_3, NULL, &menu1, NULL};
menu_t menu2 = {"System settings", &menu3, &menu1, NULL, NULL, NULL};
menu_t menu3 = {"Actions", &menu4, &menu2, &submenu3_1, NULL, NULL};
menu_t submenu3_1 = {"Print", NULL, NULL, NULL, &menu3, printData};
menu_t menu4 = {"About", NULL, &menu3, NULL, NULL, NULL};

menu_t *currentMenu = &menu1;
uint8_t markerRow = 1;
volatile int16_t lastEncoderValue = 0;

uint8_t socknumlist[] = {2, 3, 4, 5, 6, 7};
uint8_t RX_BUF[1024];
uint8_t TX_BUF[1024];
wiz_NetInfo net_info = {
    .mac = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED},
    .dhcp = NETINFO_DHCP};
volatile bool ip_assigned = false;
uint8_t dhcp_buffer[1024];
uint8_t dns_buffer[1024];
char dynamic_page[2048];

uint8_t newData;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Init_EXTI_Callbacks(void);

void wizchipSelect(void);
void wizchipUnselect(void);
void wizchipReadBurst(uint8_t *buff, uint16_t len);
void wizchipWriteBurst(uint8_t *buff, uint16_t len);
uint8_t wizchipReadByte(void);
void wizchipWriteByte(uint8_t byte);
void Callback_IPAssigned(void);
void Callback_IPConflict(void);
void W5500Init();
void generate_index_page(char *buffer, size_t bufferSize, struct weatherData *weather);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

struct weatherData weather;

typedef void (*EXTI_Callback_t)(void);

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
  Init_EXTI_Callbacks();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_USART6_UART_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start(&htim3);
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  __HAL_TIM_SET_COUNTER(&htim2, 0);

  HAL_SPI_Init(&hspi1);

  bmp280_init_default_params(&bmp280.params);
  bmp280.addr = BMP280_I2C_ADDRESS_0;
  bmp280.i2c = &hi2c1;

  while (!bmp280_init(&bmp280, &bmp280.params))
  {
    sprintf((char *)buffer, "BMP280 initialization failed\n");
    HAL_UART_Transmit(&huart2, (uint8_t *)buffer, strlen(buffer), 1000);
    HAL_Delay(2000);
  }

  printer.uart = &huart6;
  PRINTER_Init(&printer);

  LCD_Init();
  LCD_Cls();

  HAL_Delay(1000);
  bmp280_read_float(&bmp280, &weather.temperature, &weather.pressure, &weather.humidity);
  newData = 1;
  updateMenuTitles(&menu1, &weather);
  updateDisplay();

  W5500Init();
  httpServer_init(TX_BUF, RX_BUF, MAX_HTTPSOCK, socknumlist);
  reg_httpServer_cbfunc(NVIC_SystemReset, NULL);

  HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (newData)
    {
      updateMenuTitles(&menu1, &weather);
      generate_index_page(dynamic_page, sizeof(dynamic_page), &weather);
      reg_httpServer_webContent((uint8_t *)"index.html", (uint8_t *)dynamic_page);
      updateDisplay();
      newData = 0;
    }

    static menu_t *lastMenu = NULL;

    int16_t encoderValue = __HAL_TIM_GET_COUNTER(&htim2);
    if (encoderValue != lastEncoderValue)
    {
      if (encoderValue > lastEncoderValue)
      {
        if (currentMenu->next != NULL)
        {
          Encoder_Clockwise();
        }
      }
      else if (encoderValue < lastEncoderValue)
      {
        if (currentMenu->prev != NULL)
        {
          Encoder_CounterClockwise();
        }
      }

      lastEncoderValue = encoderValue;
    }

    if (currentMenu != lastMenu)
    {
      updateDisplay();
      lastMenu = currentMenu;
    }

    for (int i = 0; i < MAX_HTTPSOCK; i++)
      httpServer_run(i);
  }
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_SYSTICK_Callback(void)
{
  uint32_t currentTime = HAL_GetTick();
  if (currentTime - lastTaskTime >= TASK_INTERVAL)
  {
    bmp280_read_float(&bmp280, &weather.temperature, &weather.pressure, &weather.humidity);
    newData = 1;
    lastTaskTime = currentTime;

#ifdef DEBUG_PRINT
    sprintf(buffer, "Temperature: %.2f C, Pressure: %.2f hPa, Humidity: %.2f\n", weather.temperature, weather.pressure, weather.humidity);
    HAL_UART_Transmit(&huart2, (uint8_t *)buffer, strlen(buffer), 100);
#endif
  }
}

void EXTI0_Callback(void)
{
  sprintf(buffer, "Temperature: %.2f C, Pressure: %.2f hPa, Humidity: %.2f\n", weather.temperature, weather.pressure, weather.humidity);
  PRINTER_Print(&printer, buffer);
}

void EXTI8_Callback(void)
{
  selectMenu();

#ifdef DEBUG_PRINT
  sprintf(buffer, "Selected menu: %s\n", currentMenu->name);
  HAL_UART_Transmit(&huart2, (uint8_t *)buffer, strlen(buffer), 100);
#endif
}

void EXTI9_Callback(void)
{
  navigateBack();

#ifdef DEBUG_PRINT
  sprintf(buffer, "Navigated back to: %s\n", currentMenu->name);
  HAL_UART_Transmit(&huart2, (uint8_t *)buffer, strlen(buffer), 100);
#endif
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  uint8_t pin_index = __builtin_ctz(GPIO_Pin);
  if (pin_index < MAX_EXTI_PINS && EXTI_Callbacks[pin_index] != NULL)
  {
    EXTI_Callbacks[pin_index]();
  }

#ifdef DEBUG_PRINT
  sprintf(buffer, "Pin %d Interrupt\n", pin_index);
  HAL_UART_Transmit(&huart2, (uint8_t *)buffer, strlen(buffer), 100);
#endif
}

void Init_EXTI_Callbacks(void)
{
  EXTI_Callbacks[0] = EXTI0_Callback;
  EXTI_Callbacks[1] = NULL;
  EXTI_Callbacks[2] = NULL;
  EXTI_Callbacks[3] = NULL;
  EXTI_Callbacks[4] = NULL;
  EXTI_Callbacks[5] = NULL;
  EXTI_Callbacks[6] = NULL;
  EXTI_Callbacks[7] = NULL;
  EXTI_Callbacks[8] = EXTI8_Callback;
  EXTI_Callbacks[9] = EXTI9_Callback;
  EXTI_Callbacks[10] = NULL;
  EXTI_Callbacks[11] = NULL;
  EXTI_Callbacks[12] = NULL;
  EXTI_Callbacks[13] = NULL;
  EXTI_Callbacks[14] = NULL;
  EXTI_Callbacks[15] = NULL;
}

void printData(void)
{
  char buffer[64];
  sprintf(buffer, "Temperature: %.2f C\nPressure: %.2f hPa\nHumidity: %.2f %%\n", weather.temperature, weather.pressure, weather.humidity);

  PRINTER_Print(&printer, buffer);
}

void updateMenuTitles(menu_t *menu, struct weatherData *weather)
{
  static char titleBuffer[4][32];

  snprintf(titleBuffer[0], sizeof(titleBuffer[0]), "Temperature: %.2f C", weather->temperature);
  menu->child->name = titleBuffer[0];

  snprintf(titleBuffer[1], sizeof(titleBuffer[1]), "Pressure: %.2f hPa", weather->pressure);
  menu->child->next->name = titleBuffer[1];

  snprintf(titleBuffer[2], sizeof(titleBuffer[2]), "Humidity: %.2f %%", weather->humidity);
  menu->child->next->next->name = titleBuffer[2];

  snprintf(titleBuffer[3], sizeof(titleBuffer[3]), "Dewpoint: N/A");
  menu->child->next->next->next->name = titleBuffer[3];
}

void wizchipSelect(void)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}

void wizchipUnselect(void)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

void wizchipReadBurst(uint8_t *buff, uint16_t len)
{
  HAL_SPI_Receive(&hspi1, buff, len, HAL_MAX_DELAY);
}

void wizchipWriteBurst(uint8_t *buff, uint16_t len)
{
  HAL_SPI_Transmit(&hspi1, buff, len, HAL_MAX_DELAY);
}

uint8_t wizchipReadByte(void)
{
  uint8_t byte;
  wizchipReadBurst(&byte, sizeof(byte));
  return byte;
}

void wizchipWriteByte(uint8_t byte)
{
  wizchipWriteBurst(&byte, sizeof(byte));
}

void Callback_IPAssigned(void)
{
  ip_assigned = true;
}

void Callback_IPConflict(void)
{
  ip_assigned = false;
}

void W5500Init()
{
  reg_wizchip_cs_cbfunc(wizchipSelect, wizchipUnselect);
  reg_wizchip_spi_cbfunc(wizchipReadByte, wizchipWriteByte);
  reg_wizchip_spiburst_cbfunc(wizchipReadBurst, wizchipWriteBurst);

  uint8_t rx_tx_buff_sizes[] = {2, 2, 2, 2, 2, 2, 2, 2};
  wizchip_init(rx_tx_buff_sizes, rx_tx_buff_sizes);

  setSHAR(net_info.mac);
  DHCP_init(DHCP_SOCKET, dhcp_buffer);

  reg_dhcp_cbfunc(
      Callback_IPAssigned,
      Callback_IPAssigned,
      Callback_IPConflict);

  uint32_t ctr = 10000;
  while ((!ip_assigned) && (ctr > 0))
  {
    DHCP_run();
    ctr--;
  }
  if (!ip_assigned)
  {
    return;
  }

  getIPfromDHCP(net_info.ip);
  getGWfromDHCP(net_info.gw);
  getSNfromDHCP(net_info.sn);

  wizchip_setnetinfo(&net_info);
}

void generate_index_page(char *buffer, size_t bufferSize, struct weatherData *weather)
{
  snprintf(buffer, bufferSize,
           "<!DOCTYPE html>"
           "<html lang='en'>"
           "<head>"
           "<meta charset='UTF-8'>"
           "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
           "<meta http-equiv='refresh' content='10'>"
           "<link href=\"data:image/x-icon;base64,A\" rel=\"icon\" type=\"image/x-icon\">"
           "<title>STM32 Stacja Pogodowa</title>"
           "<style>"
           "body {"
           "  font-family: 'Roboto', sans-serif;"
           "  background-color: #f4f4f9;"
           "  color: #333;"
           "  margin: 0;"
           "  padding: 0;"
           "  display: flex;"
           "  flex-direction: column;"
           "  align-items: center;"
           "  justify-content: center;"
           "  min-height: 100vh;"
           "}"
           "h1 {"
           "  font-size: 1.8em;"
           "  color: #4CAF50;"
           "  margin-bottom: 20px;"
           "}"
           ".card {"
           "  background: #fff;"
           "  border-radius: 8px;"
           "  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);"
           "  padding: 20px;"
           "  text-align: center;"
           "  width: 90%%;"
           "  max-width: 400px;"
           "}"
           ".data {"
           "  font-size: 1.2em;"
           "  margin: 10px 0;"
           "  color: #555;"
           "}"
           ".footer {"
           "  margin-top: 20px;"
           "  font-size: 0.9em;"
           "  color: #aaa;"
           "}"
           "</style>"
           "<link href='https://fonts.googleapis.com/css2?family=Roboto:wght@400;700&display=swap' rel='stylesheet'>"
           "</head>"
           "<body>"
           "<h1>STM32 Stacja Pogodowa</h1>"
           "<div class='card'>"
           "<p class='data'>Temperatura: %.2f °C</p>"
           "<p class='data'>Ciśnienie: %.2f hPa</p>"
           "<p class='data'>Wilgotność: %.2f %%</p>"
           "</div>"
           "<div class='footer'>Projekt na zaliczenie WSYW i PMIK.</div>"
           "<div class='footer'>Piotr Pokornowski 325061</div>"
           "</body>"
           "</html>",
           weather->temperature,
           weather->pressure,
           weather->humidity);
}

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
  {
  }
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
#endif /* USE_FULL_ASSERT */
