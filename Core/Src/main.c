/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "rng.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "SEGGER_RTT.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/** 
  * @brief Structure to be stored in OTP (One-Time-Programmable) slot
  */
#ifdef __GNUC__
typedef struct __attribute__((packed))
#else
typedef __packed struct
#endif
{
  uint8_t additional_data[6]; /*!< 48 bits of data to fill OTP slot (e.g: BD or MAC address, key..) */
  uint8_t hse_tuning;         /*!< Load capacitance to be applied on HSE pad */
  uint8_t index;              /*!< Structure index */
} OTP_DATA_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define VERSION "v1.0.0"
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define OTP_HSE_STR_IDX ((uint8_t)0x00) /*!< Index of the structure OTP_DATA_t  */

#define ADDITIONAL_DATA_PTR ((uint32_t)0x2002FFF0) /*!< SRAM Address to store Bluetooth address transmitted by ST-Link CLI script */

#define STORE_ADDRESS ((uint32_t)OTP_AREA_BASE) /*!< Store address for OTP_Data structure choosen between
                                                  -true OTP address (one-time-programming): #define STORE_ADDRESS OTP_AREA_BASE
                                                  -or typical Flash Address (can be removed): #define STORE_ADDRESS 0x080xxxxxx */

#define OTP_AREA_SIZE ((uint32_t)1024) /*!< OTP area size in bytes */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint32_t hsetune_val = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
/* USER CODE BEGIN PFP */
static void SetHSECalibration(uint32_t val);
static void SaveHSETune(uint32_t val);
static HAL_StatusTypeDef FetchHSETune(void);

static void StartHSE(void);
void Error_Handler(void);

void RTT_Read(void);
static void SetBLEPublicAddress(uint8_t* p_addr);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  /* Set HSE On */
  StartHSE();
  /* Output HSE clock on PA8 pin configured in MCO mode (for verification purpose) */
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

/* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RNG_Init();
  /* USER CODE BEGIN 2 */
  // Configure RTT Up Buffer & Send Start Message
  SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
  SEGGER_RTT_printf(0, "STM32WB15 HSE CALIB RTT %s\r\n", VERSION);
  
  /* HSE calibration is retrieved from OTP area */
  if (FetchHSETune() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    RTT_Read();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4|RCC_CLOCKTYPE_HCLK2
                              |RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS;
  PeriphClkInitStruct.SmpsClockSelection = RCC_SMPSCLKSOURCE_HSI;
  PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLKDIV_RANGE1;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN Smps */

  /* USER CODE END Smps */
}

/* USER CODE BEGIN 4 */
/**
  * @brief  J-Link RTT Read: Reads one character at a time
  * @param  None
  * @retval None
  */
void RTT_Read(void)
{
  char input;
  uint32_t size = SEGGER_RTT_Read(0, &input, 1u);
  
  if (size > 0)
  {
    switch (input)
    {
      case '1':
        if (hsetune_val < 0x3F) 
        {
          hsetune_val++;
        }
        SetHSECalibration(hsetune_val);
        break;
        
      case '2':
        SaveHSETune(hsetune_val);
        break;
        
      case '3':
        if (hsetune_val > 0)
        {
          hsetune_val--;
        }
        SetHSECalibration(hsetune_val);
        break;
        
      default:
        break;
    }
  }
}

/**
  * @brief  EXTI line detection callbacks
  * @param  GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  
}

/**
  * @brief  Sets the BLE Public Address
  * @param  Address Pointer
  * @retval None
  */
static void SetBLEPublicAddress(uint8_t* p_addr)
{
  uint32_t rgn;
  uint32_t company_id;
  uint32_t device_id;
  
  // Generate a Random Number
  HAL_RNG_GenerateRandomNumber(&hrng, &rgn);
  
  if (rgn != 0xFFFFFFFF)
  {
    company_id = LL_FLASH_GetSTCompanyID();
    device_id = LL_FLASH_GetDeviceID();
    
    /**
     * Public Address with the ST company ID
     * bit[47:24] : 24bits (OUI) equal to the company ID
     * bit[23:16] : Device ID.
     * bit[15:0] : The last 16bits from the random generated number
     * Note: In order to use the Public Address in a final product, a dedicated
     * 24bits company ID (OUI) shall be bought.
     */
    p_addr[0] = (uint8_t)(rgn & 0x000000FF);
    p_addr[1] = (uint8_t)((rgn & 0x0000FF00) >> 8);
    p_addr[2] = (uint8_t)device_id;
    p_addr[3] = (uint8_t)(company_id & 0x000000FF);
    p_addr[4] = (uint8_t)((company_id & 0x0000FF00) >> 8);
    p_addr[5] = (uint8_t)((company_id & 0x00FF0000) >> 16);
  }
  else
  {
    SetBLEPublicAddress(p_addr);
  }
}

/**
  * @brief  Stop HSE clock
  * @param  None.
  * @retval None.
  */
static void StopHSE(void)
{
  __HAL_RCC_HSE_CONFIG(RCC_HSE_OFF);
  while (LL_RCC_HSE_IsReady())
  {};
}

/**
  * @brief  Start HSE clock
  * @param  None.
  * @retval None.
  */
static void StartHSE(void)
{
  __HAL_RCC_HSE_CONFIG(RCC_HSE_ON);
  while (LL_RCC_HSE_IsReady())
  {};
}

/**
  * @brief  Set load capacitance value in HSE config register.
  * @param  HSE capacitor tuning between 0 and 63
  * @retval None.
  */
static void SetHSECalibration(uint32_t val)
{
  StopHSE();
  __HAL_RCC_HSE_CAPACITORTUNING(val);
  StartHSE();
  
  SEGGER_RTT_printf(0, "HSE Tune Value: %d (Min: 0 | Max: 63)\r\n", val);
}


/**
* @brief  Parse OTP area and return first free slot index.
* @param  None.
* @retval OTP slot address offset
*/
static uint32_t GetOTPFreeIdx(void)
{
  uint32_t idx = 0;
  for (idx = 0; idx < OTP_AREA_SIZE; idx += sizeof(OTP_DATA_t))
  {
    if (*(uint64_t*)(STORE_ADDRESS + idx) == 0xFFFFFFFFFFFFFFFF)
    {
      break;
    }
  }
  if (idx >= OTP_AREA_SIZE)
  {
    SEGGER_RTT_printf(0, "ERROR: No more OTP space; procedure aborted");
    Error_Handler(); /* No more OTP space; procedure aborted */
  }
  return (idx);
}

/**
  * @brief  Store OTP structure (Bluetooth device address + HSE load capacitance)
  * @param  HSE capacitor tuning between 0 and 63
  *         Additional data address is retreived from SRAM at ADDITIONAL_DATA_PTR @
  * @retval HAL Error status.
  */
static void SaveHSETune(uint32_t val)
{
  HAL_StatusTypeDef err = HAL_ERROR;
  uint32_t idx;
  OTP_DATA_t otp_data;

  /* Fill OTP_BT_t structure */
  SetBLEPublicAddress(otp_data.additional_data);
  otp_data.hse_tuning = val & 0x3F;
  otp_data.index = OTP_HSE_STR_IDX;

  /* Fetch first OTP free slot index */
  idx = GetOTPFreeIdx();
  
  /* Store OTP structure in OTP area */
   __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS); /* Clear all Flash flags before write operation*/
    
  err = HAL_FLASH_Unlock();
  err |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, STORE_ADDRESS + idx, *(uint64_t*) (&otp_data));

  err |= HAL_FLASH_Lock();
  if (err != HAL_OK) {
    Error_Handler();
  }
  
  SEGGER_RTT_printf(0, "Saved HSE Tune Value: %d | Stored in OTP Address: 0x%X\r\n", val, STORE_ADDRESS + idx);
}

/**
  * @brief  Parse OTP area and return last address of the structure with given index.
  * @param  Structure index. Used when different structures are stored in OTP area.
  * @retval OTP slot address offset
  */
static int32_t CheckOTPIndex(uint8_t fetch_idx)
{
  int32_t idx = OTP_AREA_SIZE;
  OTP_DATA_t * potp_data;

  /* parse OTP area until finding the latest available structure with given index */
  /* If no slot is found, returned value is <0 (ie -sizeof(OTP_DATA_t)) */
  do
  {
    idx -= sizeof(OTP_DATA_t);
    potp_data = (OTP_DATA_t*)(STORE_ADDRESS + idx); /* fetch otp structure at given idx */
  }
  while ((potp_data->index != fetch_idx) && (idx >= 0));

  return (idx);
}

/**
  * @brief  Fetch load capacitance value from OTP structure. Set HSE config register with the capacitance value.
  * @param  None.
  * @retval HAL Error status.
  */
static HAL_StatusTypeDef FetchHSETune(void)
{
  int32_t idx;
  OTP_DATA_t *potp_data;

  /* Get last calibration index in OTP area
  * if idx<0 it means that no structure with idx=OTP_HSE_STR_IDX has been found in OTP;
  * => so calibration is not done yet
  */
  idx = CheckOTPIndex(OTP_HSE_STR_IDX);
  if (idx >= 0)
  {
    potp_data = (OTP_DATA_t *) (STORE_ADDRESS + idx);
    SEGGER_RTT_printf(0, "Current HSE Tuned Value: %d\r\n", potp_data->hse_tuning);
    return (HAL_OK);
  }
  else 
  {
    SEGGER_RTT_printf(0, "No Current HSE Tuned Value\r\n");
    return (HAL_ERROR);
  }
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

#ifdef  USE_FULL_ASSERT
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
