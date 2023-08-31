# Install script for directory: /home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/esp-idf

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/home/stepan/.espressif/tools/xtensa-esp32-elf/esp-2022r1-11.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/xtensa/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_ringbuf/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/efuse/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/driver/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_pm/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/mbedtls/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_app_format/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/bootloader_support/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/bootloader/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esptool_py/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/partition_table/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_partition/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/app_update/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/spi_flash/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/pthread/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_system/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_rom/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/hal/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/log/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/heap/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/soc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_hw_support/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/freertos/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/newlib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/cxx/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_common/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_timer/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/app_trace/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_event/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/nvs_flash/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_phy/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/vfs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/lwip/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_netif/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/wpa_supplicant/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_wifi/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/bt/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/unity/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/cmock/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/console/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/http_parser/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp-tls/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_adc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_eth/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_gdbstub/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_hid/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/tcp_transport/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_http_client/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_http_server/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_https_ota/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_https_server/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_lcd/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/protobuf-c/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/protocomm/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_local_ctrl/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_psram/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/espcoredump/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/wear_levelling/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/sdmmc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/fatfs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/idf_test/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/ieee802154/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/json/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/mqtt/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/openthread/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/perfmon/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/spiffs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/ulp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/usb/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/wifi_provisioning/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/espressif__esp_websocket_client/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/espressif__zlib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/main/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/espressif__jsmn/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/audio_sal/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/adf_utils/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/display_service/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_dispatcher/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/audio_hal/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp-sr/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/espressif__nghttp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp-adf-libs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/audio_pipeline/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_peripherals/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/audio_board/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/audio_recorder/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/wifi_service/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/clouds/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/dueros_service/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_actions/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/tone_partition/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/audio_stream/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/battery_service/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/bluetooth_service/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/coredump_upload_service/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_codec_dev/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp_event_cast/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/input_key_service/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/ota_service/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/playlist/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/esp-idf-speex/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/esp-idf/my_board/cmake_install.cmake")
endif()

