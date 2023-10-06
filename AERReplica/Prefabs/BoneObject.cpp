#include "stdafx.h"
#include "SceneGraph/GameObject.h"
#include "BoneObject.h"

BoneObject::BoneObject(BaseMaterial* pMaterial, float length)
{
	m_Length = length;
	m_pMaterial = pMaterial;
}

void BoneObject::AddBone(BoneObject* pBone)
{
	pBone->GetTransform()->Translate(m_Length, 0, 0);
	AddChild(pBone);
}

void BoneObject::CalculateBindPose()
{
	XMStoreFloat4x4(&m_BindPose, XMMatrixInverse(nullptr, XMLoadFloat4x4(&GetTransform()->GetWorld())));

	for (auto& c : GetChildren<BoneObject>()) {
		c->CalculateBindPose();
	}
}

void BoneObject::Initialize(const SceneContext&)
{
	auto pEmpty = new GameObject();
	AddChild(pEmpty);
	auto pModel = new ModelComponent(L"Meshes/Bone.ovm");
	pModel->SetMaterial(m_pMaterial);
	pEmpty->AddComponent(pModel);

	pEmpty->GetTransform()->Rotate(0, -90, 0);
	pEmpty->GetTransform()->Scale(m_Length);
}
