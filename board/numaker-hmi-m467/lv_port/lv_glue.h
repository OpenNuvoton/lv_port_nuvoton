/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue header
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __LV_GLUE_H__
#define __LV_GLUE_H__

#include <stdio.h>
#include "lvgl.h"
#include "nu_bitutil.h"
#include "nu_pin.h"

#define NVT_ALIGN(size, align)      (((size) + (align) - 1) & ~((align) - 1))
#define NVT_ALIGN_DOWN(size, align) ((size) & ~((align) - 1))

#define CONFIG_TICK_PER_SECOND      1000
#define PORT_OFFSET                 0x40

typedef struct
{
    void *pvVramStartAddr;  // VRAM Start address

    uint32_t u32VramSize;   // VRAM total size in byte

    uint32_t u32ResWidth;   // Resolution - Width

    uint32_t u32ResHeight;  // Resolution - Height

    uint32_t u32BytePerPixel;  // Byte per Pixel

} S_LCD_INFO;

typedef enum
{
    evLCD_CTRL_GET_INFO,
    evLCD_CTRL_RECT_UPDATE,
    evLCD_CTRL_CNT
} E_LCD_CTRL;

int lcd_device_initialize(void);
int lcd_device_finalize(void);
int lcd_device_open(void);
void lcd_device_close(void);
int lcd_device_control(int cmd, void *argv);

int touchpad_device_initialize(void);
int touchpad_device_finalize(void);
int touchpad_device_open(void);
int touchpad_device_read(lv_indev_data_t *psInDevData);
void touchpad_device_close(void);
int touchpad_device_control(int cmd, void *argv);
void systick_init(void);
void sysDelay(uint32_t ms);
uint32_t sysGetCurrentTime(void);

#endif /* __LV_GLUE_H__ */
