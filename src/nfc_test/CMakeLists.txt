target_sources(app PRIVATE
    nfc_test.c
    nfc_test_field_detect.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)