#include "main.h"
#include <stdio.h>

static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);

void Sleep(uint32_t nms)
{
    uint32_t tstart = HAL_GetTick();
    while ((HAL_GetTick() - tstart) < nms);
}

static volatile int audioReady = 0;
static uint16_t audioBuf[65536];
void BSP_AUDIO_IN_TransferComplete_CallBack(void)
{
    audioReady = 1;
    //BSP_AUDIO_IN_Pause();
}


int main(void)
{
    RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

    /* Enable the CPU Cache */
    CPU_CACHE_Enable();

    /* STM32F7xx HAL library initialization:
         - Configure the Flash ART accelerator on ITCM interface
         - Systick timer is configured by default as source of time base, but user
           can eventually implement his proper time base source (a general purpose
           timer for example or other time source), keeping in mind that Time base
           duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
           handled in milliseconds basis.
         - Set NVIC Group Priority to 4
         - Low Level Initialization
       */
    HAL_Init();

    /* Configure the system clock to 216 MHz */
    SystemClock_Config();

    /* LCD clock configuration */
    /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
    /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 Mhz */
    /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/5 = 38.4 Mhz */
    /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_4 = 38.4/4 = 9.6Mhz */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 5;
    PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_4;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    /* Configure LED1 */
    BSP_LED_Init(LED1);

    /* Configure LCD : Only one layer is used */
    BSP_LCD_Init();

    //Touchscreen
    BSP_TS_Init(FT5336_MAX_WIDTH, FT5336_MAX_HEIGHT);

    /* LCD Initialization */
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS+(BSP_LCD_GetXSize()*BSP_LCD_GetYSize()*4));

    /* Enable the LCD */
    BSP_LCD_DisplayOn();

    /* Select the LCD Background Layer  */
    BSP_LCD_SelectLayer(0);

    /* Clear the Background Layer */
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    /* Select the LCD Foreground Layer  */
    BSP_LCD_SelectLayer(1);

    /* Clear the Foreground Layer */
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    /* Configure the transparency for foreground and background :
     Increase the transparency */
    BSP_LCD_SetTransparency(0, 255);
    BSP_LCD_SetTransparency(1, 240);

    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_DisplayStringAt(0, 0, (uint8_t*)"STM32F746G Discovery", CENTER_MODE);
    BSP_LCD_SelectLayer(0);
    BSP_LCD_DisplayStringAt(0, 40, (uint8_t*)"sample project", CENTER_MODE);

    BSP_LCD_SelectLayer(1);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_SetFont(&Font16);

    uint8_t ret;
    ret = BSP_AUDIO_IN_Init(INPUT_DEVICE_DIGITAL_MICROPHONE_2, 100, I2S_AUDIOFREQ_44K);
    if (ret != AUDIO_OK)
    {
        BSP_LCD_SetTextColor(LCD_COLOR_RED);
        BSP_LCD_DisplayStringAtLine(2, "BSP_AUDIO_IN_Init failed");
    }

    uint32_t ctr = 0;
    char buf[70];
    TS_StateTypeDef ts;
    while (1)
    {
        BSP_TS_GetState(&ts);
        if (ts.touchDetected)
        {
            sprintf(buf, "x %d, y %d, wt %d", ts.touchX[0], ts.touchY[0], ts.touchWeight[0]);
            BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGREEN);
            BSP_LCD_ClearStringLine(3);
            BSP_LCD_DisplayStringAtLine(3, buf);
        }
        else
        {
            BSP_LCD_ClearStringLine(3);
        }

        sprintf(buf, "%.3f seconds", (float)HAL_GetTick() / 1000.);
        BSP_LCD_DisplayStringAt(0, 70, (uint8_t*)buf, CENTER_MODE);

        if (audioReady == 0)
        {
            ret = BSP_AUDIO_IN_Record(audioBuf, 2048);
            if (ret != AUDIO_OK)
            {
                BSP_LCD_SetTextColor(LCD_COLOR_RED);
                BSP_LCD_DisplayStringAtLine(2, "BSP_AUDIO_IN_Record failed");
            }
            audioReady = 2;
        }
        else if (audioReady == 1)
        {
            BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);
            BSP_LCD_FillRect(0, 140, 480, 140);
            int i;
            for (i = 0; i < 2048; i+=4)
            {
                if (i/4 >= 480)
                    break;
                BSP_LCD_DrawPixel(i/4, 210 - ((int16_t)audioBuf[i])/ 500, LCD_COLOR_LIGHTBLUE);
            }
            audioReady = 0;
        }
    }
    return 0;
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 216000000
  *            HCLK(Hz)                       = 216000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 432
  *            PLL_P                          = 2
  *            PLL_Q                          = 9
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;
    HAL_StatusTypeDef ret = HAL_OK;

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 432;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 9;

    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    if(ret != HAL_OK)
    {
        while(1)
        {
            ;
        }
    }

    /* Activate the OverDrive to reach the 216 MHz Frequency */
    ret = HAL_PWREx_EnableOverDrive();
    if(ret != HAL_OK)
    {
        while(1)
        {
            ;
        }
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
    if(ret != HAL_OK)
    {
        while(1)
        {
            ;
        }
    }
}


//CPU L1-Cache enable.
static void CPU_CACHE_Enable(void)
{
    /* Enable I-Cache */
    SCB_EnableICache();

    /* Enable D-Cache */
    SCB_EnableDCache();
}

#ifdef  USE_FULL_ASSERT

void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif
