#include <limits>

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <algorithm>
#include <optional>
#include "geometry.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

struct texture
{
	int width, height;
	std::vector<vec3f> data;
	
	void load(const char* path) noexcept
	{
		int r = -1;
		stbi_uc* pixmap = stbi_load(path, &width, &height, &r, 0);
		if (!pixmap || 3 != r)
			std::cerr << "texture load error : " <<  r;
		
		data.resize(width * height);
		for (int j = height - 1; j >= 0; j--) 
		{
			for (int i = 0; i < width; i++) 
			{
				data[i + j * width] = vec3f(pixmap[(i + j * width) * 3 + 0], pixmap[(i + j * width) * 3 + 1], pixmap[(i + j * width) * 3 + 2]) * (1 / 255.);
			}
		}
		stbi_image_free(pixmap);
	}

};

struct light
{
	light(vec3f p, float in) noexcept : pos(p), intensity(in) {};
	vec3f pos;
	float intensity;
	static constexpr float ambient = 0.0;
};

struct material
{
	color col;
	float ka, kd, ks, kr;
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

	[[nodiscard]] std::optional<hitInfo> ray_intersect(vec3f const& origin, vec3f const& dir) const noexcept
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
	plan(vec3f const& p, vec3f const& n, material const& m) noexcept : drawable{m}, pos(p), normal(n) { }

	[[nodiscard]] std::optional<hitInfo> ray_intersect(vec3f const& origin, vec3f const& dir) const noexcept
	{
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

	renderer(size_t iwidth, size_t iheight, float ifov, const char* env_map_path) noexcept
	: image(iwidth * iheight), width(iwidth), height(iheight), fov(ifov)
	{
		env_map.load(env_map_path);
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
			return get_env_map_color(origin, dir);

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
			vec3f const r_dir = refract(dir, hInfo->normal, hInfo->mtrl.refraction_index).normalize();
			vec3f const r_origin = dot(r_dir, hInfo->normal) < 0 ? hInfo->pos - hInfo->normal * 1e-3 : hInfo->pos + hInfo->normal * 1e-3;
			refract_col = cast_ray(r_origin, r_dir, depth + 1);
		}
		
		float diffuse_light_intensity = 0, specular_light_intensity = 0;
		for (auto const& light_it : lights)
		{
			vec3f const light_dir = (light_it.pos - hInfo->pos).normalize();
			
			// shadows
			vec3f const shadow_start = dot(light_dir, hInfo->normal) < 0 ? hInfo->pos - hInfo->normal * 1e-3 : hInfo->pos + hInfo->normal * 1e-3;
			if (auto const& shadow_hit = scene_intersect(shadow_start, light_dir))
			{
				if ((shadow_start - shadow_hit->pos).norm2() <= (shadow_start - light_it.pos).norm2())
					continue;
			}
			
			vec3f const R = reflect(-light_dir, hInfo->normal).normalize();

			diffuse_light_intensity += light_it.intensity * std::max(0.0f, dot(light_dir, hInfo->normal));
			specular_light_intensity += light_it.intensity * std::pow(std::max(0.0f, dot(R, -dir)), hInfo->mtrl.specular_exponent);
		}
		
		return	hInfo->mtrl.col * hInfo->mtrl.ka * light::ambient +
				hInfo->mtrl.col * diffuse_light_intensity * hInfo->mtrl.kd +
				vec4f(1., 1., 1., 1.) * specular_light_intensity * hInfo->mtrl.ks + 
				reflect_col * hInfo->mtrl.reflect + hInfo->mtrl.kr * refract_col;
	}

	void init_scene() noexcept
	{
		material const      ivory = { color{0.4f, 0.4f, 0.3f, 1.0f},	0.15, 0.6, 0.3, 0.0, 0.1, 1.0,50. };
		material const      glass = { color{0.6,  0.7, 0.8, 1.0f},	0.15, 0.0, 0.5, 0.8, 0.0, 1.5,125. };
		material const red_rubber = { color{0.3,  0.1, 0.1, 1.0f},	0.15, 0.9, 0.1, 0.0, 0.0, 1.0,10. };
		material const blue_rubber = { color{0.1,  0.1, 0.6, 1.0f},	0.15, 0.9, 0.3, 0.0, 0.0, 1.0,10. };
		material const yellow_rubber = { color{0.4,  0.4, 0.1, 1.0f}, 0.15, 0.9, 0.3, 0.0, 0.0, 1.0,10. };
		material const     mirror = { color{ 1.0, 1.0, 1.0, 1.0f},	0.15, 0.0, 0.9, 0.0,0.8, 1.0,1425. };
		spheres.emplace_back(vec3f(0, 8, -30), 8, mirror);
		spheres.emplace_back(vec3f(7, 4, -18), 4, mirror);
		spheres.emplace_back(vec3f(-3, -0.5, -16), 2, red_rubber);
		spheres.emplace_back(vec3f(-1, -1.5, -12), 2, glass);
		spheres.emplace_back(vec3f(1.5, -0.5, -20), 3, ivory);
		spheres.emplace_back(vec3f(-14, -0.5, -20), 3, red_rubber);
		//plans.emplace_back(vec3f(0, -10, 0), vec3f(0, 1, 0), blue_rubber);

		lights.emplace_back(vec3f(-20, 20, 20), 1.5);
		lights.emplace_back(vec3f(30, 50, -25), 1.8);
		lights.emplace_back(vec3f(0, 0, 0), 1.7);
	}

	void render() noexcept
	{
		float const tf2 = tanf(fov / 2.0f);
		#pragma loop(hint_parallel(8))
		for (size_t i = 0; i < height; i++)
		{
			for (size_t j = 0; j < width; j++)
			{
				for (unsigned m = 0; m < msaa; m++)
				{
					float const sample_offset = msaa > 1 ? static_cast<float>(m) / (msaa / 2) : 0.5f;
					float const x = (2 * (j + sample_offset) / static_cast<float>(width) - 1) * tf2 * width / static_cast<float>(height);
					float const y = -(2 * (i + sample_offset) / static_cast<float>(height) - 1) * tf2;
					vec3f const dir = vec3f(x, y, -1).normalize();
					image[j + i * width] = image[j + i * width] + cast_ray(vec3f(0, 0, 0), dir);
				}
				image[j + i * width].x /= static_cast<float>(msaa);
				image[j + i * width].y /= static_cast<float>(msaa);
				image[j + i * width].z /= static_cast<float>(msaa);
				image[j + i * width].w /= static_cast<float>(msaa);
			}
		}
	}

	color get_env_map_color(vec3f origin, vec3f dir) const noexcept
	{
		float const theta = atan2(dir.z, dir.x);
		float const phi = -atan2(dir.y, sqrt(dir.z * dir.z + dir.x * dir.x));

		size_t const x = std::max(std::min((theta + M_PI) / (M_PI * 2) * static_cast<double>(env_map.width), static_cast<double>(env_map.width)), 0.0);
		size_t const y = std::max(std::min((phi + M_PI) / (M_PI * 2) * static_cast<double>(env_map.height), static_cast<double>(env_map.height)), 0.0);

		vec3f const col = env_map.data.at(y * env_map.width + x);
		return color{ col.x, col.y, col.z, 1.0f} ;
	}

	void game_boy_pass() noexcept
	{
		float const l1 = 0.9;
		float const l2 = 0.7;
		float const l3 = 0.5;
		
		for(auto& c : image)
		{
			float const n = c.norm();
			if (n > l1)
			{
				c.x = 0.607;
				c.y = 0.737;
				c.z = 0.058;
			}
			else if (n > l2)
			{
				c.x = 0.545;
				c.y = 0.674;
				c.z = 0.058;

			}
			else if (n > l3)
			{
				c.x = 0.188;
				c.y = 0.384;
				c.z = 0.188;
			}
			else
			{
				c.x = 0.058;
				c.y = 0.219;
				c.z = 0.058;
			}
		}
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

	color clear_color = Color::black;
	unsigned max_depth = 8;
	unsigned msaa = 1;
	
	private:

	texture env_map;
	
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
	renderer render(1280, 720, M_PI/3, "envmap.jpg");
	render.clear_color = {0.7f, 0.7f, 0.7f , 1.0f};
	render.init_scene();
	render.render();
	render.save();
	render.game_boy_pass();
	render.save("out gameboy.jpg");
	return 0;
}