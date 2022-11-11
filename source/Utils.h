#pragma once
#include <cassert>
#include <fstream>
#include "Math.h"
#include "DataTypes.h"

namespace dae
{
	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS
		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			auto diffRayToSphere = ray.origin - sphere.origin;
			const float B = Vector3::Dot(2 * ray.direction, diffRayToSphere);
			const float C = Vector3::Dot(diffRayToSphere, diffRayToSphere) - sphere.radius * sphere.radius;

			auto discriminant = B * B - 4 * C;

			if (discriminant < 0.00001f)
			{
				return false;
			}

			auto t = (-B - sqrt(discriminant)) / 2;

			if (t < ray.min || t > ray.max)
			{
				return false;
			}

			auto hitPoint = ray.origin + t * ray.direction;
			hitRecord = HitRecord{ hitPoint, (hitPoint - sphere.origin) / sphere.radius, t, true, sphere.materialIndex };
			return true;
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Sphere(sphere, ray, temp, true);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const float numerator = Vector3::Dot(plane.origin - ray.origin, plane.normal);
			const float denominator = Vector3::Dot(ray.direction, plane.normal);

			const float t = numerator / denominator;

			if (t < ray.min || t > ray.max)
			{
				return false;
			}

			const auto hitPoint = ray.origin + t * ray.direction;

			hitRecord = HitRecord{ hitPoint, plane.normal, t, true, plane.materialIndex };
			return true;
		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Plane(plane, ray, temp, true);
		}
#pragma endregion
#pragma region Triangle HitTest
		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			auto dot = Vector3::Dot(triangle.normal, ray.direction);

			if (!ignoreHitRecord)
			{
				switch (triangle.cullMode)
				{
					case dae::TriangleCullMode::FrontFaceCulling:
						if (dot < 0)
						{
							return false;
						}
						break;
					case dae::TriangleCullMode::BackFaceCulling:
						if (dot > 0)
						{
							return false;
						}
						break;
					case dae::TriangleCullMode::NoCulling:
					default:
						break;
				}
			}

			auto L = triangle.v0 + triangle.v1 + triangle.v2 / 3 - ray.origin;
			auto t = Vector3::Dot(L, triangle.normal) / Vector3::Dot(ray.direction, triangle.normal);

			// out of range of ray
			if (t < ray.min || t > ray.max)
			{
				return false;
			}

			// get point in plane of triangle
			auto p = ray.origin + t * ray.direction;

			// check if point is on correct side of each of the triangles side
			auto edgeA = triangle.v1 - triangle.v0;
			auto pointToSide = p - triangle.v0;

			if (Vector3::Dot(triangle.normal, Vector3::Cross(edgeA, pointToSide)) < 0)
			{
				return false;
			}

			auto edgeB = triangle.v2 - triangle.v1;
			pointToSide = p - triangle.v1;

			if (Vector3::Dot(triangle.normal, Vector3::Cross(edgeB, pointToSide)) < 0)
			{
				return false;
			}

			auto edgeC = triangle.v0 - triangle.v2;
			pointToSide = p - triangle.v2;

			if (Vector3::Dot(triangle.normal, Vector3::Cross(edgeC, pointToSide)) < 0)
			{
				return false;
			}

			hitRecord = HitRecord{ p, triangle.normal, t, true, triangle.materialIndex };
			return true;
		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}
#pragma endregion
#pragma region TriangeMesh HitTest
		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			for (size_t i = 0; i < mesh.indices.size(); i += 3)
			{
				Triangle triangle = { mesh.transformedPositions[mesh.indices[i]], mesh.transformedPositions[mesh.indices[i + 1]], mesh.transformedPositions[mesh.indices[i + 2]] };
				triangle.normal = mesh.transformedNormals[i / 3];
				triangle.materialIndex = mesh.materialIndex;
				triangle.cullMode = mesh.cullMode;

				HitRecord lastHit;

				if (HitTest_Triangle(triangle, ray, lastHit))
				{
					if (hitRecord.t > lastHit.t)
					{
						hitRecord = lastHit;
					}
				}
			}

			return hitRecord.didHit;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_TriangleMesh(mesh, ray, temp, true);
		}
#pragma endregion
	}

	namespace LightUtils
	{
		//Direction from target to light
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3 origin)
		{
			if (light.type == LightType::Point)
			{
				return light.origin - origin;
			}
			else
			{
				// shrug
			}
		}

		inline ColorRGB GetRadiance(const Light& light, const Vector3& target)
		{
			if (light.type == LightType::Point)
			{
				return light.color * light.intensity / (light.origin - target).SqrMagnitude();
			}
			else
			{
				return light.color * light.intensity;
			}
		}
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<int>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.push_back((int)i0 - 1);
					indices.push_back((int)i1 - 1);
					indices.push_back((int)i2 - 1);
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');

				if (file.eof()) 
					break;
			}

			//Precompute normals
			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if(isnan(normal.x))
				{
					int k = 0;
				}

				normal.Normalize();
				if (isnan(normal.x))
				{
					int k = 0;
				}

				normals.push_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}