/**************************************************************************//**
 * @file     lv_port_disp.c
 * @brief    lvgl display port
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "lvgl.h"
#include "lv_glue.h"

#if CONFIG_LV_DISP_FULL_REFRESH
static void lv_port_disp_full(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    dcache_clean_by_mva(px_map, lv_area_get_size(area) * (LV_COLOR_DEPTH / 8));

    /* Use PANDISPLAY without H/W copying */
    LV_ASSERT(lcd_device_control(evLCD_CTRL_PAN_DISPLAY, (void *)px_map) == 0);

    /* vsync-after: Use ping-pong screen-sized buffers only.*/
    LV_ASSERT(lcd_device_control(evLCD_CTRL_WAIT_VSYNC, (void *)NULL) == 0);

    lv_display_flush_ready(disp);
}

#else

static void *buf3_next = NULL;
static void lv_port_disp_partial(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{

    S_LCD_INFO *psLCDInfo = (S_LCD_INFO *)lv_display_get_driver_data(disp);

    /* Update dirty region. */
    int32_t x, y;
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);
    uint32_t *pDisp = (uint32_t *)nc_ptr(psLCDInfo->pvVramStartAddr + (LV_HOR_RES_MAX * area->y1 + area->x1) * sizeof(uint32_t));
    uint32_t *pSrc = (uint32_t *)px_map;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            pDisp[x] = pSrc[x];
        }
        pDisp += LV_HOR_RES_MAX;
        pSrc += w;
    }

    lv_display_flush_ready(disp);

}

#endif

void lv_port_disp_init(void)
{
    lv_display_t *disp;
    static S_LCD_INFO sLcdInfo = {0};
    void *buf1, *buf2;
    uint32_t u32FBSize;

    /* Initial display device */
    LV_ASSERT(lcd_device_initialize() == 0);
    LV_ASSERT(lcd_device_open() == 0);
    LV_ASSERT(lcd_device_control(evLCD_CTRL_GET_INFO, (void *)&sLcdInfo) == 0);

    u32FBSize = sLcdInfo.u32ResHeight * sLcdInfo.u32ResWidth * sLcdInfo.u32BytePerPixel;
    buf1 = (void *)sLcdInfo.pvVramStartAddr;
    buf2 = (void *)buf1 + u32FBSize;

    disp = lv_display_create(sLcdInfo.u32ResWidth, sLcdInfo.u32ResHeight);
    LV_ASSERT(disp != NULL);

    lv_display_set_driver_data(disp, &sLcdInfo);

#if CONFIG_LV_DISP_FULL_REFRESH
    LV_LOG_INFO("Use two screen-size buffer, buf1: 0x%08x, buf2: 0x%08x: 0x%08x", buf1, buf2);
    lv_color_format_t cf = lv_display_get_color_format(disp);

    lv_display_set_flush_cb(disp, lv_port_disp_full); /*Set a flush callback to draw to the display*/
    lv_display_set_buffers(disp, buf1, buf2, u32FBSize, LV_DISPLAY_RENDER_MODE_FULL); /*Set an initialized buffer*/

#else
    buf3_next = (void *)(buf2 + u32FBSize);

    LV_LOG_INFO("Use two screen-size shadow buffer, 0x%08x, 0x%08x.", buf2, buf3_next);

    lv_display_set_flush_cb(disp, lv_port_disp_partial);               /*Set a flush callback to draw to the display*/
    lv_display_set_buffers(disp, buf2, buf3_next, u32FBSize, LV_DISPLAY_RENDER_MODE_PARTIAL); /*Set an initialized buffer*/
#endif
}
