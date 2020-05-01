#pragma once
#include <algorithm>
#include "geometry.h"
#include "material.h"
#include <optional>

// https://raytracing.github.io/books/RayTracingTheNextWeek.html#boundingvolumehierarchies

struct hit_info
{
	vec3f pos;
	vec3f normal;
	material mtrl;
};

struct AABB
{
	vec3f min{}, max{};
	
	bool ray_hit(vec3f origin, vec3f dir) const
	{
		float tmin = -std::numeric_limits<float>::max();
		float tmax = std::numeric_limits<float>::max();
		for (int dim = 0; dim < 3; dim++)
		{
			auto const invD = 1.0f / dir[dim];
			auto t0 = (min[dim] - origin[dim]) * invD;
			auto t1 = (max[dim] - origin[dim]) * invD;

			if (invD < 0.0f)
				std::swap(t0, t1);
			
			tmin = std::max(t0, tmin);
			tmax = std::min(t1, tmax);
			if (tmax <= tmin)
				return false;
		}
		return true;
	}

	static AABB surrounding_box(AABB box0, AABB box1) noexcept
	{
		vec3f const small(	std::min(box0.min.x, box1.min.x),
							std::min(box0.min.y, box1.min.y),
							std::min(box0.min.z, box1.min.z));
		
		vec3f const big(	std::max(box0.max.x, box1.max.x),
							std::max(box0.max.y, box1.max.y),
							std::max(box0.max.z, box1.max.z));
		
		return AABB{ small, big };
	}
};

class hittable
{
public:
	[[nodiscard]] virtual std::optional<hit_info> ray_intersect(vec3f const& origin, vec3f const& dir) const noexcept = 0;
	[[nodiscard]] virtual AABB bounding_box() const noexcept = 0;
	virtual ~hittable() = default;
};

struct sphere : hittable
{
	vec3f pos;
	float radius;
	material mtrl;
	sphere(vec3f p, float r, material m) : pos(p), radius(r), mtrl(m) {}
	
	[[nodiscard]] std::optional<hit_info> ray_intersect(vec3f const& origin, vec3f const& dir) const noexcept override
	{
		hit_info hinfo;
		vec3f const f = origin - pos;
		if (f.norm2() < radius * radius)
			return {};

		float const a = dot(dir, dir);
		float const b = 2 * dot(dir, f);
		float const c = dot(f, f) - radius * radius;
		float const delta = b * b - 4 * a * c;
		if (delta <= 0.0f)
			return {};
		float const sdelta = sqrt(delta);
		float t0 = (-b + sdelta) / 2.0f;
		float t1 = (-b - sdelta) / 2.0f;
		if (t0 > t1)
			std::swap(t0, t1);
		if (t0 < 0) {
			t0 = t1; // if t0 is negative, let's use t1 instead 
			if (t0 < 0) return {}; // both t0 and t1 are negative
		}
		float const t = t0;
		hinfo.pos = origin + dir * t;
		hinfo.normal = (hinfo.pos - pos).normalize();
		hinfo.mtrl = mtrl;
		return hinfo;
	}
	[[nodiscard]] AABB bounding_box() const noexcept override
	{
		return { pos - radius, pos + radius };
	}
};

// @TODO convert to quad
struct plan
{
	vec3f pos;
	vec3f normal;
	material mtrl;
	
	[[nodiscard]] std::optional<hit_info> ray_intersect(vec3f const& origin, vec3f const& dir) const noexcept
	{
		float const d = dot(-normal, dir);
		if (d > FLT_EPSILON)
		{
			vec3f const p = pos - origin;
			float const t = dot(p, -normal) / d;

			if (t < 0)
				return {};

			hit_info result;
			result.pos = origin + dir * t;
			result.normal = normal;
			result.mtrl = mtrl;

			return { result };
		}
		return {};
	}
};

