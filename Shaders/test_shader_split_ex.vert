#version 450
#extension GL_EXT_nonuniform_qualifier : require


layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in mat4 spit;
// Instanced attributes
layout (location = 6) in uint entityIndex_i;
layout (location = 7) in int matIndex_i;

layout(binding = 1, set = 0) uniform UBO {
    float uTime;
} ubo;

layout(binding = 2, set = 0) uniform CameraView {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
} camera;

layout(binding = 1, set = 1) uniform Transform {
    mat4 model[1024]; // TODO: This is a notably limitation of the into buffer solution
} matEntities;

// TODO: these ones need to be arrayed
layout(binding = 1, set = 2) uniform  LocalColor {
    vec3 col;
    mat4x4 andBrittnay;
} localColor[8];

layout(location = 0) out vec3 fragInColor;

void main() {
    fragInColor = localColor[nonuniformEXT(matIndex_i)].col;
    gl_Position = matEntities.model[entityIndex_i] * vec4(inPos, 1.0);
}
