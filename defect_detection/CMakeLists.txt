cmake_minimum_required(VERSION 3.16)
project(defect_detection VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)

# MSVC: 将源文件视为 UTF-8 编码，解决中文注释/字符串编译问题
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ──────────────────────────────────────────────
# 依赖查找
# ──────────────────────────────────────────────
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui videoio imgcodecs)
find_package(onnxruntime QUIET)            # 可选：ONNX Runtime
find_package(spdlog QUIET)                 # 可选：高性能日志
find_package(yaml-cpp QUIET)               # 可选：YAML 配置解析
find_package(Threads REQUIRED)

# ──────────────────────────────────────────────
# 各模块源文件
# ──────────────────────────────────────────────
set(CAMERA_SRC    src/camera/camera.cpp)
set(PREPROC_SRC   src/preprocess/preprocessor.cpp)
set(INFERENCE_SRC src/inference/inferencer.cpp)
set(POSTPROC_SRC  src/postprocess/postprocessor.cpp)
set(COMM_SRC      src/communication/serial_client.cpp)
set(STATS_SRC     src/statistics/statistician.cpp)
set(UI_SRC        src/ui/ui_manager.cpp)
set(UTILS_SRC     src/utils/config.cpp
                   src/utils/logger.cpp
                   src/utils/timer.cpp)

# ──────────────────────────────────────────────
# 静态库目标
# ──────────────────────────────────────────────
add_library(defect_camera       STATIC ${CAMERA_SRC})
add_library(defect_preprocess   STATIC ${PREPROC_SRC})
add_library(defect_inference    STATIC ${INFERENCE_SRC})
add_library(defect_postprocess  STATIC ${POSTPROC_SRC})
add_library(defect_comm         STATIC ${COMM_SRC})
add_library(defect_statistics   STATIC ${STATS_SRC})
add_library(defect_ui           STATIC ${UI_SRC})
add_library(defect_utils        STATIC ${UTILS_SRC})

set(DEFECT_LIBS
    defect_camera
    defect_preprocess
    defect_inference
    defect_postprocess
    defect_comm
    defect_statistics
    defect_ui
    defect_utils
)

# ──────────────────────────────────────────────
# 包含目录
# ──────────────────────────────────────────────
foreach(lib ${DEFECT_LIBS})
    target_include_directories(${lib} PUBLIC include)
    target_include_directories(${lib} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()

# ──────────────────────────────────────────────
# 链接依赖
# ──────────────────────────────────────────────
foreach(lib ${DEFECT_LIBS})
    target_link_libraries(${lib} PUBLIC ${OpenCV_LIBS})
    if(onnxruntime_FOUND)
        target_link_libraries(${lib} PUBLIC onnxruntime::onnxruntime)
    endif()
    if(spdlog_FOUND)
        target_link_libraries(${lib} PUBLIC spdlog::spdlog)
    endif()
    if(yaml-cpp_FOUND)
        target_link_libraries(${lib} PUBLIC yaml-cpp)
    endif()
    target_link_libraries(${lib} PUBLIC Threads::Threads)
endforeach()

# 模块间依赖关系
target_link_libraries(defect_inference PUBLIC defect_preprocess defect_utils)
target_link_libraries(defect_postprocess PUBLIC defect_utils)
target_link_libraries(defect_statistics PUBLIC defect_utils)
target_link_libraries(defect_ui PUBLIC defect_postprocess defect_statistics)
target_link_libraries(defect_comm PUBLIC defect_utils)

# ──────────────────────────────────────────────
# 主程序
# ──────────────────────────────────────────────
add_executable(defect_detection src/main.cpp)
target_link_libraries(defect_detection PRIVATE ${DEFECT_LIBS})

# ──────────────────────────────────────────────
# 将配置文件和模型目录复制到构建目录
# ──────────────────────────────────────────────
file(COPY config/ DESTINATION ${CMAKE_BINARY_DIR}/config)
file(COPY models/ DESTINATION ${CMAKE_BINARY_DIR}/models)
