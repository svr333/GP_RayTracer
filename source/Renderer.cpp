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

			// no hit, color black
			if (!closestHit.didHit)
			{
				m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));

				continue;
			}

			auto lights = pScene->GetLights();

			for (size_t i = 0; i < lights.size(); i++)
			{
				auto direction = LightUtils::GetDirectionToLight(lights[i], closestHit.origin).Normalized();

				// obstacle in way, light does not give direct hit, also results in giving shadows
				if (m_ShadowsEnabled && pScene->DoesHit({ closestHit.origin + closestHit.normal * 0.1f, direction.Normalized(), 0.0001f, direction.Magnitude() }))
				{
					continue;
				}

				auto dot = Vector3::Dot(closestHit.normal, direction);

				if (dot < 0)
				{
					continue;
				}

				auto radiance = LightUtils::GetRadiance(lights[i], closestHit.origin);

				switch (m_CurrentLightingMode)
				{
				case dae::Renderer::LightingMode::ObservedArea:
					finalColor += { dot, dot, dot };
					break;
				case dae::Renderer::LightingMode::Radiance:
					finalColor += radiance * dot;
					break;
				case dae::Renderer::LightingMode::BRDF:
					finalColor += materials[closestHit.materialIndex]->Shade(closestHit, direction, viewRay.direction);
					break;
				case dae::Renderer::LightingMode::Combined:
					finalColor += radiance * materials[closestHit.materialIndex]->Shade(closestHit, direction, viewRay.direction) * dot;
					break;
				default:
					break;
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

void Renderer::CycleLightingMode()
{
	m_CurrentLightingMode = (LightingMode)((int)m_CurrentLightingMode + 1);
	m_CurrentLightingMode = (LightingMode)((int)m_CurrentLightingMode % (int)LightingMode::Max);
}
