#pragma once
#include <vector>
#include "BVH.h"
#include "shapes.h"

#define USE_BVH

struct light
{
	light(vec3f p, float in) noexcept : pos(p), intensity(in) {};
	vec3f pos;
	float intensity;
	static constexpr float ambient = 0.0;
};

struct scene
{
	void create_acceleration_structure()
	{
		bvh.create(objects);
	}
	
	[[nodiscard]] std::optional<hit_info> intersect(vec3f const& origin, vec3f const& dir) const noexcept
	{
#ifdef USE_BVH
		return bvh.ray_intersect(origin, dir);
#else	
		std::optional<hit_info> result;
		float dist = std::numeric_limits<float>::max();
		for (auto const& object : objects)
		{
			if (auto const hInfo = object->ray_intersect(origin, dir))
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
#endif
	}

	BVH_node bvh;
	std::vector<light> lights;
	std::vector<std::shared_ptr<hittable>> objects;
};
