# ESP32-S3 Bee & Varroa Detection Pipeline

This project implements a **two-stage TinyML vision pipeline** on the **ESP32-S3** using the **ESP-IDF** and **ESP-DL** frameworks to detect **bees** and **Varroa mites** from JPEG images that are either directly captured by an OV2640/OV5640 camera or pre-captured with a reasonable resolution (i.e., around UXGA 1600x1200px) and stored in a folder on a MicroSD card. The two detection models are trained using **ESP-Detection** framework.

The system is designed for **offline inference** on resource-constrained hardware, emphasizing efficient memory usage (PSRAM), modular detectors, and end-to-end image processing.

--------

## ✨ Features

* 📷 JPEG images capturing from an OV2640/OV5640 camera module connected to the board or loading from a MicroSD card 
* 🐝 Stage 1: Bee detection on full images
* 🦠 Stage 2: Varroa mite detection on cropped bee regions
* ✂️ Dynamic cropping and coordinate remapping
* 🖍️ Bounding box visualization (bees & mites)
* 🔄 Image resizing and annotation
* 💾 Result storage (cropped bees, annotated images, statistics)
* 🧠 Optimized for PSRAM usage on ESP32-S3

--------

## 🧠 System Overview

> **Input modes supported**:
>
> * 📷 **Live camera capture** using an OV2640/OV5640 camera module connected to the board
> * 📁 **Offline image inference** from a MicroSD card

The pipeline follows a **two-stage inference approach**:

1. **Bee Detection**

   * Input: Full RGB image decoded from JPEG
   * Model: `BeeDetDetect`
   * Output: Bounding boxes of detected bees

2. **Varroa Detection**

   * Input: Cropped bee regions
   * Model: `VarroaDetDetect`
   * Output: Bounding boxes of detected mites per bee

Detected bee and mite coordinates are mapped back to the original image for visualization.

--------

## 📁 Directory Structure (MicroSD)

```
/sdcard
├── original/        # Captured images (if using Live camera capture)
├── images/          # Input JPEG images (if using Offline image inference)
├── bees/            # Cropped bee images
├── annotated/       # Final annotated & resized images
├── results.txt      # Detection statistics log
```

--------

## 🛠️ Core Components

### app_main.cpp

![Pipeline](pipeline.png)

### Key Modules

* `camera.h` – Camera interface
* `bee_detector.h` – Bee detection model wrapper
* `varroa_detector.h` – Varroa detection model wrapper
* `microsd.h` – SD card utilities
* `dl_image_jpeg.hpp` – JPEG decode/encode utilities (ESP-DL)

--------

## 🧮 Memory Management

* PSRAM allocation via `heap_caps_malloc(MALLOC_CAP_SPIRAM)`
* Explicit cleanup after each inference stage
* Runtime PSRAM monitoring using:

```text
heap_caps_get_free_size(MALLOC_CAP_SPIRAM)
```

This ensures stable execution during batch image processing.

---

## 📷 Using Live Camera Instead of SD Card Images

The system processes **JPEG images stored on a MicroSD card** under `/sdcard/images` to save time when testing.

If you want to run **inference using an OV2640/5640 camera module attached to the ESP32-S3 board**, make the following changes in `app_main.cpp`:

### 1️⃣ Disable SD-card image iteration

Comment out the following code blocks:

```cpp
auto images = list_jpeg_files("/sdcard/images");

for (const auto &path : images) {
   size_t jpeg_size;
   uint8_t *jpeg_data = load_file_from_microsd(path.c_str(), &jpeg_size);
   if (!jpeg_data)
      continue;
```

And

```cpp
// Read jpg file
dl::image::jpeg_img_t jpeg_img = {
   .data = jpeg_data,
   .data_len = jpeg_size
};
```

And also comment out this cleanup line:

```cpp
heap_caps_free(jpeg_data);  // Cleanup
```

--------

### 2️⃣ Enable continuous camera capture

Uncomment the main loop:

```cpp
while (1) {
```

Then uncomment the camera capture and original image saving section:

```cpp
// Take a photo
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
};
```

After JPEG decoding is complete, uncomment the frame buffer release:

```cpp
esp_camera_fb_return(fb);  // Release the frame buffer
```

This enables **live camera-based inference**, allowing the system to continuously capture images, run detection, and save annotated results.

--------

## 🎚️ Detection Parameters Configuration

Both **bee detection** and **Varroa detection** models expose tunable inference parameters.

You can modify the following thresholds in the respective source files:

* `bee_detector.cpp`
* `varroa_detector.cpp`

### Adjustable Parameters

* **Confidence Threshold**
  Filters out low-confidence detections.

* **NMS Threshold (Non-Maximum Suppression)**
  Controls how overlapping bounding boxes are merged.

* **Max Detections**
  Limits the maximum number of detected objects per inference.

Adjusting these values allows you to trade off between **detection accuracy**, **false positives**, and **runtime performance** depending on deployment conditions.

---

## 📊 Output Example

For each processed image:

* Number of detected bees
* Total detected Varroa mites
* Saved cropped bee images
* Saved annotated overview image
* Appended results in `results.txt`

Example log entry:

```
Image 00000012: Bee(s) = 3, Varroa(s) = 5
```

---

## 🚀 Build & Flash

This project is built using **ESP-IDF**.

```bash
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

Ensure that PSRAM is enabled in `menuconfig` before building and flashing.

--------

## 🎯 Use Case

This system is designed for **smart beehive monitoring**, enabling:

* Automated health inspection
* Early detection of Varroa infestation
* Edge AI inference without cloud dependency

---

## 📌 Notes

* Camera capture code is included but commented out (image-based inference is currently used).
* All inference runs fully offline.
* Models are optimized and quantized for ESP32-S3 using ESP-DL tools.

---

## 📜 License

This project is intended for academic and research purposes.

---

## 👤 Author

**Kim Do Cao**
Computer Science & Engineering (CSE) Student intake 2022
Project / Embedded AI / TinyML / ESP32-S3
