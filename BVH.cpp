#include "BVH.h"
#include <ctime>
#include <algorithm>

std::optional<hit_info> BVH::node::ray_intersect(vec3f const& origin, vec3f const& dir) const noexcept
{
	if (!box.ray_hit(origin, dir))
		return {};
	
	auto const lhit = left->ray_intersect(origin, dir);
	auto const rhit = right->ray_intersect(origin, dir);
	
	if (lhit && rhit)
		return (lhit->pos - origin).norm2() < (rhit->pos - origin).norm2() ? lhit : rhit;

	return lhit ? lhit : rhit;
}

BVH::BVH(gsl::span<hittable> const& hittables) noexcept
	: root(*this, hittables)
{
}

BVH::node::node(BVH& p, gsl::span<hittable> hittables) noexcept
	: parent(&p)
{
	srand(time(0));
	int const axis = rand() % 3;
	auto box_cmp = [axis](auto a, auto b)
	{
		return a.bounding_box().min[axis] < b.bounding_box().min[axis];
	};
	
	std::sort(hittables.begin(), hittables.end(), box_cmp);
	auto const mid = hittables.size() / 2;
	left = &p.nodes.emplace_back(hittables.subspan(0, mid));
	right = &p.nodes.emplace_back(hittables.subspan(mid, hittables.size() - mid));
	box = AABB::surrounding_box(left->bounding_box(), right->bounding_box());
}
