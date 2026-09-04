cmake_minimum_required(VERSION 2.8.3)
project(fast_livo)

set(CMAKE_BUILD_TYPE "Release")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set common compile options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fexceptions")

# Specific settings for Debug build
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")

# Detect CPU architecture
message(STATUS "Current CPU architecture: ${CMAKE_SYSTEM_PROCESSOR}")

# Specific settings for Release build
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64|ARM|AARCH64)")
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    # 64-bit ARM optimizations (e.g., RK3588 and Jetson Orin NX)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -mcpu=native -mtune=native -ffast-math")
    message(STATUS "Using 64-bit ARM optimizations: -O3 -mcpu=native -mtune=native -ffast-math")
  else()
    # 32-bit ARM optimizations with NEON support
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -mcpu=native -mtune=native -mfpu=neon -ffast-math")
    message(STATUS "Using 32-bit ARM optimizations: -O3 -mcpu=native -mtune=native -mfpu=neon -ffast-math")
  endif()
  add_definitions(-DARM_ARCH)
else()
  # x86-64 (Intel/AMD) optimizations
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -mtune=native -mno-avx -funroll-loops") #-flto
  message(STATUS "Using general x86 optimizations: -O3 -march=native -mtune=native -mno-avx -funroll-loops")
  add_definitions(-DX86_ARCH)
endif()

# Debian's PCL binaries are built with Eigen's 16-byte ABI.  -march=native
# would otherwise select Eigen's 32-byte AVX allocator in this translation
# unit.  -mno-avx preserves SSE vectorization and keeps allocator ownership
# compatible with the system PCL binaries.
add_definitions(-DEIGEN_MAX_ALIGN_BYTES=16)

# Define project root directory
add_definitions(-DROOT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/\")

# Detect CPU core count for potential multithreading optimization
include(ProcessorCount)
ProcessorCount(N)
message(STATUS "Processor count: ${N}")

# Set the number of cores for multithreading
set(FAST_LIVO_MP_PROC_NUM "" CACHE STRING
    "Override native OpenMP worker count for diagnostics (default: host policy)")
if(FAST_LIVO_MP_PROC_NUM)
  set(PROC_NUM "${FAST_LIVO_MP_PROC_NUM}")
  add_definitions(-DMP_EN -DMP_PROC_NUM=${PROC_NUM})
  message(STATUS "Multithreading override. Cores: ${PROC_NUM}")
elseif(N GREATER 4)
  math(EXPR PROC_NUM "4")
  add_definitions(-DMP_EN -DMP_PROC_NUM=${PROC_NUM})
  message(STATUS "Multithreading enabled. Cores: ${PROC_NUM}")
elseif(N GREATER 1)
  math(EXPR PROC_NUM "${N}")
  add_definitions(-DMP_EN -DMP_PROC_NUM=${PROC_NUM})
  message(STATUS "Multithreading enabled. Cores: ${PROC_NUM}")
else()
  add_definitions(-DMP_PROC_NUM=1)
  message(STATUS "Single core detected. Multithreading disabled.")
endif()

# Diagnostic-only signatures are compiled out of the normal production build.
option(FAST_LIVO_NATIVE_DIAGNOSTICS
       "Emit diagnostic-only native visual first-divergence signatures" OFF)
if(FAST_LIVO_NATIVE_DIAGNOSTICS)
  add_definitions(-DFAST_LIVO_NATIVE_DIAGNOSTICS)
  message(STATUS "Native visual diagnostics enabled")
endif()

# Check for OpenMP support
find_package(OpenMP QUIET)
if(OpenMP_CXX_FOUND)
  message(STATUS "OpenMP found")
  add_compile_options(${OpenMP_CXX_FLAGS})
else()
  message(STATUS "OpenMP not found, proceeding without it")
endif()

# Check for mimalloc support
find_package(mimalloc QUIET)
if(mimalloc_FOUND)
  message(STATUS "mimalloc found")
else()
  message(STATUS "mimalloc not found, proceeding without it")
endif()

# Find catkin and required dependencies
find_package(catkin REQUIRED COMPONENTS
  geometry_msgs
  nav_msgs
  sensor_msgs
  roscpp
  rospy
  std_msgs
  pcl_ros
  tf
  message_generation
  eigen_conversions
  vikit_common
  vikit_ros
  cv_bridge
  image_transport
  rosbag
)

find_package(Eigen3 REQUIRED)
find_package(PCL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Sophus REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread)

set(Sophus_LIBRARIES libSophus.so)

# Define the catkin package
catkin_package(
  CATKIN_DEPENDS geometry_msgs nav_msgs roscpp rospy std_msgs message_runtime cv_bridge vikit_common vikit_ros image_transport rosbag
  DEPENDS EIGEN3 PCL OpenCV Sophus
)

# Include directories for dependencies
include_directories(
  ${catkin_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIR}
  ${PCL_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  ${Sophus_INCLUDE_DIRS}
  include
)

# Add libraries
add_library(vio src/vio.cpp src/frame.cpp src/visual_point.cpp)
add_library(lio src/voxel_map.cpp)
add_library(pre src/preprocess.cpp)
add_library(imu_proc src/IMU_Processing.cpp)
add_library(laser_mapping src/LIVMapper.cpp)

# The reader is transport-only.  It invokes the public callbacks and the one
# shared scheduler step; no estimator or map algorithm is linked twice.
add_library(native_offline_reader src/native_offline_reader.cpp)
target_link_libraries(native_offline_reader ${catkin_LIBRARIES})

# Add the main executable
add_executable(fastlivo_mapping src/main.cpp)

add_executable(fastlivo_native_offline tools/fastlivo_native_offline.cpp)
target_link_libraries(fastlivo_native_offline
  laser_mapping
  native_offline_reader
  vio
  lio
  pre
  imu_proc
  ${catkin_LIBRARIES}
  ${PCL_LIBRARIES}
  ${OpenCV_LIBRARIES}
  ${Sophus_LIBRARIES}
  ${Boost_LIBRARIES}
)

add_executable(native_offline_reader_tests tests/native_offline_reader_test.cpp)
target_link_libraries(native_offline_reader_tests
  native_offline_reader
  ${catkin_LIBRARIES}
)
add_test(NAME native_offline_reader_tests COMMAND native_offline_reader_tests)
add_test(NAME native_runner_status_tests
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/native_runner_status_test.sh)
add_test(NAME cached_mean_oracle_tests
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/cached_mean_oracle_test.sh)
option(FAST_LIVO_SANITIZER_TESTS
       "Build sanitizer-only native regression tests" OFF)
if(FAST_LIVO_SANITIZER_TESTS)
  add_executable(point_to_plane_init_tests tests/point_to_plane_init_test.cpp)
  target_link_libraries(point_to_plane_init_tests ${catkin_LIBRARIES})
  target_compile_options(point_to_plane_init_tests PRIVATE
    -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_options(point_to_plane_init_tests PRIVATE
    -fsanitize=undefined)
  add_test(NAME point_to_plane_init_tests COMMAND point_to_plane_init_tests)
endif()

# Link libraries to the executable
target_link_libraries(fastlivo_mapping
  laser_mapping
  vio
  lio
  pre
  imu_proc
  ${catkin_LIBRARIES}
  ${PCL_LIBRARIES}
  ${OpenCV_LIBRARIES}
  ${Sophus_LIBRARIES}
  ${Boost_LIBRARIES}
)

# Link mimalloc if found
if(mimalloc_FOUND)
  target_link_libraries(fastlivo_mapping mimalloc)
endif()
