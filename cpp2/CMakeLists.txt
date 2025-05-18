cmake_minimum_required(VERSION 3.5)
project(RobotNurseHelper_Zenbo)

option(USE_GPU "USE GPU" ON)
option(COMPILE_WITH_G_FLAG "COMPILE WITH G FLAG" ON)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#The three settings used by Qt
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

function(my_protobuf_generate_cpp PROTO_SRCS PROTO_HDRS PROTO_FILES)
  foreach(PROTO_FILE ${PROTO_FILES})
    # Extract the base name of the .proto file
    get_filename_component(PROTO_BASE ${PROTO_FILE} NAME_WE)

    # Define the output .pb.cc and .pb.h files
    set(PROTO_CC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_BASE}.pb.cc")
    set(PROTO_HH "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_BASE}.pb.h")

    # Run protoc to generate the C++ files
    add_custom_command(
      OUTPUT ${PROTO_CC} ${PROTO_HH}
      COMMAND protoc --cpp_out=${CMAKE_CURRENT_BINARY_DIR} --proto_path=${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILE}
      DEPENDS ${PROTO_FILE}
      COMMENT "Generating C++ protobuf files from ${PROTO_FILE}"
      VERBATIM
    )

    # Add the generated files to the output lists
    list(APPEND ${PROTO_SRCS} ${PROTO_CC})
    list(APPEND ${PROTO_HDRS} ${PROTO_HH})
  endforeach()

  # Set the output lists as variables in the parent scope
  set(${PROTO_SRCS} ${${PROTO_SRCS}} PARENT_SCOPE)
  set(${PROTO_HDRS} ${${PROTO_HDRS}} PARENT_SCOPE)
endfunction()


#set the custom path for the find_package
list(APPEND CMAKE_PREFIX_PATH "usr/local")

# use Mediapipe's Protobuf
set(MEDIAPIPE_DIR "$ENV{HOME}/mediapipe")
get_filename_component(MEDIAPIPE_DIRNAME "${MEDIAPIPE_DIR}" NAME)

#protocol buffer v3.19.1 is used by mediapipe
#The variable protobuf_source_dir is used in the libprotobuf.cmake file
#The protobuf_source_dir is used to find the protobuf source code
set(protobuf_source_dir "${MEDIAPIPE_DIR}/bazel-${MEDIAPIPE_DIRNAME}/external/com_google_protobuf")
include("${CMAKE_CURRENT_SOURCE_DIR}/libprotobuf.cmake")     #this is the modified cmake file provided by LibMP's author
set(PROTO_FILE RobotCommand.proto)
#Because I don't use the pre-built opencv, I don't have the protobuf_generate_cpp function. I need to define it by myself.
my_protobuf_generate_cpp(PROTO_SRC PROTO_HDR ${PROTO_FILE})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(OpenCV COMPONENTS core highgui videoio imgproc imgcodecs REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets Network Multimedia REQUIRED)   

file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
set(ZENBONURSEHELPER_BUILD_DIR "$ENV{HOME}/ZenboNurseHelper_build")
set(OLLAMA_HPP_INCLUDE_DIR "${ZENBONURSEHELPER_BUILD_DIR}/ollama-hpp/include")
set(ONNX_HPP_INCLUDE_DIR "${ZENBONURSEHELPER_BUILD_DIR}/onnxruntime-linux-x64-1.12.1/include")
set(SILERO_HPP_INCLUDE_DIR "${ZENBONURSEHELPER_BUILD_DIR}/silero-vad/examples/cpp")
set(SOURCES JPEG.cpp
            utility_directory.cpp
            utility_TimeRecorder.cpp
            utility_compute.cpp
            utility_string.cpp
            utility_csv.cpp
            main.cpp
            ${PROTO_SRC}
            mainwindow.cpp
            mainwindow.h
            mainwindow.ui
            ThreadPortAudio.cpp
            ThreadProcessImage.cpp
            ThreadTablet.cpp
            ThreadWhisper.cpp
            ThreadOllama.cpp
            SocketHandler.cpp
            SendMessageManager.cpp
            GetLandmarks.cpp
            LandmarkToRobotAction.cpp
            silero-vad-onnx.cpp
    )

# whisper.cpp v1.7.5
add_library(LIBWHISPER SHARED IMPORTED)
set(WHISPER_DIR $ENV{HOME}/ZenboNurseHelper_build/whisper.cpp)
set_target_properties(LIBWHISPER PROPERTIES
  IMPORTED_LOCATION "${WHISPER_DIR}/build/src/libwhisper.so")
target_include_directories(LIBWHISPER INTERFACE "${WHISPER_DIR}/include" "${WHISPER_DIR}/ggml/include" ${WHISPER_DIR}/examples)
set(WHISPER_EXAMPLE_SOURCES ${WHISPER_DIR}/examples/common.cpp)

add_executable(RobotNurseHelper ${SOURCES} ${HEADERS} ${WHISPER_EXAMPLE_SOURCES})



# LibMP
add_library(LIBMP SHARED IMPORTED)
set(LIBMP_TARGET_NAME "libmp")
set(LIBMP_BIN_DIR "${MEDIAPIPE_DIR}/bazel-bin/mediapipe/examples/desktop/${LIBMP_TARGET_NAME}")
message(STATUS "USE_GPU: ${USE_GPU}")
if(USE_GPU)
  set(LIBMP_SHARED_LIBRARY "${LIBMP_BIN_DIR}/${LIBMP_TARGET_NAME}_gpu.so")
  target_compile_definitions(RobotNurseHelper
  PRIVATE
    USE_GPU
  )
else()
  set(LIBMP_SHARED_LIBRARY "${LIBMP_BIN_DIR}/${LIBMP_TARGET_NAME}.so")
  message(STATUS "Enter USE CPU")
endif()
set_target_properties(LIBMP PROPERTIES
  IMPORTED_LOCATION "${LIBMP_SHARED_LIBRARY}"
)
set(LIBMP_HEADER_DIR ${MEDIAPIPE_DIR}/mediapipe/examples/desktop/libmp)
include_directories(${LIBMP_HEADER_DIR})

# whisper.cpp v1.7.5
#add_library(LIBWHISPER SHARED IMPORTED)
#set_target_properties(LIBWHISPER PROPERTIES
#  IMPORTED_LOCATION "$ENV{HOME}/ZenboNurseHelper_build/whisper.cpp/build/src/libwhisper.so")
#target_include_directories(LIBWHISPER INTERFACE "$ENV{HOME}/ZenboNurseHelper_build/whisper.cpp/include" "$ENV{HOME}/ZenboNurseHelper_build/whisper.cpp/ggml/include")

# MediaPipe Formats
file(GLOB_RECURSE MP_FORMATS_SRC "${MEDIAPIPE_DIR}/bazel-bin/mediapipe/framework/formats/*.cc")
add_library(MP_FORMATS STATIC ${MP_FORMATS_SRC})
target_include_directories(MP_FORMATS PUBLIC "${MEDIAPIPE_DIR}/bazel-bin")
target_link_libraries(MP_FORMATS libprotobuf)
target_link_libraries(RobotNurseHelper MP_FORMATS)
target_link_libraries(RobotNurseHelper LIBMP)

# ONNX
add_library(LIBONNX SHARED IMPORTED)
set(ONNX_DIR $ENV{HOME}/ZenboNurseHelper_build/onnxruntime-linux-x64-1.12.1)
set_target_properties(LIBONNX PROPERTIES
  IMPORTED_LOCATION "${ONNX_DIR}/lib/libonnxruntime.so")

#Enalbe the debug mode
if(COMPILE_WITH_G_FLAG)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")   
endif()


include_directories(${PROTOBUF_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${OLLAMA_HPP_INCLUDE_DIR} ${ONNX_HPP_INCLUDE_DIR} ${SILERO_HPP_INCLUDE_DIR})
set_target_properties(${TARGET_NAME} PROPERTIES "CMAKE_CXX_FLAGS" "${CMAKE_CXX_FLAGS}")

target_link_libraries(RobotNurseHelper LIBWHISPER
    Qt::Core Qt::Widgets Qt::Network
    Qt::Multimedia ${OpenCV_LIBS} portaudio LIBONNX)
