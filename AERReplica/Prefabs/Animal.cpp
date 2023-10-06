#include "stdafx.h"
#include "Animal.h"
#include "Materials/BasicMaterial_Deferred_Skinned.h"

Animal::Animal()
{
}

Animal::~Animal()
{
}

void Animal::Initialize(const SceneContext&)
{
	auto Fmod = SoundManager::Get()->GetSystem();

	FMOD_RESULT res = Fmod->createStream("Resources/Sounds/lama01.Wav", FMOD_3D | FMOD_3D_LINEARROLLOFF, nullptr, &sounds[0]);
	SoundManager::Get()->ErrorCheck(res);
	res = Fmod->createStream("Resources/Sounds/lama02.Wav", FMOD_3D | FMOD_3D_LINEARROLLOFF, nullptr, &sounds[1]);
	SoundManager::Get()->ErrorCheck(res);
	res = Fmod->createStream("Resources/Sounds/lama02.Wav", FMOD_3D | FMOD_3D_LINEARROLLOFF, nullptr, &sounds[2]);
	SoundManager::Get()->ErrorCheck(res);

	auto mat = MaterialManager::Get()->CreateMaterial<BasicMaterial_Deferred_Skinned>();
	auto model = AddComponent(new ModelComponent(L"Meshes/Llam.ovm"));
	mat->SetDiffuseMap(L"Textures/Islands/atlas.png");
	model->SetMaterial(mat);
	if (auto anim = model->GetAnimator()) {
		Idle = &anim->GetClip(0);
		Idle.AddNotify(3, [this] {
			auto Fmod = SoundManager::Get()->GetSystem();
			FMOD::Channel* c;
			Fmod->playSound(sounds[rand() % 3], nullptr, false, &c);
			c->set3DMinMaxDistance(0.f, 10.f);
			auto pos = GetTransform()->GetPosition();
			FMOD_VECTOR posi = { pos.x, pos.y, pos.z };
			FMOD_VECTOR veli = { 0.f,0.f,0.f };
			c->set3DAttributes(&posi, &veli);
			});
		GetScene()->SetTimerByEvent(this, MathHelper::randF(0.f, 2.f), [this, anim] {
			
			anim->Updator = [this](float delta, BoneArray bones) {
				Idle.Update(delta);
				Idle.MakeTransforms(bones);
			};
		});
	}
}

void Animal::Update(const SceneContext&)
{
}
