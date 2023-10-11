#include "WorleyNoise.h"
#include <random>
#include <iostream>

WorleyNoise::WorleyNoise(unsigned int seed, int xSize, int ySize, double featurePointRatio)
	: m_XSize{ xSize }
	, m_YSize{ ySize }
	, m_BiggestSide{ xSize }
{
	if (m_BiggestSide < m_YSize)
		m_BiggestSide = m_YSize;

	std::mt19937 rng{ seed };
	std::uniform_int_distribution<int> distrubutionX{ 0, xSize - 1 };
	std::uniform_int_distribution<int> distrubutionY{ 0, ySize - 1 };

	//calculate amount of featurePoints
	int amountOfFeaturePoints{ static_cast<int>(xSize * ySize * featurePointRatio) };

	//initalize feature points
	m_FeaturePoints.resize(amountOfFeaturePoints);
	for (int index{}; index < amountOfFeaturePoints; ++index)
	{
		m_FeaturePoints[index].x = distrubutionX(rng);
		m_FeaturePoints[index].y = distrubutionY(rng);
	}
}

float WorleyNoise::GetNoise(int x, int y)
{
	float minimumDistance{ std::numeric_limits<float>::max() };

	int closestPointIndex{};

	for (int index{}; index < m_FeaturePoints.size(); ++index)
	{
		int diffX{ m_FeaturePoints[index].x - x};
		int diffY{ m_FeaturePoints[index].y - y};

		float distance{ std::sqrt(static_cast<float>(diffX * diffX + diffY * diffY)) };

		if (distance < minimumDistance)
		{
			minimumDistance = std::min(minimumDistance, distance);
			closestPointIndex = index;
		}
	}

	return DeterministicFunction(closestPointIndex);

	//for (auto& featurePoint : m_FeaturePoints)
	//{
	//	int diffX{ featurePoint.x - x };
	//	int diffY{ featurePoint.y - y };
	//	
	//	float distance{ std::sqrt(static_cast<float>(diffX * diffX + diffY * diffY)) };
	//	minimumDistance = std::min(minimumDistance, distance);
	//}	
	//
	//return minimumDistance / static_cast<float>(m_BiggestSide / 9.f);
}

float WorleyNoise::MapToRange(float input, float minInput, float maxInput, float minOutput, float maxOutput)
{
	return minOutput + (maxOutput - minOutput) * ((input - minInput) / (maxInput - minInput));
}

float WorleyNoise::DeterministicFunction(int input)
{
	return MapToRange(static_cast<float>(input), 0.f, static_cast<float>(m_FeaturePoints.size()), 0.f, 1.f);
}