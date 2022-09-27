#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

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

		Vector3 forward{0.266f, -0.453f, 0.860f};
		//Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{0.f};
		float totalYaw{0.f};

		Matrix cameraToWorld{};

		Matrix CalculateCameraToWorld()
		{
			const Vector3 camRight{ (Vector3::Cross(up, forward)).Normalized()};
			const Vector3 camUp{ (Vector3::Cross(forward, right)).Normalized() };

			cameraToWorld = Matrix{ { camRight.x, camRight.y, camRight.z, 0 },
									{ camUp.x, camUp.y, camUp.z, 0 },
									{ forward.x, forward.y, forward.z, 0 },
									{ origin.x, origin.y, origin.z, 1 } };

			return cameraToWorld;
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);


			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			//todo: W2
			//assert(false && "Not Implemented Yet");

			CalculateCameraToWorld();
		}
	};
}
