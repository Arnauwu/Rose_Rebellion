#include "UIManager.h"
#include "UICheckBox.h" 
#include "UISlider.h"
#include "UIButton.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"

UIManager::UIManager() :Module()
{
	name = "UIManager";
}

UIManager::~UIManager() {}

bool UIManager::Start()
{
	return true;
}

std::shared_ptr<UIElement> UIManager::CreateUIElement(UIElementType type, int id, const char* text, float anchorX, float anchorY, float wPerc, float hPerc, Module* observer)
{
	std::shared_ptr<UIElement> uiElement = nullptr;

	switch (type)
	{
	case UIElementType::BUTTON:
		uiElement = std::make_shared<UIButton>(id, anchorX, anchorY, wPerc, hPerc, text);
		break;
	case UIElementType::CHECKBOX:
		uiElement = std::make_shared<UICheckBox>(id, anchorX, anchorY, wPerc, hPerc, text);
		break;
	case UIElementType::SLIDER:
		uiElement = std::make_shared<UISlider>(id, anchorX, anchorY, wPerc, hPerc, text);
		break;
	}

	if (uiElement != nullptr) {
		uiElement->SetObserver(observer);
		UIElementsList.push_back(uiElement);
	}

	return uiElement;
}

bool UIManager::Update(float dt)
{
	for (const auto& uiElement : UIElementsList)
	{
		//If the entity is marked for deletion, add it to the pendingDelete list
		if (uiElement->pendingToDelete)
		{
			pendingDelete.push_back(uiElement);
		}
		else {
			uiElement->Update(dt);
		}
	}

	//Now iterates over the pendingDelete list and destroys the uiElement
	for (const auto uiElement : pendingDelete)
	{
		uiElement->CleanUp();
		UIElementsList.remove(uiElement);
	}

	pendingDelete.clear();

	return true;
}
bool UIManager::PostUpdate()
{
	Draw();
	return true;
}

void UIManager::Draw() const
{
	for (auto const& uiElement : UIElementsList) {
		if (uiElement->visible) uiElement->Draw();
	}
}

void UIManager::RecalculateAllUI()
{
	for (const auto& uiElement : UIElementsList)
	{
		if (uiElement != nullptr) {
			uiElement->RecalculateBounds();
		}
	}
}
bool UIManager::CleanUp()
{
	for (const auto& uiElement : UIElementsList)
	{
		uiElement->CleanUp();
	}

	return true;
}

