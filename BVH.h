#pragma once
#include <vector>
#include "gsl/span"
#include "shapes.h"

class BVH : hittable
{
	struct node : hittable
	{
		node(BVH& p, gsl::span<hittable> hittables) noexcept;

		[[nodiscard]] std::optional<hit_info> ray_intersect(vec3f const& origin, vec3f const& dir) const noexcept override;
		[[nodiscard]] AABB bounding_box() const noexcept override
		{
				return box;
		}
		
		AABB box;
		BVH* parent;
		hittable* left = nullptr, *right = nullptr;
	};

	explicit BVH(gsl::span<hittable> const& hittables) noexcept;

	[[nodiscard]] std::optional<hit_info> ray_intersect(vec3f const& origin, vec3f const& dir) const noexcept override
	{
		return root.ray_intersect(origin, dir);
	}
	
	[[nodiscard]] AABB bounding_box() const noexcept override
	{
		return root.bounding_box();
	}

	
	std::vector<node> nodes;
	node root;
};
