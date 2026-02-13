target_sources(app PRIVATE
    main.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(crc32)
add_subdirectory(nfc_test)
add_subdirectory(shell)
add_subdirectory(adc_test)