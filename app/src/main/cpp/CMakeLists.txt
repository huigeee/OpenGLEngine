cmake_minimum_required(VERSION 3.22.1)

project("myapplication")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add glm header-only library
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/glm)

# Add stb_image (header-only)
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb_image)

# Add libpng (explicitly STATIC, unaffected by BUILD_SHARED_LIBS)
add_library(png STATIC
    libpng/png.c
    libpng/pngerror.c
    libpng/pngget.c
    libpng/pngmem.c
    libpng/pngread.c
    libpng/pngrio.c
    libpng/pngrtran.c
    libpng/pngrutil.c
    libpng/pngset.c
    libpng/pngtrans.c
    libpng/pngwio.c
    libpng/pngwrite.c
    libpng/pngwtran.c
    libpng/pngwutil.c
)
target_include_directories(png PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libpng)

# Build assimp as SHARED — AGP will pick it up and package into APK
set(BUILD_SHARED_LIBS                     ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER             ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER             ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER            ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_COLLADA_IMPORTER         ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT                      ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS                    OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS             OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES                  OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL                        OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB                     ON  CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS             OFF CACHE BOOL "" FORCE)
add_subdirectory(assimp/assimp)

add_library(
    native-renderer
    SHARED
    native-renderer.cpp
    ogl/Scene.cpp
    ogl/MainScene.cpp
    ogl/utils/Camera.cpp
    ogl/utils/Matrix.cpp
    ogl/utils/Shader.cpp
    ogl/utils/ShaderManager.cpp
    ogl/utils/DebugQuad.cpp
    ogl/utils/ResourceLoader.cpp
    ogl/utils/EGLCore.cpp
    ogl/skybox/SkyBoxEffect.cpp
    ogl/effect/TAAEffect.cpp
    ogl/effect/BloomEffect.cpp
    ogl/effect/TonemapEffect.cpp
    ogl/effect/DoFEffect.cpp
    ogl/effect/FXAAEffect.cpp
    ogl/effect/FogEffect.cpp
    ogl/effect/VolumetricLightEffect.cpp
    ogl/shadow/ShadowEffect.cpp
    ogl/object/Object3D.cpp
    ogl/object/Triangle.cpp
    ogl/object/Cube.cpp
    ogl/object/Sphere.cpp
    ogl/object/Sphere2.cpp
    ogl/object/Cube2.cpp
    ogl/object/Plane.cpp
    ogl/object/ClothMesh.cpp
    ogl/object/GroundPlane.cpp
    ogl/material/Material.cpp
    ogl/material/UnlitMaterial.cpp
    ogl/material/LitMaterial.cpp
    ogl/material/PBRMaterial.cpp
    ogl/material/SimpleTextureMaterial.cpp
    ogl/material/DebugIBLMaterial.cpp
    ogl/material/IBLPBRMaterial.cpp
    ogl/material/ClothMaterial.cpp
    ogl/material/SkinnedMaterial.cpp
    ogl/physics/ClothSimulation.cpp
    ogl/physics/CollisionPrimitives.cpp
    ogl/model/ModelLoader.cpp
    ogl/model/ModelObject.cpp
    ogl/model/MeshNode.cpp
    ogl/model/SceneNode.cpp
    ogl/model/Animator.cpp
    ogl/particle/ParticleSystem.cpp
    ogl/particle/FireEffect.cpp
    ogl/particle/RainEffect.cpp
    ogl/particle/SnowEffect.cpp
    ogl/particle/CloudEffect.cpp
    ogl/tween/Tween.cpp
    stb_image/stb_image.cpp
)

# assimp sets CMAKE_DEBUG_POSTFIX="d" globally, strip it from our libraries
set_target_properties(native-renderer PROPERTIES DEBUG_POSTFIX "")
set_target_properties(assimp          PROPERTIES DEBUG_POSTFIX "")

find_library(log-lib log)
find_library(z-lib   z)

target_include_directories(
    native-renderer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/assimp/assimp/include
)

target_link_libraries(
    native-renderer
    ${log-lib}
    ${z-lib}
    EGL
    GLESv3
    android
    glm
    stb_image
    png
    assimp
)
