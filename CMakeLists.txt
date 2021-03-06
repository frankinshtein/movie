cmake_minimum_required(VERSION 3.0)

PROJECT(movie)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL AppleClang)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL Intel)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall /WX")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4710")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4738")
endif()

OPTION(LIBMOVIE_EXTERNAL_BUILD  "LIBMOVIE_EXTERNAL_BUILD"  OFF)
OPTION(LIBMOVIE_EXAMPLES_BUILD  "LIBMOVIE_EXAMPLES_BUILD"  ON)

IF( NOT LIBMOVIE_EXTERNAL_BUILD )
    SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_GENERATOR} )
    SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_GENERATOR} )
ENDIF()

MACRO( ADD_FILTER group_name )
	SOURCE_GROUP( ${group_name} FILES ${ARGN} )
	SET( SRC_FILES ${SRC_FILES} ${ARGN} )
ENDMACRO()

SET( INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include )
SET( SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src )

ADD_FILTER(
public
	${INCLUDE_DIR}/movie/movie.h
    ${INCLUDE_DIR}/movie/movie.hpp
    ${INCLUDE_DIR}/movie/movie_node.h
    ${INCLUDE_DIR}/movie/movie_data.h
    ${INCLUDE_DIR}/movie/movie_instance.h
    ${INCLUDE_DIR}/movie/movie_resource.h
    ${INCLUDE_DIR}/movie/movie_skeleton.h
    ${INCLUDE_DIR}/movie/movie_type.h
    ${INCLUDE_DIR}/movie/movie_typedef.h
    ${INCLUDE_DIR}/movie/movie_config.h    
)

ADD_FILTER(
source
	${SOURCE_DIR}/movie_data.c
    ${SOURCE_DIR}/movie_instance.c
    ${SOURCE_DIR}/movie_math.c
    ${SOURCE_DIR}/movie_math.h
    ${SOURCE_DIR}/movie_memory.h
    ${SOURCE_DIR}/movie_node.c
    ${SOURCE_DIR}/movie_skeleton.c
    ${SOURCE_DIR}/movie_stream.c
    ${SOURCE_DIR}/movie_stream.h
    ${SOURCE_DIR}/movie_struct.h
    ${SOURCE_DIR}/movie_transformation.c
    ${SOURCE_DIR}/movie_transformation.h
    ${SOURCE_DIR}/movie_debug.h
)

INCLUDE_DIRECTORIES( ${PROJECT_NAME} ${INCLUDE_DIR} ) 

ADD_LIBRARY( ${PROJECT_NAME} STATIC ${SRC_FILES} )

if( LIBMOVIE_EXAMPLES_BUILD )
    SET( EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples )

    set(
    sandbox_SOURCES
        ${EXAMPLES_DIR}/sandbox/sandbox.c
    )

    INCLUDE_DIRECTORIES( sandbox ${INCLUDE_DIR} )
    
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)

    ADD_EXECUTABLE( sandbox ${sandbox_SOURCES} )

    TARGET_LINK_LIBRARIES( sandbox movie )
    
    set(
    test1_SOURCES
        ${EXAMPLES_DIR}/test/common.c
        ${EXAMPLES_DIR}/test/common.h
        ${EXAMPLES_DIR}/test/test1.c
    )

    INCLUDE_DIRECTORIES( test1 ${INCLUDE_DIR} )

    ADD_EXECUTABLE( test1 ${test1_SOURCES} )

    TARGET_LINK_LIBRARIES( test1 movie )    
    
    set(
    test2_SOURCES
        ${EXAMPLES_DIR}/test/common.c
        ${EXAMPLES_DIR}/test/common.h
        ${EXAMPLES_DIR}/test/test2.c
    )

    INCLUDE_DIRECTORIES( test2 ${INCLUDE_DIR} )

    ADD_EXECUTABLE( test2 ${test2_SOURCES} )

    TARGET_LINK_LIBRARIES( test2 movie )
    
    set(
    test3_SOURCES
        ${EXAMPLES_DIR}/test/common.c
        ${EXAMPLES_DIR}/test/common.h
        ${EXAMPLES_DIR}/test/test3.c
    )

    INCLUDE_DIRECTORIES( test3 ${INCLUDE_DIR} )

    ADD_EXECUTABLE( test3 ${test3_SOURCES} )

    TARGET_LINK_LIBRARIES( test3 movie )    
endif()