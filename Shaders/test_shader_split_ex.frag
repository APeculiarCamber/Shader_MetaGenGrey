#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(binding = 0, set = 0) uniform sampler2D terrainSampler;
layout(binding = 1, set = 0) uniform UBO {
    float uTime;
} ubo;

layout(binding = 0, set = 1) uniform MatColor {
    vec3 col;
} matColor;

// TODO: these ones need to be arrayed
layout(binding = 0, set = 2) uniform sampler2D localImages[8];

layout(location = 0) out vec4 fragInColor;

void main() {
    fragInColor = vec4(1, 1, 1, 1);
}
