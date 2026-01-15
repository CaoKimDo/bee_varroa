#include "camera.h"
#include "camera_pins.h"
#include "esp_camera.h"
#include "esp_log.h"

static const char *TAG = "[camera]";

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,
    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 20000000,

    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,  // YUV422, GRAYSCALE, RGB565, JPEG
    .frame_size = FRAMESIZE_UXGA,

    .jpeg_quality = 4,  // 0-63 (For OV series camera sensors, lower number means higher quality. Consider 8-10 for better quality)
    .fb_count = 1,  // When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode. (Higher value requires more PSRAM)
    .fb_location = CAMERA_FB_IN_PSRAM,  // CAMERA_FB_IN_DRAM or CAMERA_FB_IN_PSRAM
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY  // CAMERA_GRAB_LATEST (Set when buffers should be filled. For capturing still images, use CAMERA_GRAB_WHEN_EMPTY)
};

// Initialize the camera
void init_camera(void) {
    esp_err_t err = esp_camera_init(&camera_config);

    if (err != ESP_OK)
        ESP_LOGE(TAG, "Camera initialization failed: %s", esp_err_to_name(err));
    else {
        // Discard initial frames
        for (int i = 0; i < 15; ++i) {
            camera_fb_t *fb = esp_camera_fb_get();
            esp_camera_fb_return(fb);
        }
        ESP_LOGI(TAG, "Successfully initialized camera");
    }
}

// Take pictures
camera_fb_t* take_picture(void) {
    camera_fb_t *camera_fb = esp_camera_fb_get();
        
    if (!camera_fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return NULL;
    } else {
        ESP_LOGI(TAG, "Picture taken! Size: %d bytes", camera_fb->len);
        return camera_fb;
        //esp_camera_fb_return(camera_fb);  // Release the frame buffer so the driver can reuse it
    }
}
