#include "BVH.h"
#include <ctime>
#include <algorithm>

std::optional<hit_info> BVH_node::ray_intersect(vec3f const& origin, vec3f const& dir) const noexcept
{
	if (!box.ray_hit(origin, dir))
		return {};
	
	auto const lhit = left->ray_intersect(origin, dir);
	auto const rhit = right->ray_intersect(origin, dir);
	
	if (lhit && rhit)
		return (lhit->pos - origin).norm2() < (rhit->pos - origin).norm2() ? lhit : rhit;

	return lhit ? lhit : rhit;
}

BVH_node::BVH_node(gsl::span<std::shared_ptr<hittable>> hittables) noexcept
{
	create(hittables);
}

void BVH_node::create(gsl::span<std::shared_ptr<hittable>> hittables) noexcept
{
	srand(time(0));
	int const axis = rand() % 3;
	auto box_cmp = [axis](auto a, auto b)
	{
		return a->bounding_box().min[axis] < b->bounding_box().min[axis];
	};

	if (hittables.size() == 1)
	{
		left = right = hittables.front();
	}
	else if (hittables.size() == 2)
	{
		if (box_cmp(hittables[0], hittables[1]))
		{
			left = hittables[0];
			right = hittables[1];
		}
		else
		{
			left = hittables[1];
			right = hittables[0];
		}
	}
	else
	{
		std::sort(hittables.begin(), hittables.end(), box_cmp);
		auto const mid = hittables.size() / 2;
		left = std::make_unique<BVH_node>(hittables.first(mid));
		right = std::make_unique<BVH_node>(hittables.subspan(mid, hittables.size() - mid));
	}
	
	box = AABB::surrounding_box(left->bounding_box(), right->bounding_box());
}
