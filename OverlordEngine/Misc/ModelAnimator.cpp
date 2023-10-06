#include "stdafx.h"
#include "ModelAnimator.h"

void AnimationInstance::Update(float delta)
{
	auto passedTicks = fmod(delta * anim->ticksPerSecond, anim->duration);

	float oldFrame = frame;
	frame += passedTicks;
	if (frame >= anim->duration) {
		frame -= anim->duration;
	}
	for (auto& [time, call] : Callbacks) {
		if (time > oldFrame && time <= frame)
			call();
	}

	for (auto it = 0; it != anim->keys.size(); ++it) {
		if (anim->keys[it].tick > frame) {
			nextframe = it;
			previousframe = it - 1 < 0 ? (int)anim->keys.size() - 1 : it - 1;
			break;
		}
		nextframe = 0;
		previousframe = (int)anim->keys.size() - 1;
	}

	blendFactor = (frame - anim->keys[previousframe].tick) / (anim->keys[nextframe].tick - anim->keys[previousframe].tick);
}

void AnimationInstance::MakeTransforms(BoneArray bones) const
{
	if (!anim) return;
	auto& A = anim->keys[previousframe].boneTransforms;
	auto& B = anim->keys[nextframe].boneTransforms;
	for (int i = 0; i < bones.size(); i++) {
		bones[i] = Interpolate(A[i], B[i], blendFactor);
	}
}

void AnimationBlendSpace1D::Evaluate(float delta, BoneArray bones, float axisValue)
{

	const std::pair<float, AnimationInstance>* first = nullptr;
	const std::pair<float, AnimationInstance>* second = nullptr;

	if (anims.size() == 0) {
		for (auto& t : bones) t = XMFLOAT4X4();
		return;
	}

	if (axisValue <= anims[0].first) {

		for (auto& a : anims)
			a.second.Update(delta);

		anims[0].second.MakeTransforms(bones);
		return;
	}
	else if (axisValue >= anims.rbegin()->first) {

		for (auto& a : anims)
			a.second.Update(delta);

		anims.rbegin()->second.MakeTransforms(bones);
		return;
	}
	else {
		for (int i = (int)anims.size() - 1; i >= 0; i--) {
			if (anims[i].first <= axisValue) {
				first = &anims[i];
				if (i + 1 < anims.size()) second = &anims[i + 1];
				break;
			}
		}

		if (first) {
			if (second) {
				float last = first->first;
				float next = second->first;
				float scale = (axisValue - last) / (next - last);
				//float p = first->second.GetFactor() * (1 - scale) + second->second.GetFactor() * scale;

				for (auto& a : anims)
					a.second.Update(delta);

				std::vector<XMFLOAT4X4> A(bones.size(), XMFLOAT4X4{});
				std::vector<XMFLOAT4X4> B(bones.size(), XMFLOAT4X4{});

				first->second.MakeTransforms(A);
				second->second.MakeTransforms(B);

				for (int i = 0; i < bones.size(); i++) {

					bones[i] = Interpolate(A[i], B[i], scale);
				}
			}
			else {
				first->second.MakeTransforms(bones);
			}
		}
	}
}

ModelAnimator::ModelAnimator(MeshFilter* pMeshFilter):
	m_pMeshFilter{pMeshFilter}
{
	XMFLOAT4X4 identity;
	XMStoreFloat4x4(&identity, XMMatrixIdentity());
	m_Transforms.assign(m_pMeshFilter->m_BoneCount, identity);
}

void ModelAnimator::Update(const SceneContext& sceneContext)
{
	if (Updator) Updator(sceneContext.pGameTime->GetElapsed() * m_AnimationSpeed * (m_Reversed ? -1.f : 1.f), m_Transforms);

	/*for (int i = 0; i < m_Transforms.size(); ++i) {
		const BoneData& b = m_pMeshFilter->Bones[i];
		int p = b.parent;
		if (p < 0 || p > i) continue;
		XMStoreFloat4x4(&m_Transforms[i], XMMatrixMultiply(XMLoadFloat4x4(&m_Transforms[p]), XMLoadFloat4x4(&m_Transforms[i])));
	}*/
	/*for (int i = 0; i < m_Transforms.size(); ++i) {
		const BoneData& b = m_pMeshFilter->Bones[i];
		XMStoreFloat4x4(&m_Transforms[i], XMMatrixMultiply(XMLoadFloat4x4(&m_Transforms[i]), (XMLoadFloat4x4(&b.localTransform))));
		XMStoreFloat4x4(&m_Transforms[i], XMMatrixMultiply(XMLoadFloat4x4(&m_Transforms[i]), (XMLoadFloat4x4(&b.offset))));
	}*/
}

void ModelAnimator::SetAnimation(const std::wstring& clipName)
{
	for (auto& c : m_pMeshFilter->m_AnimationClips) {
		if (c.name == clipName) {
			SetAnimation(c);
			break;
		}
	}
	if (!m_ClipSet) {
		Reset(true);
		Logger::LogWarning(L"Invalid animation clip %s", clipName);
	}
}

void ModelAnimator::SetAnimation(UINT clipNumber)
{
	m_ClipSet = false;
	if (clipNumber < m_pMeshFilter->m_AnimationClips.size()) {
		SetAnimation(m_pMeshFilter->m_AnimationClips[clipNumber]);
	}
	else {
		Logger::LogWarning(L"Invalid animation clip %d", clipNumber);
	}
}

void ModelAnimator::SetAnimation(const AnimationClip& clip)
{
	m_ClipSet = true;
	m_CurrentClip = clip;
	Reset(false);
}

void ModelAnimator::Reset(bool pause)
{
	if (pause) m_IsPlaying = false;

	m_TickCount = 0;
	m_AnimationSpeed = 1.f;

	if (m_ClipSet) {
		m_Transforms = m_CurrentClip.keys[0].boneTransforms;
	}
	else {
		XMFLOAT4X4 identity;
		XMStoreFloat4x4(&identity, XMMatrixIdentity());
		m_Transforms.assign(m_pMeshFilter->m_BoneCount, identity);
	}
}

XMFLOAT4X4 Interpolate(const XMFLOAT4X4& transformA, const XMFLOAT4X4& transformB, float blendFactor)
{
	XMVECTOR posA, rotA, scaleA;
	XMMatrixDecompose(&scaleA, &rotA, &posA, (XMLoadFloat4x4(&transformA)));

	XMVECTOR posB, rotB, scaleB;
	XMMatrixDecompose(&scaleB, &rotB, &posB, (XMLoadFloat4x4(&transformB)));

	XMFLOAT4X4 mat;
	XMStoreFloat4x4(&mat, (
		XMMatrixScalingFromVector(XMVectorLerp(scaleA, scaleB, blendFactor)) *
		XMMatrixRotationQuaternion(XMQuaternionSlerp(rotA, rotB, blendFactor)) *
		XMMatrixTranslationFromVector(XMVectorLerp(posA, posB, blendFactor))
		));

	return mat;
}
