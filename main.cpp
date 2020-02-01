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
	const color black = { 0.0f, 0.0f, 0.0f, 0.0f };
	const color red = { 1.0f, 0, 0, 1.0f };
	const color blue = { 0, 0, 1.0f, 1.0f };
	const color green = { 0, 1.0f, 0, 1.0f };
	const color yellow = { 1.0f, 1.0f, 0, 1.0f };
	const color white = { 1.0f, 1.0f, 1.0f, 1.0f };
}

struct light
{
	light(vec3f p, float in) noexcept : pos(p), intensity(in) {};
	vec3f pos;
	float intensity;
};

struct hitInfo
{
	vec3f pos;
	vec3f normal;
	vec3f tangent;
};

struct material
{
	color col;
	float specularExponent= 10.f;
	float kd, ks;
	vec2f albedo;
	float reflect = 0.0f;
};

struct drawable
{
	material mtrl;
};

struct sphere : drawable
{
	vec3f pos;
	float radius;

	sphere(vec3f const& p, float r, color const& c = Color::green) noexcept : drawable{ material{c} }, pos(p), radius(r) {};
	sphere(vec3f const& p, float r, material const& m ) noexcept : drawable{m}, pos(p), radius(r) {};

	[[nodiscard]] std::optional<hitInfo> rayIntersect(vec3f const& origin, vec3f dir) const noexcept
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
		float t = t0;
		hinfo.pos = origin + dir * t;
		hinfo.normal = (hinfo.pos - pos).normalize();
		return hinfo;
	}
};

class renderer
{
	public:

	renderer(size_t iwidth, size_t iheight, float ifov) noexcept : image(iwidth * iheight), width(iwidth), height(iheight), fov(ifov)
	{
		clear();
	}
	
	void initScene()
	{
		material m = { Color::yellow , 250.0f };
		m.kd = 1.0f;
		m.ks = 0.0f;
		m.col = Color::blue;
		spheres.emplace_back(vec3f(7, 5, -18), 4, m);
		m.col = Color::red;
		spheres.emplace_back(vec3f(-3, 0, -16), 2, m);
		m.col = Color::green;
		spheres.emplace_back(vec3f(-1, -1.5, -12), 2, m);
		m.col = Color::white;
		m.reflect = 0.9f;
		spheres.emplace_back(vec3f(1.5, -0.5, -20), 3, m);
		vec3f lpos(0, 0, -1);
		//lights.emplace_back(vec3f(-20, 20, 20), 1.5);
		lights.emplace_back(vec3f(30, 20, 30), 1.0);
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
	 
	[[nodiscard]] color cast_ray(vec3f const& origin, vec3f const& dir, unsigned depth = 0) noexcept
	{
		bool hit = false;
		std::pair<float, color> lastHit;
		lastHit.first = std::numeric_limits<float>::max();
		for (auto const& sphere : spheres)
		{
			if (auto const hInfo = sphere.rayIntersect(origin, dir))
			{
				float diffuseLightIntensity = 0, specularLightIntensity = 0;
				// @TODO fix multiples lights
				for (auto const& lightIt : lights)
				{
					vec3f const lightDir = (lightIt.pos - hInfo->pos).normalize();
					vec3f R = reflect(-lightDir, hInfo->normal);
					float S = dot(camDir, R);
					diffuseLightIntensity += lightIt.intensity * std::max(0.0f, dot(lightDir, hInfo->normal));
					specularLightIntensity += lightIt.intensity * std::pow(std::max(0.0f, dot(R, -dir)), sphere.mtrl.specularExponent);
				}
				color col = sphere.mtrl.col * (sphere.mtrl.kd * diffuseLightIntensity + sphere.mtrl.ks * specularLightIntensity);
				if (sphere.mtrl.reflect > 0.0f && depth < max_depth)
				{
					vec3f r_dir = reflect(dir, hInfo->normal).normalize();
					vec3f r_origin = dot(r_dir, hInfo->normal) < 0 ? hInfo->pos - hInfo->normal * 1e-3 : hInfo->pos + hInfo->normal * 1e-3;
					color r_color = cast_ray(r_origin, r_dir, depth+1);
					col = lerp(col, r_color, sphere.mtrl.reflect);
				}
				
				float l = (hInfo->pos - camPos).norm();
				if (l < lastHit.first)
					lastHit = std::make_pair(l, col);
				hit = true;
			}
		}
		if (hit)
			return lastHit.second;
		return clear_color;
	}

	void clear(color clearColor = Color::black) noexcept
	{
		std::fill(image.begin(), image.end(), clearColor);
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
	
	color clear_color = Color::yellow;
	
	private:

	unsigned max_depth = 4;
	std::vector<sphere> spheres;
	std::vector<light> lights;
	std::vector<color> image;
	size_t width, height;
	float fov;
	vec3f camPos;
	vec3f camDir;
};

void vec_print(vec3f const& v)
{
	std::cout << "x: " << v.x << " y: " << v.y << " z: " << v.z << "\n";
}

int main()
{
	renderer render(1280, 720, M_PI/3);
	render.clear_color = {0.7f, 0.7f, 0.7f , 1.0f};
	render.initScene();
	render.render();
	render.save();
	return 0;
}