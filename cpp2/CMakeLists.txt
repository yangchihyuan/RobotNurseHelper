cmake_minimum_required(VERSION 3.5)
project(ZenboNurseHelper_MP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#The three settings used by Qt
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

#set the custom path for the find_package
list(APPEND CMAKE_PREFIX_PATH "usr/local")
#find_package(Protobuf REQUIRED) #It will use the deb installed protobuf 3.21.12-8.2. Does it cause a problem?
#try to use mediapipe's protobuf
# Protobuf
# use Protobuf bundled with LibMP
set(MEDIAPIPE_DIR "/home/$ENV{HOME}/mediapipe")
get_filename_component(MEDIAPIPE_DIRNAME "${MEDIAPIPE_DIR}" NAME)
#The version is 3.19.1
set(protobuf_source_dir "${MEDIAPIPE_DIR}/bazel-${MEDIAPIPE_DIRNAME}/external/com_google_protobuf")
include("${CMAKE_CURRENT_SOURCE_DIR}/libprotobuf.cmake")

find_package(OpenCV REQUIRED COMPONENTS core highgui videoio imgproc imgcodecs)
#There are both Qt5 and Qt6 in the system, but I use Qt6.
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Multimedia)   
#find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Multimedia)

set(PROTO_FILE ServerSend.proto)
#protobuf_generate_cpp(PROTO_SRC PROTO_HDR ${PROTO_FILE})     #where are the generated files?
file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
file(GLOB LIBMP_Header HEADERS $ENV{HOME}/chihyuan/mediapipe/mediapipe/examples/desktop/libmp/libmp.h)

# LibMP
add_library(LIBMP SHARED IMPORTED)
set(LIBMP_TARGET_NAME "libmp")
set(LIBMP_SOURCE_DIR "${MEDIAPIPE_DIR}/mediapipe/examples/desktop/${LIBMP_TARGET_NAME}")
set(LIBMP_BIN_DIR "${MEDIAPIPE_DIR}/bazel-bin/mediapipe/examples/desktop/${LIBMP_TARGET_NAME}")
set(LIBMP_SHARED_LIBRARY "${LIBMP_BIN_DIR}/${LIBMP_TARGET_NAME}.so")
set_target_properties(LIBMP PROPERTIES
IMPORTED_LOCATION "${LIBMP_SHARED_LIBRARY}"
INTERFACE_INCLUDE_DIRECTORIES "${LIBMP_SOURCE_DIR}"
)

# MediaPipe Formats
file(GLOB_RECURSE MP_FORMATS_SRC "${MEDIAPIPE_DIR}/bazel-bin/mediapipe/framework/formats/*.cc")
if(MSVC)
  # MSVC warning suppressions
  add_definitions(${MSVC_WARNINGS})
endif(MSVC)
add_library(MP_FORMATS STATIC ${MP_FORMATS_SRC})
target_include_directories(MP_FORMATS PUBLIC "${MEDIAPIPE_DIR}/bazel-bin")
target_link_libraries(MP_FORMATS libprotobuf)

#file(GLOB GGML_OBJECT $ENV{HOME}/whisper.cpp/ggml/src/*.o)
#file(GLOB EXAMPLES_OBJECT $ENV{HOME}/whisper.cpp/examples/*.o)
set(SOURCES JPEG.cpp
            utility_directory.cpp
            utility_TimeRecorder.cpp
            utility_compute.cpp
            utility_string.cpp
            utility_csv.cpp
            main.cpp
#            human_pose_estimator.cpp
#            Pose.cpp
#            ${PROTO_SRC}
            ServerSend.pb.cc
            ServerSend.pb.h
            mainwindow.cpp
            mainwindow.h
            mainwindow.ui
            ProcessImageThread.cpp
            SendCommandThread.cpp
            ProcessAudioThread.cpp
            SocketHandler.cpp
            ThreadTablet.cpp
    )

add_executable(ZenboNurseHelper ${SOURCES} ${HEADERS} ${LIBMP_Header})
target_link_libraries(ZenboNurseHelper MP_FORMATS)
target_link_libraries(ZenboNurseHelper LIBMP)

#enable debug information
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")   


include_directories(${PROTOBUF_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})
#get_filename_component(PROTO_HDR_DIRECTORY ${PROTO_HDR} DIRECTORY)
#include_directories(${PROTO_HDR_DIRECTORY})

set_target_properties(${TARGET_NAME} PROPERTIES "CMAKE_CXX_FLAGS" "${CMAKE_CXX_FLAGS}")

#target_link_libraries(ZenboNurseHelper PRIVATE ${PROTOBUF_LIBRARIES}
#    Qt::Widgets Qt::Network
#    Qt::Multimedia ${OpenCV_LIBS} portaudio LIBMP)

target_link_libraries(ZenboNurseHelper
    Qt::Widgets Qt::Network
    Qt::Multimedia ${OpenCV_LIBS} portaudio)