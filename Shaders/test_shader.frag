#version 450
#extension GL_EXT_nonuniform_qualifier : require

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
    mat4x4 andBrittnay;
} localColor[8];

layout(location = 0) out vec4 fragInColor;

void main() {
    fragInColor = vec4(1, 1, 1, 1);
}
