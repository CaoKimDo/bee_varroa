#pragma once

#include "esp_camera.h"

void init_camera(void);
camera_fb_t* take_picture(void);
