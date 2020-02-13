cmake_minimum_required(VERSION 3.14)
project(nfc-srix-tools C)
set(CMAKE_C_STANDARD 99)

# Use PkgConfig to find libnfc
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNFC REQUIRED libnfc)
include_directories(${LIBNFC_INCLUDE_DIRS})
link_directories(${LIBNFC_LIBRARY_DIRS})
add_definitions(${LIBNFC_CFLAGS_OTHER})

# srix-dump
add_executable(srix-dump dump_tag.c logging.c nfc_utils.c)
target_link_libraries(srix-dump ${LIBNFC_LIBRARIES})

# srix-read
add_executable(srix-read read_dump.c logging.c nfc_utils.c)
target_link_libraries(srix-read ${LIBNFC_LIBRARIES})

# srix-restore
add_executable(srix-restore restore_dump.c logging.c nfc_utils.c)
target_link_libraries(srix-restore ${LIBNFC_LIBRARIES})

# srix-reset
add_executable(srix-reset otp_reset.c logging.c nfc_utils.c)
target_link_libraries(srix-reset ${LIBNFC_LIBRARIES})