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
#include <vector>
#include <random>
#include <chrono>
#include <future>

GenerateWorldPerlinNoiseComponent::GenerateWorldPerlinNoiseComponent(dae::GameObject* pOwner, int worldXSize, int worldYSize)
	: Component(pOwner)
	, m_WorldXSize{ worldXSize }
	, m_WorldYSize{ worldYSize }
{
	m_PreviousPerlinNoiseSeed = m_PerlinNoiseSeed;

	//create the perlin noise generator
	m_Perlin2D = std::make_unique<PerlinNoise2D>(m_PerlinNoiseSeed);

	m_WorldBiggestSide = worldXSize;
	if (worldYSize > m_WorldBiggestSide)
		m_WorldBiggestSide = worldYSize;

	//create the worley noise generator
	m_Worley = std::make_unique<WorleyNoise>(1, m_WorldBiggestSide, m_WorldBiggestSide);

	PWSTR pPath; //variable to receive the path memory block pointer
	auto result{ SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &pPath) };
	std::wstring documentFolderPath{};
	if (SUCCEEDED(result))
		documentFolderPath = pPath;

	CoTaskMemFree(pPath);  //free up the path memory block

	documentFolderPath.append(L"\\WorldGenerator");
	std::filesystem::create_directory(documentFolderPath);

	m_JPGSaveFolderPath = documentFolderPath;

	m_CellKindColors.push_back(std::vector<float>{ 0.f, 1.f, 0.f });
	m_CellKindColors.push_back(std::vector<float>{ 0.f, 0.f, 1.f });

	m_CellKindValueRange.push_back({ 0.f, 0.475f });
	m_CellKindValueRange.push_back({ 0.475f, 1.f });

	m_PixelFormat = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);

	const int numThreads = std::thread::hardware_concurrency();
	m_ThreadsPixelData.resize(numThreads);

	//Regenerate();
	RegenerateMultiThread();
}

GenerateWorldPerlinNoiseComponent::~GenerateWorldPerlinNoiseComponent()
{
	SDL_FreeFormat(m_PixelFormat);
}

void GenerateWorldPerlinNoiseComponent::Update()
{
	if (m_ShouldRegenerate)
	{
		RegenerateMultiThread();
		m_ShouldRegenerate = false;
	}

	if (m_ShouldSaveTextureAsJPG)
	{
		SafeWorldAsJPG();
		m_ShouldSaveTextureAsJPG = false;
	}

	if (m_ShouldCreateNewCellKind)
	{
		CreateNewCellKind();
		m_ShouldCreateNewCellKind = false;
	}

	for (auto& index : m_CellKindIndicesToRemove)
	{
		if (index < 0 || index > m_CellKindColors.size())
			continue;

		m_CellKindColors.erase(std::remove(m_CellKindColors.begin(), m_CellKindColors.end(), m_CellKindColors[index]));
		m_CellKindValueRange.erase(std::remove(m_CellKindValueRange.begin(), m_CellKindValueRange.end(), m_CellKindValueRange[index]));
	}

	m_CellKindIndicesToRemove.clear();

	if (m_IsRegenerating)
	{
		//check if all threads are done with their task
		for (auto& thread : m_Threads)
		{
			if (!thread.joinable())
				return;
		}

		//if they are all done join them
		for (auto& thread : m_Threads)
			thread.join();

		//create the image
		CreateNewWorldTexture();

		//clear the thread vector
		m_Threads.clear();

		m_IsRegenerating = false;
	}
}

void GenerateWorldPerlinNoiseComponent::RenderImGui()
{
	auto pRenderer{ dae::Renderer::GetInstance().GetSDLRenderer() };
	int width{}, height{};
	SDL_GetRendererOutputSize(pRenderer, &width, &height);
	
	float menuWidth{ 200 };
	
	ImGui::SetNextWindowPos(ImVec2(static_cast<float>(width) - menuWidth, 0.f));
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(menuWidth), static_cast<float>(height)));
	ImGui::Begin("Generation Settings", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

	bool hasScrollbar{};

	if (ImGui::GetScrollMaxY() > 0.f)
		hasScrollbar = true;

	if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
	{
		float itemWidth{ menuWidth - 15.f };
		if (hasScrollbar)
			itemWidth -= ImGui::GetStyle().ScrollbarSize;
		ImGui::PushItemWidth(itemWidth);

		if (ImGui::BeginTabItem("World"))
		{
			if (ImGui::Button("Regenerate", ImVec2(itemWidth, 25)))
				m_ShouldRegenerate = true;

			if (ImGui::Button("Safe as jpg", ImVec2(itemWidth, 25)))
				m_ShouldSaveTextureAsJPG = true;

			if (ImGui::CollapsingHeader("Perlin noise"))
			{
				ImGui::Text("Seed:");
				ImGui::InputInt("1", &m_PerlinNoiseSeed);

				ImGui::Spacing();

				ImGui::TextWrapped("amount of octaves (higher = more detail):");
				ImGui::SliderInt("2", &m_PerlinNoiseAmountOfOctaves, 1, 30);

				ImGui::Spacing();

				ImGui::TextWrapped("persistence (detail strenght):");
				ImGui::SliderFloat("3", &m_PerlinNoisePersistence, 0.f, 1.f);

				ImGui::Spacing();
			}

			if (ImGui::CollapsingHeader("World"))
			{
				ImGui::Indent(5.f);

				if (hasScrollbar)
					ImGui::PushItemWidth(menuWidth - 20.f - ImGui::GetStyle().ScrollbarSize);
				else
				ImGui::PushItemWidth(menuWidth - 20.f);

				if (ImGui::CollapsingHeader("Size"))
				{
					ImGui::TextWrapped("World width(X) in pixels:");
					ImGui::InputInt("4", &m_WorldXSize);

					ImGui::Spacing();

					ImGui::TextWrapped("World height(Y) in pixels:");
					ImGui::InputInt("5", &m_WorldYSize);

					ImGui::Spacing();
				}
				
				if (ImGui::CollapsingHeader("Cell kinds"))
				{
					ImGui::Indent(5.f);
					
					if (hasScrollbar)
						ImGui::PushItemWidth(menuWidth - 25.f - ImGui::GetStyle().ScrollbarSize);
					else
						ImGui::PushItemWidth(menuWidth - 25.f);

					for (int index{}; index < m_CellKindColors.size(); ++index)
					{
						auto indexToString{ std::to_string(index) };
						bool visibility{ true };
						if (ImGui::CollapsingHeader((indexToString).c_str(), &visibility))
						{
							CellKindDragDropSource(index);

							CellKindDragDropTarget(index);

							if (!visibility)
							{
								m_CellKindIndicesToRemove.push_back(index);
							}

							ImGui::Text("color:");
							ImGui::ColorEdit3(("color" + indexToString).c_str(), m_CellKindColors[index].data());
							
							ImGui::Text("minimum value:");
							if (ImGui::SliderFloat(("min" + indexToString).c_str(), &m_CellKindValueRange[index].first, 0.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
							{
								if (index - 1 >= 0)
								{
									m_CellKindValueRange[index - 1].second = m_CellKindValueRange[index].first;
								}

								if (m_CellKindValueRange[index].first > m_CellKindValueRange[index].second)
									m_CellKindValueRange[index].second = m_CellKindValueRange[index].first;
							}

							ImGui::Text("maximum value:");
							if (ImGui::SliderFloat(("max" + indexToString).c_str(), &m_CellKindValueRange[index].second, 0.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
							{
								if (index + 1 < m_CellKindValueRange.size())
								{
									m_CellKindValueRange[index + 1].first = m_CellKindValueRange[index].second;
								}

								if (m_CellKindValueRange[index].second < m_CellKindValueRange[index].first)
									m_CellKindValueRange[index].first = m_CellKindValueRange[index].second;
							}

							ImGui::Spacing();
						}
						else
						{
							CellKindDragDropSource(index);
							
							CellKindDragDropTarget(index);
						}
					}

					if(ImGui::Button("add cell kind", ImVec2(itemWidth, 20)))
					{
						m_ShouldCreateNewCellKind = true;
					}

					ImGui::PopItemWidth();
					ImGui::Unindent();
				}
				ImGui::PopItemWidth();
				ImGui::Unindent();
			}

			ImGui::EndTabItem();
		}

		//if (ImGui::BeginTabItem("Biome"))
		//{
		//
		//	ImGui::EndTabItem();
		//}
		//
		//if (ImGui::BeginTabItem("Cave"))
		//{
		//
		//	ImGui::EndTabItem();
		//}

		ImGui::PopItemWidth();
		ImGui::EndTabBar();
	}

	ImGui::End();
}

void GenerateWorldPerlinNoiseComponent::CellKindDragDropSource(int index)
{
	if (ImGui::BeginDragDropSource())
	{
		int* pData{ new int {index} };
		ImGui::SetDragDropPayload("CellKind", pData, sizeof(int));
		ImGui::Text("Drag and drop to swap color");
		ImGui::EndDragDropSource();
	}
}

void GenerateWorldPerlinNoiseComponent::CellKindDragDropTarget(int index)
{
	if (ImGui::BeginDragDropTarget())
	{
		auto pPayload{ ImGui::AcceptDragDropPayload("CellKind") };

		if (pPayload)
		{
			int data{ *reinterpret_cast<int*>(pPayload->Data) };

			if (data < 0 || data > m_CellKindColors.size())
				return;

			auto tempColor{ m_CellKindColors[data] };

			m_CellKindColors[data] = m_CellKindColors[index];
			m_CellKindColors[index] = tempColor;
		}

		ImGui::EndDragDropTarget();
	}
}

void getThreeValues(float inputFloat, float& value1, float& value2, float& value3) {
	std::default_random_engine generator(static_cast<unsigned int>(inputFloat * 100)); // Seed based on inputFloat
	std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

	value1 = distribution(generator);
	value2 = distribution(generator);
	value3 = distribution(generator);
}

SDL_Color GenerateWorldPerlinNoiseComponent::CalculateCellColor(int x, int y) const
{
	//constexpr float devider{ 128 };
	//
	//double noise{ (m_Perlin2D->GetNoise(x / devider, y / devider, m_PerlinNoiseAmountOfOctaves, m_PerlinNoisePersistence) + 1) / 2.f };
	//
	//for (int index{}; index < m_CellKindValueRange.size(); ++index)
	//{
	//	if (noise > m_CellKindValueRange[index].first && noise < m_CellKindValueRange[index].second)
	//		return FloatVectorToSDLColor(m_CellKindColors[index]);
	//}
	//
	//if (noise < 0.475f)
	//	return SDL_Color(0, 255u, 0);
	//else
	//	return SDL_Color(0, 0, 255u);

	float noise{ m_Worley->GetNoise(x, y) };
	float v1{};
	float v2{};
	float v3{};
	getThreeValues(noise, v1, v2, v3);
	return SDL_Color(static_cast<Uint8>(v1 * 255.f), static_cast<Uint8>(v2 * 255.f), static_cast<Uint8>(v3 * 255.f));
	//return SDL_Color(static_cast<Uint8>(noise * 255.f), static_cast<Uint8>(noise * 255.f), static_cast<Uint8>(noise * 255.f));
}

SDL_Color GenerateWorldPerlinNoiseComponent::FloatVectorToSDLColor(const std::vector<float>& values) const
{
	if (values.size() != 3)
		return SDL_Color();

	return SDL_Color(static_cast<Uint8>(values[0] * 255.f), static_cast<Uint8>(values[1] * 255.f), static_cast<Uint8>(values[2] * 255.f));
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

bool GenerateWorldPerlinNoiseComponent::ShouldRegenerate()
{
	if (m_PerlinNoiseSeed != m_PreviousPerlinNoiseSeed)
	{
		m_Perlin2D->SetSeed(m_PerlinNoiseSeed);
		m_PreviousPerlinNoiseSeed = m_PerlinNoiseSeed;
		return true;
	}

	if (m_PreviousWorldXSize != m_WorldXSize || m_PreviousWorldYSize != m_WorldYSize)
	{
		m_PreviousWorldXSize = m_WorldXSize;
		m_PreviousWorldYSize = m_WorldYSize;
		return true;
	}

	return false;
}

void GenerateWorldPerlinNoiseComponent::Regenerate()
{
	if (!ShouldRegenerate()) return;

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

void GenerateWorldPerlinNoiseComponent::RegenerateMultiThread()
{
	m_IsRegenerating = true;

	if (!ShouldRegenerate()) return;

	// Set the texture's color data
	auto& pixelFormat = m_PixelFormat;
	m_Pixels.resize(m_WorldBiggestSide * m_WorldBiggestSide);
	
	const int numThreads = static_cast<int>(m_ThreadsPixelData.size());

	auto amountOfCols = m_WorldYSize;
	auto rowsPerThread{ static_cast<int>(std::ceil(m_WorldBiggestSide / static_cast<float>(numThreads))) };

	const int chunkSize = rowsPerThread * m_WorldBiggestSide;

	auto biggestSide{ m_WorldBiggestSide };

	for (auto& pixelData : m_ThreadsPixelData)
	{
		pixelData.first.resize(chunkSize + m_WorldBiggestSide);
	}

	for (int threadIndex = 0; threadIndex < numThreads; ++threadIndex)
	{
		int startRow = threadIndex * rowsPerThread;
		int endRow = startRow + rowsPerThread + 1;

		auto& pixelData = m_ThreadsPixelData[threadIndex];

		m_Threads.push_back(std::thread([this, startRow, endRow, biggestSide, &pixelData, threadIndex, chunkSize, amountOfCols, pixelFormat]()
			{
				int index = 0;
				for (int rowNr = startRow; rowNr < endRow; ++rowNr)
				{
					for (int colNr = 0; colNr < amountOfCols; ++colNr)
					{
						if (index >= pixelData.first.size())
							break;

						auto color{ CalculateCellColor(rowNr, colNr) };
						pixelData.first[index++] = SDL_MapRGBA(pixelFormat, color.r, color.g, color.b, color.a);
					}
				}
				pixelData.second = startRow * biggestSide;
			}));
	}
}

void GenerateWorldPerlinNoiseComponent::CreateNewWorldTexture()
{
	for (auto& pixels : m_ThreadsPixelData)
	{
		int currentPixelsVectorIndex{};
		for (int index{ pixels.second }; index < pixels.first.size() + pixels.second; ++index)
		{
			if (index >= m_Pixels.size())
				continue;

			m_Pixels[index] = pixels.first[currentPixelsVectorIndex++];
		}
	}

	auto pColorTexture{ SDL_CreateTexture(dae::Renderer::GetInstance().GetSDLRenderer(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, m_WorldXSize, m_WorldYSize) };
	SDL_UpdateTexture(pColorTexture, nullptr, m_Pixels.data(), m_WorldBiggestSide * sizeof(uint32_t));

	if (!m_pTextureComponent)
	{
		auto pOwner{ GetOwner() };

		// Create a texture component
		auto textureComp{ std::make_unique<dae::TextureComponent>(pOwner, pColorTexture) };

		// Add a renderComponent to the owner of this one
		pOwner->AddComponent(std::make_unique<dae::RenderComponent>(pOwner, std::move(textureComp)));

		m_pTextureComponent = pOwner->GetComponent<dae::TextureComponent>();
	}
	else
		m_pTextureComponent->SetSDLTexture(pColorTexture);
}

void GenerateWorldPerlinNoiseComponent::CreateNewCellKind()
{
	m_CellKindColors.push_back(std::vector<float>{ 0.f, 0.f, 0.f });
	m_CellKindValueRange.push_back({ m_CellKindValueRange[m_CellKindValueRange.size() - 1].second, 1.f });
}