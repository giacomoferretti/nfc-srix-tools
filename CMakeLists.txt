cmake_minimum_required(VERSION 3.14)
project(nfc-srix C)

set(CMAKE_C_STANDARD 99)

add_executable(nfc-srix-read read_tag.c logging.c logging.h)
target_link_libraries(nfc-srix-read nfc)