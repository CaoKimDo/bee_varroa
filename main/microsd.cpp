#include <dirent.h>
#include "driver/sdmmc_host.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "microsd.h"
#include "sdmmc_cmd.h"
#include "sdmmc_pins_and_width.h"
#include <sys/stat.h>

static const char *TAG = "[microsd]";

std::vector<std::string> list_jpeg_files(const char *dir_path) {
    std::vector<std::string> files;

    DIR *dir = opendir(dir_path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory: %s", dir_path);
        return files;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
        if (entry->d_type == DT_REG) {
            std::string name(entry->d_name);
            if (name.ends_with(".jpg") || name.ends_with(".jpeg"))
                files.push_back(std::string(dir_path) + "/" + name);
        }

    closedir(dir);
    return files;
}

uint8_t *load_file_from_microsd(const char *path, size_t *out_size) {
    FILE *file = fopen(path, "rb");

    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", path);

        *out_size = 0;
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    uint8_t *buffer = static_cast<uint8_t *>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM));
    if (!buffer) {
        ESP_LOGE(TAG, "Failted to allocated PSRAM (%d bytes)", size);

        fclose(file);
        *out_size = 0;
        return nullptr;
    }

    fread(buffer, 1, size, file);
    fclose(file);

    *out_size = size;
    return buffer;
}

void save_file_to_microsd(const char *path, uint8_t *data, size_t length) {
    FILE *file = fopen(path, "wb");

    if (!file)
        ESP_LOGE(TAG, "Failed to open file: %s", path);
    else {
        fwrite(data, 1, length, file);
        fclose(file);

        ESP_LOGI(TAG, "Successfully saved file: %s (%d bytes)", path, length);
    }
}

void init_microsd(void) {
    sdmmc_host_t sdmmc_host = SDMMC_HOST_DEFAULT();
    sdmmc_host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    sdmmc_host.flags &= ~SDMMC_HOST_FLAG_DDR;
    
    sdmmc_slot_config_t sdmmc_slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    sdmmc_slot_config.width = SDMMC_BUS_WIDTH;
    sdmmc_slot_config.cmd = SDMMC_PIN_CMD;
    sdmmc_slot_config.clk = SDMMC_PIN_CLK;
    sdmmc_slot_config.d0 = SDMMC_PIN_D0;

    esp_vfs_fat_mount_config_t esp_vfs_fat_mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 128 * 256,  // 32KB recommended for 8GB Micro SD card
        .disk_status_check_enable = false,
        .use_one_fat = false
    };
    
    sdmmc_card_t *sdmmc_card;

    esp_err_t err = esp_vfs_fat_sdmmc_mount("/sdcard", &sdmmc_host, &sdmmc_slot_config, &esp_vfs_fat_mount_config, &sdmmc_card);
    
    if (err != ESP_OK)
        ESP_LOGE(TAG, "Failed to mount the Micro SD card: %s", esp_err_to_name(err));
    else {
        sdmmc_card_print_info(stdout, sdmmc_card);
        ESP_LOGI(TAG, "Successfully mounted the Micro SD card");
    }

    // Create new directories for storing original photos, annotated images & bee images
    char dir[64];
    snprintf(dir, sizeof(dir), "/sdcard/original");
    mkdir(dir, 0777);
    snprintf(dir, sizeof(dir), "/sdcard/annotated");
    mkdir(dir, 0777);
    snprintf(dir, sizeof(dir), "/sdcard/bees");
    mkdir(dir, 0777);

    const char *results_file = "/sdcard/results.txt";
    char *data = "Inference Results Log\n=====================\n";  // Initial content
    
    // Check if results.txt exists. If it doesn't, create it.
    struct stat st;
    if (stat(results_file, &st) != 0)
        save_file_to_microsd(
            results_file,
            reinterpret_cast<uint8_t *>(data),
            strlen(data)
        );
    else
        ESP_LOGI(TAG, "results.txt already exists.");
}
