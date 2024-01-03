#version 450

const vec2 OFFSETS[6] =
    vec2[](vec2(-2.0, -2.0), vec2(-2.0, 2.0), vec2(2.0, -2.0), vec2(2.0, -2.0), vec2(-2.0, 2.0), vec2(2.0, 2.0));

layout(location = 0) out vec2 fragOffset;

layout(location = 1) out vec2 uv;

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

layout(push_constant) uniform Push {
    vec4 position;
    float radius;
}
push;

void main() {
    fragOffset = OFFSETS[gl_VertexIndex];
    vec3 cameraRightWorld = {ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]};
    vec3 cameraUpWorld = {ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]};

    vec3 positionWorld =
        push.position.xyz + push.radius * fragOffset.x * cameraRightWorld + push.radius * fragOffset.y * cameraUpWorld;

    gl_Position = ubo.projection * ubo.view * vec4(positionWorld, 1.0);

    // convert -1..1 to 0..1 fragOffset
    fragOffset = (fragOffset + 2.0) / 2.0;
    uv = vec2(fragOffset.x / 2, -fragOffset.y / 2);
}