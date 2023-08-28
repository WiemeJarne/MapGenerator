#include "PlayerController.h"
#include <Windows.h>
#include <XInput.h>
#include "InputManager.h"

class PlayerController::PlayerControllerImpl final
{
public:
	PlayerControllerImpl(int controllerIndex)
	{
		if (controllerIndex == -1)
		{
			m_ControllerIndex = dae::InputManager::GetInstance().GetAmountOfControllers();
		}
		else
		{
			m_ControllerIndex = controllerIndex;
		}
	}

	~PlayerControllerImpl() = default;
	PlayerControllerImpl(const PlayerControllerImpl& other) = delete;
	PlayerControllerImpl(PlayerControllerImpl&& other) = default;
	PlayerControllerImpl& operator=(const PlayerControllerImpl& other) = delete;
	PlayerControllerImpl& operator=(PlayerControllerImpl&& other) = delete;

	void HandleControllerInput()
	{
		if (m_ControllerIndex < 0)
			return;

		HandleControllerButtonCommands();
		HandleControllerAxisCommands();
	}

	void HandleControllerButtonCommands()
	{
		CopyMemory(&m_PreviousState, &m_CurrentState, sizeof(XINPUT_STATE));
		ZeroMemory(&m_CurrentState, sizeof(XINPUT_STATE));
		XInputGetState(m_ControllerIndex, &m_CurrentState);

		auto buttonChanges{ m_CurrentState.Gamepad.wButtons ^ m_PreviousState.Gamepad.wButtons };
		m_ButtonsPressedThisFrame = buttonChanges & m_CurrentState.Gamepad.wButtons;
		m_ButtonsReleasedThisFrame = buttonChanges & (~m_CurrentState.Gamepad.wButtons);

		for (const auto& command : m_ControllerButtonCommands)
		{
			switch (command.first.first) //check what state the button should be for the command to be executed
			{
			case dae::KeyState::down:
				//check if the button is down in the current frame if so execute the command
				if (IsDownThisFrame(command.first.second))
				{
					command.second->Execute();
				}
				break;

			case dae::KeyState::up:
				//check if the button is up in the current frame if so execute the command
				if (IsUpThisFrame(command.first.second))
				{
					command.second->Execute();
				}
				break;

			case dae::KeyState::pressed:
				//check if the button is pressed if so execute the command
				if (IsPressed(command.first.second))
				{
					command.second->Execute();
				}
				break;
			}
		}
	}

	void HandleControllerAxisCommands()
	{
		constexpr float epsilon{ 0.01f };

		for (const auto& command : m_ControllerAxisCommands)
		{
			glm::vec2 thumbstickDirection{};

			switch (command.first)
			{
			case ControllerAxis::tumbStickLeft:
			case ControllerAxis::tumbStickRight:
				thumbstickDirection = GetThumbstickDirection(command.first);

				if (abs(thumbstickDirection.x) > epsilon || abs(thumbstickDirection.y) > epsilon)
				{
					command.second->SetThumbstickDirection(thumbstickDirection);
					command.second->Execute();
				}

				break;
			}
		}
	}

	glm::vec2 GetThumbstickDirection(ControllerAxis thumbStick) const
	{
		glm::vec2 thumbStickDirection{};

		switch (thumbStick)
		{
		case ControllerAxis::tumbStickLeft:
			thumbStickDirection = glm::vec2(m_CurrentState.Gamepad.sThumbLX, m_CurrentState.Gamepad.sThumbLY);
			break;

		case ControllerAxis::tumbStickRight:
			thumbStickDirection = glm::vec2(m_CurrentState.Gamepad.sThumbRX, m_CurrentState.Gamepad.sThumbRY);
			break;
		}

		if (m_IsThumbstickLeftYAxisInverted)
		{
			thumbStickDirection.y *= -1;
		}

		const float deadZone{ 0.15f * m_ThumbStickMaxValue };

		if (abs(thumbStickDirection.x) < deadZone && abs(thumbStickDirection.y) < deadZone)
			return glm::vec2(0.f, 0.f);

		if (abs(thumbStickDirection.x) < deadZone)
			thumbStickDirection.x = 0.f;
		else if (abs(thumbStickDirection.y) < deadZone)
			thumbStickDirection.y = 0.f;

		return(thumbStickDirection - deadZone) / (m_ThumbStickMaxValue - deadZone);
	}

	void AddCommand(std::unique_ptr<commands::Command> command, Control controllerKey)
	{
		m_ControllerButtonCommands.insert(std::pair<Control, std::unique_ptr<commands::Command>>(controllerKey, std::move(command)));
	}

	void AddCommand(std::unique_ptr<commands::ThumbstickCommand> command, ControllerAxis controllerAxis)
	{
		m_ControllerAxisCommands.insert(std::pair<ControllerAxis, std::unique_ptr<commands::ThumbstickCommand>>(controllerAxis, std::move(command)));
	}

	void InvertThumbstickLeftYAxis()
	{
		m_IsThumbstickLeftYAxisInverted = true;
	}

private:

	bool IsDownThisFrame(ControllerKey button) const
	{
		return m_ButtonsPressedThisFrame & static_cast<unsigned int>(button);
	}

	bool IsUpThisFrame(ControllerKey button) const
	{
		return m_ButtonsReleasedThisFrame & static_cast<unsigned int>(button);
	}

	bool IsPressed(ControllerKey button) const
	{
		return m_CurrentState.Gamepad.wButtons & static_cast<unsigned int>(button);
	}

	XINPUT_STATE m_PreviousState{};
	XINPUT_STATE m_CurrentState{};
	int m_ControllerIndex{};
	unsigned int m_ButtonsPressedThisFrame{};
	unsigned int m_ButtonsReleasedThisFrame{};
	using ControllerButtonCommandsMap = std::map<Control, std::unique_ptr<commands::Command>>;
	ControllerButtonCommandsMap m_ControllerButtonCommands{};
	using ControllerAxisCommandsMap = std::map<ControllerAxis, std::unique_ptr<commands::ThumbstickCommand>>;
	ControllerAxisCommandsMap m_ControllerAxisCommands{};
	const float m_ThumbStickMaxValue{ 32768.f };
	bool m_IsThumbstickLeftYAxisInverted{};
};

PlayerController::PlayerController()
	: m_PlayerControllerImpl{ std::make_unique<PlayerControllerImpl>(-1) } //when the playerController is created for keyboard input then the controller index is set to -1
{}

PlayerController::PlayerController(int controllerIndex)
	: m_PlayerControllerImpl{ std::make_unique<PlayerControllerImpl>(controllerIndex) }
{}

PlayerController::~PlayerController() = default;

void PlayerController::Update()
{
	m_PlayerControllerImpl->HandleControllerInput();
}

void PlayerController::AddCommand(std::unique_ptr<commands::Command> command, Control controllerKey)
{
	m_PlayerControllerImpl->AddCommand(std::move(command), controllerKey);
}

void PlayerController::AddCommand(std::unique_ptr<commands::ThumbstickCommand> command, ControllerAxis controllerAxis)
{
	m_PlayerControllerImpl->AddCommand(std::move(command), controllerAxis);
}

void PlayerController::InvertThumbstickLeftYAxis()
{
	m_PlayerControllerImpl->InvertThumbstickLeftYAxis();
}