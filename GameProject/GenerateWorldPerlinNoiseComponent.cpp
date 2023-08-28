#include "GenerateWorldPerlinNoiseComponent.h"
#include "GameObject.h"
#include <iostream>
#include <SDL.h>
#include "SDL_image.h"
#include "Renderer.h"
#include "RenderComponent.h"
#include "imgui.h"
#include <Windows.h>
#include <shlobj.h>
#include <locale> 
#include <codecvt>
#include <filesystem>

GenerateWorldPerlinNoiseComponent::GenerateWorldPerlinNoiseComponent(dae::GameObject* pOwner, int worldXSize, int worldYSize)
	: Component(pOwner)
	, m_WorldXSize{ worldXSize }
	, m_WorldYSize{ worldYSize }
{
	m_PreviousPerlinNoiseSeed = m_PerlinNoiseSeed;

	//create the perlin noise generator
	m_Perlin2D = std::make_unique<PerlinNoise2D>(m_PerlinNoiseSeed);

	Regenerate();

	PWSTR pPath; //variable to receive the path memory block pointer
	auto result{ SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &pPath) };
	std::wstring documentFolderPath{};
	if (SUCCEEDED(result))
		documentFolderPath = pPath;

	CoTaskMemFree(pPath);  //free up the path memory block

	documentFolderPath.append(L"\\WorldGenerator");
	std::filesystem::create_directory(documentFolderPath);

	m_JPGSaveFolderPath = documentFolderPath;
}

void GenerateWorldPerlinNoiseComponent::Update()
{
	if (m_ShouldRegenerate)
	{
		Regenerate();
		m_ShouldRegenerate = false;
	}

	if (m_ShouldSaveTextureAsJPG)
	{
		SafeWorldAsJPG();
		m_ShouldSaveTextureAsJPG = false;
	}
}

void GenerateWorldPerlinNoiseComponent::RenderImGui()
{
	auto pRenderer{ dae::Renderer::GetInstance().GetSDLRenderer() };
	int width{}, height{};
	SDL_GetRendererOutputSize(pRenderer, &width, &height);

	const float menuWidth{ 200 };
	
	ImGui::SetNextWindowPos(ImVec2(static_cast<float>(width) - menuWidth, 0.f));
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(menuWidth), static_cast<float>(height)));
	ImGui::Begin("Settings", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
	
	if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("World Generation"))
		{
			const float itemWidth{ menuWidth - 15.f };

			ImGui::PushItemWidth(itemWidth);

			if (ImGui::Button("regenerate", ImVec2(itemWidth, 25)))
				m_ShouldRegenerate = true;

			if (ImGui::Button("safe as jpg", ImVec2(itemWidth, 25)))
				m_ShouldSaveTextureAsJPG = true;

			ImGui::Text("Perlin noise");

			DrawImGuiLine(menuWidth);

			ImGui::Spacing();

			ImGui::Text("seed:");
			ImGui::InputInt("1", &m_PerlinNoiseSeed);

			ImGui::Spacing();

			ImGui::TextWrapped("amount of octaves (higher = more detail):");
			ImGui::SliderInt("2", &m_PerlinNoiseAmountOfOctaves, 1, 30);

			ImGui::Spacing();

			ImGui::TextWrapped("persistence (detail strenght):");
			ImGui::SliderFloat("3", &m_PerlinNoisePersistence, 0.f, 1.f);

			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::Text("World");

			DrawImGuiLine(menuWidth);

			ImGui::Spacing();

			ImGui::TextWrapped("world width(X) in pixels:");
			ImGui::InputInt("4", &m_WorldXSize);

			ImGui::Spacing();

			ImGui::TextWrapped("world height(Y) in pixels:");
			ImGui::InputInt("5", &m_WorldYSize);

			ImGui::PopItemWidth();			

			ImGui::EndTabItem();
		}
		//if (ImGui::BeginTabItem("Details"))
		//{
		//	ImGui::Text("ID: 0123456789");
		//	ImGui::EndTabItem();
		//}
		ImGui::EndTabBar();
	}

	ImGui::End();
}

void GenerateWorldPerlinNoiseComponent::DrawImGuiLine(float width, float spaceBelowPreviousItem) const
{
	ImVec2 min{ ImGui::GetItemRectMin() };
	ImVec2 max{ min.x + width, ImGui::GetItemRectMax().y };
	max.y += spaceBelowPreviousItem;
	min.y = max.y;
	ImGui::GetWindowDrawList()->AddLine(min, max, ImColor(1.f, 1.f, 1.f), 1.f);
}

SDL_Color GenerateWorldPerlinNoiseComponent::CalculateCellColor(int x, int y) const
{
	constexpr float devider{ 128 };
	
	double noise{ (m_Perlin2D->GetNoise(x / devider, y / devider, m_PerlinNoiseAmountOfOctaves, m_PerlinNoisePersistence) + 1) / 2.f };
	if (noise < 0.475f)
		return SDL_Color(0, 255u, 0);
	else
		return SDL_Color(0, 0, 255u);
}

void GenerateWorldPerlinNoiseComponent::SafeWorldAsJPG()
{
	//initialize sdl image
	if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG)
		std::cout << "SDL_image initialization failed: " << IMG_GetError() << '\n';

	int biggestSide{ m_WorldXSize };

	if (m_WorldYSize > m_WorldXSize)
		biggestSide = m_WorldYSize;
	
	std::wstring documentFolderPath{ m_JPGSaveFolderPath };
	
	documentFolderPath.append(L"\\world0.jpg");

	bool foundFileName{};
	int nr{};

	while (!foundFileName)
	{
		if (std::filesystem::exists(documentFolderPath))
		{
			documentFolderPath.erase(documentFolderPath.find(std::to_wstring(nr) + L".jpg"));
			++nr;
			documentFolderPath.append(std::to_wstring(nr) + L".jpg");
		}
		else foundFileName = true;
	}

	//convert std::wstring to std::string
	int utf8Length = WideCharToMultiByte(CP_UTF8, 0, documentFolderPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
	auto converted = std::string(utf8Length, '\0');
	WideCharToMultiByte(CP_UTF8, 0, documentFolderPath.c_str(), -1, &converted[0], utf8Length, nullptr, nullptr);
	
	//safe the texture
	auto pSurface{ SDL_CreateRGBSurfaceWithFormatFrom(m_Pixels.data(), m_WorldXSize, m_WorldYSize, 32, biggestSide * sizeof(uint32_t), SDL_PIXELFORMAT_RGBA8888)};
	if (pSurface)
	{
		if (IMG_SaveJPG(pSurface, converted.c_str(), 100) == 0)
			std::cout << "world saved! at " + converted + '\n';
		else
			std::cout << IMG_GetError() << '\n';
		SDL_FreeSurface(pSurface);
	}
}

void GenerateWorldPerlinNoiseComponent::Regenerate()
{
	if (m_PerlinNoiseSeed != m_PreviousPerlinNoiseSeed)
	{
		m_Perlin2D->SetSeed(m_PerlinNoiseSeed);
		m_PreviousPerlinNoiseSeed = m_PerlinNoiseSeed;
	}

	if (m_PreviousWorldXSize != m_WorldXSize || m_PreviousWorldYSize != m_WorldYSize)
	{
		//create the texture
		auto pColorTexture{ SDL_CreateTexture(dae::Renderer::GetInstance().GetSDLRenderer(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, m_WorldXSize, m_WorldYSize) };

		//save the biggest side
		int biggestSide{ m_WorldYSize };

		if (m_WorldXSize > m_WorldYSize)
			biggestSide = m_WorldXSize;

		//set the texture's color data
		SDL_PixelFormat* pixelFormat = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
		m_Pixels.resize(biggestSide * biggestSide);

		for (int rowNr{}; rowNr < biggestSide; ++rowNr)
		{
			for (int colNr{}; colNr < biggestSide; ++colNr)
			{
				auto color{ CalculateCellColor(rowNr, colNr) };
				m_Pixels[rowNr * biggestSide + colNr] = SDL_MapRGBA(pixelFormat, color.r, color.g, color.b, color.a);
			}
		}

		SDL_FreeFormat(pixelFormat);

		SDL_UpdateTexture(pColorTexture, nullptr, m_Pixels.data(), biggestSide * sizeof(uint32_t));

		if (!m_pTextureComponent)
		{
			auto pOwner{ GetOwner() };

			//create a texture component
			auto textureComp{ std::make_unique<dae::TextureComponent>(pOwner, pColorTexture) };

			//add a renderComponent to the owner of this one
			pOwner->AddComponent(std::make_unique<dae::RenderComponent>(pOwner, std::move(textureComp)));

			m_pTextureComponent = pOwner->GetComponent<dae::TextureComponent>();
		}
		else
			m_pTextureComponent->SetSDLTexture(pColorTexture);
	}
}