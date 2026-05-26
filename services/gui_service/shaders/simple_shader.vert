#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 uv;
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 etPos;
layout(set = 0, binding = 0) uniform GlobalUbo{
    mat4 projectionViewMatrix;
    vec3 directionToLight;
    vec3 etLocation;
} ubo;
layout(push_constant) uniform Push{
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;
const float AMBIENT = 0.05;
void main(){
    gl_Position = ubo.projectionViewMatrix * push.modelMatrix * vec4(position, 1.0);
    vec4 worldPos = push.modelMatrix * vec4(position,1);
    fragPos = worldPos.xyz;
    etPos = ubo.etLocation;
    vec3 normalWorldSpace = normalize(mat3(push.normalMatrix) * normal);
    float lightIntensity = AMBIENT + max(dot(normalWorldSpace, ubo.directionToLight), 0);
    fragColor = lightIntensity * color;
}