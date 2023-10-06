#include "LightPass_Helpers.fx"

//VARIABLES
//*********
float4x4 gWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 gMatrixViewProjInv;
float3 gEyePos = float3(0, 0, 0);
Light gCurrentLight;

//G-BUFFER DATA
Texture2D gTextureDiffuse;
Texture2D gTextureSpecular;
Texture2D gTextureNormal;
Texture2D gTextureDepth;


SamplerState gTextureSampler
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Wrap;// or Mirror or Clamp or Border
	AddressV = Wrap;// or Mirror or Clamp or Border
};

//FIRST PASS STATES
//*****************

DepthStencilState gDepthStencilState_FIRST
{
	DepthEnable = TRUE; //Depth-Testing enabled
	DepthWriteMask = ZERO; //Don't write to the DepthBuffer (Read-Only DSV)
	DepthFunc = GREATER; //If the source data is greater than the destination data, the comparison passes

	StencilEnable = TRUE; //Stencil-Testing enabled
	FrontFaceStencilFunc = ALWAYS; //Always pass the comparison
	FrontFaceStencilPass = DECR_SAT; //Decrement the stencil value by 1, and clamp the result.
};

RasterizerState gRasterizerState_FIRST
{
	CullMode = BACK;
	FillMode = SOLID;
};

BlendState gBlendState_FIRST
{
	BlendEnable[0] = false;
};


//SECOND PASS STATES
//******************
DepthStencilState gDepthStencilState_SECOND
{
	DepthEnable = TRUE; //Depth-Testing Enabled
	DepthWriteMask = ZERO; //Depth-Writing disabled (Read-Only DSV)
	DepthFunc = GREATER_EQUAL;  //If the source data is greater than or equal to the destination data, the comparison passes.

	StencilEnable = TRUE; //Stencil-Testing Enabled
	BackFaceStencilFunc = EQUAL; //If the source data is equal to the destination data, the comparison passes
	BackFaceStencilPass = KEEP; //Keep the existing stencil data
};

RasterizerState gRasterizerState_SECOND
{
	CullMode = FRONT;
	FillMode = SOLID;
	DepthClipEnable = FALSE;
};

BlendState gBlendState_SECOND
{
	BlendEnable[0] = true;
	SrcBlend = ONE;
	DestBlend = ONE;
	BlendOp = ADD;
};

//VS-PS IO
struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};


//VERTEX SHADER
//*************
VS_OUTPUT VS(float3 position:POSITION, float2 texCoord:TEXCOORD)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	output.Position = mul(float4(position,1), gWorldViewProjection);
	output.TexCoord = texCoord;

	return output;
}

struct PS_OUTPUT
{
	float4 Color: SV_TARGET0;
	float4 Bloom: SV_TARGET1;
};

//PIXEL SHADER
//************
[earlydepthstencil] //Execute Depth&Stencil tests before PS execution (not during as usual)
PS_OUTPUT PS(VS_OUTPUT input)
{
	int2 screenCoord = input.Position.xy;
	int3 loadCoord = int3(screenCoord, 0);

	float depth = gTextureDepth.Load(loadCoord).r;
	float textureWidth;
	float textureHeight;
	gTextureDepth.GetDimensions(textureWidth, textureHeight);
	float3 P = DepthToWorldPosition(depth, screenCoord, float2(textureWidth, textureHeight), gMatrixViewProjInv);

	float3 V = normalize(P - gEyePos);

	float3 diffuse = gTextureDiffuse.Load(loadCoord).rgb;
	float4 specular = gTextureSpecular.Load(loadCoord);
	float shininess = exp2(specular.a * 10.5f);
	float3 N = gTextureNormal.Load(loadCoord).xyz;

	Material mat = (Material)0;
	mat.Diffuse = diffuse;
	mat.Specular = specular.rgb;
	mat.Shininess = shininess;

	LightingResult result;
	if (gCurrentLight.Type == 0) {
		result = DoPointLighting(gCurrentLight, mat, V, N, P);
	}
	else {
		result = DoSpotLighting(gCurrentLight, mat, V, N, P);
	}
	
	PS_OUTPUT Out = (PS_OUTPUT)0;

	Out.Color = float4((mat.Diffuse * result.Diffuse) + (mat.Specular * result.Specular), 1.0);

	Out.Bloom = clamp(Out.Color - float4(1, 1, 1, 0), float4(0, 0, 0, 1), float4(10, 10, 10, 1));
	
	return Out;
}

technique11 Default
{
	//PASS ONE
	pass one
	{
		SetRasterizerState(gRasterizerState_FIRST);
		SetDepthStencilState(gDepthStencilState_FIRST, 1);
		SetBlendState(gBlendState_FIRST, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

		SetVertexShader( CompileShader ( vs_4_0, VS() ));
		SetGeometryShader( NULL );
		SetPixelShader( NULL );
	}

	//PASS TWO
	pass two
	{
		SetRasterizerState(gRasterizerState_SECOND);
		SetDepthStencilState(gDepthStencilState_SECOND, 1);
		SetBlendState(gBlendState_SECOND, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

		SetVertexShader(CompileShader (vs_4_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader (ps_4_0, PS()));
	}
}

