//Resharper Disable All

	#include "stdafx.h"
	#include "PostBloom.h"

PostBloom::PostBloom():
	PostProcessingMaterial(L"Effects/Post/BloomBlur.fx")
{
	
}

PostBloom::~PostBloom()
{
	SafeDelete(BlurTarget1);
	SafeDelete(BlurTarget2);
}

void PostBloom::Initialize(const GameContext& context)
{
	RENDERTARGET_DESC rtDesc{};
	const auto w = context.windowWidth;
	const auto h = context.windowHeight;
	rtDesc.width = w;
	rtDesc.height = h;
	rtDesc.colorFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;

	rtDesc.enableColorBuffer = true;
	rtDesc.enableColorSRV = true;
	rtDesc.enableDepthBuffer = false;

	BlurTarget1 = new RenderTarget(context.d3dContext);
	HANDLE_ERROR(BlurTarget1->Create(rtDesc))

		BlurTarget2 = new RenderTarget(context.d3dContext);
	HANDLE_ERROR(BlurTarget2->Create(rtDesc))
}

void PostBloom::Draw(const SceneContext& sceneContext, RenderTarget* pSource)
{
	PostProcessingMaterial::Draw(sceneContext, pSource);
	return;

	//UpdateBaseEffectVariables(sceneContext, pSource); //Update Base Effect variables
	//m_pBaseEffect->GetVariableByName("gBlur")->AsScalar()->SetBool(true);
	//auto blurTex = DeferredRenderer::Get()->GetBloomBuffer();
	//bool horizontal = false;
	//constexpr ID3D11ShaderResourceView* const pSRV[3] = { nullptr };
	//auto tex = blurTex;
	//for (int i = 0; i < 31; ++i) {
	//	m_pBaseEffect->GetVariableByName("gBloomTexture")->AsShaderResource()->SetResource(tex->GetColorShaderResourceView());
	//	m_pBaseEffect->GetVariableByName("gHorizontal")->AsScalar()->SetBool(horizontal);
	//	DrawPass(sceneContext, m_pBaseTechnique, horizontal ? BlurTarget1 : BlurTarget2); //Draw with Base Technique to Output RT
	//	sceneContext.d3dContext.pDeviceContext->PSSetShaderResources(0, 3, pSRV);
	//	tex = horizontal ? BlurTarget1 : BlurTarget2;
	//	horizontal = !horizontal;
	//}
	//m_pBaseEffect->GetVariableByName("gBlur")->AsScalar()->SetBool(false);

	//DrawPass(sceneContext, m_pBaseTechnique, m_pOutputTarget);


	////Release Source SRV from pipeline
	//sceneContext.d3dContext.pDeviceContext->PSSetShaderResources(0, 3, pSRV);
}
