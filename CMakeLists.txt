cmake_minimum_required(VERSION 3.30)
project(github_mirror C)

set(CMAKE_C_STANDARD 17)
include(FetchContent)

# CURL
find_package(CURL REQUIRED)

# CJSON
FetchContent_Declare(
        cjson
        GIT_REPOSITORY https://github.com/DaveGamble/cJSON
        GIT_TAG v1.7.18
        EXCLUDE_FROM_ALL
)
set(ENABLE_CJSON_TEST OFF CACHE BOOL "Enable cJSON tests" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
FetchContent_MakeAvailable(cjson)

# CMocka
find_package(cmocka REQUIRED)

# Python
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# GraphQL Queries
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB GRAPHQL_FILES "${CMAKE_CURRENT_SOURCE_DIR}/queries/*.graphql")
set(GENERATED_HEADERS "")

foreach (GQL_FILE ${GRAPHQL_FILES})
    get_filename_component(FILENAME ${GQL_FILE} NAME)
    if (NOT FILENAME STREQUAL "schema.graphql")
        get_filename_component(BASENAME ${GQL_FILE} NAME_WE)
        set(HEADER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/include/queries/${BASENAME}.h")
        add_custom_command(
                OUTPUT ${HEADER_FILE}
                COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/hack/graphql_to_header.py
                ${GQL_FILE}
                ${HEADER_FILE}
                ${BASENAME}
                DEPENDS ${GQL_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/hack/graphql_to_header.py
                COMMENT "Generating header ${HEADER_FILE} from ${GQL_FILE}"
        )
        list(APPEND GENERATED_HEADERS ${HEADER_FILE})
    endif ()
endforeach ()

add_custom_target(generate_graphql_headers ALL DEPENDS ${GENERATED_HEADERS})

# Executable
add_executable(github_mirror
        src/main.c
        src/github.c
        src/github.h
        src/buffer.c
        src/buffer.h
        ${GENERATED_HEADERS}
        src/github_types.h
        src/github_types.c
        src/config.c
        src/config.h
        src/git.c
        src/git.h
        src/precheck.c
        src/precheck.h
)
target_link_libraries(github_mirror PRIVATE cjson CURL::libcurl)
target_include_directories(github_mirror PRIVATE ${cjson_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_dependencies(github_mirror generate_graphql_headers)
install(TARGETS github_mirror DESTINATION bin)

# Compile Options
target_compile_options(github_mirror PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)

# Debug Compile Flags
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling flags for Debug build")
    set(SANITIZERS "address,undefined")
#    set(SANITIZERS "")

    target_compile_options(github_mirror PRIVATE
            -fno-omit-frame-pointer
            -fstack-protector-strong
            -fsanitize=${SANITIZERS}
    )
    target_link_options(github_mirror PRIVATE "-fsanitize=${SANITIZERS}")
endif ()

# Testing
enable_testing()

add_executable(test_buffer tests/test_buffer.c src/buffer.c)
target_link_libraries(test_buffer PRIVATE cmocka::cmocka)
target_compile_definitions(test_buffer PRIVATE TEST_ALLOC)

add_executable(test_config tests/test_config.c src/config.c)
target_link_libraries(test_config PRIVATE cmocka::cmocka)
target_compile_definitions(test_config PRIVATE TEST_ALLOC)

add_test(NAME test_buffer COMMAND test_buffer)
add_test(NAME test_config COMMAND test_config)

# Packaging
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "github_mirror")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_CONTACT "ansg191@anshulg.com")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Anshul Gupta")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.28), libcurl4 (>= 7.64)")

set(CPACK_GENERATOR "DEB")
include(CPack)
