#version 330 core

in vec3 aPosition;
out vec2 rasterCoord;
out vec2 floorCoord;
out vec4 screenCoord;
out vec4 FragPosLightSpace;
out vec4 bPosition;

uniform mat4 projectionMatrix;
uniform mat4 modelMatrix;
uniform mat4 lightSpaceMatrix;
uniform vec3 eyePos;
uniform float grdTexSize;

void main(void)
{
    vec3 eye = eyePos.xyz;
    eye.y = 0.0;
    vec4 worldPos = vec4(aPosition + eye, 1.0);
    bPosition = vec4(aPosition.x, aPosition.y - eyePos.y, aPosition.z, 0.0);
    
    // Eliminating large world translation additions and matrix subtractions to resolve POV/ortho jittering.
    // In camera space: pos = R * (WorldPos - CameraPos) = R * (aPosition + eye - eyePos) = R * (aPosition - vec3(0, eyePos.y, 0))
    mat4 viewRot = modelMatrix;
    viewRot[3] = vec4(0.0, 0.0, 0.0, 1.0);
    vec3 cameraRelativePos = aPosition - vec3(0.0, eyePos.y, 0.0);
    gl_Position = projectionMatrix * viewRot * vec4(cameraRelativePos, 1.0);
    
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    rasterCoord = 0.1*(aPosition.zx+eye.zx+vec2(5.0, 5.0));
    floorCoord = (aPosition.zx+eye.zx+vec2(grdTexSize, grdTexSize))/(grdTexSize)-vec2(0.5, 0.5);
    screenCoord = gl_Position;
}
