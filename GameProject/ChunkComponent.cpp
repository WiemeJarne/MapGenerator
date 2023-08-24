#include "ChunkComponent.h"
#include "GenerateWorldPerlinNoiseComponent.h"
#include "Renderer.h"
#include "GameObject.h"

ChunkComponent::ChunkComponent(dae::GameObject* pOwner, int topLeftX, int topLeftY, GenerateWorldPerlinNoiseComponent* worldGenerateComponent)
	: Component(pOwner)
	, m_WorldGenerateComponent{ worldGenerateComponent }
{
	m_CellColors.resize(256);

	for (int rowNr{}; rowNr < 16; ++rowNr)
	{
		for (int colNr{}; colNr < 16; ++colNr)
		{
			m_CellColors[rowNr * 16 + colNr] = m_WorldGenerateComponent->CalculateCellColor(rowNr + topLeftX, colNr + topLeftY);
		}
	}
}

void ChunkComponent::Render() const
{
	auto renderer{ dae::Renderer::GetInstance().GetSDLRenderer() };
	auto topLeftPos{ GetOwner()->GetLocalPos() };

	for (int rowNr{}; rowNr < 16; ++rowNr)
	{
		for (int colNr{}; colNr < 16; ++colNr)
		{
			auto color{ m_CellColors[rowNr * 16 + colNr] };
			SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
			SDL_RenderDrawPoint(renderer, static_cast<int>(topLeftPos.x + rowNr), static_cast<int>(topLeftPos.y + colNr));
		}
	}
}