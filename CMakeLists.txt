cmake_minimum_required(VERSION 3.25)
project(github-mirror VERSION 0.2.2 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
include(CheckCCompilerFlag)

# Custom compiler flags
set(custom_compiler_flags)
set(SANITIZERS "address,undefined")
list(APPEND custom_compiler_flags
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND custom_compiler_flags
            -fno-omit-frame-pointer
            -fstack-protector-strong
            -fsanitize=${SANITIZERS}
    )
endif ()

# CURL
find_package(CURL REQUIRED)

# CJSON
find_package(cJSON REQUIRED)
set(ENABLE_CJSON_TEST OFF CACHE BOOL "Enable cJSON tests" FORCE)

# CMocka
find_package(cmocka REQUIRED)

# Python
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# GraphQL Queries
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE GRAPHQL_FILES "${CMAKE_CURRENT_SOURCE_DIR}/queries/*.graphql")
set(GENERATED_HEADERS "")

foreach (GQL_FILE ${GRAPHQL_FILES})
    get_filename_component(FILENAME ${GQL_FILE} NAME)
    if (NOT FILENAME STREQUAL "schema.graphql")
        get_filename_component(DIR ${GQL_FILE} DIRECTORY)
        get_filename_component(DIR ${DIR} NAME)
        get_filename_component(BASENAME ${GQL_FILE} NAME_WE)
        set(HEADER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/include/queries/${DIR}/${BASENAME}.h")
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
add_executable(github-mirror
        src/main.c
        src/buffer.c
        src/config.c
        src/client.c
        src/git.c
        src/precheck.c
        src/github/client.c
        src/github/types.c
        src/srht/client.c
        src/srht/types.c
        ${GENERATED_HEADERS}
)
target_link_libraries(github-mirror PRIVATE cjson CURL::libcurl)
target_include_directories(github-mirror PRIVATE ${CJSON_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(github-mirror PRIVATE
        GITHUB_MIRROR_VERSION="${PROJECT_VERSION}"
)
add_dependencies(github-mirror generate_graphql_headers)
install(TARGETS github-mirror DESTINATION bin)

# Compile Options
foreach (compiler_flag ${custom_compiler_flags})
    CHECK_C_COMPILER_FLAG(${compiler_flag} "FLAG_SUPPORTED_${current_variable}")
    if (FLAG_SUPPORTED_${current_variable})
        message(STATUS "Compiler flag ${compiler_flag} is supported")
        target_compile_options(github-mirror PRIVATE ${compiler_flag})
    else ()
        message(WARNING "Compiler flag ${compiler_flag} is not supported")
    endif ()
endforeach ()

# Debug and Sanitizers is not empty
if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND SANITIZERS)
    target_link_options(github-mirror PRIVATE "-fsanitize=${SANITIZERS}")
endif ()

# Manpages
file(GLOB MANPAGES "${CMAKE_CURRENT_SOURCE_DIR}/man/*.1")
install(FILES ${MANPAGES} DESTINATION share/man/man1)
file(GLOB MANPAGES "${CMAKE_CURRENT_SOURCE_DIR}/man/*.5")
install(FILES ${MANPAGES} DESTINATION share/man/man5)

# Testing
enable_testing()

add_executable(test_buffer tests/test_buffer.c src/buffer.c)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(test_buffer PRIVATE cmocka::cmocka)
else ()
    target_link_libraries(test_buffer PRIVATE ${CMOCKA_LIBRARIES})
endif ()
target_compile_definitions(test_buffer PRIVATE
        TEST_ALLOC
        GITHUB_MIRROR_VERSION="${PROJECT_VERSION}"
)

add_executable(test_config tests/test_config.c src/config.c)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(test_config PRIVATE cmocka::cmocka)
else ()
    target_link_libraries(test_config PRIVATE ${CMOCKA_LIBRARIES})
endif ()
target_compile_definitions(test_config PRIVATE
        TEST_ALLOC
        GITHUB_MIRROR_VERSION="${PROJECT_VERSION}"
)

add_test(NAME test_buffer COMMAND test_buffer)
add_test(NAME test_config COMMAND test_config)

# Packaging
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "github-mirror")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "ansg191@anshulg.com")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Anshul Gupta")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.28), libcurl4 (>= 7.64), libcjson1 (>= 1.7.18)")

set(CPACK_GENERATOR "DEB")
include(CPack)
