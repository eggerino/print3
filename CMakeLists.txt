cmake_minimum_required(VERSION 3.11)
project(print3)

add_executable(${PROJECT_NAME}
    "src/deserialize/file.c"
    "src/deserialize/obj.c"
    "src/deserialize/off.c"
    "src/deserialize/stdin.c"
    "src/deserialize/stl.c"
    "src/deserialize/util.c"
    "src/args.c"
    "src/main.c"
    "src/scene.c"
    "src/viewer.c"
)

target_link_libraries(${PROJECT_NAME} m)

if (WIN32)
    set(RAYLIB_STATIC raylib.lib)
else()
    set(RAYLIB_STATIC libraylib.a)
endif()
target_include_directories(${PROJECT_NAME} PRIVATE "dep/raylib/include")
target_link_directories(${PROJECT_NAME} PRIVATE "dep/raylib/lib")
target_link_libraries(${PROJECT_NAME} ${RAYLIB_STATIC})
