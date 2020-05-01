#pragma once
#include <vector>
#include "shapes.h"

struct light
{
	light(vec3f p, float in) noexcept : pos(p), intensity(in) {};
	vec3f pos;
	float intensity;
	static constexpr float ambient = 0.0;
};

struct scene
{
	[[nodiscard]] std::optional<hit_info> scene_intersect(vec3f const& origin, vec3f const& dir) noexcept
	{
		std::optional<hit_info> result;
		float dist = std::numeric_limits<float>::max();
		for (auto const& object : objects)
		{
			if (auto const hInfo = object.ray_intersect(origin, dir))
			{
				float const c_dist = (hInfo->pos - origin).norm2();
				if (c_dist < dist)
				{
					dist = c_dist;
					result = hInfo;
				}
			}
		}
		
		return result;
	}
	
	std::vector<light> lights;
	std::vector<hittable> objects;
};
