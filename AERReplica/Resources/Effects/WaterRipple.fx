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
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
};

Texture2D gWaterNoise;

//VS IN & OUT
//***********
struct VS_Input
{
	float3 Position: POSITION;
	float3 Normal: NORMAL;
	float3 Tangent: TANGENT;
	float3 Binormal: BINORMAL;
	float2 TexCoord: TEXCOORD0;
	float3 Color: COLOR0;
};

struct VS_Output
{
	float4 Position: SV_POSITION;
	float3 Normal: NORMAL;
	float3 Tangent: TANGENT;
	float3 Binormal: BINORMAL;
	float2 TexCoord: TEXCOORD0;
	float3 Color: COLOR;
};

struct PS_Output
{
	float4 LightAccumulation : SV_TARGET0;
	float4 Diffuse : SV_TARGET1;
	float4 Specular : SV_TARGET2;
	float4 Normal : SV_TARGET3;
	float4 Bloom : SV_TARGET4;
};

Texture2D gRippleTexture;
float gTime;

// The main vertex shader
VS_Output MainVS(VS_Input input) {

	VS_Output output = (VS_Output)0;

	output.Position = mul(float4(input.Position, 1.0), gWorldViewProj);

	output.Normal = normalize(mul(input.Normal, (float3x3)gWorld));
	output.Tangent = normalize(mul(input.Tangent, (float3x3)gWorld));
	output.Binormal = normalize(mul(input.Binormal, (float3x3)gWorld));

	output.TexCoord = input.TexCoord;
    output.Color = input.Color;

	return output;
}

const float offset = 0.5;

// The main pixel shader
PS_Output MainPS(VS_Output input){

    float rate = gTime * 0.5;
	PS_Output output = (PS_Output)0;
    float2x2 rotator =
    {
        sin(rate), cos(rate), cos(rate), -sin(rate)
    };
	float2x2 rotator2 =
    {
       -sin(rate), cos(rate), cos(rate),  sin(rate)
    };
	//Fill GBuffer
    float red = gRippleTexture.Sample(gTextureSampler, mul(input.TexCoord - offset, rotator) + offset).r;
    float value = gRippleTexture.Sample(gTextureSampler, mul(input.TexCoord - offset, rotator2) + offset).b;

    float4 diffuse = 2.0 * red;
    diffuse = lerp(diffuse, float4(1.0, 1.0, 2.0, 1.0) * value, value);
	
    float alpha = diffuse.a;
    clip(alpha - 0.1f);

    output.Diffuse = diffuse;
	float4 ambient = 0.1;
	ambient *= diffuse;
    output.LightAccumulation = ambient;


    output.Normal = float4(input.Normal, 0.0);

    float3 specular = float3(0.2,0.5,1.0);
	float shininess = log2(0) / 10.5f;
	output.Specular = float4(specular, shininess);
	
	return output;
}

// Default Technique
technique10 Default {
	pass p0 {
		SetDepthStencilState(gDepthState, 0);
		SetRasterizerState(gRasterizerState);
		SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

		SetVertexShader(CompileShader(vs_4_0, MainVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, MainPS()));
	}
}