#include <stdexcept>
#include "Renderer.h"
#include "SceneManager.h"
#include "TextureComponent.h"
#include "imgui_impl_opengl2.h"
#include "imgui_impl_sdl2.h"
#include <iostream>

int GetOpenGLDriverIndex()
{
	auto openglIndex = -1;
	const auto driverCount = SDL_GetNumRenderDrivers();
	for (auto i = 0; i < driverCount; i++)
	{
		SDL_RendererInfo info;
		if (!SDL_GetRenderDriverInfo(i, &info))
			if (!strcmp(info.name, "opengl"))
				openglIndex = i;
	}
	return openglIndex;
}

void dae::Renderer::Init(SDL_Window* window)
{
	m_window = window;
	m_renderer = SDL_CreateRenderer(window, GetOpenGLDriverIndex(), SDL_RENDERER_ACCELERATED);
	if (m_renderer == nullptr) 
	{
		throw std::runtime_error(std::string("SDL_CreateRenderer Error: ") + SDL_GetError());
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplSDL2_InitForOpenGL(window, SDL_GL_GetCurrentContext());
	ImGui_ImplOpenGL2_Init();
}

void dae::Renderer::Render()
{
	const auto& color = GetBackgroundColor();
	SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
	SDL_RenderClear(m_renderer);

	SceneManager::GetInstance().Render();

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame(m_window);
	ImGui::NewFrame();

	SceneManager::GetInstance().RenderImGui();

	if (m_showDemo)
		ImGui::ShowDemoWindow();
	ImGui::Render();
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
	
	SDL_RenderPresent(m_renderer);
}

void dae::Renderer::Destroy()
{
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	if (m_renderer != nullptr)
	{
		SDL_DestroyRenderer(m_renderer);
		m_renderer = nullptr;
	}
}

void dae::Renderer::RenderTexture(const TextureComponent& texture, const float x, const float y) const
{
	int cameraXPos{};
	int cameraYPos{};

	if (m_pActiveCamera)
	{
		cameraXPos = m_pActiveCamera->GetXPos();
		cameraYPos = m_pActiveCamera->GetYPos();
	}

	SDL_Rect dst{};

	SDL_QueryTexture(texture.GetSDLTexture(), nullptr, nullptr, &dst.w, &dst.h);

	dst.x = static_cast<int>(x) + cameraXPos;
	dst.y = static_cast<int>(y) + cameraYPos;

	if (m_pActiveCamera)
	{
		float zoom{ (1.f / m_pActiveCamera->GetZoom()) };
		dst.h = static_cast<int>(dst.h * zoom);
		dst.w = static_cast<int>(dst.w * zoom);
	}
	
	SDL_RenderCopy(GetSDLRenderer(), texture.GetSDLTexture(), nullptr, &dst);
}

void dae::Renderer::RenderTexture(const TextureComponent& texture, const float x, const float y, const float width, const float height) const
{
	SDL_Rect dst{};
	dst.x = static_cast<int>(x);
	dst.y = static_cast<int>(y);
	dst.w = static_cast<int>(width);
	dst.h = static_cast<int>(height);
	SDL_RenderCopy(GetSDLRenderer(), texture.GetSDLTexture(), nullptr, &dst);
}

inline SDL_Renderer* dae::Renderer::GetSDLRenderer() const { return m_renderer; }

void dae::Renderer::ActiveCameraChanged(Camera* pCamera)
{
	m_pActiveCamera = pCamera;
}
