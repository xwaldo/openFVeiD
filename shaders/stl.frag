#version 330 core

in vec4 bPosition;
in vec3 bNormal;
in vec2 bUv;

in vec4 screenCoord;
in vec3 baryCoord;

out vec4 oFragColor;
uniform vec3 lightDir;
uniform vec3 eyePos;

uniform sampler2DShadow shadowTex;
uniform samplerCube skyTex;
uniform sampler2D occlusionTex;

uniform int shadowMode;
uniform vec3 solidColor;

uniform int wire;
uniform float edgeWidth;

// Mist
uniform int mistEnabled;
uniform float mistNear;
uniform float mistFar;
uniform vec3 mistColor;

in vec4 FragPosLightSpace;

void main(void)
{
  vec3 m_color = solidColor;
  vec3 normal = normalize(bNormal);
  
  float diffusal = max(dot(normal, -lightDir), 0.0);
  
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

  // draw wireframe
  if (wire == 1) {
    float edgeDist = min(min(baryCoord.x, baryCoord.y), baryCoord.z);
    if (edgeDist < edgeWidth) {
      oFragColor = vec4(0.2, 0.2, 0.2, 1.0);
    }
  }
}
