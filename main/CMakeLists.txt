set(srcs "app_main.cpp" "bee_detector.cpp" "camera.cpp" "microsd.cpp" "varroa_detector.cpp")

set(include_dirs ".")

set(requires driver esp32-camera esp-dl fatfs sdmmc vfs)

idf_build_get_property(component_targets __COMPONENT_TARGETS)
if ("___idf_espressif__esp-dl" IN_LIST component_targets)
    idf_component_get_property(espdl_dir espressif__esp-dl COMPONENT_DIR)
elseif("___idf_esp-dl" IN_LIST component_targets)
    idf_component_get_property(espdl_dir esp-dl COMPONENT_DIR)
endif()

set(cmake_dir ${espdl_dir}/fbs_loader/cmake)
include(${cmake_dir}/utilities.cmake)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

foreach(model_file esp32s3_416_416_bee.espdl esp32s3_128_128_varroa.espdl)
    target_add_aligned_binary_data(${COMPONENT_LIB} ${model_file} BINARY)
endforeach()
