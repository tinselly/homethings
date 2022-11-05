

target_sources(
    app 

    PRIVATE 

    main.c
    strip.c
    color.c
    mqtt.c
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
