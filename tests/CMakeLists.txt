cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(crc_test)

target_sources(app PRIVATE
    test_crc_checksum.c
    crc32.c
)