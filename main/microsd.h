#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

std::vector<std::string> list_jpeg_files(const char *dir_path);
uint8_t *load_file_from_microsd(const char *path, size_t *out_size);
void save_file_to_microsd(const char *path, uint8_t *data, size_t length);
void init_microsd(void);
