#pragma once
#include <memory>
#include "shapes.h"
#include "gsl/span"

struct BVH_node : hittable
{
	BVH_node() = default;
	BVH_node(gsl::span<std::shared_ptr<hittable>> hittables) noexcept;
	void create(gsl::span<std::shared_ptr<hittable>> hittables) noexcept;

	[[nodiscard]] std::optional<hit_info> ray_intersect(vec3f const& origin, vec3f const& dir) const noexcept override;
	[[nodiscard]] AABB bounding_box() const noexcept override
		{
				return box;
		}
	
	AABB box;
	std::shared_ptr<hittable> left = nullptr, right = nullptr;
};
