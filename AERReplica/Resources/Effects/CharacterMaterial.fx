//*************************//
// BASIC EFFECT [DEFERRED] //
//*************************//

/*
	- Diffuse Color/Texture
	- Specular Color
	- SpecularLevel Texture (Blinn)
	- Specular Intensity (Shininess)
	- NormalMap Texture
	- Ambient Color
	- Ambient Intensity [0-1]
	- Opacity Texture/Value [0-1]
*/

//GLOBAL MATRICES
//***************
// The World View Projection Matrix
float4x4 gWorldViewProj : WORLDVIEWPROJECTION;
// The ViewInverse Matrix - the third row contains the camera position!
float4x4 gViewInverse : VIEWINVERSE;
// The World Matrix
float4x4 gWorld : WORLD;

float4x4 gBones[100];
//STATES
//******
RasterizerState gRasterizerState
{
	FillMode = SOLID;
	CullMode = NONE;
};

BlendState gBlendState
{
	BlendEnable[0] = FALSE;
};

DepthStencilState gDepthState
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
};

//SAMPLER STATES
//**************
SamplerState gTextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	//Filter = ANISOTROPIC;
	AddressU = WRAP;
	AddressV = WRAP;
	AddressW = WRAP;
};

//DIFFUSE
//*******
Texture2D gDiffuseMap
<
	string UIName = "Diffuse Texture";
	string UIWidget = "Texture";
> ;

//SPECULAR
//********
float4 gSpecularColor
<
	string UIName = "Specular Color";
	string UIWidget = "Color";
> = float4(0.1, 0.1, 0.1, 1);

int gShininess <
	string UIName = "Shininess";
	string UIWidget = "Slider";
	float UIMin = 1;
	float UIMax = 100;
	float UIStep = 0.1f;
> = 0;

//AMBIENT
//*******
float4 gAmbientColor
<
	string UIName = "Ambient Color";
	string UIWidget = "Color";
> = float4(1.0, 1.0, 1.0, 1.0);

float gAmbientIntensity
<
	string UIName = "Ambient Intensity";
	string UIWidget = "slider";
	float UIMin = 0;
	float UIMax = 1;
> = 0.1f;

//OPACITY
//***************
float gOpacityLevel <
	string UIName = "Opacity";
	string UIWidget = "slider";
	float UIMin = 0;
	float UIMax = 1;
> = 1.0f;

float gWhiteout = 0.0;

Buffer<float3> vertices;

//VS IN & OUT
//***********
struct VS_Input
{
	float3 Position: POSITION;
	float3 Normal: NORMAL;
	float3 Tangent: TANGENT;
	float3 Binormal: BINORMAL;
	float2 TexCoord: TEXCOORD0;
	float4 BoneIndices : BLENDINDICES;
	float4 BoneWeights : BLENDWEIGHTS;
};

struct VS_Output
{
	float4 Position: SV_POSITION;
	float3 Normal: NORMAL;
	float3 Tangent: TANGENT;
	float3 Binormal: BINORMAL;
	float2 TexCoord: TEXCOORD0;
};

struct PS_Output
{
	float4 LightAccumulation : SV_TARGET0;
	float4 Diffuse : SV_TARGET1;
	float4 Specular : SV_TARGET2;
	float4 Normal : SV_TARGET3;
};

// The main vertex shader
VS_Output MainVS(VS_Input input, uint vertexID : SV_VertexID)
{

	VS_Output output = (VS_Output)0;

    float lastWeight = 0.0f;
    float4 p = 0.0;
    float3 norm = 0.0;
    float3 pos = lerp(input.Position, vertices.Load(3 * vertexID + 0), gWhiteout);

    //Blend vertex position & normal
    for (int i = 0; i < 4; ++i)
    {
        lastWeight += input.BoneWeights[i];
        p += input.BoneWeights[i] * mul(float4(pos, 1.0), gBones[input.BoneIndices[i]]);
        norm += input.BoneWeights[i] * mul(input.Normal, (float3x3) gBones[input.BoneIndices[i]]);
    }
    lastWeight = 1.0f - lastWeight;

    p += lastWeight * mul(float4(pos, 1.0), gBones[input.BoneIndices[3]]);
    norm += lastWeight * mul(input.Normal, (float3x3) gBones[input.BoneIndices[3]]);

	output.Position = mul(float4(p.xyz, 1.0f), gWorldViewProj);

	output.Normal = normalize(mul(norm, (float3x3)gWorld));
	output.Tangent = normalize(mul(input.Tangent, (float3x3)gWorld));
	output.Binormal = normalize(mul(input.Binormal, (float3x3)gWorld));

	output.TexCoord = input.TexCoord;

	return output;
}

// The main pixel shader
PS_Output MainPS(VS_Output input){

	PS_Output output = (PS_Output)0;

	//Fill GBuffer
    float4 diffuse = lerp(gDiffuseMap.Sample(gTextureSampler, input.TexCoord), 50.0, gWhiteout);

	output.Diffuse = diffuse;

	float alpha = diffuse.a * gOpacityLevel;
	clip(alpha - 0.1f);

	float4 ambient = gAmbientColor;

	ambient *= diffuse;
	ambient *= gAmbientIntensity;

	output.LightAccumulation = ambient;

	float3 normal = input.Normal;
	output.Normal = float4(normal, 0.0);

	float3 specular = gSpecularColor.rgb;
	float shininess = log2(gShininess) / 10.5f;

	output.Specular = float4(specular, shininess);

	return output;
}

// Default Technique
technique11 Default {
	pass p0 {
		SetDepthStencilState(gDepthState, 0);
		SetRasterizerState(gRasterizerState);
		SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

		SetVertexShader(CompileShader(vs_4_0, MainVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, MainPS()));
	}
}