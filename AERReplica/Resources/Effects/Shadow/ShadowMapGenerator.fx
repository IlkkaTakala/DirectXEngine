float4x4 gWorld;
float4x4 gLightViewProj;
float4x4 gFarLightViewProj;
float4x4 gBones[100];
 
DepthStencilState depthStencilState
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
};

RasterizerState rasterizerState
{
	FillMode = SOLID;
	CullMode = FRONT;
};

//--------------------------------------------------------------------------------------
// Vertex Shader [STATIC]
//--------------------------------------------------------------------------------------
float4 ShadowMapVS(float3 position:POSITION):SV_POSITION
{
	float4 output = mul(float4(position, 1.0), mul(gWorld, gLightViewProj));
	return output;
}
float4 ShadowMapVSFar(float3 position:POSITION):SV_POSITION
{
	float4 output = mul(float4(position, 1.0), mul(gWorld, gFarLightViewProj));
	return output;
}

//--------------------------------------------------------------------------------------
// Vertex Shader [SKINNED]
//--------------------------------------------------------------------------------------
float4 ShadowMapVS_Skinned(float3 position:POSITION, float4 BoneIndices : BLENDINDICES, float4 BoneWeights : BLENDWEIGHTS) : SV_POSITION
{
    float lastWeight = 0.0f;
    float4 p = 0.0;

    //Blend vertex position & normal
    for (int i = 0; i < 4; ++i)
    {
        lastWeight += BoneWeights[i];
        p += BoneWeights[i] * mul(float4(position, 1.0), gBones[BoneIndices[i]]);
    }
    lastWeight = 1.0f - lastWeight;

    p += lastWeight * mul(float4(position, 1.0), gBones[BoneIndices[3]]);

    float4 output = mul(float4(p.xyz, 1.0f), mul(gWorld, gLightViewProj));
	//TODO: return the position of the ANIMATED vertex in correct space (hint: seen from the view of the light)
	return output;
}
 
//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------
void ShadowMapPS_VOID(float4 position:SV_POSITION){}

technique11 GenerateShadows
{
	pass P0
	{
		SetRasterizerState(rasterizerState);
	    SetDepthStencilState(depthStencilState, 0);
		SetVertexShader(CompileShader(vs_4_0, ShadowMapVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ShadowMapPS_VOID()));
	}
	pass P1
	{
		SetRasterizerState(rasterizerState);
	    SetDepthStencilState(depthStencilState, 0);
		SetVertexShader(CompileShader(vs_4_0, ShadowMapVSFar()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ShadowMapPS_VOID()));
	}
}

technique11 GenerateShadows_Skinned
{
	pass P0
	{
		SetRasterizerState(rasterizerState);
		SetDepthStencilState(depthStencilState, 0);
		SetVertexShader(CompileShader(vs_4_0, ShadowMapVS_Skinned()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ShadowMapPS_VOID()));
	}
}