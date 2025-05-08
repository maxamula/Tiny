cbuffer ViewProjBuffer : register(b0)
{
    float4x4 view_proj;
    float3 camera_pos;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float2 coords : TEXCOORD0;
};

static const float3 pos[4] =
{
    float3(-1, 0, -1),
    float3(-1, 0, 1),
    float3(1, 0, -1),
    float3(1, 0, 1)
};

static const float grid_size = 10.0f;

VertexOut VSMain(uint vertexId : SV_VertexID)
{
    VertexOut output;
    float4 position = float4(pos[vertexId] * grid_size, 1.0f);
    position.xz += camera_pos.xz;
    output.position = mul(position, view_proj);
    output.coords = position.xz;
    return output;
}


#define mod(x,y) ((x) - (y) * floor((x)/(y)))

static const float cell_size = 1.0f;
static const float half_cell_size = cell_size * 0.5f;
static const float subcell_size = 0.1f;
static const float half_subcell_size = subcell_size * 0.5f;

static const float cell_line_thickness = 0.01f;
static const float subcell_line_thickness = 0.001f;

static const float4 cell_color = float4(0.75, 0.75, 0.75, 1.0f);
static const float4 subcell_color = float4(0.5, 0.5, 0.5, 1.0f);

static const float height_to_fade_distance_ratio = 25.0f;
static const float min_fade_distance = grid_size * 0.05f;
static const float max_fade_distance = grid_size * 2.5f;

float4 PSMain(VertexOut frag) : SV_Target
{
    float distance_to_camera = length(frag.coords - camera_pos.xz);
    float cell_size_ = cell_size * (distance_to_camera / height_to_fade_distance_ratio);
    float subcell_size_ = subcell_size * (distance_to_camera / height_to_fade_distance_ratio);
    
    float2 cell_coords = mod(frag.coords + half_cell_size, cell_size_);
    float2 subcell_coords = mod(frag.coords + half_subcell_size, subcell_size_);
    
    float2 distance_to_cell = abs(cell_coords - half_cell_size);
    float2 distance_to_subcell = abs(subcell_coords - half_subcell_size);
    
    float2 d = fwidth(frag.coords);
    float2 adjusted_cell_line_thickness = 0.5f * (cell_line_thickness + d);
    float2 adjusted_subcell_line_thickness = 0.5f * (subcell_line_thickness + d);
    
    float4 color = float4(0.0f, 1.0f, 0.0f, 1.0f);
    
    
    if (any(distance_to_subcell < adjusted_subcell_line_thickness))
    {
        color = subcell_color;
    }
    else if (any(distance_to_cell < adjusted_cell_line_thickness))
    {
        color = cell_color;
    }
    else
    {
        clip(-1);
    }
    
    return color;
}