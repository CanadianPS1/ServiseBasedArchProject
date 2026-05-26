#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 etPos;
layout(location = 0) out vec4 outColor;
layout(push_constant) uniform Push{
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;
vec3 fogColor = vec3(1.0,1.0,1.0);
float fogStart = 0.0;
float fogEnd = 9.0;
void main(){
    float distance = length(fragPos - etPos);
    float fogFactor = (fogEnd - distance) / (fogEnd - fogStart);
    fogFactor = clamp(fogFactor,0.0,1.0);
    vec3 finalColor = mix(fogColor,fragColor,fogFactor);
    outColor = vec4(finalColor,1.0);
}