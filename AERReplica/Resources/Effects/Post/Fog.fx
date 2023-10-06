//=============================================================================
//// Shader uses position and texture
//=============================================================================
SamplerState samPoint
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Mirror;
    AddressV = Mirror;
};

Texture2D gTexture;
Texture2D gBloomTexture;
Texture2D gNormalTexture;
Texture2D gDepthTexture;

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

//IN/OUT STRUCTS
//--------------
struct VS_INPUT
{
    float3 Position : POSITION;
	float2 TexCoord : TEXCOORD0;

};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD1;
};


//VERTEX SHADER
//-------------
PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;

	output.Position = float4(input.Position, 1.0);
	output.TexCoord = input.TexCoord;
	
	return output;
}

const float density = 1.5;
const float end = 700;
const float start = 5;

float LinearizeDepth(float depth, float near, float far)
{
    return near * far / (far + depth * (near - far));
}

//PIXEL SHADER
//------------
float4 PS(PS_INPUT input) : SV_Target
{
	float value = gDepthTexture.Sample(samPoint, input.TexCoord).r;
	float3 color = gTexture.Sample(samPoint, input.TexCoord).rgb;
    float d = LinearizeDepth(value, .1, 4500);
    float f = 1.0 - (end - d) / (end - start);
	
    float fog = 1.0 - 1.0 / (exp2(f * density));
	
    return float4(color + fog * float3(1.2, 0.7, 0.5), 1.0f);
}


//TECHNIQUE
//---------
technique11 Fog
{
    pass P0
    {          
		SetDepthStencilState(gDepthState, 0);
		SetRasterizerState(gRasterizerState);
		SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
    }
}

