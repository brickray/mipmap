#ifndef H_MIPMAP_H
#define H_MIPMAP_H

#include <glm\glm.hpp>
#include <vector>
#include <omp.h>
using namespace std;
using namespace glm;

inline bool IsPowerOf2(int x){
	return x&(x - 1) == 0;
}

inline int RoundUpPow2(int v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return v + 1;
}

inline float Log2(float x) {
	static float invLog2 = 1.f / logf(2.f);
	return logf(x) * invLog2;
}

inline int Log2Int(float v) {
	return (int)floorf(Log2(v));
}

struct TexInfo{
	int w, h;
	vector<vec3> data;
};

//power of 2 mipmap
bool GenerateMipmap(int width, int height, vector<vec3>& input, vector<TexInfo>& ret){
	fprintf(stdout, "image res [%d, %d]\n", width, height);
	int newWidth= width, newHeight = height;
	vector<vec3> orig;
	if (!(IsPowerOf2(width) && IsPowerOf2(height))){
		newWidth = RoundUpPow2(width);
		newHeight = RoundUpPow2(height);
		fprintf(stdout, "new res [%d, %d]\n", newWidth, newHeight);

		orig.resize(newWidth*newHeight);

#pragma omp parallel for schedule(dynamic, 1)
		for (int i = 0; i < newHeight; ++i){
			for (int j = 0; j < newWidth; ++j){
				float x = float(width) / newWidth * j;
				float y = float(height) / newHeight * i;
				int xx = floor(x); xx = xx == width ? xx - 1 : xx;
				int yy = floor(y); yy = yy == height ? yy - 1 : yy;

				int idx = i*newWidth + j;
				vec3 c = input[yy*width + xx] + input[yy*width + xx + 1]
					+ input[(yy + 1)*width + xx] + input[(yy + 1)*width + xx + 1];

				orig[idx] = 0.25f*c;
			}
		}
	}

	int nLevel = Log2Int(newWidth > newHeight ? newWidth : newHeight) + 1;
	fprintf(stdout, "pyramid size %d\n", nLevel);

	ret.resize(nLevel);
	ret[0].w = newWidth;
	ret[0].h = newHeight;
	ret[0].data.resize(newWidth * newHeight);
	for (int i = 0; i < newWidth*newHeight; ++i)
		ret[0].data[i] = orig[i];

	for (int i = 1; i < nLevel; ++i){
		int prevW = ret[i - 1].w;
		int prevH = ret[i - 1].h;
		int w = newWidth >> i; w = w > 1 ? w : 1;
		int h = newHeight >> i; h = h > 1 ? h : 1;
		ret[i].data.resize(w*h);
		ret[i].w = w;
		ret[i].h = h;

#pragma omp parallel for schedule(dynamic, 1)
		for (int j = 0; j < h; ++j){
			for (int k = 0; k < w; ++k){
				int idx = j*w + k;

				int x1 = prevW == 1 ? 0 : 2 * k;
				int x2 = prevW == 1 ? 0 : 2 * k + 1;
				int y1 = prevH == 1 ? 0 : 2 * j;
				int y2 = prevH == 1 ? 0 : 2 * j + 1;
				int idx1 = y1*prevW + x1;
				int idx2 = y2*prevW + x2;
				int idx3 = y1*prevW + x1;
				int idx4 = y2*prevW + x2;
				vec3 c = ret[i - 1].data[idx1] + ret[i - 1].data[idx2]
					+ ret[i - 1].data[idx3] + ret[i - 1].data[idx3];

				ret[i].data[idx] = c*0.25f;
			}
		}
	}

	return true;
}

//non power of 2 mipmap
//round down
bool GenerateMipmapNPOT(int width, int height, vector<vec3>& input, vector<TexInfo>& ret){
	fprintf(stdout, "image res [%d, %d]\n", width, height);
	int nLevel = Log2Int(width > height ? width : height) + 1;
	fprintf(stdout, "pyramid size %d\n", nLevel);

	ret.resize(nLevel);
	ret[0].w = width;
	ret[0].h = height;
	ret[0].data.resize(width * height);
	for (int i = 0; i < width*height; ++i)
		ret[0].data[i] = input[i];

	for (int i = 1; i < nLevel; ++i){
		int prevW = ret[i - 1].w;
		int prevH = ret[i - 1].h;
		int w = prevW >> 1; w = w > 1 ? w : 1;
		int h = prevH >> 1; h = h > 1 ? h : 1;
		ret[i].w = w;
		ret[i].h = h;
		ret[i].data.resize(w*h);

		vector<vec3> temp(prevH * w);
		for (int j = 0; j < prevH; ++j){
			for (int k = 0; k < w; ++k){
				float w1 = (float)(w - k) / prevW;
				float w2 = (float)(w) / prevW;
				float w3 = (float)(k + 1 == w ? k : k + 1) / prevW;

				int x1 = prevW == 1 ? 0 : 2 * k;
				int x2 = prevW == 1 ? 0 : 2 * k + 1;
				int x3 = prevW == 1 ? 0 : (k + 1 == w ? 2 * k + 1 : 2 * k + 2);
				int idx1 = j*prevW + x1;
				int idx2 = j*prevW + x2;
				int idx3 = j*prevW + x3;
				vec3 c = w1*ret[i - 1].data[idx1] +
					w2*ret[i - 1].data[idx2] +
					w3*ret[i - 1].data[idx3];

				temp[j*w + k] = c;
			}
		}

		for (int j = 0; j < w; ++j){
			for (int k = 0; k < h; ++k){
				float w1 = (float)(h - k) / prevH;
				float w2 = (float)(h) / prevH;
				float w3 = (float)(k + 1 == w ? k : k + 1) / prevH;
				
				int x1 = prevH == 1 ? 0 : 2 * k;
				int x2 = prevH == 1 ? 0 : 2 * k + 1;
				int x3 = prevH == 1 ? 0 : (k + 1 == h ? 2 * k + 1 : 2 * k + 2);
				int idx1 = x1*w + j;
				int idx2 = x2*w + j;
				int idx3 = x3*w + j;
				vec3 c = w1*temp[idx1] +
					w2*temp[idx2] +
					w3*temp[idx3];

				ret[i].data[k*w + j] = c;
			}
		}
	}

	return true;
}

#endif