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

void load()
{
	auto& scene = dae::SceneManager::GetInstance().CreateScene("Demo");

	auto go = std::make_shared<dae::GameObject>();
	auto renderComponent = std::make_shared<RenderComponent>(std::weak_ptr<dae::GameObject>(go), "background.tga");
	go->AddComponent(renderComponent);
	scene.Add(go);

	go = std::make_shared<dae::GameObject>();
	renderComponent = std::make_shared<RenderComponent>(go, "logo.tga");
	go->AddComponent(renderComponent);
	go->SetWorldPosition(216, 180);
	scene.Add(go);
	
	go = std::make_shared<dae::GameObject>();
	go->SetWorldPosition(80, 20);
	auto font = dae::ResourceManager::GetInstance().LoadFont("Lingua.otf", 36);
	auto to = std::make_shared<dae::TextComponent>(go, "Programming 4 Assignment", font, nullptr);
	renderComponent = std::make_shared<RenderComponent>(go, to->GetTexture());
	go->AddComponent(to);
	go->AddComponent(renderComponent);
	scene.Add(go);
	
	go = std::make_shared<dae::GameObject>();
	renderComponent = std::make_shared<RenderComponent>(go);
	font = dae::ResourceManager::GetInstance().LoadFont("Lingua.otf", 20);
	to = std::make_shared<dae::TextComponent>(go, "0", font, renderComponent);
	auto fpsComponent = std::make_shared<FPSComponent>(go, to);
	go->AddComponent(to);
	go->AddComponent(renderComponent);
	go->AddComponent(fpsComponent);
	scene.Add(go);
}

int main(int, char*[]) {
	dae::Minigin engine("../Data/");
	engine.Run(load);
    return 0;
}