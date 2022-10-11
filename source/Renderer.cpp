//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window * pWindow) :
	m_pWindow(pWindow),
	m_pBuffer(SDL_GetWindowSurface(pWindow))
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}

void Renderer::Render(Scene* pScene) const
{
	auto useHardShadows = false;

	Camera& camera = pScene->GetCamera();
	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();

	auto aspectRatio = m_Width / float(m_Height);
	auto FOV = tan(camera.fovAngle / 2);

	const Matrix cameraToWorld = camera.CalculateCameraToWorld();

	for (int px{}; px < m_Width; ++px)
	{
		double xcs = ((2 * (px + 0.5) / m_Width) - 1) * aspectRatio * FOV;

		for (int py{}; py < m_Height; ++py)
		{
			double ycs = (1 - (2 * (py + 0.5) / m_Height)) * FOV;

			auto rayDirection = cameraToWorld.TransformVector(xcs * Vector3::UnitX + ycs * Vector3::UnitY + Vector3::UnitZ);
			Ray viewRay{ camera.origin, rayDirection.Normalized() };

			HitRecord closestHit{};
			ColorRGB finalColor{};

			pScene->GetClosestHit(viewRay, closestHit);

			if (closestHit.didHit)
			{
				//finalColor = materials[closestHit.materialIndex]->Shade();

				auto lights = pScene->GetLights();

				for (size_t i = 0; i < lights.size(); i++)
				{
					auto radiance = LightUtils::GetRadiance(lights[i], closestHit.origin);
					auto direction = LightUtils::GetDirectionToLight(lights[i], closestHit.origin).Normalized();
					auto dot = Vector3::Dot(closestHit.normal, direction);

					if (dot >= 0)
					{
						finalColor += radiance * dot;
					}
				}

				if (useHardShadows)
				{
					auto lights = pScene->GetLights();

					for (size_t i = 0; i < lights.size(); i++)
					{
						auto direction = LightUtils::GetDirectionToLight(lights[i], closestHit.origin);

						if (pScene->DoesHit({ closestHit.origin + closestHit.normal * 0.1f, direction.Normalized(), 0.0001f, direction.Magnitude() }))
						{
							finalColor *= 0.5f;
						}
					}
				}
			}

			finalColor.MaxToOne();

			m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}
