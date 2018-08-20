#version 450
#extension GL_ARB_separate_shader_objects : enable

// Images
layout(binding = 2) uniform sampler2D texSampler;

// Input
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in float inLife;

// Output
layout(location = 0) out vec4 outColor;

vec4 CalcControlAdditiveBlend(in vec4 color, in float burn)
{
    vec3 testCol =  color.rgb *  color.a;
    float alpha = color.a * (1.0f - burn);
    return vec4(testCol.rgb, alpha);
}

void main()
{
    vec4 col = texture(texSampler, fragTexCoord);

    col.a *= inLife;

    outColor = CalcControlAdditiveBlend(col, 1.0f);
}