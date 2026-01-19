target_sources(app PRIVATE
    crc32.c
    crc32_test.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)