#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

// Images
layout(set = 1, binding = 1) uniform sampler2D texSampler;

// Input
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in float fragTexMul;

// Output
layout(location = 0) out vec4 outColor;

// Entry Points
void main()
{
    outColor = texture(texSampler, fragTexCoord * fragTexMul);
}