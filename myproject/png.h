#include "graphics.h"

#ifndef PNG_H
#define PNG_H

typedef struct _PNG_INFO {
	int w;
	int h;
	int channels;
} PNG_INFO;

class PNG {
	int width;
	int height;
	int channels;
	uint32_t *img;

public:
	PNG(const char *imagePath);
	PNG(size_t bufsize, unsigned char* bufpng);
	~PNG();

	void Draw(Scene2D *scene, int startX, int startY);
	void GetInfo(PNG_INFO* out);
};

#endif
