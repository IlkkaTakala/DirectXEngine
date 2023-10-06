#include "stdafx.h"
#include "Pickable.h"
#include "Character.h"

#include "Scenes/AERGameScene.h"

#include "Materials/BasicMaterial_Deferred.h"

UINT Pickable::MaterialIdx = 0;

Pickable::Pickable() : GameObject(), m_Rotation(0.f)
{
}

Pickable::~Pickable()
{
	pickSound->release();
}

void Pickable::Initialize(const SceneContext&)
{
	Mesh = AddComponent(new ModelComponent(L"Meshes/Pickable.ovm", false));
	
	if (MaterialIdx == 0) {
		auto mat = MaterialManager::Get()->CreateMaterial<BasicMaterial_Deferred>();
		MaterialIdx = mat->GetMaterialId();
		mat->SetVariable_Vector(L"gDiffuseColor", { 10,10,10,1 });
	}
	Mesh->SetMaterial(MaterialIdx);

	Collider = AddComponent(new RigidBodyComponent(true));
	const auto pDefaultMaterial = PxGetPhysics().createMaterial(0.5f, 0.5f, 0.5f);
	GetScene()->SetTimerByEvent(this, 0.5f, [&, pDefaultMaterial]() {
		Collider->AddCollider(PxSphereGeometry{ 0.5f }, *pDefaultMaterial, true);
		Collider->SetCollisionIgnoreGroups(CollisionGroup::Group1);
		Collider->SetCollisionGroup(CollisionGroup::Group2);
		});
	SetOnTriggerCallBack([this](GameObject* /*pTriggerObject*/, GameObject* pOtherObject, PxTriggerAction /*action*/) {
		if (auto it = dynamic_cast<Character*>(pOtherObject)) {
			RemovePickup();
			it->GiveScore();
		}
	});

	auto Fmod = SoundManager::Get()->GetSystem();

	FMOD_RESULT res = Fmod->createStream("Resources/Sounds/bell.Wav", FMOD_DEFAULT, nullptr, &pickSound);
	SoundManager::Get()->ErrorCheck(res);

	const auto updator = [](ParticleEmitter* system, float delta) {
		ParticleSystemConstruction::Updator u(system);
		u.UpdateLifetime(delta);
		u.UpdateVelocities(delta);
		u.Alpha(delta, CurveData({ {0.f, 0.f}, {0.1f, 1.f}, { 1.f, 0.f } }));
		//u.Color(delta, VectorCurveData({ {0.f, {1.f}} }));
		u.SpriteSize(delta, CurveData({ {0.f, 0.3f}, {0.5f, 1.0f}, {1.f, 0.f} }));
	};

	const auto constructor = [](ParticleEmitter* /*sys*/, Particle& p) {
		p.initialSize = MathHelper::randF(.3f, .8f);
		p.velocity = { 0, .1f, 0 };
		p.max_lifetime = MathHelper::randF(.2f, .3f);
		p.isActive = false;
		p.lifetime = 0.f;
		ParticleSystemConstruction::Constructor::SphereLocation(p,
			MathHelper::randF(1.f, 1.f)
		);
		p.alpha = 0.f;
		p.initialColour = { 10.f, 10.f, 25.f };
	};
	const auto updator2 = [](ParticleEmitter* system, float delta) {
		ParticleSystemConstruction::Updator u(system);
		u.UpdateLifetime(delta);
		//u.AddVelocity(delta, { 0,-2,0 });
		//u.UpdateVelocities(delta);
		u.Alpha(delta, CurveData({ {0.f, 0.f}, {0.1f, 1.f}, {0.9f, 1.f}, { 1.f, 0.f } }));
		//u.Color(delta, VectorCurveData({ {0.f, {1.f}} }));
		//u.SpriteSize(delta, CurveData({ {0.f, 0.3f}, {1.f, 1.0f} }));
	};

	const auto constructor2 = [](ParticleEmitter* /*sys*/, Particle& p) {
		p.initialSize = 5.f;
		p.max_lifetime = 1.f;
		p.isActive = false;
		p.lifetime = 0.f;
		p.vertexInfo.Position = XMFLOAT3{ 0.f, 0.f, 0.f };
		p.alpha = 0.f;
		p.initialColour = { 1.f, 1.f, 3.f };
	};

	auto pParticle = AddChild(new GameObject);
	AliveParticles = pParticle->AddComponent(new ParticleSystemComponent());
	AliveParticles->AddEmitter({ L"Textures/Star.png", constructor, updator }, [] { return ParticleSystemConstruction::MakeSpawnRate(15.f); });
	AliveParticles->AddEmitter({ L"Textures/Ring.png", constructor2, updator2 }, [] { return ParticleSystemConstruction::MakeSpawnRate(1.f); });

	const auto updator1 = [](ParticleEmitter* system, float delta) {
		ParticleSystemConstruction::Updator u(system);
		u.UpdateLifetime(delta);
		u.AddVelocity(delta, { 0,-2,0 });
		u.UpdateVelocities(delta);
		u.Alpha(delta, CurveData({ {0.f, 0.f}, {0.1f, 1.f}, { 1.f, 0.f } }));
		//u.Color(delta, VectorCurveData({ {0.f, {1.f}} }));
		u.SpriteSize(delta, CurveData({ {0.f, 0.3f}, {1.f, 1.0f} }));
	};

	const auto constructor1 = [](ParticleEmitter* /*sys*/, Particle& p) {
		p.initialSize = MathHelper::randF(.1f, .3f);
		p.max_lifetime = MathHelper::randF(0.5f, 1.5);
		p.rotationRate = MathHelper::randF(-1.f, 1.f);
		p.isActive = false;
		p.lifetime = 0.f;
		ParticleSystemConstruction::Constructor::SphereLocation(p,
			MathHelper::randF(1.f, 1.f)
		);
		XMStoreFloat3(&p.velocity, XMVector3Normalize(XMLoadFloat3(&p.vertexInfo.Position)) * MathHelper::randF(.5f, 1.5f));
		p.alpha = 0.f;
		p.initialColour = { 10.f, 10.f, 10.f };
		p.vertexInfo.Color = { 10.f, 10.f, 10.f, 10.f };
	};

	auto pParticle1 = AddChild(new GameObject);
	DeathParticles = pParticle1->AddComponent(new ParticleSystemComponent());
	DeathParticles->AddEmitter({ L"Textures/Star.png", constructor1, updator1 }, [] { return ParticleSystemConstruction::MakeSpawnBurst(150); });
	DeathParticles->Deactivate();
}

void Pickable::Update(const SceneContext& s)
{
	m_Rotation += s.pGameTime->GetElapsed();
	GetTransform()->Rotate(0, m_Rotation * 30.f, 0, true);
}

void Pickable::RemovePickup()
{
	auto Fmod = SoundManager::Get()->GetSystem();
	Fmod->playSound(pickSound, nullptr, false, nullptr);

	Mesh->Deactivate();
	AliveParticles->Deactivate();
	DeathParticles->Activate();
	GetScene()->SetTimerByEvent(this, 0.f, [this]() {
		SetOnTriggerCallBack(nullptr);
		Collider->SetCollisionIgnoreGroups(CollisionGroup::Group0);
		});

	dynamic_cast<AERGameScene*>(GetScene())->RemoveShard(this);
	GetScene()->SetTimerByEvent(this, 3.f, [this]() {
		Destroy();
		});

}
