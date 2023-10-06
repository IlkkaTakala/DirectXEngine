#include "stdafx.h"
#include "WaterMaterial.h"

WaterMaterial::WaterMaterial() :
	Material(L"Effects/WaterEffect.fx")
{
}

void WaterMaterial::InitializeEffectVariables()
{
	auto tex = ContentManager::Load<TextureData>(L"Textures/Islands/wateredge.png");
	SetVariable_Texture(L"gWaterNoise", tex);
}

void WaterMaterial::OnUpdateModelVariables(const SceneContext& /*sceneContext*/, const ModelComponent*) const
{
}

RippleMaterial::RippleMaterial() :
	Material(L"Effects/WaterRipple.fx")
{
}

void RippleMaterial::InitializeEffectVariables()
{

}

void RippleMaterial::OnUpdateModelVariables(const SceneContext&, const ModelComponent*) const
{
}
