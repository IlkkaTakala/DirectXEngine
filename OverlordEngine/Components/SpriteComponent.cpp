#include "stdafx.h"
#include "SpriteComponent.h"

SpriteComponent::SpriteComponent(const std::wstring& spriteAsset, const XMFLOAT2& pivot, const XMFLOAT4& color):
	m_SpriteAsset(spriteAsset),
	m_Pivot(pivot),
	m_Color(color)
{}

void SpriteComponent::Initialize(const SceneContext& /*sceneContext*/)
{
	m_pTexture = ContentManager::Load<TextureData>(m_SpriteAsset);
}

void SpriteComponent::SetTexture(const std::wstring& spriteAsset)
{
	m_SpriteAsset = spriteAsset;
	m_pTexture = ContentManager::Load<TextureData>(m_SpriteAsset);
}

void SpriteComponent::Draw(const SceneContext& /*sceneContext*/)
{
	if (!m_IsActive) return;
	if (!m_pTexture)
		return;

	//Here you need to draw the SpriteComponent using the Draw of the sprite renderer
	// The sprite renderer is a singleton
	// you will need to position (X&Y should be in screenspace, Z contains the depth between [0,1]), the rotation and the scale from the owning GameObject
	// You can use the MathHelper::QuaternionToEuler function to help you with the z rotation 

	XMFLOAT3 pos = m_pGameObject->GetTransform()->GetWorldPosition();
	float depth = m_pGameObject->GetTransform()->GetPosition().z;
	float rot = MathHelper::QuaternionToEuler(m_pGameObject->GetTransform()->GetWorldRotation()).z;
	XMFLOAT3 scale = m_pGameObject->GetTransform()->GetWorldScale();
	SpriteRenderer::Get()->AppendSprite(m_pTexture, { pos.x, pos.y }, m_Color, m_Pivot, { scale.x, scale.y }, rot, depth);
}
