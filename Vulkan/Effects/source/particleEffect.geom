#version 450
#extension GL_KHR_vulkan_glsl : enable

// Const
const int TEX_NUM = 7;
const int COLORS_ID = 6;

// Images
layout (set = 1, binding = 1) uniform sampler2D colSampler;

// Uniform Buffers
layout(set = 0, binding = 0) uniform SCamUniBuffer
{
    mat4 view;
    mat4 proj;
} cam_ubo;

layout(set = 1, binding = 2) uniform SParticleTexUniBuffer
{
    float atlas_width;
    float atlas_height;
} tex_ubo[7];

// Size of this array depends on registered techs
layout(set = 1, binding = 3) uniform SParticleTechUniBuffer
{
    float burn;
    float max_size;
    int   texture_id;
} tech_ubo[7]; 

// Describes what kind of primitives our shader should process
layout (points) in;

// Input
layout (location = 0) in float inLife[];
layout (location = 1) in int   inTechId[];

// Output
layout (triangle_strip, max_vertices = 4) out;
layout (location = 0) out vec2      geom_texcoord;
layout (location = 1) out float     outLife;
layout (location = 2) flat out int  outTechId;

// Functions
void CalcFrameCoordInTexAtlas(out vec2 texCoord[4], in vec2 texParam, in float frame)
{
    float texSizeX = 1.0f / texParam.r;
    float texSizeY = 1.0f / texParam.g;
    vec2  vIndex = vec2(floor(mod(frame, texParam.r)), floor(frame / texParam.g));
    vec4  vTexCoords = vec4(vIndex.x * texSizeX, vIndex.y * texSizeY, (vIndex.x + 1) * texSizeX, (vIndex.y + 1) * texSizeY);

    texCoord[0] = vec2(vTexCoords.x, vTexCoords.y);
    texCoord[1] = vec2(vTexCoords.z, vTexCoords.y);
    texCoord[2] = vec2(vTexCoords.x, vTexCoords.w);
    texCoord[3] = vec2(vTexCoords.z, vTexCoords.w);
}

float CalcColorsYCoord()
{
    float one_col_size = (1.0f / float(COLORS_ID));
    return one_col_size * float(tech_ubo[inTechId[0]].texture_id) + (one_col_size / 2.0);
}

// Entry Points
void main()
{
    // Prepare size variable
    float size_alpha = texture(colSampler, vec2(inLife[0], CalcColorsYCoord())).a;
    float size = (tech_ubo[inTechId[0]].max_size / 2.0f) * size_alpha;

    // Prepare used texture id
    int tex_id = tech_ubo[inTechId[0]].texture_id;

    // Prepare tex atlas variables
    float tex_atl_x = tex_ubo[tex_id].atlas_width;
    float tex_atl_y = tex_ubo[tex_id].atlas_height;

    // Calculate current particle frame and get uv coords for it
    vec2 texCoord[4];
    float frame = tex_atl_x * tex_atl_y * (1.0f - inLife[0]);
    CalcFrameCoordInTexAtlas(texCoord, vec2(tex_atl_x, tex_atl_y), frame);

    gl_Position = cam_ubo.proj * (gl_in[0].gl_Position + vec4(-size, size, 0.0f, 0.0f));
    geom_texcoord = texCoord[0];
    outLife = inLife[0];
    outTechId = inTechId[0];
    EmitVertex();
    
    gl_Position = cam_ubo.proj * (gl_in[0].gl_Position + vec4(size, size, 0.0f, 0.0f));
    geom_texcoord = texCoord[1];
    outLife = inLife[0];
    outTechId = inTechId[0];
    EmitVertex();
    
    gl_Position = cam_ubo.proj * (gl_in[0].gl_Position + vec4(-size, -size, 0.0f, 0.0f));
    geom_texcoord = texCoord[2];
    outLife = inLife[0];
    outTechId = inTechId[0];
    EmitVertex();
    
    gl_Position = cam_ubo.proj * (gl_in[0].gl_Position + vec4(size, -size, 0.0f, 0.0f));
    geom_texcoord = texCoord[3];
    outLife = inLife[0];
    outTechId = inTechId[0];
    EmitVertex();

    EndPrimitive();
}