#version 450
#extension GL_ARB_separate_shader_objects : enable

// Uniform Buffers
layout (set = 0, binding = 0) uniform       SCamUniBuffer
{
    mat4 view;
    mat4 proj;
} cam_ubo;

// Input
layout (location = 0) in vec3               inPosition;
layout (location = 1) in float              inLife;
layout (location = 2) in int                inTechId;

// Output
layout (location = 0) out float             outLife;
layout (location = 1) out int               outTechId;

// Entry Points
void main()
{
    gl_Position = cam_ubo.view * vec4(inPosition, 1.0f);
    outLife = inLife;
    outTechId = inTechId;
}