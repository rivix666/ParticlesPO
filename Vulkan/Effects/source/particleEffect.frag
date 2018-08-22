#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

// Images
layout (set = 1, binding = 0) uniform sampler2D texSamplers[7];

// Uniform Buffers
// Size of this array depends on registered techs
layout (set = 1, binding = 2) uniform SParticleTechUniBuffer
{
    float burn;
    float max_size;
    int   texture_id;
} tech_ubo[2]; 

// Input
layout (location = 0) in vec2       inTexCoord;
layout (location = 1) in float      inLife;
layout (location = 2) flat in int   inTechId;

// Depth
layout (input_attachment_index = 0, set = 2, binding = 0) uniform subpassInput inputAttachment;

// Output
layout (location = 0) out vec4 outColor;

// Functions
vec4 CalcControlAdditiveBlend(in vec4 color, in float burn)
{
    vec3 testCol =  color.rgb *  color.a;
    float alpha = color.a * (1.0f - burn);
    return vec4(testCol.rgb, alpha);
}

float CalcSoftParticles(in float depth)
{
    // DxSoftPart
    /////////////////////////////////////////////////////////
    vec4 depthViewSample = vec4(gl_FragCoord.xy, depth, 1.0f);
    vec4 depthViewParticle = vec4(gl_FragCoord.xyz, 1.0f);
    float depthDiff = depthViewSample.z / depthViewSample.w - depthViewParticle.z / depthViewParticle.w;
    return clamp(depthDiff * 100.0f, 0.0f, 1.0f); // dzia³a przy mno¿eniu ale czemu? normalnie by³o / fadeFactor (czyli 1.0f)

    // Nvidia
    /////////////////////////////////////////////////////////
    //float Input = (depth - gl_FragCoord.w) / 1.0f;
    //float Output = 0.5 * pow( clamp( 2*(( Input > 0.5) ? 1-Input : Input), 0.0, 1.0 ), 2);
    //depth = ( Input > 0.5) ? 1-Output : Output;
}

// Entry Points
void main()
{
    // Sample texture under given id
    int tex_id = tech_ubo[inTechId].texture_id;
    vec4 col = texture(texSamplers[tex_id], inTexCoord);

    // Get previous depth value for current fragment
    float depthFade = CalcSoftParticles(subpassLoad(inputAttachment).x);

    // Modify alpha channel by particle life and depthFade
    col.a *= inLife;
    col.a *= depthFade;

    // Change colors with burn modifier
    col = CalcControlAdditiveBlend(col, tech_ubo[inTechId].burn);

    outColor = col;
}