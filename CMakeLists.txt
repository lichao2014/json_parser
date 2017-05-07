cmake_minimum_required(VERSION 2.8)

project(json_parser)


aux_source_directory(. SRC)
file(GLOB INC *.h)

add_executable(json_parser ${SRC} ${INC})

