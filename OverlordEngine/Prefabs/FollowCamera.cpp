#include "stdafx.h"
#include "FollowCamera.h"

void FollowCamera::SetOffset(const XMFLOAT3& offset, float radius)
{
	
	m_HasCollider = true;
	auto off = XMLoadFloat3(&offset);
	XMStoreFloat3(&m_Offset, XMVector3Normalize(off));
	XMStoreFloat(&m_Distance, XMVector3Length(off));
	if (m_CurrentDistance < 0.1f) m_CurrentDistance = m_Distance;
	m_Radius = radius;
	m_pCamera->GetTransform()->Translate(offset);
}

void FollowCamera::SetActive(bool value)
{
	m_pCamera->SetActive(value);
}

bool FollowCamera::IsActiveCamera() const
{
	return m_pCamera->IsActive();
}

void FollowCamera::Initialize(const SceneContext& /*sceneContext*/)
{
	const auto obj = AddChild(new GameObject);
	m_pCamera = obj->AddComponent(new CameraComponent);
}

void FollowCamera::Update(const SceneContext& sceneContext)
{
	if (m_HasCollider) {
		XMVECTOR off = XMLoadFloat3(&m_Offset);
		XMVECTOR dir = ::XMVector3Rotate(off, XMLoadFloat4(&GetTransform()->GetWorldRotation()));

		XMFLOAT3 unit;
		XMStoreFloat3(&unit, dir);
		m_CurrentDistance += sceneContext.pGameTime->GetElapsed() * 10.f;
		if (m_CurrentDistance > m_Distance) 
			if (m_CurrentDistance > m_Distance + 0.5f) {
				m_CurrentDistance -= sceneContext.pGameTime->GetElapsed() * 20.f;
				if (m_CurrentDistance < m_Distance) m_CurrentDistance = m_Distance;
			}
			else m_CurrentDistance = m_Distance;

		XMFLOAT3 pos = GetTransform()->GetWorldPosition();
		XMFLOAT4 rot = GetTransform()->GetWorldRotation();
		PxTransform pose({ pos.x, pos.y, pos.z }, { rot.x, rot.y, rot.z, rot.w });

		PxSweepBuffer hit;
		PxQueryFilterData filter(PxQueryFlag::eSTATIC);
		filter.data.word0 = (UINT)CollisionGroup::Group0;
		filter.data.word1 = (UINT)CollisionGroup::Group0;
		PxHitFlags hitFlags = PxHitFlag::eDEFAULT;
		if (GetScene()->GetPhysxProxy()->GetPhysxScene()->sweep(
			PxSphereGeometry(m_Radius), pose,
			{ unit.x, unit.y, unit.z }, m_CurrentDistance, hit, hitFlags, filter
		)) {
			m_CurrentDistance = hit.block.distance;
		}

		m_pCamera->GetTransform()->Translate(off * m_CurrentDistance);
	}
}

