cmake_minimum_required(VERSION 3.9)
project(em_movie)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)
include_directories( ${PROJECT_NAME} ${EMSCRIPTEN_DIR}/system/include)
include_directories( ${PROJECT_NAME} ../../include)
set(CMAKE_C_FLAGS  "-s EXPORT_NAME=\"'AEMovie'\" -s MODULARIZE=1 \
-Oz \
--memory-init-file 0 \
--emit-symbol-map \
--post-js ../src/em_libmovie_post.js \
-s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=\"['memcpy','memset','malloc','free','strlen']\" \
")
file(GLOB em_player_sources src/*.c)
file(GLOB ae_movie_sources ../../src/*.c)
add_executable( ${PROJECT_NAME} ${em_player_sources} ${ae_movie_sources})

#
