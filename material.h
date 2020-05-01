#pragma once
#include <vector>
#include "color.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct material
{
	color col;
	float ka, kd, ks, kr;
	float reflect = 0.0f;
	float refraction_index = 0.0f;
	float specular_exponent = 10.f;
};

struct texture
{
	int width, height;
	std::vector<vec3f> data;

	void load(const char* path) noexcept
	{
		assert(path);
		int r = -1;
		stbi_uc* pixmap = stbi_load(path, &width, &height, &r, 0);
		if (!pixmap || r != 3)
			std::cerr << "texture load error : " << r;

		data.resize(width * height);
		for (int j = height - 1; j >= 0; j--)
		{
			for (int i = 0; i < width; i++)
			{
				data[i + j * width] = vec3f(pixmap[(i + j * width) * 3 + 0],
					pixmap[(i + j * width) * 3 + 1],
					pixmap[(i + j * width) * 3 + 2]) * (1 / 255.);
			}
		}
		stbi_image_free(pixmap);
	}

};
