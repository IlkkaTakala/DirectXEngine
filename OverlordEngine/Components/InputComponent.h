#pragma once
class InputComponent : public BaseComponent
{
public:

	void Initialize(const SceneContext& /*sceneContext*/) override {}

	void SetUserFocus(User user);
	void SetInputEnabled(bool enabled);
	virtual void OnSceneDetach(GameScene* /*pScene*/) override;

	void BindAction(const std::string& action, std::function<void(XMFLOAT2)> callback);

	User GetCurrentUser() const { return CurrentUser; }

	void TriggerAction(int action, XMFLOAT2 value);

private:

	User CurrentUser{ 0 };
	bool ReceivesInput{ true };

	std::map<int, std::function<void(XMFLOAT2)>> Actions;
};

