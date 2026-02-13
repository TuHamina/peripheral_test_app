target_sources(app PRIVATE
    adc_test.c
    adc_app.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)