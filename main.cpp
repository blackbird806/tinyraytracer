#include <limits>

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <algorithm>
#include <optional>
#include <random>
#include <chrono>
#define STB_IMAGE_IMPLEMENTATION
#include "geometry.h"
#include "shapes.h"
#include "color.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "scene.h"

class renderer
{
	public:

	renderer(size_t iwidth, size_t iheight, float ifov, const char* env_map_path) noexcept
	: image(iwidth * iheight), width(iwidth), height(iheight), fov(ifov)
	{
		env_map.load(env_map_path);
	}
	 
	[[nodiscard]] color cast_ray(vec3f const& origin, vec3f const& dir, unsigned depth = 0) noexcept
	{
		auto const hInfo = render_scene.intersect(origin, dir);
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
		for (auto const& light_it : render_scene.lights)
		{
			vec3f const light_dir = (light_it.pos - hInfo->pos).normalize();
			
			// shadows
			vec3f const shadow_start = dot(light_dir, hInfo->normal) < 0 ? hInfo->pos - hInfo->normal * 1e-3 : hInfo->pos + hInfo->normal * 1e-3;
			if (auto const& shadow_hit = render_scene.intersect(shadow_start, light_dir))
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

		material mtrls[] = { ivory , glass , red_rubber, blue_rubber, yellow_rubber, mirror };
		
		std::random_device rd{};
		std::mt19937       gen{ rd() };
		std::uniform_int_distribution<>			m{ 0, std::size(mtrls)-1};
		std::normal_distribution<float>			xzd{ 1.f, 2.f };
		std::normal_distribution<float>			yd{ 1.f, 2.f };
		std::uniform_real_distribution<float>	radd{ .1f, .7f };

		for (int i = 0; i < 40; i++)
		{
			render_scene.objects.push_back(std::make_shared<sphere>(vec3f(sin(i) * 3, cos(i) * 3, -i), 0.5, mtrls[m(gen)]));
		}
		
		render_scene.lights.emplace_back(vec3f(-20, 20, 20), 1.5);
		render_scene.lights.emplace_back(vec3f(30, 50, -25), 1.8);
		render_scene.lights.emplace_back(vec3f(0, 0, 0), 1.7);
	}

	void render() noexcept
	{
		render_scene.create_acceleration_structure();
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
		float const phi = atan2(dir.z, dir.x);
		float const theta = acos(dir.y);

		size_t const x = env_map.width * (phi / M_PI + 1) / 2;
		size_t const y = env_map.height * (theta / M_PI);

		vec3f const col = env_map.data[std::clamp<size_t>(y * env_map.width + x, 0, env_map.data.size()-1)];
		return color{ col.x, col.y, col.z, 1.0f };
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
	unsigned max_depth = 1;
	unsigned msaa = 1;
	
	private:

	scene render_scene;
	texture env_map;

	std::vector<color> image;
	size_t width, height;
	float fov;
	vec3f camPos;
	vec3f camDir;
};

int main()
{
	renderer render(1920, 1080, M_PI/2.5, "envmap.jpg");
	render.clear_color = {0.7f, 0.7f, 0.7f , 1.0f};
	render.init_scene();
	
	auto const start = std::chrono::steady_clock::now();
	render.render();
	auto const end = std::chrono::steady_clock::now();
	
	std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count() << " s" << std::endl;
	render.save();
	//std::cin.get();
	return 0;
}