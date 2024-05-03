/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue for M032 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"
#include "disp.h"
#include "touch_adc.h"


#define CONFIG_VRAM_TOTAL_ALLOCATED_SIZE    NVT_ALIGN((LV_HOR_RES_MAX * CONFIG_DISP_LINE_BUFFER_NUMBER * (LV_COLOR_DEPTH/8)), 4)

static uint8_t s_au8FrameBuf[CONFIG_VRAM_TOTAL_ALLOCATED_SIZE] __attribute__((aligned(4)));

/* User can define ADC touch calibration matrix. */
#if defined(__320x240__)
S_CALIBRATION_MATRIX g_sCalMat = { 25, 6478, -3868636, 5310, -18, -3168446, 65536 };
#endif

void sysDelay(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

int lcd_device_initialize(void)
{
    GPIO_T *PORT;

    /* Set GPIO Output mode for ILI9341 pins. */
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_DISP_PIN_DC) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_DISP_PIN_DC)), GPIO_MODE_OUTPUT);

    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_DISP_PIN_RESET) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_DISP_PIN_RESET)), GPIO_MODE_OUTPUT);

    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_DISP_PIN_BACKLIGHT) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_DISP_PIN_BACKLIGHT)), GPIO_MODE_OUTPUT);

    /* Open SPI */
    USPI_Open(CONFIG_DISP_SPI, USPI_MASTER, USPI_MODE_0, 16, CONFIG_DISP_SPI_CLOCK);

    /* Set sequence to MSB first */
    USPI_SET_MSB_FIRST(CONFIG_DISP_SPI);

#if defined(CONFIG_DISP_SPI_SS_PIN)
    PORT    = (GPIO_T *)(GPIOA_BASE + (NU_GET_PORT(CONFIG_DISP_SPI_SS_PIN) * PORT_OFFSET));
    GPIO_SetMode(PORT, NU_GET_PIN_MASK(NU_GET_PIN(CONFIG_DISP_SPI_SS_PIN)), GPIO_MODE_OUTPUT);
    GPIO_PIN_DATA(NU_GET_PORT(CONFIG_DISP_SPI_SS_PIN), NU_GET_PIN(CONFIG_DISP_SPI_SS_PIN)) = 1;
#else
    /* Set CS pin to HIGH */
    USPI_SET_SS_HIGH(CONFIG_DISP_SPI);
#endif

    /* Set sequence to MSB first */
    SPI_SET_MSB_FIRST(CONFIG_DISP_SPI);

    disp_init();

    return 0;
}

int lcd_device_open(void)
{
    return 0;
}

int lcd_device_control(int cmd, void *argv)
{
    switch (cmd)
    {
    case evLCD_CTRL_GET_INFO:
    {
        S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)argv;

        LV_ASSERT(argv != NULL);

        psLCDInfo->pvVramStartAddr = (void *)s_au8FrameBuf;
        psLCDInfo->u32VramSize = CONFIG_VRAM_TOTAL_ALLOCATED_SIZE;
        psLCDInfo->u32ResWidth = LV_HOR_RES_MAX;
        psLCDInfo->u32ResHeight = LV_VER_RES_MAX;
        psLCDInfo->u32BytePerPixel = (LV_COLOR_DEPTH / 8);
        psLCDInfo->evLCDType = evLCD_TYPE_MPU;
    }
    break;

    case evLCD_CTRL_RECT_UPDATE:
    {
        disp_fillrect((uint16_t *)s_au8FrameBuf, (const lv_area_t *)argv);
    }
    break;

    default:
        LV_ASSERT(0);
    }

    return 0;
}

void lcd_device_close(void)
{
}

int lcd_device_finalize(void)
{
    return 0;
}

uint32_t nu_adc_sampling(uint32_t channel)
{
    ADC_Open(CONFIG_AD, ADC_ADCR_DIFFEN_SINGLE_END, ADC_ADCR_ADMD_SINGLE, (0x1 << channel));

    ADC_SET_INPUT_CHANNEL(CONFIG_AD, (0x1 << channel));

    ADC_CLR_INT_FLAG(CONFIG_AD, ADC_ADF_INT);

    ADC_ENABLE_INT(CONFIG_AD, ADC_ADF_INT);

    ADC_START_CONV(CONFIG_AD);

    while (ADC_GET_INT_FLAG(CONFIG_AD, ADC_ADF_INT) == 0);

    return ADC_GET_CONVERSION_DATA(CONFIG_AD, channel) & 0x0FFF;
}

int touchpad_device_initialize(void)
{
    /* Enable ADC converter */
    ADC_POWER_ON(ADC);

    return 0;
}

int touchpad_device_open(void)
{

    extern int ad_touch_calibrate(void);
    //ad_touch_calibrate();

    return 0;
}

#define CONFIG_TRIGGER_PERIOD     LV_DISP_DEF_REFR_PERIOD
int touchpad_device_read(lv_indev_data_t *psInDevData)
{
    static lv_indev_data_t sLastInDevData = {0};
    static uint32_t u32NextTriggerTime = 0;

    uint32_t adc_x, adc_y;

    LV_ASSERT(psInDevData);

    psInDevData->state   = sLastInDevData.state;
    psInDevData->point.x = sLastInDevData.point.x;
    psInDevData->point.y = sLastInDevData.point.y;

    if (xTaskGetTickCount() < u32NextTriggerTime)
    {
        goto exit_touchpad_device_read;
    }

    /* Get X, Y ADC converting data */
    adc_x  = indev_touch_get_x();
    adc_y  = indev_touch_get_y();
    u32NextTriggerTime = xTaskGetTickCount() + CONFIG_TRIGGER_PERIOD;

    if ((adc_x < 4000) && (adc_y < 4000))
    {
        psInDevData->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        psInDevData->state = LV_INDEV_STATE_RELEASED;
    }

    if (psInDevData->state == LV_INDEV_STATE_PRESSED)
    {
        extern int ad_touch_map(int32_t *sumx, int32_t *sumy);
        if (ad_touch_map((int32_t *)&adc_x, (int32_t *)&adc_y) == 0)
        {

            psInDevData->point.x = ((int16_t)adc_x < 0) ? 0 :
                                   ((int16_t)adc_x >= LV_HOR_RES_MAX) ? (LV_HOR_RES_MAX - 1) :
                                   adc_x;

            psInDevData->point.y = ((int16_t)adc_y < 0) ? 0 :
                                   ((int16_t)adc_y >= LV_VER_RES_MAX) ? (LV_VER_RES_MAX - 1) :
                                   adc_y;
        }
        else
        {
            psInDevData->point.x = (int16_t)adc_x;

            psInDevData->point.y = (int16_t)adc_y;
        }

        sLastInDevData.point.x  = psInDevData->point.x;
        sLastInDevData.point.y  = psInDevData->point.y;
    }

    sLastInDevData.state = psInDevData->state;

    LV_LOG_INFO("%s (%d, %d)", psInDevData->state ? "Press" : "Release", psInDevData->point.x, psInDevData->point.y);

exit_touchpad_device_read:

    return (psInDevData->state == LV_INDEV_STATE_PRESSED) ? 1 : 0;
}

int touchpad_device_control(int cmd, void *argv)
{
    return 0;
}

void touchpad_device_close(void)
{
    /* Disable ADC converter */
    ADC_POWER_DOWN(ADC);
}

int touchpad_device_finalize(void)
{
    return 0;
}
