

target_sources(
    app 

    PRIVATE 

    main.c
    strip.c
    color.c

    # ble_strip.c
)

target_include_directories(
    app 

    PRIVATE 

    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(
    app

    PRIVATE 
    
    -Wall
    -Werror=incompatible-pointer-types
    -Werror=implicit-function-declaration
)
