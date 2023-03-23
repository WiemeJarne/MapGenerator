#include "MoveLeftCommand.h"
#include "Timer.h"
#include <iostream>

using namespace commands;

MoveLeftCommand::MoveLeftCommand(dae::GameObject* go, float moveSpeed)
	: Command(go)
	, m_MoveSpeed { moveSpeed }
{
	m_PreviousLocalPos = go->GetLocalPos();
}

void MoveLeftCommand::Execute()
{
	auto go{ GetGameObject() };
	m_PreviousLocalPos = go->GetLocalPos();
	glm::vec3 newPos{ m_PreviousLocalPos };
	newPos.x = m_PreviousLocalPos.x - Timer::GetInstance().GetElapsedSec() * m_MoveSpeed;
	go->SetLocalPosition(newPos.x, newPos.y);
}

void MoveLeftCommand::Undo()
{
	auto go{ GetGameObject() };
	go->SetLocalPosition(m_PreviousLocalPos.x, m_PreviousLocalPos.y);
}