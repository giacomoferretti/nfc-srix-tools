cmake_minimum_required(VERSION 3.14)
project(nfc-srix-tools C)

set(CMAKE_C_STANDARD 99)

add_executable(srix-dump dump_tag.c logging.c logging.h)
target_link_libraries(srix-dump nfc)

add_executable(srix-read read_dump.c logging.c logging.h)