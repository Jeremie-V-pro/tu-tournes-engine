#version 450
const float LOD_SCALE = 7.13;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 fragUV;
layout(location = 4) out vec4 lodScales;

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

layout(set = 1, binding = 0) uniform sampler2D displacement1;
layout(set = 1, binding = 1) uniform sampler2D derivatives1;
layout(set = 1, binding = 2) uniform sampler2D turbulence1;

layout(set = 1, binding = 3) uniform sampler2D displacement2;
layout(set = 1, binding = 4) uniform sampler2D derivatives2;
layout(set = 1, binding = 5) uniform sampler2D turbulence2;

layout(set = 1, binding = 6) uniform sampler2D displacement3;
layout(set = 1, binding = 7) uniform sampler2D derivatives3;
layout(set = 1, binding = 8) uniform sampler2D turbulence3;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
}
push;

void main() {
    float lengthScale1 = 250;
    float lengthScale2 = 17;
    float lengthScale3 = 5;

    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);

    // Calculate world-space UV coordinates
    vec2 worldUV = vec2(positionWorld.x, positionWorld.z);

    // Calculate view vector
    vec3 viewVector = vec3(ubo.invView[3] - positionWorld);

    // Calculate view distance
    float viewDist = length(viewVector);

    // Calculate LOD scales
    float lod_c1 = min(LOD_SCALE * lengthScale1 / viewDist, 1);
    float lod_c2 = min(LOD_SCALE * lengthScale2 / viewDist, 1);
    float lod_c3 = min(LOD_SCALE * lengthScale3 / viewDist, 1);

    // Initialize displacement and largeWavesBias
    vec3 displacement = vec3(0.0);
    float largeWavesBias = 0.0;

    // Sample displacement textures and accumulate displacement
    displacement.xyz += vec3(texture(displacement1, worldUV / lengthScale1).xy * lod_c1,
                             texture(displacement1, worldUV / lengthScale1).z * lod_c1 * 2);
    largeWavesBias = displacement.z;
    displacement.xyz += vec3(texture(displacement2, worldUV / lengthScale2).xy * lod_c2,
                             texture(displacement2, worldUV / lengthScale2).z * lod_c2 * 2);
    displacement.xyz += vec3(texture(displacement3, worldUV / lengthScale3).xy * lod_c3,
                             texture(displacement3, worldUV / lengthScale3).z * lod_c3 * 2);

    // Update vertex position
    vec4 Finalposition = positionWorld + vec4(mat3(push.modelMatrix) * displacement.xzy, 1);

    // Output values
    gl_Position = ubo.projection * ubo.view * Finalposition;
    fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    fragPosWorld = Finalposition.xyz / 2.f;
    fragColor = color;
    fragUV = worldUV;
    lodScales = vec4(lod_c1, lod_c2, lod_c3, max(displacement.y - largeWavesBias * 0.8 + 0.1, 0) / 4.8);
}