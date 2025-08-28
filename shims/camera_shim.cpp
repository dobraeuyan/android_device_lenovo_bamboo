#define LOG_TAG "camera_shim_msm8909"

#include <dlfcn.h>
#include <stdlib.h>
#include <hardware/camera.h>
#include <hardware/hardware.h>
#include <cutils/log.h>

#include <camera/CameraParameters.h>
#include <utils/String8.h>
#include <utils/Mutex.h>

using namespace android;

static Mutex gLock;
static camera_module_t* gVendorModule = nullptr;

static int check_vendor_module()
{
    if (gVendorModule) return 0;
    int rv = hw_get_module_by_class(CAMERA_HARDWARE_MODULE_ID, "vendor",
                                    (const hw_module_t**)&gVendorModule);
    if (rv) ALOGE("Could not open camera.vendor.msm8909.so: %d", rv);
    return rv;
}

static void removeSubstring(String8& str, size_t pos, size_t length) {
    if (pos >= str.size()) return;
    
    size_t actualLength = length;
    if (pos + length > str.size()) {
        actualLength = str.size() - pos;
    }
    
    size_t totalSize = str.size();
    char* buf = str.lockBuffer(totalSize);
    
    memmove(buf + pos, 
            buf + pos + actualLength, 
            totalSize - pos - actualLength);
    
    str.unlockBuffer(totalSize - actualLength);
}

static inline void strip_nv12venus(CameraParameters& p)
{
    const char* kPF = p.get(CameraParameters::KEY_PREVIEW_FORMAT);
    const char* kPFVals = p.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS);
    
    if (kPF && !strcmp(kPF, "nv12-venus")) {
        p.set(CameraParameters::KEY_PREVIEW_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);
    }
    
    if (kPFVals && strstr(kPFVals, "nv12-venus")) {
        String8 vals(kPFVals);
        const char* target = "nv12-venus";
        ssize_t pos;
        
        while ((pos = vals.find(target)) >= 0) {
            bool rmLeadingComma = (pos > 0 && vals[pos-1] == ',');
            bool rmTrailingComma = (pos + (ssize_t)strlen(target) < (ssize_t)vals.size() && vals[pos + strlen(target)] == ',');
            
            if (rmLeadingComma && rmTrailingComma) {
                removeSubstring(vals, pos, strlen(target) + 1);
            } else if (rmLeadingComma) {
                removeSubstring(vals, pos-1, strlen(target) + 1);
            } else if (rmTrailingComma) {
                removeSubstring(vals, pos, strlen(target) + 1);
            } else {
                removeSubstring(vals, pos, strlen(target));
            }
        }
        while (vals.size() > 0 && vals[0] == ',') {
            removeSubstring(vals, 0, 1);
        }
        while (vals.size() > 0 && vals[vals.size()-1] == ',') {
            removeSubstring(vals, vals.size()-1, 1);
        }
        
        p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, vals.string());
    }
}

static inline bool is_video_mode(const CameraParameters& p)
{
    const char* hint = p.get(CameraParameters::KEY_RECORDING_HINT);
    return hint && !strcmp(hint, "true");
}

static inline void force_yuv420sp_if_video(CameraParameters& p)
{
    if (!is_video_mode(p)) return;
    const char* pf = p.get(CameraParameters::KEY_PREVIEW_FORMAT);
    if (!pf || strcmp(pf, CameraParameters::PIXEL_FORMAT_YUV420SP) != 0) {
        p.set(CameraParameters::KEY_PREVIEW_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);
    }
    const char* vff = p.get("video-frame-format");
    if (vff && !strcmp(vff, "nv12-venus")) {
        p.set("video-frame-format", CameraParameters::PIXEL_FORMAT_YUV420SP);
    }
}

typedef struct wrapper_camera_device_t {
    camera_device_t base;
    int id;
    camera_device_t* vendor;
} wrapper_camera_device_t;

#define VENDOR_CALL(dev, func, ...) \
    ({ wrapper_camera_device_t* _w = (wrapper_camera_device_t*)(dev); \
       _w->vendor->ops->func(_w->vendor, ##__VA_ARGS__); })

#define CAMERA_ID(dev) (((wrapper_camera_device_t*)(dev))->id)

static int camera_set_preview_window(struct camera_device* device, struct preview_stream_ops* w)
{ return VENDOR_CALL(device, set_preview_window, w); }
static void camera_set_callbacks(struct camera_device* d, camera_notify_callback n, camera_data_callback da, camera_data_timestamp_callback dt, camera_request_memory m, void* u)
{ VENDOR_CALL(d, set_callbacks, n, da, dt, m, u); }
static void camera_enable_msg_type(struct camera_device* d, int32_t t){ VENDOR_CALL(d, enable_msg_type, t);} 
static void camera_disable_msg_type(struct camera_device* d, int32_t t){ VENDOR_CALL(d, disable_msg_type, t);} 
static int camera_msg_type_enabled(struct camera_device* d, int32_t t){ return VENDOR_CALL(d, msg_type_enabled, t);} 
static int camera_start_preview(struct camera_device* d){ return VENDOR_CALL(d, start_preview);} 
static void camera_stop_preview(struct camera_device* d){ VENDOR_CALL(d, stop_preview);} 
static int camera_preview_enabled(struct camera_device* d){ return VENDOR_CALL(d, preview_enabled);} 
static int camera_store_meta_data_in_buffers(struct camera_device* d, int e){ return VENDOR_CALL(d, store_meta_data_in_buffers, e);} 
static void camera_stop_recording(struct camera_device* d){ VENDOR_CALL(d, stop_recording);} 
static int camera_recording_enabled(struct camera_device* d){ return VENDOR_CALL(d, recording_enabled);} 
static void camera_release_recording_frame(struct camera_device* d, const void* o){ VENDOR_CALL(d, release_recording_frame, o);} 
static int camera_auto_focus(struct camera_device* d){ return VENDOR_CALL(d, auto_focus);} 
static int camera_cancel_auto_focus(struct camera_device* d){ return VENDOR_CALL(d, cancel_auto_focus);} 
static int camera_take_picture(struct camera_device* d){ return VENDOR_CALL(d, take_picture);} 
static int camera_cancel_picture(struct camera_device* d){ return VENDOR_CALL(d, cancel_picture);} 
static int camera_send_command(struct camera_device* d, int32_t c, int32_t a1, int32_t a2){ return VENDOR_CALL(d, send_command, c, a1, a2);} 
static void camera_release(struct camera_device* d){ VENDOR_CALL(d, release);} 
static int camera_dump(struct camera_device* d, int fd){ return VENDOR_CALL(d, dump, fd);} 

static char* camera_get_parameters(struct camera_device* device)
{
    char* params = VENDOR_CALL(device, get_parameters);
    if (!params) return nullptr;

    CameraParameters p; p.unflatten(String8(params));
    strip_nv12venus(p);

    String8 out = p.flatten();
    VENDOR_CALL(device, put_parameters, params);
    return strdup(out.string());
}

static void camera_put_parameters(struct camera_device*, char* params)
{
    if (params) free(params);
}

static int camera_set_parameters(struct camera_device* device, const char* in)
{
    if (!in) return -EINVAL;
    CameraParameters p; p.unflatten(String8(in));
    force_yuv420sp_if_video(p);
    const char* pf_in = p.get(CameraParameters::KEY_PREVIEW_FORMAT);
    if (pf_in && !strcmp(pf_in, "nv12-venus")) {
        p.set(CameraParameters::KEY_PREVIEW_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);
    }
    const char* vff_in = p.get("video-frame-format");
    if (vff_in && !strcmp(vff_in, "nv12-venus")) {
        p.set("video-frame-format", CameraParameters::PIXEL_FORMAT_YUV420SP);
    }
    String8 out = p.flatten();
    return VENDOR_CALL(device, set_parameters, out.string());
}

static int camera_start_recording(struct camera_device* device)
{
    CameraParameters p; p.unflatten(String8(camera_get_parameters(device)));
    force_yuv420sp_if_video(p);
    String8 out = p.flatten();
    VENDOR_CALL(device, set_parameters, out.string());
    return VENDOR_CALL(device, start_recording);
}

static int camera_device_close(hw_device_t* device)
{
    if (!device) return 0;
    wrapper_camera_device_t* w = (wrapper_camera_device_t*)device;
    if (w->vendor) w->vendor->common.close((hw_device_t*)w->vendor);
    if (w->base.ops) free(w->base.ops);
    free(w);
    return 0;
}

static int camera_device_open(const hw_module_t* module, const char* name, hw_device_t** device)
{
    if (check_vendor_module()) return -EINVAL;

    int camera_id = atoi(name);

    wrapper_camera_device_t* w = (wrapper_camera_device_t*)calloc(1, sizeof(*w));
    if (!w) return -ENOMEM;

    int rv = gVendorModule->common.methods->open((const hw_module_t*)gVendorModule, name, (hw_device_t**)&w->vendor);
    if (rv) { free(w); return rv; }

    camera_device_ops_t* ops = (camera_device_ops_t*)calloc(1, sizeof(*ops));
    if (!ops) { camera_device_close((hw_device_t*)w); return -ENOMEM; }

    w->base.common.tag = HARDWARE_DEVICE_TAG;
    w->base.common.version = CAMERA_DEVICE_API_VERSION_1_0;
    w->base.common.module = (hw_module_t*)module;
    w->base.common.close = camera_device_close;
    w->base.ops = ops;
    w->id = camera_id;

    ops->set_preview_window = camera_set_preview_window;
    ops->set_callbacks = camera_set_callbacks;
    ops->enable_msg_type = camera_enable_msg_type;
    ops->disable_msg_type = camera_disable_msg_type;
    ops->msg_type_enabled = camera_msg_type_enabled;
    ops->start_preview = camera_start_preview;
    ops->stop_preview = camera_stop_preview;
    ops->preview_enabled = camera_preview_enabled;
    ops->store_meta_data_in_buffers = camera_store_meta_data_in_buffers;
    ops->start_recording = camera_start_recording;
    ops->stop_recording = camera_stop_recording;
    ops->recording_enabled = camera_recording_enabled;
    ops->release_recording_frame = camera_release_recording_frame;
    ops->auto_focus = camera_auto_focus;
    ops->cancel_auto_focus = camera_cancel_auto_focus;
    ops->take_picture = camera_take_picture;
    ops->cancel_picture = camera_cancel_picture;
    ops->set_parameters = camera_set_parameters;
    ops->get_parameters = camera_get_parameters;
    ops->put_parameters = camera_put_parameters;
    ops->send_command = camera_send_command;
    ops->release = camera_release;
    ops->dump = camera_dump;

    *device = &w->base.common;
    return 0;
}

static int camera_get_number_of_cameras(void)
{
    if (check_vendor_module()) return 0;
    return gVendorModule->get_number_of_cameras();
}

static int camera_get_camera_info(int id, struct camera_info* info)
{
    if (check_vendor_module()) return -EINVAL;
    return gVendorModule->get_camera_info(id, info);
}

static struct hw_module_methods_t camera_module_methods = {
    .open = camera_device_open,
};

camera_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = CAMERA_MODULE_API_VERSION_1_0,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = CAMERA_HARDWARE_MODULE_ID,
        .name = "Camera Shim MSM8909 (nv12-venus hider)",
        .author = "dobraeuyan",
        .methods = &camera_module_methods,
        .dso = nullptr,
        .reserved = {0},
    },
    .get_number_of_cameras = camera_get_number_of_cameras,
    .get_camera_info = camera_get_camera_info,
    .set_callbacks = nullptr,
    .get_vendor_tag_ops = nullptr,
    .open_legacy = nullptr,
    .set_torch_mode = nullptr,
    .init = nullptr,
    .reserved = {0},
};
