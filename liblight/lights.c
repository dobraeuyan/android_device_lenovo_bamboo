/*Copyright (C) 2013 The CyanogenMod Project
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

#define LOG_TAG "lights.msm8909"

#include <cutils/log.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <hardware/lights.h>

#define LED_PATH                "/sys/class/leds/notify-led/"
#define BRIGHTNESS_FILE         "brightness"
#define BLINK_FILE              "blink" 

char const* const LCD_BACKLIGHT_PATH = "/sys/class/leds/lcd-backlight/brightness";
char const* const LED_BRIGHTNESS_PATH = LED_PATH BRIGHTNESS_FILE;
char const* const LED_BLINK_PATH = LED_PATH BLINK_FILE;

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static struct light_state_t g_notification_state;
static struct light_state_t g_battery_state;

static int write_int(const char* path, int value) {
    int fd;
    static int already_warned = 0;

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        if (already_warned == 0) {
            ALOGE("Error opening %s: %s\n", path, strerror(errno));
            already_warned = 1;
        }
        return -errno;
    }

    char buffer[20];
    int bytes = snprintf(buffer, sizeof(buffer), "%d\n", value);
    ssize_t amt = write(fd, buffer, (size_t)bytes);
    close(fd);

    return amt == -1 ? -errno : 0;
}

static int rgb_to_brightness(const struct light_state_t* state) {
    if (state->color == 0) {
        return 0;
    }
    unsigned int color = state->color & 0x00ffffff;
    unsigned int r = (color >> 16) & 0xff;
    unsigned int g = (color >> 8) & 0xff;
    unsigned int b = color & 0xff;
    return ((77 * r + 150 * g + 29 * b) >> 8);
}

static int handle_led_update() {
    pthread_mutex_lock(&g_lock);
    
    struct light_state_t* state_to_show;

    if ((g_battery_state.color & 0x00ffffff) != 0) {
        state_to_show = &g_battery_state;
    } else {
        state_to_show = &g_notification_state;
    }

    int brightness = rgb_to_brightness(state_to_show);

    if (brightness == 0) {
        write_int(LED_BLINK_PATH, 0);
        write_int(LED_BRIGHTNESS_PATH, 0);
        pthread_mutex_unlock(&g_lock);
        return 0;
    }

int use_breath_effect = 0;
    if (state_to_show->flashMode == LIGHT_FLASH_TIMED || state_to_show == &g_battery_state) {
        use_breath_effect = 1;
    }

    if (use_breath_effect) {
        write_int(LED_BRIGHTNESS_PATH, 0);
        write_int(LED_BLINK_PATH, 1);
    } else {
        write_int(LED_BLINK_PATH, 0);
        write_int(LED_BRIGHTNESS_PATH, brightness);
    }
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int set_light_backlight(struct light_device_t* dev, const struct light_state_t* state) {
    int err;
    int brightness = rgb_to_brightness(state);
    pthread_mutex_lock(&g_lock);
    err = write_int(LCD_BACKLIGHT_PATH, brightness);
    pthread_mutex_unlock(&g_lock);
    return err;
}

static int set_light_notifications(struct light_device_t* dev, const struct light_state_t* state) {
    g_notification_state = *state;
    handle_led_update();
    return 0;
}

static int set_light_battery(struct light_device_t* dev, const struct light_state_t* state) {
    g_battery_state = *state;
    handle_led_update();
    return 0;
}

static int set_light_attention(struct light_device_t* dev, const struct light_state_t* state) {
    g_notification_state = *state;
    handle_led_update();
    return 0;
}

static int close_lights(struct hw_device_t* device) {
    if (device) free(device);
    return 0;
}

static int open_lights(const struct hw_module_t* module, const char* id, struct hw_device_t** device) {
    int (*set_light)(struct light_device_t* dev, const struct light_state_t* state);

    if (strcmp(LIGHT_ID_BACKLIGHT, id) == 0) set_light = set_light_backlight;
    else if (strcmp(LIGHT_ID_NOTIFICATIONS, id) == 0) set_light = set_light_notifications;
    else if (strcmp(LIGHT_ID_BATTERY, id) == 0) set_light = set_light_battery;
    else if (strcmp(LIGHT_ID_ATTENTION, id) == 0) set_light = set_light_attention;
    else return -EINVAL;

    struct light_device_t* dev = malloc(sizeof(struct light_device_t));
    if (!dev) {
        ALOGE("Memory error when opening the lights device.");
        return -ENOMEM;
    }

    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t*)dev;
    ALOGI("Lights device '%s' initialized successfully.", id);

    return 0;
}

static struct hw_module_methods_t lights_module_methods = { .open = open_lights };

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = 1,
    .hal_api_version = HARDWARE_HAL_API_VERSION,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "Lenovo msm8909 Lights HAL (Brightness + Breathing Battery)",
    .author = "dobraeuyan",
    .methods = &lights_module_methods,
};
