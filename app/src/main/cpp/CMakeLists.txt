cmake_minimum_required(VERSION 3.22.1)

project("ogl")

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
    ogl/oglengine/Scene.cpp
    ogl/project/MainScene.cpp
    ogl/oglengine/utils/Camera.cpp
    ogl/oglengine/utils/Matrix.cpp
    ogl/oglengine/utils/Shader.cpp
    ogl/oglengine/utils/ShaderManager.cpp
    ogl/oglengine/utils/DebugQuad.cpp
    ogl/oglengine/utils/ResourceLoader.cpp
    ogl/oglengine/utils/EGLCore.cpp
    ogl/oglengine/skybox/SkyBoxEffect.cpp
    ogl/oglengine/effect/TAAEffect.cpp
    ogl/oglengine/effect/BloomEffect.cpp
    ogl/oglengine/effect/TonemapEffect.cpp
    ogl/oglengine/effect/DoFEffect.cpp
    ogl/oglengine/effect/FXAAEffect.cpp
    ogl/oglengine/effect/FogEffect.cpp
    ogl/oglengine/effect/VolumetricLightEffect.cpp
    ogl/oglengine/shadow/ShadowEffect.cpp
    ogl/oglengine/object/Object3D.cpp
    ogl/oglengine/object/Triangle.cpp
    ogl/oglengine/object/Cube.cpp
    ogl/oglengine/object/Sphere.cpp
    ogl/oglengine/object/Sphere2.cpp
    ogl/oglengine/object/Cube2.cpp
    ogl/oglengine/object/Plane.cpp
    ogl/oglengine/object/ClothMesh.cpp
    ogl/oglengine/object/GroundPlane.cpp
    ogl/oglengine/material/Material.cpp
    ogl/oglengine/material/UnlitMaterial.cpp
    ogl/oglengine/material/LitMaterial.cpp
    ogl/oglengine/material/PBRMaterial.cpp
    ogl/oglengine/material/SimpleTextureMaterial.cpp
    ogl/oglengine/material/DebugIBLMaterial.cpp
    ogl/oglengine/material/IBLPBRMaterial.cpp
    ogl/oglengine/material/ClothMaterial.cpp
    ogl/oglengine/material/SkinnedMaterial.cpp
    ogl/oglengine/physics/ClothSimulation.cpp
    ogl/oglengine/physics/CollisionPrimitives.cpp
    ogl/oglengine/model/ModelLoader.cpp
    ogl/oglengine/model/ModelObject.cpp
    ogl/oglengine/model/MeshNode.cpp
    ogl/oglengine/model/SceneNode.cpp
    ogl/oglengine/model/Animator.cpp
    ogl/oglengine/particle/ParticleSystem.cpp
    ogl/oglengine/particle/FireEffect.cpp
    ogl/oglengine/particle/RainEffect.cpp
    ogl/oglengine/particle/SnowEffect.cpp
    ogl/oglengine/particle/CloudEffect.cpp
    ogl/oglengine/particle/JetEffect.cpp
    ogl/project/customeffect/RippleEffect.cpp
    ogl/project/customeffect/CircleFlameEffect.cpp
    ogl/project/customeffect/Motorcycle.cpp
    ogl/oglengine/tween/Tween.cpp
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
