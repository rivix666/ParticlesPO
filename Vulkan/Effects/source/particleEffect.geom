#version 450
#extension GL_ARB_separate_shader_objects : enable

// Uniform Buffers
layout(binding = 0) uniform SCamUniBuffer
{
    mat4 view;
    mat4 proj;
} cam_ubo;

// Input
// Describes what kind of primitives our shader should process
layout (points) in;
layout (location = 0) in float inLife[];

// Output
layout (triangle_strip, max_vertices = 4) out;
layout (location = 0) out vec2  geom_texcoord;
layout (location = 1) out float outLife;

// Const
const float SIZE = 0.1f;

void main()
{
    outLife = inLife[0];
    vec4 pos = gl_in[0].gl_Position;

    gl_Position = cam_ubo.proj * (pos + vec4(-SIZE, SIZE, 0.0f, 0.0f));
    geom_texcoord = vec2(-1.0f, 1.0f);
    EmitVertex();

    gl_Position = cam_ubo.proj * (pos + vec4(-SIZE, -SIZE, 0.0f, 0.0f));
    geom_texcoord = vec2(-1.0f, -1.0f);
    EmitVertex();

    gl_Position = cam_ubo.proj * (pos + vec4(SIZE, SIZE, 0.0f, 0.0f));
    geom_texcoord = vec2(1.0f, 1.0f);
    EmitVertex();

    gl_Position = cam_ubo.proj * (pos + vec4(SIZE, -SIZE, 0.0f, 0.0f));
    geom_texcoord = vec2(1.0f, -1.0f);
    EmitVertex();

    EndPrimitive();
}