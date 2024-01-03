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

layout(set = 2, binding = 0) uniform sampler2D displacement1;
layout(set = 2, binding = 1) uniform sampler2D derivatives;
layout(set = 2, binding = 2) uniform sampler2D turbulence;

layout(set = 2, binding = 3) uniform sampler2D displacement2;
layout(set = 2, binding = 4) uniform sampler2D derivatives2;
layout(set = 2, binding = 5) uniform sampler2D turbulence2;

layout(set = 2, binding = 6) uniform sampler2D displacement3;
layout(set = 2, binding = 7) uniform sampler2D derivatives3;
layout(set = 2, binding = 8) uniform sampler2D turbulence3;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
}
push;

void main() {
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);

    vec3 objectPos = vec3(push.modelMatrix[3][0], push.modelMatrix[3][1], push.modelMatrix[3][2]);

    // Calculate world-space UV coordinates
    vec2 worldUV = vec2(objectPos.xy);

    // Calculate view vector
    vec3 viewVector = vec3(ubo.invView[3] - positionWorld);

    // Calculate view distance
    float viewDist = length(viewVector);

    // Calculate LOD scales
    float lod_c1 = min(LOD_SCALE * 250 / viewDist, 1);
    float lod_c2 = min(LOD_SCALE * 17 / viewDist, 1);
    float lod_c3 = min(LOD_SCALE * 5 / viewDist, 1);

    // Initialize displacement and largeWavesBias
    float displacement = 0.0f;
    vec3 rotation = vec3(0.0f, 0.f, 0.f);
    float largeWavesBias = 0.0;

    // Sample displacement textures and accumulate displacement
    displacement += texture(displacement1, worldUV / 250 / 2).z * lod_c1;
    largeWavesBias = displacement;
    displacement += texture(displacement2, worldUV / 17 / 2).z * lod_c2;
    displacement += texture(displacement3, worldUV / 5 / 2).z * lod_c3;

    for (float i = 0; i < 1; i = i += 0.01f) {
        rotation += texture(derivatives, (worldUV + (-0.5f + i)) / 250 / 2).xyz * lod_c1;
        // rotation += texture(derivatives2, (worldUV+(-0.5f+i))/17).xyz * lod_c2;
        // rotation += texture(derivatives3, (worldUV+(-0.5f+i))/5).xyz * lod_c3;
    }
    rotation = rotation / 100.f;

    rotation = normalize(rotation) * 0.05f;

    // Transposer le point à l'origine du système de coordonnées temporaire (centre de rotation)
    vec3 translatedPosition = positionWorld.xyz - objectPos;

    // Appliquer la rotation autour de l'axe Z
    float cosThetaZ = cos(rotation.z);
    float sinThetaZ = sin(rotation.z);
    mat3 rotationMatrixZ = mat3(cosThetaZ, -sinThetaZ, 0.0, sinThetaZ, cosThetaZ, 0.0, 0.0, 0.0, 1.0);
    vec3 rotatedPositionZ = rotationMatrixZ * translatedPosition;

    // Appliquer la rotation autour de l'axe Y
    float cosThetaY = cos(rotation.y);
    float sinThetaY = sin(rotation.y);
    mat3 rotationMatrixY = mat3(cosThetaY, 0.0, sinThetaY, 0.0, 1.0, 0.0, -sinThetaY, 0.0, cosThetaY);
    vec3 rotatedPositionZY = rotationMatrixY * rotatedPositionZ;

    // Appliquer la rotation autour de l'axe X
    float cosThetaX = cos(rotation.x);
    float sinThetaX = sin(rotation.x);
    mat3 rotationMatrixX = mat3(1.0, 0.0, 0.0, 0.0, cosThetaX, -sinThetaX, 0.0, sinThetaX, cosThetaX);
    vec3 finalRotatedPosition = rotationMatrixX * rotatedPositionZY;

    // Re-transposer le point au centre de rotation initial
    vec3 finalPosition = finalRotatedPosition + objectPos;

    // Mettre à jour la positionWorld
    positionWorld.xyz = finalPosition;

    // Update vertex position
    vec4 Finalposition = positionWorld + vec4(0, displacement, 0, 0);

    gl_Position = ubo.projection * ubo.view * Finalposition;
    fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    fragPosWorld = positionWorld.xyz;
    fragColor = color;
    fragUV = uv;
}
