#include "../include/ShaderManager.h"

const char* ShaderManager::getUnlitVertex() {
    return R"(
        #version 300 es
        uniform mat4 uModelMatrix;
        uniform mat4 uViewMatrix;
        uniform mat4 uProjectionMatrix;
        uniform mat4 uPrevModelMatrix;
        uniform mat4 uPrevViewMatrix;
        uniform mat4 uPrevProjectionMatrix;
        uniform vec2 uJitterOffset;
        layout(location = 0) in vec4 aPosition;
        layout(location = 1) in vec3 aNormal;
        out vec3 vColor;
        out vec3 vWorldPos;
        flat out vec4 nowScreenPos;
        flat out vec4 preScreenPos;
        void main() {
            vec4 worldPos = uModelMatrix * aPosition;
            vColor = aNormal;  // aNormal is used for color
            vWorldPos = worldPos.xyz;
            mat4 p = uProjectionMatrix;
            p[2][0] += uJitterOffset.x;
            p[2][1] += uJitterOffset.y;
            gl_Position = p * uViewMatrix * worldPos;
            nowScreenPos = uProjectionMatrix * uViewMatrix * worldPos;
            vec4 prevWorldPos = uPrevModelMatrix * aPosition;
            preScreenPos = uPrevProjectionMatrix * uPrevViewMatrix * prevWorldPos;
        }
    )";
}

const char* ShaderManager::getUnlitFragment() {
    return R"(
        #version 300 es
        precision mediump float;
        uniform vec4 uColor;
        uniform sampler2D uShadowMap;
        uniform mat4 uLightSpaceMatrix;
        uniform float uShadowEnabled;
        uniform float uShadowBias;
        uniform float uShadowPCFEnabled;
        uniform float uShadowIntensity;
        
        in vec3 vColor;
        in vec3 vWorldPos;
        flat in vec4 nowScreenPos;
        flat in vec4 preScreenPos;
        
        layout(location = 0) out vec4 fragColor;
        layout(location = 1) out vec2 velocity;
        
        // PCF soft shadow with 4-tap filtering
        float calculateShadowPCF(vec3 worldPos) {
            if (uShadowEnabled < 0.5) return 1.0;
            
            vec4 lightSpacePos = uLightSpaceMatrix * vec4(worldPos, 1.0);
            
            if (abs(lightSpacePos.w) < 0.0001) return 1.0;
            lightSpacePos.xyz /= lightSpacePos.w;
            
            vec3 projCoords = lightSpacePos.xyz * 0.5 + 0.5;
            
            // Check if in shadow map range
            if (projCoords.z < 0.0 || projCoords.z > 1.0 || 
                projCoords.x < 0.0 || projCoords.x > 1.0 || 
                projCoords.y < 0.0 || projCoords.y > 1.0) {
                return 1.0;  // Not in shadow (outside light frustum)
            }
            
            // Use offset for PCF sampling
            float offset = 0.0005;  // ~16 pixels at 2048 resolution
            
            // Sample 4 points and count lit samples using mix/step to avoid branch issues
            float litCount = 0.0;
            float currentDepth = projCoords.z - uShadowBias;
            
            // Use mix + step to avoid if-else branch (GPU may not optimize uniform bool branches)
            // step(a, b) returns 1.0 if b >= a, else 0.0
            litCount += mix(0.0, 1.0, step(currentDepth, texture(uShadowMap, projCoords.xy - vec2(offset)).r));
            litCount += mix(0.0, 1.0, step(currentDepth, texture(uShadowMap, projCoords.xy + vec2(offset, -offset)).r));
            litCount += mix(0.0, 1.0, step(currentDepth, texture(uShadowMap, projCoords.xy + vec2(-offset, offset)).r));
            litCount += mix(0.0, 1.0, step(currentDepth, texture(uShadowMap, projCoords.xy + vec2(offset)).r));
            
            // litCount = 0.0 to 4.0, divide by 4 to get 0.0 (full shadow) to 1.0 (full lit)
            float shadow = litCount * 0.25;
            
            // Apply intensity
            return mix(1.0, shadow, uShadowIntensity);
        }
        
        // Simple shadow calculation (no PCF)
        float calculateShadow(vec3 worldPos) {
            if (uShadowEnabled < 0.5) return 1.0;
            
            vec4 lightSpacePos = uLightSpaceMatrix * vec4(worldPos, 1.0);
            
            if (abs(lightSpacePos.w) < 0.0001) return 1.0;
            lightSpacePos.xyz /= lightSpacePos.w;
            
            vec3 projCoords = lightSpacePos.xyz * 0.5 + 0.5;
            
            // Check if in shadow map range
            if (projCoords.z < 0.0 || projCoords.z > 1.0 || 
                projCoords.x < 0.0 || projCoords.x > 1.0 || 
                projCoords.y < 0.0 || projCoords.y > 1.0) {
                return 1.0;  // Not in shadow (outside light frustum)
            }
            
            float closestDepth = texture(uShadowMap, projCoords.xy).r;
            float currentDepth = projCoords.z - uShadowBias;
            
            // If current depth > closest depth, we're BEHIND the object -> IN SHADOW (return 0.0)
            // If current depth <= closest depth, we're in front -> LIT (return 1.0)
            float shadow = (currentDepth > closestDepth) ? 0.0 : 1.0;
            
            // Apply intensity: mix between 1.0 (lit) and shadow value
            return mix(1.0, shadow, uShadowIntensity);
        }
        
        void main() {
            float shadow = 1.0;
            if (uShadowPCFEnabled > 0.5) {
                 shadow = calculateShadowPCF(vWorldPos);
            } else {
                 shadow = calculateShadow(vWorldPos);
            }
            
            // Final color: multiply by shadow value
            vec3 finalColor = uColor.rgb * shadow;
            fragColor = vec4(finalColor, uColor.a);
            
            // Calculate velocity for TAA
            vec2 newPos = ((nowScreenPos.xy / nowScreenPos.w) * 0.5 + 0.5);
            vec2 prePos = ((preScreenPos.xy / preScreenPos.w) * 0.5 + 0.5);
            velocity = newPos - prePos;
        }
    )";
}

const char* ShaderManager::getLitVertex() {
    return R"(
        #version 300 es
        uniform mat4 uModelMatrix;
        uniform mat4 uViewMatrix;
        uniform mat4 uProjectionMatrix;
        uniform mat4 uPrevModelMatrix;
        uniform mat4 uPrevViewMatrix;
        uniform mat4 uPrevProjectionMatrix;
        uniform vec2 uJitterOffset;
        layout(location = 0) in vec4 aPosition;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec2 aTexCoord;
        out vec3 vNormal;
        out vec3 vWorldPos;
        out vec2 TexCoords;
        flat out vec4 nowScreenPos;
        flat out vec4 preScreenPos;
        void main() {
            vec4 worldPos = uModelMatrix * aPosition;
            vWorldPos = worldPos.xyz;
            vNormal = normalize((uModelMatrix * vec4(aNormal, 0.0))).xyz;
            TexCoords = aTexCoord;
            mat4 p = uProjectionMatrix;
            p[2][0] += uJitterOffset.x;
            p[2][1] += uJitterOffset.y;
            gl_Position = p * uViewMatrix * worldPos;
            nowScreenPos = uProjectionMatrix * uViewMatrix * worldPos;
            vec4 prevWorldPos = uPrevModelMatrix * aPosition;
            preScreenPos = uPrevProjectionMatrix * uPrevViewMatrix * prevWorldPos;
        }
    )";
}

const char* ShaderManager::getLitFragment() {
    return R"(
        #version 300 es
        precision mediump float;
        uniform vec3 uBaseColor;
        uniform vec3 uCameraPos;
        uniform vec3 uLightPos;
        uniform vec3 uLightColor;
        uniform vec3 uSH[9];
        uniform float uMetallic;
        uniform float uRoughness;
        uniform sampler2D uAlbedoMap;
        uniform sampler2D uMetallicMap;
        uniform sampler2D uRoughnessMap;
        uniform sampler2D uAOMap;
        uniform float uAlbedoMapEnabled;
        uniform float uMetallicMapEnabled;
        uniform float uRoughnessMapEnabled;
        uniform float uAOMapEnabled;
        in vec3 vNormal;
        in vec3 vWorldPos;
        in vec2 TexCoords;
        flat in vec4 nowScreenPos;
        flat in vec4 preScreenPos;
        layout(location = 0) out vec4 fragColor;
        layout(location = 1) out vec2 velocity;

        const float PI = 3.14159265359;
        const float PI4 = 12.5663706144;

        vec3 srgbToLinear(vec3 srgb) {
            return mix(srgb / 12.92, pow((srgb + 0.055) / 1.055, vec3(2.4)), step(0.04045, srgb));
        }

        vec3 irradianceFromSH(vec3 N) {
            const float c1 = 0.429043;
            const float c2 = 0.511664;
            const float c3 = 0.247708;
            const float c4 = 0.886227;
            float sx = N.x, sy = N.y, sz = N.z;
            return max(vec3(0.0),
                uSH[0] * c4 +
                uSH[1] * (2.0*c2*sx) +
                uSH[2] * (2.0*c2*sz) +
                uSH[3] * (2.0*c2*sy) +
                uSH[4] * (2.0*c1*sx*sy) +
                uSH[5] * (2.0*c1*sy*sz) +
                uSH[6] * (c3*(3.0*sz*sz - 1.0)) +
                uSH[7] * (2.0*c1*sx*sz) +
                uSH[8] * (c1*(sx*sx - sy*sy))
            );
        }

        void main() {
            vec3 albedo = uBaseColor;
            float metallic = uMetallic;
            float roughness = max(uRoughness, 0.04);
            float ao = 1.0;

            vec3 N = normalize(vNormal);
            vec3 V = normalize(uCameraPos - vWorldPos);
            float NdotV = max(dot(N, V), 0.0);
            vec3 F0 = vec3(0.04);
            F0 = mix(F0, albedo, metallic);

            if (uAlbedoMapEnabled > 0.5) {
                albedo = srgbToLinear(texture(uAlbedoMap, TexCoords).rgb);
            }
            if (uMetallicMapEnabled > 0.5) {
                metallic = texture(uMetallicMap, TexCoords).r;
            }
            if (uRoughnessMapEnabled > 0.5) {
                roughness = max(texture(uRoughnessMap, TexCoords).r, 0.04);
            }
            if (uAOMapEnabled > 0.5) {
                ao = texture(uAOMap, TexCoords).r;
            }

            vec3 irradiance = irradianceFromSH(N);
            vec3 ambient = irradiance * albedo * PI4 * (1.0 - metallic) * ao;

            vec3 Lo = vec3(0.0);
            {
                vec3 L = normalize(uLightPos - vWorldPos);
                vec3 H = normalize(V + L);
                float dist = length(uLightPos - vWorldPos);
                float atten = 1.0 / (dist * dist);
                vec3 radiance = uLightColor * atten;
                float NdotL = max(dot(N, L), 0.0);
                vec3 F0 = vec3(0.04);
                F0 = mix(F0, albedo, metallic);
                vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);
                vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
                Lo += (kD * albedo / PI + F) * radiance * NdotL;
            }

            vec3 color = ambient + Lo;
            color = color / (color + vec3(1.0));
            fragColor = vec4(pow(color, vec3(1.0/2.2)), 1.0);
            vec2 newPos = ((nowScreenPos.xy / nowScreenPos.w) * 0.5 + 0.5);
            vec2 prePos = ((preScreenPos.xy / preScreenPos.w) * 0.5 + 0.5);
            velocity = newPos - prePos;
        }
    )";
}

const char* ShaderManager::getPBRIBLVertex() {
    return R"(
        #version 300 es
        uniform mat4 uModelMatrix;
        uniform mat4 uViewMatrix;
        uniform mat4 uProjectionMatrix;
        uniform mat4 uPrevModelMatrix;
        uniform mat4 uPrevViewMatrix;
        uniform mat4 uPrevProjectionMatrix;
        uniform vec2 uJitterOffset;
        layout(location = 0) in vec4 aPosition;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec2 aTexCoord;
        out vec3 vNormal;
        out vec3 vWorldPos;
        out vec2 TexCoords;
        flat out vec4 nowScreenPos;
        flat out vec4 preScreenPos;
        void main() {
            vec4 worldPos = uModelMatrix * aPosition;
            vWorldPos = worldPos.xyz;
            vNormal = normalize(mat3(uModelMatrix) * aNormal);
            TexCoords = aTexCoord;
            mat4 p = uProjectionMatrix;
            p[2][0] += uJitterOffset.x;
            p[2][1] += uJitterOffset.y;
            gl_Position = p * uViewMatrix * worldPos;
            nowScreenPos = uProjectionMatrix * uViewMatrix * worldPos;
            vec4 prevWorldPos = uPrevModelMatrix * aPosition;
            preScreenPos = uPrevProjectionMatrix * uPrevViewMatrix * prevWorldPos;
        }
    )";
}

const char* ShaderManager::getPBRIBLFragment() {
    return R"(
        #version 300 es
        precision highp float;

        uniform vec3 uBaseColor;
        uniform float uMetallic;
        uniform float uRoughness;
        uniform vec3 uLightPos0;
        uniform vec3 uLightPos1;
        uniform vec3 uLightPos2;
        uniform vec3 uLightColor0;
        uniform vec3 uLightColor1;
        uniform vec3 uLightColor2;
        uniform vec3 uCameraPos;
        uniform samplerCube uIrradianceMap;
        uniform samplerCube uPrefilterCubemap;
        uniform sampler2D uBRDFLUT;
        uniform float uPrefiltLevels;
        uniform float uUseCubemap;
        uniform float uEnableDirectLight;
        uniform float uDebugMode;

        in vec3 vNormal;
        in vec3 vWorldPos;
        in vec2 TexCoords;
        flat in vec4 nowScreenPos;
        flat in vec4 preScreenPos;

        uniform sampler2D uAlbedoMap;
        uniform sampler2D uNormalMap;
        uniform sampler2D uMetallicMap;
        uniform sampler2D uRoughnessMap;
        uniform sampler2D uAOMap;
        uniform float uAlbedoMapEnabled;
        uniform float uNormalMapEnabled;
        uniform float uMetallicMapEnabled;
        uniform float uRoughnessMapEnabled;
        uniform float uAOMapEnabled;
        layout(location = 0) out vec4 fragColor;
        layout(location = 1) out vec2 velocity;

        const float PI = 3.14159265359;

        vec3 srgbToLinear(vec3 srgb) {
            return mix(srgb / 12.92, pow((srgb + 0.055) / 1.055, vec3(2.4)), step(0.04045, srgb));
        }

        float DistributionGGX(vec3 N, vec3 H, float roughness) {
            float a = roughness * roughness;
            float a2 = a * a;
            float NdotH = max(dot(N, H), 0.0);
            float NdotH2 = NdotH * NdotH;
            float nom = a2;
            float denom = (NdotH2 * (a2 - 1.0) + 1.0);
            denom = PI * denom * denom;
            return nom / denom;
        }

        float GeometrySchlickGGX(float NdotV, float roughness) {
            float a = roughness;
            float k = (a * a) / 2.0;
            return NdotV / (NdotV * (1.0 - k) + k);
        }

        float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
            float NdotV = max(dot(N, V), 0.0);
            float NdotL = max(dot(N, L), 0.0);
            return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
        }

        vec3 fresnelSchlick(float cosTheta, vec3 F0) {
            return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
        }

        vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
            return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
        }

        vec3 getNormalFromMap() {
            vec3 tangentNormal = texture(uNormalMap, TexCoords).xyz * 2.0 - 1.0;
            vec3 Q1 = dFdx(vWorldPos);
            vec3 Q2 = dFdy(vWorldPos);
            vec2 st1 = dFdx(TexCoords);
            vec2 st2 = dFdy(TexCoords);
            vec3 N = normalize(vNormal);
            vec3 T = normalize(Q1 * st2.y - Q2 * st1.y);
            vec3 B = -normalize(cross(N, T));
            mat3 TBN = mat3(T, B, N);
            return normalize(TBN * tangentNormal);
        }

        void main() {

            vec3 albedo;
            if (uAlbedoMapEnabled > 0.5) {
                albedo = pow(texture(uAlbedoMap, TexCoords).rgb, vec3(2.2));
            } else {
                albedo = pow(uBaseColor, vec3(2.2));
            }

            float metallic;
            if (uMetallicMapEnabled > 0.5) {
                metallic = texture(uMetallicMap, TexCoords).r;
            } else {
                metallic = uMetallic;
            }

            float roughness;
            if (uRoughnessMapEnabled > 0.5) {
                roughness = max(texture(uRoughnessMap, TexCoords).r, 0.04);
            } else {
                roughness = max(uRoughness, 0.04);
            }

            float ao = 1.0;
            if (uAOMapEnabled > 0.5) {
                ao = texture(uAOMap, TexCoords).r;
            }

            vec3 N = normalize(vNormal);
            if (uNormalMapEnabled > 0.5) {
                N = getNormalFromMap();
            }

            vec3 V = normalize(uCameraPos - vWorldPos);
            vec3 R = reflect(-V, N);
            float NdotV = max(dot(N, V), 0.0001);

            vec3 F0 = vec3(0.04);
            F0 = mix(F0, albedo, metallic);

            vec3 Lo = vec3(0.0);
            if (uEnableDirectLight > 0.5) {
                vec3 lightPositions[3];
                vec3 lightColors[3];
                lightPositions[0] = uLightPos0; lightColors[0] = uLightColor0;
                lightPositions[1] = uLightPos1; lightColors[1] = uLightColor1;
                lightPositions[2] = uLightPos2; lightColors[2] = uLightColor2;

                for (int i = 0; i < 3; ++i) {
                    vec3 L = normalize(lightPositions[i] - vWorldPos);
                    vec3 H = normalize(V + L);
                    float distance = length(lightPositions[i] - vWorldPos);
                    float attenuation = 1.0 / (distance * distance);
                    vec3 radiance = lightColors[i] * attenuation;

                    float NDF = DistributionGGX(N, H, roughness);
                    float G = GeometrySmith(N, V, L, roughness);
                    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

                    vec3 kS = F;
                    vec3 kD = vec3(1.0) - kS;
                    kD *= 1.0 - metallic;

                    vec3 numerator = NDF * G * F;
                    float denominator = 4.0 * NdotV * max(dot(N, L), 0.0) + 0.0001;
                    vec3 specular = numerator / denominator;

                    float NdotL = max(dot(N, L), 0.0);
                    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
                }
            }

            vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);

            vec3 kS_ibl = F;
            vec3 kD_ibl = vec3(1.0) - kS_ibl;
            kD_ibl *= 1.0 - metallic;

            vec3 irradiance = texture(uIrradianceMap, N).rgb;
            vec3 diffuse = irradiance * albedo;

            vec3 prefilteredColor = vec3(0.0);
            if (uUseCubemap > 0.5) {
                float lod = roughness * 4.0;
                prefilteredColor = textureLod(uPrefilterCubemap, R, lod).rgb;
            }

            vec2 brdf = texture(uBRDFLUT, vec2(NdotV, roughness)).rg;
            vec3 specular_ibl = prefilteredColor * (F * brdf.x + brdf.y);

            vec3 ambient = (kD_ibl * diffuse + specular_ibl) * ao;

            // Debug: show each component
//             fragColor = vec4(metallic, metallic, metallic, 1.0); return;              // diffuse IBL
//             fragColor = vec4(roughness, roughness, roughness, 1.0); return;              // diffuse IBL
//             fragColor = vec4(irradiance * 3.0, 1.0); return;              // diffuse IBL
//             fragColor = vec4(prefilteredColor * 3.0, 1.0); return;         // prefiltered IBL
//             fragColor = vec4(vec3(brdf, 0.0), 1.0); return;               // BRDF LUT
//             fragColor = vec4(vec3(uUseCubemap), 1.0); return;              // useCubemap value

            vec3 color = ambient + Lo;

            color = color / (color + vec3(1.0));
            color = pow(color, vec3(1.0 / 2.2));

            fragColor = vec4(color, 1.0);
            vec2 newPos = ((nowScreenPos.xy / nowScreenPos.w) * 0.5 + 0.5);
            vec2 prePos = ((preScreenPos.xy / preScreenPos.w) * 0.5 + 0.5);
            velocity = newPos - prePos;
        }
    )";
}

const char* ShaderManager::getPBRClearCoatIBLVertex() {
    return R"(
        #version 300 es
        uniform mat4 uModelMatrix;
        uniform mat4 uViewMatrix;
        uniform mat4 uProjectionMatrix;
        uniform mat4 uPrevModelMatrix;
        uniform mat4 uPrevViewMatrix;
        uniform mat4 uPrevProjectionMatrix;
        uniform vec2 uJitterOffset;
        layout(location = 0) in vec4 aPosition;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec2 aTexCoord;
        out vec3 vNormal;
        out vec3 vWorldPos;
        out vec2 TexCoords;
        flat out vec4 nowScreenPos;
        flat out vec4 preScreenPos;
        void main() {
            vec4 worldPos = uModelMatrix * aPosition;
            vWorldPos = worldPos.xyz;
            vNormal = normalize(mat3(uModelMatrix) * aNormal);
            TexCoords = aTexCoord;
            mat4 p = uProjectionMatrix;
            p[2][0] += uJitterOffset.x;
            p[2][1] += uJitterOffset.y;
            gl_Position = p * uViewMatrix * worldPos;
            nowScreenPos = uProjectionMatrix * uViewMatrix * worldPos;
            vec4 prevWorldPos = uPrevModelMatrix * aPosition;
            preScreenPos = uPrevProjectionMatrix * uPrevViewMatrix * prevWorldPos;
        }
    )";
}

const char* ShaderManager::getPBRClearCoatIBLFragment() {
    return R"(
        #version 300 es
        precision highp float;

        uniform vec3 uBaseColor;
        uniform float uMetallic;
        uniform float uRoughness;
        uniform vec3 uLightPos0;
        uniform vec3 uLightPos1;
        uniform vec3 uLightPos2;
        uniform vec3 uLightColor0;
        uniform vec3 uLightColor1;
        uniform vec3 uLightColor2;
        uniform vec3 uCameraPos;
        uniform float uClearCoat;
        uniform float uClearCoatRoughness;
        uniform samplerCube uIrradianceMap;
        uniform samplerCube uPrefilterCubemap;
        uniform sampler2D uBRDFLUT;
        uniform float uPrefiltLevels;
        uniform float uUseCubemap;
        uniform float uEnableDirectLight;
        uniform float uDebugMode;

        in vec3 vNormal;
        in vec3 vWorldPos;
        in vec2 TexCoords;
        flat in vec4 nowScreenPos;
        flat in vec4 preScreenPos;

        uniform sampler2D uAlbedoMap;
        uniform sampler2D uNormalMap;
        uniform sampler2D uMetallicMap;
        uniform sampler2D uRoughnessMap;
        uniform sampler2D uAOMap;
        uniform float uAlbedoMapEnabled;
        uniform float uNormalMapEnabled;
        uniform float uMetallicMapEnabled;
        uniform float uRoughnessMapEnabled;
        uniform float uAOMapEnabled;
        layout(location = 0) out vec4 fragColor;
        layout(location = 1) out vec2 velocity;

        const float PI = 3.14159265359;

        vec3 srgbToLinear(vec3 srgb) {
            return mix(srgb / 12.92, pow((srgb + 0.055) / 1.055, vec3(2.4)), step(0.04045, srgb));
        }

        float DistributionGGX(vec3 N, vec3 H, float roughness) {
            float a = roughness * roughness;
            float a2 = a * a;
            float NdotH = max(dot(N, H), 0.0);
            float NdotH2 = NdotH * NdotH;
            float nom = a2;
            float denom = (NdotH2 * (a2 - 1.0) + 1.0);
            denom = PI * denom * denom;
            return nom / denom;
        }

        float GeometrySchlickGGX(float NdotV, float roughness) {
            float a = roughness;
            float k = (a * a) / 2.0;
            return NdotV / (NdotV * (1.0 - k) + k);
        }

        float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
            float NdotV = max(dot(N, V), 0.0);
            float NdotL = max(dot(N, L), 0.0);
            return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
        }

        vec3 fresnelSchlick(float cosTheta, vec3 F0) {
            return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
        }

        vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
            return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
        }

        float SchlickFresnel(float u) {
            float m = clamp(1.0 - u, 0.0, 1.0);
            float m2 = m * m;
            return m2 * m2 * m;
        }

        vec3 getNormalFromMap() {
            vec3 tangentNormal = texture(uNormalMap, TexCoords).xyz * 2.0 - 1.0;
            vec3 Q1 = dFdx(vWorldPos);
            vec3 Q2 = dFdy(vWorldPos);
            vec2 st1 = dFdx(TexCoords);
            vec2 st2 = dFdy(TexCoords);
            vec3 N = normalize(vNormal);
            vec3 T = normalize(Q1 * st2.y - Q2 * st1.y);
            vec3 B = -normalize(cross(N, T));
            mat3 TBN = mat3(T, B, N);
            return normalize(TBN * tangentNormal);
        }

        void main() {

            vec3 albedo;
            if (uAlbedoMapEnabled > 0.5) {
                albedo = pow(texture(uAlbedoMap, TexCoords).rgb, vec3(2.2));
            } else {
                albedo = srgbToLinear(uBaseColor);
            }

            float metallic;
            if (uMetallicMapEnabled > 0.5) {
                metallic = texture(uMetallicMap, TexCoords).r;
            } else {
                metallic = uMetallic;
            }

            float roughness;
            if (uRoughnessMapEnabled > 0.5) {
                roughness = max(texture(uRoughnessMap, TexCoords).r, 0.04);
            } else {
                roughness = max(uRoughness, 0.04);
            }

            float ao = 1.0;
            if (uAOMapEnabled > 0.5) {
                ao = texture(uAOMap, TexCoords).r;
            }

            float cc = uClearCoat;
            float ccRoughness = max(uClearCoatRoughness, 0.04);

            vec3 N = normalize(vNormal);
            if (uNormalMapEnabled > 0.5) {
                N = getNormalFromMap();
            }

            vec3 V = normalize(uCameraPos - vWorldPos);
            vec3 R = reflect(-V, N);
            float NdotV = max(dot(N, V), 0.0001);

            vec3 F0 = vec3(0.04);
            F0 = mix(F0, albedo, metallic);

            float ccFresnel = mix(0.04, 1.0, SchlickFresnel(NdotV));

            vec3 Lo = vec3(0.0);
            if (uEnableDirectLight > 0.5) {
                vec3 lightPositions[3];
                vec3 lightColors[3];
                lightPositions[0] = uLightPos0; lightColors[0] = uLightColor0;
                lightPositions[1] = uLightPos1; lightColors[1] = uLightColor1;
                lightPositions[2] = uLightPos2; lightColors[2] = uLightColor2;

                for (int i = 0; i < 3; ++i) {
                    vec3 L = normalize(lightPositions[i] - vWorldPos);
                    vec3 H = normalize(V + L);
                    float distance = length(lightPositions[i] - vWorldPos);
                    float attenuation = 1.0 / (distance * distance);
                    vec3 radiance = lightColors[i] * attenuation;

                    float NDF = DistributionGGX(N, H, roughness);
                    float G = GeometrySmith(N, V, L, roughness);
                    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

                    vec3 kS = F;
                    vec3 kD = vec3(1.0) - kS;
                    kD *= 1.0 - metallic;

                    vec3 numerator = NDF * G * F;
                    float denominator = 4.0 * NdotV * max(dot(N, L), 0.0) + 0.0001;
                    vec3 specular = numerator / denominator;

                    float NdotL = max(dot(N, L), 0.0);
                    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
                }
            }

            vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);

            vec3 kS_ibl = F;
            vec3 kD_ibl = vec3(1.0) - kS_ibl;
            kD_ibl *= 1.0 - metallic;

            vec3 irradiance = texture(uIrradianceMap, N).rgb;
            vec3 diffuse = irradiance * albedo;

            vec3 prefilteredColor = vec3(0.0);
            if (uUseCubemap > 0.5) {
                float lod = roughness * 4.0;
                prefilteredColor = textureLod(uPrefilterCubemap, R, lod).rgb;
            }

            vec2 brdf = texture(uBRDFLUT, vec2(NdotV, roughness)).rg;
            vec3 specular_ibl = prefilteredColor * (F * brdf.x + brdf.y);

            vec3 ambient = (kD_ibl * diffuse + specular_ibl) * ao;

            vec3 color = ambient + Lo;
            vec3 kD_NoMetal = vec3(1.0) - F0;
            color *= kD_NoMetal * (1.0 - ccFresnel * cc);

            if (cc > 0.0) {
                vec3 ccPrefiltered = vec3(0.0);
                if (uUseCubemap > 0.5) {
                    float lod = ccRoughness * 4.0;
                    ccPrefiltered = textureLod(uPrefilterCubemap, R, lod).rgb;
                }
                vec2 ccBrdf = texture(uBRDFLUT, vec2(NdotV, ccRoughness)).xy;
                color += vec3(ccFresnel) * (ccPrefiltered * ccBrdf.x + ccBrdf.y) * cc;

                if (uEnableDirectLight > 0.5) {
                    vec3 L = normalize(uLightPos0 - vWorldPos);
                    float NdotL = max(dot(N, L), 0.0);
                    float distance = length(uLightPos0 - vWorldPos);
                    float attenuation = 1.0 / (distance * distance);
                    vec3 radiance = uLightColor0 * attenuation;
                    color += vec3(ccFresnel) * radiance * NdotL * cc;
                }
            }

            color = color / (color + vec3(1.0));
            color = pow(color, vec3(1.0 / 2.2));
            fragColor = mix(vec4(color, 1.0), vec4(srgbToLinear(uBaseColor), 1.0), clamp(uDebugMode, 0.0, 1.0));
            vec2 newPos = ((nowScreenPos.xy / nowScreenPos.w) * 0.5 + 0.5);
            vec2 prePos = ((preScreenPos.xy / preScreenPos.w) * 0.5 + 0.5);
            velocity = newPos - prePos;
        }
    )";
}

const char* ShaderManager::getDebugIBLVertex() {
    return R"(
        #version 300 es
        uniform mat4 uModelMatrix;
        uniform mat4 uViewMatrix;
        uniform mat4 uProjectionMatrix;
        uniform mat4 uPrevModelMatrix;
        uniform mat4 uPrevViewMatrix;
        uniform mat4 uPrevProjectionMatrix;
        uniform vec2 uJitterOffset;
        layout(location = 0) in vec4 aPosition;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec2 aTexCoord;
        out vec3 vNormal;
        out vec3 vWorldPos;
        out vec2 TexCoords;
        flat out vec4 nowScreenPos;
        flat out vec4 preScreenPos;
        void main() {
            vec4 worldPos = uModelMatrix * aPosition;
            vWorldPos = worldPos.xyz;
            vNormal = normalize((uModelMatrix * vec4(aNormal, 0.0)).xyz);
            TexCoords = aTexCoord;
            mat4 p = uProjectionMatrix;
            p[2][0] += uJitterOffset.x;
            p[2][1] += uJitterOffset.y;
            gl_Position = p * uViewMatrix * worldPos;
            nowScreenPos = uProjectionMatrix * uViewMatrix * worldPos;
            vec4 prevWorldPos = uPrevModelMatrix * aPosition;
            preScreenPos = uPrevProjectionMatrix * uPrevViewMatrix * prevWorldPos;
        }
    )";
}

const char* ShaderManager::getDebugIBLFragment() {
    return R"(
        #version 300 es
        precision highp float;
        uniform vec3 uBaseColor;
        uniform float uMetallic;
        uniform float uRoughness;
        uniform vec3 uCameraPos;
        uniform vec3 uSH[9];
        uniform float uAlbedoMapEnabled;
        uniform float uMetallicMapEnabled;
        uniform float uRoughnessMapEnabled;
        uniform float uAOMapEnabled;
        uniform sampler2D uAlbedoMap;
        uniform sampler2D uMetallicMap;
        uniform sampler2D uRoughnessMap;
        uniform sampler2D uAOMap;
        in vec3 vNormal;
        in vec3 vWorldPos;
        in vec2 TexCoords;
        flat in vec4 nowScreenPos;
        flat in vec4 preScreenPos;
        layout(location = 0) out vec4 fragColor;
        layout(location = 1) out vec2 velocity;
        const float PI = 3.14159265359;
        void main() {
            vec3 albedo = uBaseColor;
            if (uAlbedoMapEnabled > 0.5) {
                albedo = pow(texture(uAlbedoMap, TexCoords).rgb, vec3(2.2));
            }
            float metallic = uMetallic;
            if (uMetallicMapEnabled > 0.5) metallic = texture(uMetallicMap, TexCoords).r;
            float roughness = max(uRoughness, 0.04);
            if (uRoughnessMapEnabled > 0.5) roughness = max(texture(uRoughnessMap, TexCoords).r, 0.04);
            float ao = 1.0;
            if (uAOMapEnabled > 0.5) ao = texture(uAOMap, TexCoords).r;
            vec3 N = normalize(vNormal);
            vec3 V = normalize(uCameraPos - vWorldPos);
            float NdotV = max(dot(N, V), 0.0);
            vec3 F0 = vec3(0.04);
            F0 = mix(F0, albedo, metallic);
            vec3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);
            const float c1 = 0.429043;
            const float c2 = 0.511664;
            const float c3 = 0.247708;
            const float c4 = 0.886227;
            float sx = N.x, sy = N.y, sz = N.z;
            vec3 irradiance = max(vec3(0.0),
                uSH[0] * c4 +
                uSH[1] * (2.0*c2*sx) +
                uSH[2] * (2.0*c2*sz) +
                uSH[3] * (2.0*c2*sy) +
                uSH[4] * (2.0*c1*sx*sy) +
                uSH[5] * (2.0*c1*sy*sz) +
                uSH[6] * (c3*(3.0*sz*sz - 1.0)) +
                uSH[7] * (2.0*c1*sx*sz) +
                uSH[8] * (c1*(sx*sx - sy*sy))
            );
            vec3 ambient = irradiance * albedo * 0.31830988618 * (1.0 - metallic) * ao;
            vec3 color = ambient;
            color = color / (color + vec3(1.0));
            color = pow(color, vec3(1.0/2.2));
            fragColor = vec4(color, 1.0);
            vec2 newPos = ((nowScreenPos.xy / nowScreenPos.w) * 0.5 + 0.5);
            vec2 prePos = ((preScreenPos.xy / preScreenPos.w) * 0.5 + 0.5);
            velocity = newPos - prePos;
        }
    )";
}
