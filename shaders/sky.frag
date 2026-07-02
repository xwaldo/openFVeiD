#version 330 core

out vec4 oFragColor;
uniform samplerCube skyTex;
uniform int hasTexture;
uniform vec3 fallbackColor;
in vec3 texCoord;

void main(void)
{
    vec3 color;
    if (hasTexture != 0) {
        color = texture(skyTex, normalize(texCoord)).rgb;
    } else {
        color = fallbackColor;
    }
    
    // Apply gamma correction to match the rest of the engine
    oFragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
