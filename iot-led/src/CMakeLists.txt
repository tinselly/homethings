

target_sources(
    app 

    PRIVATE 

    main.c
    strip_thread.c
    mqtt.c

    cmd.c
    animation.c
    strip.c
    color.c
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
