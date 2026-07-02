#version 330 core

out vec4 oFragColor;
uniform sampler2D rasterTex;
uniform sampler2D floorTex;
uniform sampler2DShadow shadowTex;
in vec2 rasterCoord;
in vec2 floorCoord;
in vec4 screenCoord;
in vec4 FragPosLightSpace;
in vec4 bPosition;

uniform float opacity;
uniform int border;
uniform int grid;
uniform int shadowMode;
uniform vec3 eyePos;
uniform vec3 floorColor;

// Mist
uniform int mistEnabled;
uniform float mistNear;
uniform float mistFar;
uniform vec3 mistColor;

void main(void)
{
    oFragColor = texture(floorTex, floorCoord) * vec4(floorColor, 1.0);
    oFragColor.w = opacity;
    
    // Border logic: outside main area, use mistColor directly
    if((floorCoord.x > 1 || floorCoord.y > 1 || floorCoord.x < 0 || floorCoord.y < 0) && border == 1)
    {
        oFragColor.xyz = mistColor;
    }
    
    if(grid == 1) oFragColor.xyz *= (texture(rasterTex, rasterCoord).x);
    
    // Standard Ambient lighting model
    vec3 finalColor = oFragColor.xyz * 0.8;
    
    // Apply mist
    if (mistEnabled != 0) {
        // bPosition is in camera-relative world space
        float dist = length(bPosition.xyz);
        float mistFactor = clamp((dist - mistNear) / (mistFar - mistNear), 0.0, 1.0);
        finalColor = mix(finalColor, mistColor, mistFactor);
    }

    oFragColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0);
}
