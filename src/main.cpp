#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <time.h>
#include <vector>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include <stb\stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <stb\stb_image_write.h>

#include "mipmap.h"

using namespace std;

bool ReadImage(const char* file, int& width, int& height, vector<vec3>& ret){
	int component;
	unsigned char* data = stbi_load(file, &width, &height, &component, 0);
	if (!data){
		fprintf(stderr, "read file faild\n");
		return false;
	}

	if (component != 3){
		fprintf(stderr, "image file must be in RGB format\n");
		return false;
	}
	ret.resize(width*height * 3);
	float inv = 1.f / 255.f;
	for (int i = 0; i < width*height; ++i){
		ret[i].x = data[3 * i + 0] * inv;
		ret[i].y = data[3 * i + 1] * inv;
		ret[i].z = data[3 * i + 2] * inv;
	}

	return true;
}

bool WritePng(const char* file, int width, int height, vector<vec3>& input){
	auto clamp = [](float value, float minValue, float maxValue)->float{
		if (value < minValue) return minValue;
		else if (value > maxValue) return maxValue;
		return value;
	};
	
	unsigned char* transform = new unsigned char[width*height * 3];
	for (int i = 0; i < height; ++i){
		for (int j = 0; j < width; ++j){
			unsigned pixel = i*width + j;
			transform[3 * pixel + 0] = unsigned(clamp(input[pixel].x, 0.f, 1.f) * 255.f);
			transform[3 * pixel + 1] = unsigned(clamp(input[pixel].y, 0.f, 1.f) * 255.f);
			transform[3 * pixel + 2] = unsigned(clamp(input[pixel].z, 0.f, 1.f) * 255.f);
		}
	}

	stbi_write_png(file, width, height, 3, transform, 0);

	delete[] transform;

	return true;
}

void PrintUsage(){
	fprintf(stdout, "******************Usage******************\n");
	fprintf(stdout, "mipmap.exe filename\n");
	fprintf(stdout, "*****************************************\n");
}

void main(int argc, char** argv){
	PrintUsage();

	if (argc != 2){
		fprintf(stderr, "param error\n");

		return;
	}

	string file = argv[1];
	string base = file.substr(0, file.find_last_of("/") + 1);
	int w, h;
	vector<vec3> ret;
	if (!ReadImage(file.c_str(), w, h, ret)){
		return;
	}

	vector<TexInfo> pyramid;
	clock_t start = clock();
	GenerateMipmap(w, h, ret, pyramid);
	fprintf(stdout, "generate mipmap spend [%.3fms]\n", float(clock() - start));
	
	start = clock();
	for (int i = 0; i < pyramid.size(); ++i){
		char out[256] = { 0 };
		sprintf(out, "%spyramid_%d.png", base.c_str(), i);
		WritePng(out, pyramid[i].w, pyramid[i].h, pyramid[i].data);
	}
	fprintf(stdout, "write file spend [%.3fms]\n", float(clock() - start));

	return;
}