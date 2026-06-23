cmake_minimum_required(VERSION 3.16)

project(wave_mpi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MPI REQUIRED)

add_library(wave_core
    src/core/grid.cpp
    src/core/wave_problem.cpp
    src/solver/thomas.cpp
    src/solver/cyclic_reduction.cpp
    src/serial/adi_serial.cpp
    src/mpi/decomposition.cpp
    src/mpi/mpi_x_sweep.cpp
    src/mpi/column_buffer.cpp
    src/mpi/communication_blocking.cpp
    src/mpi/communication_nonblocking.cpp
    src/mpi/mpi_y_sweep.cpp
    src/benchmark/metrics.cpp
    src/benchmark/result_writer.cpp
)

target_include_directories(wave_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(wave_core
    PUBLIC
        MPI::MPI_CXX
)

if(MSVC)
    target_compile_options(wave_core PRIVATE /W4 /O2)
else()
    target_compile_options(wave_core PRIVATE -Wall -Wextra -Wpedantic -O3)
endif()

add_executable(wave_serial src/main_serial.cpp)
target_link_libraries(wave_serial PRIVATE wave_core)

add_executable(wave_mpi src/main_mpi.cpp)
target_link_libraries(wave_mpi PRIVATE wave_core MPI::MPI_CXX)

enable_testing()

add_executable(test_thomas tests/test_thomas.cpp)
target_link_libraries(test_thomas PRIVATE wave_core)
add_test(NAME test_thomas COMMAND test_thomas)

add_executable(test_cyclic_reduction tests/test_cyclic_reduction.cpp)
target_link_libraries(test_cyclic_reduction PRIVATE wave_core)
add_test(NAME test_cyclic_reduction COMMAND test_cyclic_reduction)

add_executable(test_decomposition tests/test_decomposition.cpp)
target_link_libraries(test_decomposition PRIVATE wave_core)
add_test(NAME test_decomposition COMMAND test_decomposition)

add_executable(test_column_buffer tests/test_column_buffer.cpp)
target_link_libraries(test_column_buffer PRIVATE wave_core)
add_test(NAME test_column_buffer COMMAND test_column_buffer)

add_executable(test_metrics tests/test_metrics.cpp)
target_link_libraries(test_metrics PRIVATE wave_core)
add_test(NAME test_metrics COMMAND test_metrics)

add_executable(test_mpi_x_sweep tests/test_mpi_x_sweep.cpp)
target_link_libraries(test_mpi_x_sweep PRIVATE wave_core MPI::MPI_CXX)

add_executable(test_mpi_y_sweep tests/test_mpi_y_sweep.cpp)
target_link_libraries(test_mpi_y_sweep PRIVATE wave_core MPI::MPI_CXX)

add_executable(test_communication tests/test_communication.cpp)
target_link_libraries(test_communication PRIVATE wave_core MPI::MPI_CXX)
