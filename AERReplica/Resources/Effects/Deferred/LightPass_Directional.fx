//Deferred_DirectionalLightPas > Fullscreen Quad Render
#include "LightPass_Helpers.fx"

//VARIABLES
//*********
float4x4 gMatrixViewProjInv;
float3 gEyePos = float3(0, 0, 0);
Light gDirectionalLight;

//G-BUFFER DATA
//Texture2D gTextureAmbient; >> Already on Main RenderTarget
Texture2D gTextureDiffuse;
Texture2D gTextureSpecular;
Texture2D gTextureNormal;
Texture2D gTextureDepth;
Texture2D gShadowMap;
Texture2D gFarShadowMap;

float4x4 gViewProj_Light;
float4x4 gViewProj_LightFar;
float gShadowMapBias = 0.0005f;
float2 gShadowSize;

SamplerState gTextureSampler
{
	Filter = MIN_MAG_MIP_POINT;
    AddressU = Border;
    AddressV = Border;
    BorderColor = float4(1.0, 1.0, 1.0, 1.0);
};

SamplerComparisonState cmpSampler
{
	// sampler state
	Filter = COMPARISON_MIN_MAG_MIP_LINEAR;
	AddressU = Border;
	AddressV = Border;
	BorderColor = float4(1.0, 1.0, 1.0, 1.0);

	// sampler comparison state
	ComparisonFunc = LESS_EQUAL;
};

//VS & PS IO
//**********
struct VS_INPUT
{
	float3 Position: POSITION;
	float2 TexCoord: TEXCOORD;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

//STATES
//******
RasterizerState gRasterizerState
{
	FillMode = SOLID;
	CullMode = BACK;
};

DepthStencilState gDepthStencilState
{
	DepthEnable = FALSE;
	DepthWriteMask = ZERO;
};

BlendState gBlendState //Additive Blending (LIGHT-ACCUMULATION + LIGHTING-RESULTS)
{
	BlendEnable[0] = true;
	SrcBlend = ONE;
	DestBlend = ONE;
	BlendOp = ADD;
};

//VERTEX SHADER
//*************
VS_OUTPUT VS(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	output.Position = float4(input.Position, 1.0f);
	output.TexCoord = input.TexCoord;

	return output;
}

float map(float value, float min1, float max1, float min2, float max2)
{
	float perc = (value - min1) / (max1 - min1);
	value = perc * (max2 - min2) + min2;
	return value;
}

float2 texOffset(int u, int v)
{
	return float2(u * 1.0f / gShadowSize.x, v * 1.0f / gShadowSize.y);
}

float EvaluateShadowMap(float4 pos, float3 N)
{
    float4 lpos = mul(pos, gViewProj_Light);
    pos += float4(N, 0.0) * 0.3;
    float4 far_lpos = mul(pos, gViewProj_LightFar);
    bool useFar = false;
    float bias = gShadowMapBias;
	
	lpos.xyz /= lpos.w;
	far_lpos.xyz /= far_lpos.w;

    if (lpos.x < -1.0 || lpos.x > 1.0 || lpos.y < -1.0 || lpos.y > 1.0 || lpos.z > 0.9999)
    {
        lpos = far_lpos;
		useFar = true;
    }
	

	lpos.x = lpos.x / 2 + 0.5;
	lpos.y = lpos.y / -2 + 0.5;

	lpos.z -= bias;

	float sum = 0;
	float x, y;

	for (y = -1.5; y <= 1.5; y += 1.0)
	{
		for (x = -1.5; x <= 1.5; x += 1.0)
		{
            if (useFar)
                sum += gFarShadowMap.SampleCmpLevelZero(cmpSampler, lpos.xy + texOffset(x, y), lpos.z);
			else
                sum += gShadowMap.SampleCmpLevelZero(cmpSampler, lpos.xy + texOffset(x, y), lpos.z);
        }
	}

	float shadowFactor = sum / 16.0;

	return map(shadowFactor, 0.0, 1.0, 0.5, 1.0);
}

struct PS_OUTPUT
{
	float4 Color: SV_TARGET0;
	float4 Bloom: SV_TARGET1;
};

//PIXEL SHADER
//************
PS_OUTPUT PS(VS_OUTPUT input)
{
	//Directional LightPass Logic
	int2 screenCoord = input.Position.xy;
	int3 loadCoord = int3(screenCoord, 0);

	float depth = gTextureDepth.Load(loadCoord).r;
	float3 P = DepthToWorldPosition_QUAD(depth, input.TexCoord, gMatrixViewProjInv);

	float3 V = normalize(P - gEyePos);

	float3 diffuse = gTextureDiffuse.Load(loadCoord).rgb;
	float4 specular = gTextureSpecular.Load(loadCoord);
	float shininess = exp2(specular.a * 10.5f);
	float3 N = gTextureNormal.Load(loadCoord).xyz;
	float3 L = normalize(gDirectionalLight.Direction.xyz);

	Material mat = (Material)0;
	mat.Diffuse = diffuse;
	mat.Specular = specular.rgb;
	mat.Shininess = shininess;

	LightingResult result = DoDirectionalLighting(gDirectionalLight, mat, L, V, N);

    float shadowValue = EvaluateShadowMap(float4(P, 1.0f), N);
	
    shadowValue = lerp(0.0, shadowValue, saturate(-2 * gDirectionalLight.Direction.y));
    //if (abs(dot(N, L)) > 0.9)
    //    shadowValue = 1.0;

    PS_OUTPUT Out = (PS_OUTPUT) 0;
	
    float3 night = float3(0.0, 0.01, 0.05);

    Out.Color = lerp(float4((mat.Diffuse * result.Diffuse * night) + (mat.Specular * night), 1.0), 
	float4((mat.Diffuse * result.Diffuse * shadowValue) + (mat.Specular * result.Specular * shadowValue) + float3(0.3, 0.1, 0.2) * (1 - shadowValue), 1.0),
	shadowValue);
	
	Out.Bloom = clamp(Out.Color - float4(1,1,1,0), float4(0, 0, 0, 1), float4(10, 10, 10, 1));

	return Out;
}

//TECHNIQUE
//*********
technique11 Default
{
	pass P0
	{
		SetRasterizerState(gRasterizerState);
		SetDepthStencilState(gDepthStencilState, 0);
		SetBlendState(gBlendState, float4(0.f, 0.f, 0.f, 0.f), 0xFFFFFFFF);

		SetVertexShader(CompileShader(vs_4_0, VS() ));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS()));
	}
};
