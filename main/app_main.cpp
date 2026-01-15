#include "camera.h"
#include "bee_detector.h"
#include "dl_image_jpeg.hpp"
#include "esp_camera.h"
#include "esp_log.h"
#include "microsd.h"
#include "varroa_detector.h"

static const char *TAG = "[app_main]";

static uint32_t counter = 0;
static int crop_counter = 0, mite_counter = 0;

static void get_free_psram() {
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG,"PSRAM free: %u bytes", (unsigned)free_psram);
}

static inline void draw_bbox(
    uint8_t *img, int img_w, int img_h,
    int x1, int y1, int x2, int y2,
    uint8_t r = 255, uint8_t g = 255, uint8_t b = 255,
    int thickness = 3
) {
    // Clamp bounds
    x1 = std::max(0, x1);
    y1 = std::max(0, y1);
    x2 = std::min(img_w - 1, x2);
    y2 = std::min(img_h - 1, y2);

    for (int th = 0; th < thickness; ++th) {
        int yt1 = std::max(0, y1 - th);
        int yt2 = std::min(img_h - 1, y2 + th);
        int xt1 = std::max(0, x1 - th);
        int xt2 = std::min(img_w - 1, x2 + th);

        // Top (t) & bottom (btm)
        for (int x = xt1; x <= xt2; ++x) {
            int t = (yt1 * img_w + x) * 3;
            int btm = (yt2 * img_w + x) * 3;
            img[t] = r; img[t+1] = g; img[t+2] = b;
            img[btm] = r; img[btm+1] = g; img[btm+2] = b;
        }

        // Left (l) & right (rgt)
        for (int y = yt1; y <= yt2; ++y) {
            int l = (y * img_w + xt1) * 3;
            int rgt = (y * img_w + xt2) * 3;
            img[l] = r; img[l+1] = g; img[l+2] = b;
            img[rgt] = r; img[rgt+1] = g; img[rgt+2] = b;
        }
    }
}

static dl::image::img_t crop_img(const dl::image::img_t &img, int x1, int y1, int x2, int y2) {
    // Clamp bounds
    x1 = std::max(0, x1);
    y1 = std::max(0, y1);
    x2 = std::min(static_cast<int>(img.width), x2);
    y2 = std::min(static_cast<int>(img.height), y2);

    int crop_width = x2 - x1;
    int crop_height = y2 - y1;

    // Bytes/pixel depends on format
    int bpp = (img.pix_type == dl::image::DL_IMAGE_PIX_TYPE_RGB565) ? 2 : 3;

    dl::image::img_t cropped_img = {
        .data = heap_caps_malloc(crop_width * crop_height * bpp, MALLOC_CAP_SPIRAM),
        .width = static_cast<uint16_t>(crop_width),
        .height = static_cast<uint16_t>(crop_height),
        .pix_type = img.pix_type
    };

    const uint8_t *src_p = static_cast<const uint8_t *>(img.data);
    uint8_t *dst_p = static_cast<uint8_t *>(cropped_img.data);

    for (int y = 0; y < crop_height; ++y)
        memcpy(
            dst_p + y * crop_width * bpp,
            src_p + ((y + y1) * img.width + x1) * bpp,
            crop_width * bpp
        );

    return cropped_img;
}

extern "C" void app_main(void) {
    // Initialize components
    //init_camera();
    init_microsd();

    auto images = list_jpeg_files("/sdcard/images");

    for (const auto &path : images) {
        size_t jpeg_size;
        uint8_t *jpeg_data = load_file_from_microsd(path.c_str(), &jpeg_size);
        if (!jpeg_data)
            continue;

        /*// Take a photo
        camera_fb_t *fb = take_picture();
        if (fb == NULL) {
            esp_camera_fb_return(fb);
            return;
        }

        // Save captured photo to the Micro SD card
        char path[64];
        snprintf(path, sizeof(path), "/sdcard/original/%08ld.jpg", counter);
        save_file_to_microsd(path, fb->buf, fb->len);

        // Wrap into jpeg_img_t
        dl::image::jpeg_img_t jpeg_img = {
            .data = fb->buf,
            .data_len = fb->len
        };*/

        // Read jpg file
        dl::image::jpeg_img_t jpeg_img = {
            .data = jpeg_data,
            .data_len = jpeg_size
        };

        // Decode to RGB888 using software decoder
        dl::image::img_t img = dl::image::sw_decode_jpeg(
            jpeg_img,
            dl::image::DL_IMAGE_PIX_TYPE_RGB888
        );

        get_free_psram();

        //esp_camera_fb_return(fb);  // Release the frame buffer
        heap_caps_free(jpeg_data);  // Cleanup

        // Load, run & delete bee detector
        BeeDetDetect *bee_detector = new BeeDetDetect();
        std::list<dl::detect::result_t> results = bee_detector->run(img);
        delete bee_detector;

        VarroaDetDetect *varroa_detector = new VarroaDetDetect();  // Load varroa detector

        crop_counter = 0, mite_counter = 0;

        for (const auto &res : results) {
            ESP_LOGI(
                TAG, "[category: %s, score: %f, x1: %d, y1: %d, x2: %d, y2: %d]",
                "bee", res.score, res.box[0], res.box[1], res.box[2], res.box[3]
            );

            // Crop the image
            dl::image::img_t cropped_img = crop_img(img, res.box[0], res.box[1], res.box[2], res.box[3]);

            // Run varroa detector on cropped bee
            std::list<dl::detect::result_t> &mites = varroa_detector->run(cropped_img);

            mite_counter += mites.size();

            for (const auto &m : mites) {
                ESP_LOGI(
                    TAG, "[category: %s, score: %f, x1: %d, y1: %d, x2: %d, y2: %d]",
                    "varroa", m.score, m.box[0], m.box[1], m.box[2], m.box[3]
                );

                // Map back to original image
                int varroa_x1 = res.box[0] + m.box[0];
                int varroa_y1 = res.box[1] + m.box[1];
                int varroa_x2 = res.box[0] + m.box[2];
                int varroa_y2 = res.box[1] + m.box[3];

                // Draw bounding boxes
                draw_bbox(
                    static_cast<uint8_t *>(img.data), img.width, img.height,
                    varroa_x1, varroa_y1, varroa_x2, varroa_y2,
                    255, 0, 0  // Red
                );
            }

            // Encode to JPEG using software encoder
            dl::image::jpeg_img_t cropped_jpeg_img = dl::image::sw_encode_jpeg_base(cropped_img, 90);

            // Save cropped image to the Micro SD card
            char path[64];
            snprintf(
                path, sizeof(path), "/sdcard/bees/%08ld_%d_%s.jpg",
                counter, crop_counter++, "bee"
            );
            save_file_to_microsd(
                path,
                static_cast<uint8_t *>(cropped_jpeg_img.data),
                cropped_jpeg_img.data_len
            );

            get_free_psram();

            // Cleanup
            heap_caps_free(cropped_jpeg_img.data);
            heap_caps_free(cropped_img.data);
        }

        delete varroa_detector;  // Delete varroa detector

        // Draw bounding boxes
        for (const auto &res : results)
            draw_bbox(
                static_cast<uint8_t *>(img.data), img.width, img.height,
                res.box[0], res.box[1], res.box[2], res.box[3],
                0, 255, 0  // Green
            );

        // Prepare destination buffer for resized annotated image (800x600)
        dl::image::img_t resized_img = {
            .data = heap_caps_malloc(800 * 600 * 3, MALLOC_CAP_SPIRAM),
            .width = 800,
            .height = 600,
            .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888
        };

        // Use ImageTransformer to resize
        dl::image::ImageTransformer transformer;
        esp_err_t err = transformer.set_src_img(img)
                                   .set_dst_img(resized_img)
                                   .transform();
        if (err != ESP_OK)
            ESP_LOGE(TAG, "Transformation failed: %s", esp_err_to_name(err));
        else
            ESP_LOGI(TAG, "Successfully transformed image");

        get_free_psram();

        heap_caps_free(img.data);  // Cleanup

        // Encode resized annotated image to JPEG using software encoder
        dl::image::jpeg_img_t resized_annotated_jpeg_img = dl::image::sw_encode_jpeg_base(resized_img, 90);

        // Save annotated image to the Micro SD card
        char annotated_path[64];
        snprintf(annotated_path, sizeof(annotated_path), "/sdcard/annotated/%08ld_annotated.jpg", counter);
        save_file_to_microsd(
            annotated_path,
            static_cast<uint8_t *>(resized_annotated_jpeg_img.data),
            resized_annotated_jpeg_img.data_len
        );

        ESP_LOGI(
            TAG, "Image %08ld: Bee(s) = %d, Varroa(s) = %d",
            counter, results.size(), mite_counter
        );

        // Write statistics to results.txt
        FILE *file = fopen("/sdcard/results.txt", "a");
        if (file) {
            fprintf(
                file, "Image %08ld: Bee(s) = %d, Varroa(s) = %d\n",
                counter, results.size(), mite_counter
            );
            fclose(file);

            ESP_LOGI(TAG, "Successfully updated results.txt");
        } else
            ESP_LOGE(TAG, "Failed to open results.txt for writing");

        get_free_psram();
        ESP_LOGI(TAG, "-------- An inference has been completed! --------");

        // Cleanup
        heap_caps_free(resized_annotated_jpeg_img.data);
        heap_caps_free(resized_img.data);

        ++counter;
    }
}
