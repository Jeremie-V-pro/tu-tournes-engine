#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;


layout (location = 0) out vec4 outColor;

struct PointLight {
  vec4 position; // ignore w
  vec4 color; // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambiantLightColor; // w is intensity
    PointLight pointLights[10]; // a modifier quand la constante MAX_LIGHTS sera définie
    int numLights;
} ubo;


layout(push_constant) uniform Push{
    mat4 modelMatrix; //projection * view * model 
    mat4 normalMatrix;
} push;

void main(){

    vec3 diffuseLight = ubo.ambiantLightColor.xyz * ubo.ambiantLightColor.w;
    vec3 sepculareLight = vec3(0.0);
    vec3 surfaceNormal = normalize(fragNormalWorld);

    vec3 cameraPosWorld = ubo.invView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

    for(int i = 0; i < ubo.numLights; i++){
        PointLight light = ubo.pointLights[i];
        vec3 directionTolight = light.position.xyz - fragPosWorld;
        float attinuation = 1.0 / dot(directionTolight, directionTolight); // distance au carré
        directionTolight = normalize(directionTolight);

        float cosAnIncidence = max(dot(surfaceNormal, directionTolight), 0);
        vec3 intensity = light.color.xyz * light.color.w * attinuation;

        diffuseLight += intensity * cosAnIncidence;

        // speculare lighting
        vec3 halfAngle = normalize(directionTolight + viewDirection);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0, 1);
        blinnTerm = pow(blinnTerm, 32.0);
        sepculareLight += intensity * blinnTerm;
    }
    outColor = vec4(diffuseLight * fragColor + sepculareLight * fragColor, 1.0);
}