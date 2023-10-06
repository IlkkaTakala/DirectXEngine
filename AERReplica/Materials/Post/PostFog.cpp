//Resharper Disable All

	#include "stdafx.h"
	#include "PostFog.h"

	PostFog::PostFog():
		PostProcessingMaterial(L"Effects/Post/Fog.fx")
	{
	}

	void PostFog::UpdateBaseEffectVariables(const SceneContext& sceneContext, RenderTarget* pSource)
	{
		PostProcessingMaterial::UpdateBaseEffectVariables(sceneContext, pSource);

		m_pBaseEffect->GetVariableByName("gDepthTexture")->AsShaderResource()->SetResource(DeferredRenderer::Get()->GetBuffer()->GetDepthShaderResourceView());
	}


