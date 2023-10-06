#include "stdafx.h"
#include "SkyMaterial.h"

SkyMaterial::SkyMaterial() :
	Material(L"Effects/SkyMaterial.fx")
{
}

void SkyMaterial::InitializeEffectVariables()
{
}

void SkyMaterial::OnUpdateModelVariables(const SceneContext&, const ModelComponent*) const
{
}
