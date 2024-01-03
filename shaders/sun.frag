#version 450

layout(location = 0) in vec2 fragOffset;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec4 outColor;

struct PointLight {
    vec4 position;  // ignore w
    vec4 color;     // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 sunDirection;
    vec4 ambientLightColor;  // w is intensity
    PointLight pointLights[10];
    int numLights;
}
ubo;

layout(set = 1, binding = 0) uniform sampler2D image;

layout(push_constant) uniform Push {
    vec4 position;
    float radius;
}
push;

void main() {
    vec4 texColor = texture(image, uv);
    if (texColor.w <= 0.1) {
        discard;
    }

    outColor = vec4(texColor.rgb, texColor.w);
}
