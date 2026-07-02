#version 330 core

out vec4 oFragColor;
in vec4 bPosition;
in vec3 bNormal;
in vec3 color;
in vec2 bUv;
in vec4 screenCoord;
in vec4 FragPosLightSpace;

uniform vec3 lightDir;
uniform vec3 eyePos;

uniform sampler2DShadow shadowTex;
uniform sampler2D occlusionTex;
uniform int shadowMode;
uniform float trackShine;

// Mist
uniform int mistEnabled;
uniform float mistNear;
uniform float mistFar;
uniform vec3 mistColor;

void main(void)
{
    vec3 m_color = color;
    vec3 normal = normalize(bNormal);
    
    float diffusal = max(dot(normal, -lightDir), 0.0);
    
    // Clean CAD lighting model: Ambient + Diffuse
    // float occ = texture(occlusionTex, 0.5 * (screenCoord.xy / screenCoord.w + vec2(1.0, 1.0))).x;
    float ambient = 0.8;
    
    vec3 finalColor = (ambient + diffusal * 0.5) * m_color;
    
    // Apply mist
    if (mistEnabled != 0) {
        float dist = length(bPosition.xyz);
        float mistFactor = clamp((dist - mistNear) / (mistFar - mistNear), 0.0, 1.0);
        finalColor = mix(finalColor, mistColor, mistFactor);
    }
    
    oFragColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0);
}
