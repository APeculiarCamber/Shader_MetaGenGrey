#version 450
#extension GL_EXT_nonuniform_qualifier : require

/*
 GLOBAL DESCRIPTOR SET 0:
    -- Global terrain image
    -- Global time struct/float
    -- Global View matrix
  MAT DESCRIPTOR SET 1:
    -- MAT Color
    -- Mat-owner transforms
  LOCAL DESCRIPTOR SET 2:
    -- Local image
    -- Local color
*/
struct Tree {
    vec3 f;
    vec3 h;
};

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in mat4 spit;
// Instanced attributes
layout (location = 6) in uint entityIndex_i;
layout (location = 7) in int matIndex_i;

/*
                    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
                    { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3 },
                    { 2, 0, VK_FORMAT_R32_SINT, sizeof(float) * 5 },
                    // INSTANCE
                    { 3, 1, VK_FORMAT_R32_SINT, 0 },
                    { 4, 1, VK_FORMAT_R32_SINT, sizeof(uint32_t) },
*/
// TODO: detour for a metaprogramming auto-gen system?


layout(binding = 0, set = 0) uniform sampler2D terrainSampler;
layout(binding = 1, set = 0) uniform UBO {
    float uTime;
} ubo;

layout(binding = 2, set = 0) uniform CameraView {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
} camera;

layout(binding = 0, set = 1) uniform MatColor {
    vec3 col;
} matColor;
layout(binding = 1, set = 1) uniform Transform {
    mat4 model[1024]; // TODO: This is a notably limitation of the into buffer solution
} matEntities;

// TODO: these ones need to be arrayed
layout(binding = 0, set = 2) uniform sampler2D localImages[8];
layout(binding = 1, set = 2) uniform  LocalColor {
    vec3 col;
} localColor[8];

layout(location = 0) out vec3 fragInColor;

void main() {
    fragInColor = localColor[nonuniformEXT(matIndex_i)].col;
    gl_Position = matEntities.model[entityIndex_i] * vec4(inPos, 1.0);
}
