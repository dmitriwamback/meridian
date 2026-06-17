cmake_minimum_required(VERSION 4.1)
project(meridian)

set(CMAKE_CXX_STANDARD 20)

add_executable(meridian
    main.cpp
    src/memory/Internal.cpp
    src/math/ModelMatrix.cpp
    src/math/PerlinNoise.cpp
    src/core/buffer/IndexBuffer.cpp
    src/core/buffer/InstanceBuffer.cpp
    src/core/buffer/PushConstants.cpp
    src/core/buffer/UniformBuffer.cpp
    src/core/buffer/VertexBuffer.cpp
    src/scene/Renderer.cpp
    src/pipeline/PipelineManager.cpp
    src/pipeline/Pipeline.cpp
    src/pipeline/types/GBufferPipeline.cpp
    src/pipeline/types/StandardPipeline.cpp
    src/camera/Camera.cpp
    src/core/objects/Asset.cpp

    src/pipeline/types/bloom/BloomDownsamplePipeline.cpp
    src/pipeline/types/bloom/BloomUpsamplePipeline.cpp
    src/pipeline/types/CompositePipeline.cpp
)
target_link_libraries(meridian "/opt/homebrew/Cellar/glew/2.2.0_1/lib/libGLEW.dylib")
target_link_libraries(meridian "/opt/homebrew/Cellar/glfw/3.3.8/lib/libglfw.dylib")
target_link_libraries(meridian "/opt/homebrew/Cellar/assimp/5.4.3/lib/libassimp.dylib")
target_link_libraries(meridian "/opt/local/lib/libvulkan.1.dylib")
include_directories("/opt/homebrew/Cellar/assimp/5.4.3/include")
find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
target_link_libraries(meridian ${OPENGL_FRAMEWORK})

find_program(GLSLC glslc)
if(GLSLC)
    set(SHADER_SOURCES
        shaders/standard_vertex.vert
        shaders/standard_fragment.frag
        shaders/gbuffer_vertex.vert
        shaders/gbuffer_fragment.frag
        shaders/bloom_downsample.comp
        shaders/bloom_upsample.comp
        shaders/composite_vertex.vert
        shaders/composite_fragment.frag
    )

    set(SHADER_BINARIES)
    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
        set(SHADER_SPV "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER_NAME}.spv")

        add_custom_command(
                OUTPUT ${SHADER_SPV}
                COMMAND ${GLSLC} ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER} -o ${SHADER_SPV}
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
                COMMENT "Compiling shader ${SHADER}"
        )

        list(APPEND SHADER_BINARIES ${SHADER_SPV})
    endforeach()

    add_custom_target(Shaders DEPENDS ${SHADER_BINARIES})
    add_dependencies(meridian Shaders)
endif()