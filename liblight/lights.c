/*
 * Copyright (C) 2016 The CyanogenMod Project
 * Copyright (C) 2025 dobraeuyan
 * Licensed under the Apache License, Version 2.0
 */

#define LOG_TAG "lights.bamboo"

#include <cutils/log.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <hardware/lights.h>

/******************************************************************************/

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static struct light_state_t g_notification;
static struct light_state_t g_battery;

/* Kernel Paths matching Device Tree "white-notification" */
char const*const WHITE_LED_FILE  = "/sys/class/leds/white-notification/brightness";
char const*const WHITE_DELAY_ON  = "/sys/class/leds/white-notification/delay_on";
char const*const WHITE_DELAY_OFF = "/sys/class/leds/white-notification/delay_off";
char const*const LCD_FILE        = "/sys/class/leds/lcd-backlight/brightness";
char const*const BUTTON_FILE     = "/sys/class/leds/button-backlight/brightness";

void init_globals(void) {
    pthread_mutex_init(&g_lock, NULL);
}

static int write_int(char const* path, int value) {
    int fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[20];
        int bytes = snprintf(buffer, sizeof(buffer), "%d\n", value);
        write(fd, buffer, bytes);
        close(fd);
        return 0;
    }
    return -errno;
}

static int is_lit(struct light_state_t const* state) {
    return state->color & 0x00ffffff;
}

static int rgb_to_brightness(struct light_state_t const* state) {
    int color = state->color & 0x00ffffff;
    return ((77*((color>>16)&0x00ff)) + (150*((color>>8)&0x00ff)) + (29*(color&0x00ff))) >> 8;
}

static int set_light_backlight(struct light_device_t* dev, struct light_state_t const* state) {
    pthread_mutex_lock(&g_lock);
    write_int(LCD_FILE, rgb_to_brightness(state));
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int set_speaker_light_locked(struct light_device_t* dev, struct light_state_t const* state) {
    int onMS, offMS, brightness;

    switch (state->flashMode) {
        case LIGHT_FLASH_TIMED:
        case LIGHT_FLASH_HARDWARE:
            onMS = state->flashOnMS;
            offMS = state->flashOffMS;
            break;
        default:
            onMS = 0;
            offMS = 0;
            break;
    }

    brightness = rgb_to_brightness(state);

    if (onMS > 0 && offMS > 0 && brightness > 0) {
        /* Hardware Blink: Write Off then On to trigger blink_set in kernel */
        write_int(WHITE_DELAY_OFF, offMS);
        write_int(WHITE_DELAY_ON, onMS);
    } else {
        /* Solid / Off */
        write_int(WHITE_DELAY_ON, 0);
        write_int(WHITE_DELAY_OFF, 0);
        write_int(WHITE_LED_FILE, (brightness > 0) ? 255 : 0);
    }
    return 0;
}

static void handle_speaker_battery_locked(struct light_device_t* dev) {
    if (is_lit(&g_notification)) {
        set_speaker_light_locked(dev, &g_notification);
    } else if (is_lit(&g_battery)) {
        set_speaker_light_locked(dev, &g_battery);
    } else {
        struct light_state_t offState;
        memset(&offState, 0, sizeof(offState));
        set_speaker_light_locked(dev, &offState);
    }
}

static int set_light_battery(struct light_device_t* dev, struct light_state_t const* state) {
    pthread_mutex_lock(&g_lock);
    g_battery = *state;
    handle_speaker_battery_locked(dev);
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int set_light_notifications(struct light_device_t* dev, struct light_state_t const* state) {
    pthread_mutex_lock(&g_lock);
    g_notification = *state;
    handle_speaker_battery_locked(dev);
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int set_light_attention(struct light_device_t* dev, struct light_state_t const* state) {
    return 0; /* Unused */
}

static int set_light_buttons(struct light_device_t* dev, struct light_state_t const* state) {
    pthread_mutex_lock(&g_lock);
    write_int(BUTTON_FILE, rgb_to_brightness(state));
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int close_lights(struct light_device_t *dev) {
    if (dev) free(dev);
    return 0;
}

static int open_lights(const struct hw_module_t* module, char const* name, struct hw_device_t** device) {
    int (*set_light)(struct light_device_t* dev, struct light_state_t const* state);

    if (0 == strcmp(LIGHT_ID_BACKLIGHT, name)) set_light = set_light_backlight;
    else if (0 == strcmp(LIGHT_ID_BATTERY, name)) set_light = set_light_battery;
    else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name)) set_light = set_light_notifications;
    else if (0 == strcmp(LIGHT_ID_BUTTONS, name)) set_light = set_light_buttons;
    else return -EINVAL;

    pthread_once(&g_init, init_globals);
    struct light_device_t *dev = malloc(sizeof(struct light_device_t));
    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = (int (*)(struct hw_device_t*))close_lights;
    dev->set_light = set_light;
    *device = (struct hw_device_t*)dev;
    return 0;
}

static struct hw_module_methods_t lights_module_methods = { .open =  open_lights };

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "Bamboo Lights HAL",
    .methods = &lights_module_methods,
};
