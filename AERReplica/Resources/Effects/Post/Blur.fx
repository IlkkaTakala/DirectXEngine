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


//PIXEL SHADER
//------------
float4 PS(PS_INPUT input) : SV_Target
{
	// Step 1: find the dimensions of the texture (the texture has a method for that)	
	float x = 0.f;
	float y = 0.f;
	gTexture.GetDimensions(x, y);
	// Step 2: calculate dx and dy (UV space for 1 pixel)	
	float dx = 1 / x;
	float dy = 1 / y;
	// Step 3: Create a double for loop (5 iterations each)
	//		   Inside the loop, calculate the offset in each direction. Make sure not to take every pixel but move by 2 pixels each time
	//			Do a texture lookup using your previously calculated uv coordinates + the offset, and add to the final color
	float3 final;
	for (int u = -2; u < 3; u++) {
		for (int v = -2; v < 3; v++) {
			final += gTexture.Sample(samPoint, input.TexCoord + float2(u * dx * 2, v * dy * 2));
		}
	}
	// Step 4: Divide the final color by the number of passes (in this case 5*5)	
	final /= 25;
	// Step 5: return the final color

	return float4(final, 1.0f);
}


//TECHNIQUE
//---------
technique11 Blur
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