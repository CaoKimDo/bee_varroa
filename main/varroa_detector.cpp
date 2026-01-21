#include "varroa_detector.h"
#include "dl_detect_espdet_postprocessor.hpp"

extern const uint8_t esp32s3_128_128_varroa_espdl[] asm("_binary_esp32s3_128_128_varroa_espdl_start");

namespace varroadet_detect {
    VarroaDet::VarroaDet() {
        // Load the model from flash & minimize it
        m_model = new dl::Model((const char *)esp32s3_128_128_varroa_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
        m_model->profile_memory();  // For debugging
        m_model->minimize();

        // Image preprocessor
        m_image_preprocessor = new dl::image::ImagePreprocessor(
            m_model, {0, 0, 0}, {255, 255, 255}, dl::image::DL_IMAGE_CAP_RGB565_BIG_ENDIAN);
        m_image_preprocessor->enable_letterbox({114, 114, 114});  // MUST match training

        // Postprocessor (ESPDet/YOLO-style)
        m_postprocessor = new dl::detect::ESPDetPostProcessor(
            m_model, m_image_preprocessor,
            0.6,  // Confidence threshold
            0.7,  // NMS threshold
            160,  // Max detections
            {{8, 8, 4, 4},
             {16, 16, 8, 8},
             {32, 32, 16, 16}}
        );
    }
}  // namespace varroadet_detect

// Public wrapper
VarroaDetDetect::VarroaDetDetect() {
    load_model();
}

void VarroaDetDetect::load_model() {
    // Assign directly to m_model (DetectWrapper expects this)
    m_model = new varroadet_detect::VarroaDet();
}
