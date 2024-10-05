cmake_minimum_required(VERSION 3.11)
project(print3)

add_executable(${PROJECT_NAME}
    "src/deserialize/file.c"
    "src/deserialize/obj.c"
    "src/deserialize/off.c"
    "src/deserialize/parsing.c"
    "src/deserialize/ply.c"
    "src/deserialize/stdin.c"
    "src/deserialize/stl.c"
    "src/args.c"
    "src/main.c"
    "src/scene.c"
    "src/viewer.c"
)

target_include_directories(${PROJECT_NAME} PRIVATE "dep/raylib/include")
target_link_directories(${PROJECT_NAME} PRIVATE "dep/raylib/lib")

if (WIN32)
    target_link_libraries(${PROJECT_NAME} winmm.lib)
    target_link_libraries(${PROJECT_NAME} raylib.lib)
else()
    target_link_libraries(${PROJECT_NAME} m)
    target_link_libraries(${PROJECT_NAME} libraylib.a)
endif()
