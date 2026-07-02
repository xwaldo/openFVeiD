#version 330 core
out vec4 oFragColor;
void main()
{
    // Uniform dark gray for the shadow
    oFragColor = vec4(0.2, 0.2, 0.2, 0.6);
}
