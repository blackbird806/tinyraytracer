#include <limits>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <optional>
#include "geometry.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define NOP [](){}()

using color = vec4f;

namespace Color
{
	const color black = { 0.0f, 0.0f, 0.0f, 1.0f };
	const color none = { 0.0f, 0.0f, 0.0f, 0.0f };
	const color red = { 1.0f, 0, 0, 1.0f };
	const color blue = { 0, 0, 1.0f, 1.0f };
	const color green = { 0, 1.0f, 0, 1.0f };
	const color yellow = { 1.0f, 1.0f, 0, 1.0f };
	const color orange = { 1.0f, 0.0f, 1.0, 1.0f };
	const color white = { 1.0f, 1.0f, 1.0f, 1.0f };
}

struct light
{
	light(vec3f p, float in) noexcept : pos(p), intensity(in) {};
	vec3f pos;
	float intensity;
};

struct material
{
	color col;
	float kd, ks, kr;
	float reflect = 0.0f;
	float refraction_index = 0.0f;
	float specular_exponent = 10.f;
};

struct hitInfo
{
	vec3f pos;
	vec3f normal;
	material mtrl;
};

struct drawable
{
	material mtrl;
};

struct sphere : drawable
{
	vec3f pos;
	float radius;

	sphere(vec3f const& p, float r, color const& c = Color::green) noexcept : drawable{ material{c} }, pos(p), radius(r) {}
	sphere(vec3f const& p, float r, material const& m) noexcept : drawable{m}, pos(p), radius(r) {}

	[[nodiscard]] std::optional<hitInfo> ray_intersect(vec3f const& origin, vec3f dir) const noexcept
	{
		hitInfo hinfo;
		vec3f const f = origin - pos;
		if (f.norm() < radius)
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
};

struct plan : drawable
{
	plan(vec3f const& p, vec3f const& n, material const& m) noexcept  : drawable{m}, pos(p), normal(n) { }

	[[nodiscard]] std::optional<hitInfo> ray_intersect(vec3f const& origin, vec3f dir) const noexcept
	{
		// not sure why I need to return the inverse of the normal
		float const d = dot(-normal, dir);
		if (d > FLT_EPSILON)
		{
			vec3f const p = pos - origin;
			float const t = dot(p, -normal) / d;
			
			if (t < 0)
				return {};
			
			hitInfo result;
			result.pos = origin + dir * t;
			result.normal = normal;
			result.mtrl = mtrl;

			return {result};
		}
		return {};
	}
	
	vec3f pos;
	vec3f normal;
};

class renderer
{
	public:

	renderer(size_t iwidth, size_t iheight, float ifov) noexcept : image(iwidth * iheight), width(iwidth), height(iheight), fov(ifov)
	{
		clear();
	}
	
	void init_scene()
	{
		material      ivory = { color{0.4f, 0.4f, 0.3f, 1.0f}, 0.6, 0.3, 0.0, 0.1, 1.0,50. };
		material      glass = { color{0.6,  0.7, 0.8, 1.0f}, 0.0, 0.5, 0.8, 0.1, 1.5,125. };
		material red_rubber = { color{0.3,  0.1, 0.1, 1.0f}, 0.9, 0.1, 0.0, 0.0, 1.0,10. };
		material blue_rubber = { color{0.1,  0.1, 0.4, 1.0f}, 0.9, 0.3, 0.0, 0.15, 1.0,10. };
		material     mirror = { color{ 1.0, 1.0, 1.0, 1.0f}, 0.0, 10.0, 0.0,0.8, 1.0,1425. };

		spheres.emplace_back(vec3f(0, 8, -30), 8, mirror);
		spheres.emplace_back(vec3f(7, 4, -18), 4, mirror);
		spheres.emplace_back(vec3f(-3, -0.5, -16), 2, red_rubber);
		spheres.emplace_back(vec3f(-1, -1.5, -12), 2, glass);
		spheres.emplace_back(vec3f(1.5, -0.5, -20), 3, ivory);
		spheres.emplace_back(vec3f(-14, -0.5, -20), 3, red_rubber);
		//plans.emplace_back(vec3f(0, -5, 0), vec3f(0, -1, 0), ivory);
		plans.emplace_back(vec3f(0, 0, -30), vec3f(0, 0, 1), blue_rubber);
		lights.emplace_back(vec3f(-3, -1, -10), 0.8);
		lights.emplace_back(vec3f(0, 0, 0), 1.0);
	}

	void render() noexcept
	{
		for (size_t i = 0; i < height; i++)
		{
			for (size_t j = 0; j < width; j++)
			{
				float const x = (2 * (j + 0.5) / (float)width - 1) * tan(fov / 2.0) * width / (float)height;
				float const y = -(2 * (i + 0.5) / (float)height - 1) * tan(fov / 2.0);
				vec3f const dir = vec3f(x, y, -1).normalize();
				image[j + i * width] = cast_ray(vec3f(0, 0, 0), dir);
			}
		}
	}

	// return the closest hitpoint 
	[[nodiscard]] std::optional<hitInfo> scene_intersect(vec3f const& origin, vec3f const& dir) noexcept
	{
		std::optional<hitInfo> result;
		float dist = std::numeric_limits<float>::max();
		for (auto const& sphere : spheres)
		{
			if (auto const hInfo = sphere.ray_intersect(origin, dir))
			{
				float const c_dist = (hInfo->pos - origin).norm2();
				if (c_dist < dist)
				{
					dist = c_dist;
					result = hInfo;
				}
			}
		}

		for (auto const& plan : plans)
		{
			if (auto const hInfo = plan.ray_intersect(origin, dir))
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
	 
	[[nodiscard]] color cast_ray(vec3f const& origin, vec3f const& dir, unsigned depth = 0) noexcept
	{
		auto const hInfo = scene_intersect(origin, dir);
		if (depth > max_depth || !hInfo)
			return clear_color;

		// reflection
		color reflect_col = Color::none;
		if (hInfo->mtrl.reflect > 0.0f)
		{
			vec3f const r_dir = reflect(dir, hInfo->normal).normalize();
			vec3f const r_origin = dot(r_dir, hInfo->normal) < 0 ? hInfo->pos - hInfo->normal * 1e-3 : hInfo->pos + hInfo->normal * 1e-3;
			reflect_col = cast_ray(r_origin, r_dir, depth + 1);
		}

		// refraction
		color refract_col = Color::none;
		if (hInfo->mtrl.refraction_index > 0.0f)
		{
			vec3f const r_dir = refract(dir, hInfo->normal, hInfo->mtrl.refraction_index);
			vec3f const r_origin = dot(r_dir, hInfo->normal) < 0 ? hInfo->pos - hInfo->normal * 1e-3 : hInfo->pos + hInfo->normal * 1e-3;
			refract_col = cast_ray(r_origin, r_dir, depth + 1);
		}
		
		float diffuse_light_intensity = 0, specular_light_intensity = 0;

		for (auto const& light_it : lights)
		{
			vec3f const light_dir = (light_it.pos - hInfo->pos).normalize();
			vec3f const R = reflect(-light_dir, hInfo->normal);

			// shadows
			vec3f const shadow_start = dot(light_dir, hInfo->normal) < 0 ? hInfo->pos - hInfo->normal * 1e-3 : hInfo->pos + hInfo->normal * 1e-3;;
			if (scene_intersect(shadow_start, light_dir))
			{
				continue;
			}
			
			diffuse_light_intensity += light_it.intensity * std::max(0.0f, dot(light_dir, hInfo->normal));
			specular_light_intensity += light_it.intensity * std::pow(std::max(0.0f, dot(R, -dir)), hInfo->mtrl.specular_exponent);
		}
		
		return hInfo->mtrl.col * diffuse_light_intensity * hInfo->mtrl.kd + vec4f(1., 1., 1., 1.) * specular_light_intensity * hInfo->mtrl.ks + reflect_col * hInfo->mtrl.reflect + hInfo->mtrl.kr * refract_col;
	}

	void clear(color c_color = Color::black) noexcept
	{
		std::fill(image.begin(), image.end(), c_color);
	}

	void save(const char* fileName = "out.jpg") const noexcept
	{
		struct color8bit {
			uint8_t r, g, b, a;
		};
		std::vector<color8bit> buffer(image.size());
		for (size_t i = 0; i < buffer.size(); i++)
		{
			buffer[i].r = std::min<int>(image[i].x * 255, 255);
			buffer[i].g = std::min<int>(image[i].y * 255, 255);
			buffer[i].b = std::min<int>(image[i].z * 255, 255);
			buffer[i].a = std::min<int>(image[i].w * 255, 255);
		}
		stbi_write_jpg(fileName, width, height, 4, buffer.data(), 100);
	}

	public:
	
	color clear_color = Color::black;
	unsigned max_depth = 8;
	
	private:

	std::vector<plan> plans;
	std::vector<sphere> spheres;
	
	std::vector<light> lights;
	std::vector<color> image;
	size_t width, height;
	float fov;
	vec3f camPos;
	vec3f camDir;
};

int main()
{
	renderer render(1280, 720, M_PI/3);
	render.clear_color = {0.7f, 0.7f, 0.7f , 1.0f};
	render.init_scene();
	render.render();
	render.save();
	return 0;
}