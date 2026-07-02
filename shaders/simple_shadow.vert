#version 430 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 3) in mat4 aInstanceMatrix;
layout (location = 7) in vec4 aAttributes1;
layout (location = 8) in vec4 aAttributes2;

uniform mat4 projectionMatrix;
uniform mat4 modelMatrix;
uniform mat4 anchorBase;
uniform mat4 shadowMatrix;
uniform int isInstanced;
uniform float uTrackLength;
uniform float heartline;
uniform int isAsset;

struct SplineNode {
    vec4 pos;
    vec4 lat;
    vec4 norm;
    vec4 dir;
};

layout(std430, binding = 0) buffer SplineData {
    SplineNode nodes[];
};

void main()
{
    vec4 worldPos;
    if (isInstanced == 1) {
        // Spline-Warped Path for Assets and Ties
        float startDist = aInstanceMatrix[3].x;
        float minZ = aInstanceMatrix[3].z;
        
        float dist = startDist + (aPosition.z - minZ);
        dist = clamp(dist, 0.0, uTrackLength);
        
        float fIndex = dist / 0.1;
        int i0 = int(floor(fIndex));
        int i1 = i0 + 1;
        float t = fract(fIndex);
        
        vec3 iPos = mix(nodes[i0].pos.xyz, nodes[i1].pos.xyz, t);
        vec3 iLat = normalize(mix(nodes[i0].lat.xyz, nodes[i1].lat.xyz, t));
        vec3 iNorm = normalize(mix(nodes[i0].norm.xyz, nodes[i1].norm.xyz, t));
        
        float localX = aPosition.x * aInstanceMatrix[0][0];
        float localY = aPosition.y * aInstanceMatrix[1][1];
        
        float lateralOffset = aAttributes2.z;
        float normalOffset = aAttributes2.w;

        if (isAsset == 1) {
            normalOffset = 0.0;
        }

        bool shouldInvert = (heartline < 0.0);

        if (shouldInvert) {
            localX = -localX;
            localY = -localY;
            lateralOffset = -lateralOffset;
            normalOffset = -normalOffset;
        }

        float totalX = localX + lateralOffset;
        float totalY = localY + normalOffset;
        
        vec3 warpedPos = iPos - (iLat * totalX) - (iNorm * totalY);
        worldPos = anchorBase * vec4(warpedPos, 1.0);
    } else {
        // Standard Track Geometry
        worldPos = anchorBase * vec4(aPosition, 1.0);
    }
    
    // 1. Project WorldPos to Ground Plane (Shadow Space)
    vec4 shadowedWorldPos = shadowMatrix * worldPos;
    
    // 2. Transform to Camera View and Projection
    gl_Position = projectionMatrix * modelMatrix * shadowedWorldPos;
}
