#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"
#include <iostream>

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
			CalculateCameraToWorld();
		}

		Vector3 origin{};
		float fovAngle{90.f};

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch { 0.0f };
		float totalYaw { 0.0f };

		Matrix cameraToWorld{};
		const float camVelocity = 5.0f;
		const float angleVelocity = 0.09f;

		Matrix CalculateCameraToWorld()
		{
			const Vector3 camRight{ (Vector3::Cross(up, forward)).Normalized() };
			const Vector3 camUp{ (Vector3::Cross(forward, right)).Normalized() };

			cameraToWorld = Matrix{ camRight, camUp, forward, origin };

			return cameraToWorld;
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			// Get current keyboard state
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			// Get current mouse state
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			// WASD movement
			if (pKeyboardState[SDL_SCANCODE_W])
			{
				origin += forward * camVelocity * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_S])
			{
				origin -= forward * camVelocity * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_A])
			{
				origin -= cameraToWorld.GetAxisX() * camVelocity * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_D])
			{
				origin += cameraToWorld.GetAxisX() * camVelocity * deltaTime;
			}

			// Rotate logic
			if (mouseState & SDL_BUTTON_RMASK)
			{
				totalPitch -= mouseY * angleVelocity * deltaTime;
				totalYaw -= mouseX * angleVelocity * deltaTime;

				forward = Matrix::CreateRotation(totalPitch, totalYaw, 0.0f).TransformVector(Vector3::UnitZ);
			}

			CalculateCameraToWorld();
		}
	};
}
