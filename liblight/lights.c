/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2026 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <log/log.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>

#include <hardware/lights.h>

/******************************************************************************/

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static struct light_state_t g_notification;
static struct light_state_t g_battery;
static struct light_state_t g_attention;

char const*const LCD_FILE
        = "/sys/class/leds/lcd-backlight/brightness";

char const*const LCD_FILE2
        = "/sys/class/backlight/panel0-backlight/brightness";

char const*const WHITE_LED_FILE
        = "/sys/class/leds/white-notification/brightness";

char const*const WHITE_BLINK_FILE
        = "/sys/class/leds/white-notification/blink";

static char const* g_lcd_file = LCD_FILE;

void init_globals(void)
{
    pthread_mutex_init(&g_lock, NULL);
    if (access(LCD_FILE, F_OK) != 0) {
        g_lcd_file = LCD_FILE2;
    }
}

static int
write_int(char const* path, int value)
{
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[20];
        int bytes = snprintf(buffer, sizeof(buffer), "%d\n", value);
        ssize_t amt = write(fd, buffer, (size_t)bytes);
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
        if (already_warned == 0) {
            ALOGE("write_int failed to open %s\n", path);
            already_warned = 1;
        }
        return -errno;
    }
}

static int
write_str(char const* path, char const* value)
{
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd >= 0) {
        ssize_t len = (ssize_t)strlen(value);
        ssize_t amt = write(fd, value, (size_t)len);
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
        if (already_warned == 0) {
            ALOGE("write_str failed to open %s\n", path);
            already_warned = 1;
        }
        return -errno;
    }
}

static int
is_lit(struct light_state_t const* state)
{
    return state->color & 0x00ffffff;
}

static int
rgb_to_brightness(struct light_state_t const* state)
{
    int color = state->color & 0x00ffffff;
    return ((77*((color>>16)&0x00ff))
            + (150*((color>>8)&0x00ff)) + (29*(color&0x00ff))) >> 8;
}

static int
set_light_backlight(struct light_device_t* dev,
        struct light_state_t const* state)
{
    int err = 0;
    int brightness;
    if (!dev) {
        return -1;
    }
    if (state->brightnessMode == BRIGHTNESS_MODE_LOW_PERSISTENCE) {
        return -ENOSYS;
    }

    brightness = rgb_to_brightness(state);
    pthread_mutex_lock(&g_lock);
    err = write_int(g_lcd_file, brightness);
    pthread_mutex_unlock(&g_lock);
    return err;
}

static void
set_white_locked(struct light_state_t const* state)
{
    int onMS, offMS;
    int brightness;

    if (!is_lit(state)) {
        write_int(WHITE_LED_FILE, 0);
        return;
    }

    {
        unsigned int alpha = (state->color >> 24) & 0xff;
        brightness = alpha > 0 ? (int)alpha : rgb_to_brightness(state);
    }
    if (brightness <= 0) {
        brightness = 1;
    } else if (brightness > 255) {
        brightness = 255;
    }

    switch (state->flashMode) {
        case LIGHT_FLASH_TIMED:
            onMS = state->flashOnMS;
            offMS = state->flashOffMS;
            break;
        case LIGHT_FLASH_HARDWARE:
            onMS = state->flashOnMS > 0 ? state->flashOnMS : 1000;
            offMS = state->flashOffMS > 0 ? state->flashOffMS : 1000;
            break;
        case LIGHT_FLASH_NONE:
        default:
            onMS = 0;
            offMS = 0;
            break;
    }

    if (onMS > 0 && offMS > 0) {
        char buf[48];
        snprintf(buf, sizeof(buf), "%d %d %d\n", onMS, offMS, brightness);
        write_str(WHITE_BLINK_FILE, buf);
    } else {
        write_int(WHITE_LED_FILE, brightness);
    }
}

static void
handle_white_locked(void)
{
    if (is_lit(&g_notification)) {
        set_white_locked(&g_notification);
    } else if (is_lit(&g_attention)) {
        set_white_locked(&g_attention);
    } else {
        struct light_state_t battery = g_battery;

        if (is_lit(&battery) && battery.flashMode == LIGHT_FLASH_NONE &&
            (battery.color & 0x00ffffff) != 0x00ff00) {
            battery.flashMode = LIGHT_FLASH_TIMED;
            battery.flashOnMS = 1000;
            battery.flashOffMS = 1000;
        }
        set_white_locked(&battery);
    }
}

static int
set_light_battery(struct light_device_t* dev,
        struct light_state_t const* state)
{
    if (!dev) {
        return -1;
    }
    pthread_mutex_lock(&g_lock);
    g_battery = *state;
    handle_white_locked();
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int
set_light_notifications(struct light_device_t* dev,
        struct light_state_t const* state)
{
    if (!dev) {
        return -1;
    }
    pthread_mutex_lock(&g_lock);
    g_notification = *state;
    handle_white_locked();
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int
set_light_attention(struct light_device_t* dev,
        struct light_state_t const* state)
{
    if (!dev) {
        return -1;
    }
    pthread_mutex_lock(&g_lock);
    if (state->flashMode == LIGHT_FLASH_NONE) {
        g_attention.color = 0;
    } else {
        g_attention = *state;
    }
    handle_white_locked();
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int
close_lights(struct light_device_t *dev)
{
    if (dev) {
        free(dev);
    }
    return 0;
}

/******************************************************************************/

static int open_lights(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    int (*set_light)(struct light_device_t* dev,
            struct light_state_t const* state);

    if (0 == strcmp(LIGHT_ID_BACKLIGHT, name)) {
        set_light = set_light_backlight;
    } else if (0 == strcmp(LIGHT_ID_BATTERY, name)) {
        set_light = set_light_battery;
    } else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name)) {
        set_light = set_light_notifications;
    } else if (0 == strcmp(LIGHT_ID_ATTENTION, name)) {
        set_light = set_light_attention;
    } else {

        return -EINVAL;
    }

    pthread_once(&g_init, init_globals);

    struct light_device_t *dev = malloc(sizeof(struct light_device_t));

    if (!dev) {
        return -ENOMEM;
    }

    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = LIGHTS_DEVICE_API_VERSION_2_0;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = (int (*)(struct hw_device_t*))close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t*)dev;
    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open =  open_lights,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "bamboo lights Module",
    .author = "The LineageOS Project",
    .methods = &lights_module_methods,
};
