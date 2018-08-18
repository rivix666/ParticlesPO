#version 450
#extension GL_ARB_separate_shader_objects : enable

// Uniform Buffers
layout(binding = 0) uniform SCamUniBuffer
{
    mat4 view;
    mat4 proj;
} cam_ubo;

// Input
layout(location = 0) in vec3  inPosition;
layout(location = 1) in float inLife;

// Output
layout(location = 0) out float outLife;

void main()
{
    gl_Position = cam_ubo.proj * cam_ubo.view * vec4(inPosition, 1.0f);
    outLife = inLife;
}