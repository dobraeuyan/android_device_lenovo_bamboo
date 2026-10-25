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

char const*const WHITE_TRIGGER_FILE
        = "/sys/class/leds/white-notification/trigger";

char const*const WHITE_DELAY_ON_FILE
        = "/sys/class/leds/white-notification/delay_on";

char const*const WHITE_DELAY_OFF_FILE
        = "/sys/class/leds/white-notification/delay_off";

void init_globals(void)
{
    pthread_mutex_init(&g_lock, NULL);
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
    int fd = open(path, O_RDWR);
    if (fd >= 0) {
        ssize_t amt = write(fd, value, strlen(value));
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
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
    int brightness = rgb_to_brightness(state);
    if (!dev) {
        return -1;
    }

    pthread_mutex_lock(&g_lock);
    if (!access(LCD_FILE, F_OK)) {
        err = write_int(LCD_FILE, brightness);
    } else {
        err = write_int(LCD_FILE2, brightness);
    }
    pthread_mutex_unlock(&g_lock);
    return err;
}

static void
set_white_locked(struct light_state_t const* state)
{
    int onMS, offMS;

    switch (state->flashMode) {
        case LIGHT_FLASH_TIMED:
        case LIGHT_FLASH_HARDWARE:
            onMS = state->flashOnMS;
            offMS = state->flashOffMS;
            break;
        case LIGHT_FLASH_NONE:
        default:
            onMS = 0;
            offMS = 0;
            break;
    }

    if (onMS > 0 && offMS > 0) {
        write_str(WHITE_TRIGGER_FILE, "timer");
        write_int(WHITE_DELAY_ON_FILE, onMS);
        write_int(WHITE_DELAY_OFF_FILE, offMS);
    } else {

        write_str(WHITE_TRIGGER_FILE, "none");
        write_int(WHITE_LED_FILE, is_lit(state) ? 255 : 0);
    }
}

static void
handle_white_locked(void)
{
    if (is_lit(&g_battery)) {
        set_white_locked(&g_battery);
    } else if (is_lit(&g_attention)) {
        set_white_locked(&g_attention);
    } else {
        set_white_locked(&g_notification);
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
