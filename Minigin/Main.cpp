#include <SDL.h>

#if _DEBUG
// ReSharper disable once CppUnusedIncludeDirective
#if __has_include(<vld.h>)
#include <vld.h>
#endif
#endif

#include "Minigin.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "TextComponent.h"
#include "Scene.h"
#include "Timer.h"
#include "RenderComponent.h"
#include "GameObject.h"
#include "TextureComponent.h"
#include "FPSComponent.h"
#include "MoveInCircleComponent.h"
#include "Renderer.h"
#include "TrashTheCacheComponent.h"
#include "InputManager.h"
#include "PlayerController.h"
#include <iostream>

void load()
{
	auto& scene = dae::SceneManager::GetInstance().CreateScene("Demo");

	//auto go = std::make_shared<dae::GameObject>();
	//auto renderComponent = std::make_unique<RenderComponent>(go.get(), "background.tga");
	//go->AddComponent(std::move(renderComponent));
	//scene.Add(std::move(go));
	//
	//go = std::make_shared<dae::GameObject>();
	//renderComponent = std::make_unique<RenderComponent>(go.get(), "logo.tga");
	//go->AddComponent(std::move(renderComponent));
	//go->SetWorldPosition(216, 180);
	//scene.Add(std::move(go));
	//
	auto go = std::make_shared<dae::GameObject>();
	go->SetWorldPosition(80, 20);
	auto renderComponent = std::make_unique<RenderComponent>(go.get());
	auto font = dae::ResourceManager::GetInstance().LoadFont("Lingua.otf", 36);
	auto to = std::make_unique<dae::TextComponent>(go.get(), "Programming 4 Assignment", font, renderComponent.get());
	go->AddComponent(std::move(renderComponent));
	go->AddComponent(std::move(to));
	scene.Add(std::move(go));
	//
	//go = std::make_shared<dae::GameObject>();
	//renderComponent = std::make_unique<RenderComponent>(go.get());
	//font = dae::ResourceManager::GetInstance().LoadFont("Lingua.otf", 20);
	//to = std::make_unique<dae::TextComponent>(go.get(), "0", font, renderComponent.get());
	//auto fpsComponent = std::make_unique<FPSComponent>(go.get(), to.get());
	//go->AddComponent(std::move(to));
	//go->AddComponent(std::move(renderComponent));
	//go->AddComponent(std::move(fpsComponent));
	//scene.Add(std::move(go));
	//
	//auto child = new dae::GameObject();
	//child->SetLocalPosition(320.f, 300.f);
	//renderComponent = std::make_unique<RenderComponent>(child, "MrHotDog.png");
	//child->AddComponent(std::move(renderComponent));
	//auto moveInCircleComponent = std::make_unique<MoveInCircleComponent>(child, 30.f, 10.f);
	//child->AddComponent(std::move(moveInCircleComponent));
	//
	//auto parent = std::make_shared<dae::GameObject>();
	//parent->SetLocalPosition(320.f, 300.f);
	//renderComponent = std::make_unique<RenderComponent>(parent.get(), "PeterPepperFrontFacing.png");
	//parent->AddComponent(std::move(renderComponent));
	//moveInCircleComponent = std::make_unique<MoveInCircleComponent>(parent.get(), 30.f, 5.f, false);
	//parent->AddComponent(std::move(moveInCircleComponent));
	//
	//child->SetParent(parent.get(), true);
	//
	//scene.Add(std::move(parent));

	go = std::make_shared<dae::GameObject>();
	auto trashTheCacheComponent = std::make_unique<TrashTheCache>(go.get());
	go->AddComponent(std::move(trashTheCacheComponent));

	scene.Add(std::move(go));

	dae::Renderer::GetInstance().SetBackgroundColor(SDL_Color(62, 90, 111));

	auto controller{ std::make_unique<PlayerController>(dae::InputManager::GetInstance().GetAmountOfPlayers()) };
	controller->AddCommand(XINPUT_GAMEPAD_A, PlayerController::KeyState::down, [](){ std::cout << "A is pressed down"; });
	dae::InputManager::GetInstance().AddController(std::move(controller));
}

int main(int, char*[]) {
	dae::Minigin engine("../Data/");
	engine.Run(load);
    return 0;
}