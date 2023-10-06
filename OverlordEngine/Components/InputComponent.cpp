#include "stdafx.h"
#include "InputComponent.h"

void InputComponent::SetUserFocus(User user)
{
	InputManager::UnregisterInputComponent(this);

	CurrentUser = user;

	InputManager::RegisterInputComponent(this, user);
}

void InputComponent::SetInputEnabled(bool enabled)
{
	ReceivesInput = enabled;
}

void InputComponent::OnSceneDetach(GameScene* /*pScene*/)
{
	InputManager::UnregisterInputComponent(this);
}

void InputComponent::BindAction(const std::string& action, std::function<void(XMFLOAT2)> callback)
{
	int ID = InputManager::ActionToActionID(action);
	if (ID != -1)
		Actions.emplace(ID, callback);
}

void InputComponent::TriggerAction(int action, XMFLOAT2 value)
{
	if (!ReceivesInput) return;
	if (auto it = Actions.find(action); it != Actions.end()) {
		(*it).second(value);
	}
}