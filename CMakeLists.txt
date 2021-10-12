cmake_minimum_required(VERSION 3.10)

# set the project name
project (hardshell)

add_definitions (-D_GNU_SOURCE -D_LARGEFILE_SOURCE -O2 -Wall -Werror=vla)

include_directories ("src" "external/basic/")

set (TARGET_LIBS ${TARGET_LIBS})

set (external_src
external/basic/basic_pattern.c
external/basic/basic_hashtable.c
)

set (shell_src
src/main.c
src/parse.c
src/sandbox.c
src/shell.c
src/util.c
)

add_executable (hardshell ${shell_src} ${external_src} ${other_src})
target_link_libraries (hardshell ${TARGET_LIBS})

install(PROGRAMS $<TARGET_FILE:hardshell> DESTINATION bin)


