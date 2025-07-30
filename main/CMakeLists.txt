file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} "ui/*.c")

idf_component_register(SRCS "main.c" ${SRC_UI}
                    INCLUDE_DIRS ".")


#set partition table                    
idf_build_get_property(target IDF_TARGET)
idf_build_get_property(config_file SDKCONFIG)

file(READ ${config_file} sdkconfig_contents)

if (sdkconfig_contents MATCHES "CONFIG_IDF_TARGET_ESP32=y")
    set(PARTITION_CSV "partitions_ESP32.csv")
elseif(sdkconfig_contents MATCHES "CONFIG_IDF_TARGET_ESP32S3=y")
    set(PARTITION_CSV "partitions_ESP32S3.csv")
else()
    message(FATAL_ERROR "Unsupported flash size")
endif()

set(PARTITION_TABLE_CSV ${CMAKE_CURRENT_SOURCE_DIR}/${PARTITION_CSV})