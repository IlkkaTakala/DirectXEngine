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

RasterizerState gRasterizerState
{
	FillMode = SOLID;
	CullMode = BACK;
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


const float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

bool gHorizontal;
bool gBlur;

//PIXEL SHADER
//------------
float4 PS(PS_INPUT input) : SV_Target
{
	float x = 0.f;
	float y = 0.f;
	gTexture.GetDimensions(x, y);
	float dx = 1 / x;
	float dy = 1 / y;
	float3 final;
	for (int u = -2; u < 3; u++) {
		for (int v = -2; v < 3; v++) {
            float weight = (weights[abs(u)] + weights[abs(v)]) * 0.5;
            final += weight * gBloomTexture.Sample(samPoint, input.TexCoord + float2(u * dx * 2, v * dy * 2));
        }
	}
	//final /= 36;

	return gTexture.Sample(samPoint, input.TexCoord) + 0.6 * float4(final, 1.0f);
}


//TECHNIQUE
//---------
technique11 BloomBlur
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