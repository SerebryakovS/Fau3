# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/esp-idf/components/bootloader/subproject"
  "/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/bootloader"
  "/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/bootloader-prefix"
  "/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/bootloader-prefix/tmp"
  "/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/bootloader-prefix/src/bootloader-stamp"
  "/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/bootloader-prefix/src"
  "/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/stepan/Projects/fau2os/Fau3VOM/Fau3Client-Esp32/Fau3ClientFirmware/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
