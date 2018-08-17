#version 450
#extension GL_ARB_separate_shader_objects : enable

// Images
layout(binding = 2) uniform sampler2D texSampler;

// Input
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in float inLife;

// Output
layout(location = 0) out vec4 outColor;

void main()
{
    vec4 col = texture(texSampler, fragTexCoord);
    outColor = vec4(col.x, col.y, col.z, inLife);
}