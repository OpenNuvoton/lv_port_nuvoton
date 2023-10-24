/**************************************************************************//**
 * @file     lv_glue.c
 * @brief    lvgl glue header
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __LV_GLUE_H__
#define __LV_GLUE_H__

#include "lvgl.h"
#include "displib.h"
#include "adc.h"
#include "nu_misc.h"

/*
(1) Apply partial update:
    i. Higher FPS.
   ii. Easy tearing when sliding.

(2) Apply full-refresh:
    i. Tearingless by switching VRAM start address after receiving Vsync of VPOST.
   ii. lvgl keeps previous surface and redraw new dirty joined area, the do vsync-switch.
*/
#define CONFIG_LV_DISP_FULL_REFRESH      1

#if CONFIG_LV_DISP_FULL_REFRESH
    #define CONFIG_LCD_FB_NUM            3
#else
    #define CONFIG_LCD_FB_NUM            2
#endif

#define CONFIG_SYS_TIMER                TIMER0

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
void sysDelay(uint32_t ms);

#endif /* __LV_GLUE_H__ */
