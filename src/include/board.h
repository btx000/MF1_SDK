#ifndef _BOARD_H
#define _BOARD_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "system_config.h"

#define RLED 1
#define GLED 2
#define BLED 4

#define PLL0_OUTPUT_FREQ 800000000UL
#define PLL1_OUTPUT_FREQ 400000000UL

extern volatile uint8_t g_key_press;
extern volatile uint8_t g_key_long_press;

extern uint8_t sKey_dir;

#if (!CONFIG_CAMERA_OV2640 || !CONFIG_CAMERA_GC0328_SINGLE)
extern uint8_t kpu_image_tmp[CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 3];
#endif

#if CONFIG_LCD_VERTICAL
extern uint8_t display_image_ver[CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 2]; //显示
#endif

extern uint8_t kpu_image[2][CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 3];
extern uint8_t display_image[CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 2];

int irq_gpiohs(void *ctx);

void set_IR_LED(int state);
void set_RGB_LED(int state);
void update_key_state(void);
void board_init(void);

void web_set_RGB_LED(uint8_t val[3]);

#endif