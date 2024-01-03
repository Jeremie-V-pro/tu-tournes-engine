#version 450

const vec3 LIGHT_WATER_COLOR = vec3(0.f, 0.324f, .7f);
const vec3 DARK_WATER_COLOR = vec3(0.f, 0.137f, 0.49f);

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragUV;
layout(location = 4) in vec4 lodScales;

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

layout(set = 1, binding = 0) uniform sampler2D displacement;
layout(set = 1, binding = 1) uniform sampler2D derivatives;
layout(set = 1, binding = 2) uniform sampler2D turbulence;

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

float map(float value, float minInput, float maxInput, float minOutput, float maxOutput) {
    return (value - minInput) / (maxInput - minInput) * (maxOutput - minOutput) + minOutput;
}

void main() {
    float lengthScale1 = 250;
    float lengthScale2 = 17;
    float lengthScale3 = 5;
    float modelheight = push.modelMatrix[3][1];
    float height = map(fragPosWorld.y, 0.15f, 0.35f, 0.0, 1.0);
    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLight = vec3(0.0);

    vec4 sumderivatives = texture(derivatives, fragUV / lengthScale1);
    sumderivatives += texture(derivatives2, fragUV / lengthScale2) * lodScales.y;
    sumderivatives += texture(derivatives3, fragUV / lengthScale3) * lodScales.z;

    vec2 slope = vec2(sumderivatives.x / (1 + sumderivatives.z), sumderivatives.y / (1 + sumderivatives.w));
    vec3 worldNormal = normalize(vec3(-slope.x, 1, -slope.y));
    vec3 surfaceNormal = normalize(mat3(push.normalMatrix) * vec3(worldNormal.x, -worldNormal.y, worldNormal.z));
    // vec3 surfaceNormal = normalize(normalderivatives1 + normalderivatives2 + normalderivatives3);

    vec3 cameraPosWorld = ubo.invView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

    // diffuse Sun
    vec3 sunDirectionNorm = normalize(vec3(ubo.sunDirection));
    float cosAngIncidence = max(dot(surfaceNormal, sunDirectionNorm), 0);
    diffuseLight += cosAngIncidence * 0.5f;
    // Specular Sun
    vec3 halfAngleSun = normalize(sunDirectionNorm + viewDirection);
    float blinnTermSun = dot(surfaceNormal, halfAngleSun) * 1.002f;
    blinnTermSun = clamp(blinnTermSun, 0, 1);
    blinnTermSun = pow(blinnTermSun, 4096.0);
    specularLight += blinnTermSun;

    for (int i = 0; i < ubo.numLights; i++) {
        PointLight light = ubo.pointLights[i];
        vec3 directionToLight = light.position.xyz - fragPosWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);  // distance squared
        directionToLight = normalize(directionToLight);

        float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
        vec3 intensity = light.color.xyz * light.color.w * attenuation;

        diffuseLight += intensity * cosAngIncidence;

        // specular lighting
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0, 1);
        blinnTerm = pow(blinnTerm, 64.0);  // higher values -> sharper highlight
        specularLight += intensity * blinnTerm;
    }
    float foam = texture(turbulence, fragUV / lengthScale1).x + texture(turbulence2, fragUV / lengthScale2).x +
                 texture(turbulence3, fragUV / lengthScale3).x;

    vec3 imageColor = mix(LIGHT_WATER_COLOR, DARK_WATER_COLOR, min(height + 0.6f, 1.f));

    foam = min(1.0, max(0.0, (-foam + 2.72) * 2));  // Adjust the parameters as needed

    // Add foam to the color
    vec3 foamColor = vec3(1.0, 1.0, 1.0);
    imageColor = mix(imageColor, foamColor, foam);

    outColor = vec4((diffuseLight * imageColor + specularLight * (imageColor + vec3(0.4f))), 0.98f);
}
