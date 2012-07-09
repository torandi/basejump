#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

typedef struct {
	float x;
	float y;
	float z;
	float s;
	float t;
} vertex;

class BitmapFont: public Font {
public:
	BitmapFont(const std::string& filename){
		const char* real_filename = real_path(filename.c_str());

		/**
		 * This file-format is a tiny bit broken when it comes to handling
		 * endianness and different data-type sizes. Hopefully this code
		 * will work cross-platform.
		 */

#ifdef WIN32
#pragma pack(push,1)
#define PACK
#else
#define PACK __attribute__((packed))
#endif

		struct header {
			unsigned char magic[2];
			uint32_t image_width;
			uint32_t image_height;
			uint32_t cell_width;
			uint32_t cell_height;
			unsigned char bpp;
			char base;
		} PACK header;

#ifdef WIN32
#pragma pack(pop)
#endif

		/* open file */
		FILE* fp = fopen(real_filename, "rb");
		if ( !fp ){
			/** @todo Handle this (kind of expected) error a bit more graceful. sorry */
			fprintf(stderr, "Font `%s' could not be read: %s\n", real_filename, strerror(errno));
			abort();
		}

		/* read header */
		if ( fread(&header, sizeof(struct header), 1, fp) != 1 ){
			fprintf(stderr, "`%s' is not a valid font: could not read header\n", real_filename);
			abort();
		}

		/* validate magic */
		if ( !(header.magic[0] == 0xBF && header.magic[1] == 0xF2) ){
			fprintf(stderr, "`%s' is not a valid font: invalid magic\n", real_filename);
			abort();
		}

		/* only supports 32 BPP at the moment */
		if ( header.bpp != 32 ){
			fprintf(stderr, "Font `%s' has unsupported BPP %d (only 32 is supported).\n", real_filename, (int)header.bpp);
			abort();
		}

		/* read character width */
		if ( fread(width_lut, 1, 256, fp) != 256 ){
			fprintf(stderr, "Font `%s' is not a valid font: file truncated\n", real_filename);
			abort();
		}

		/* precalculate offsets */
		base = header.base;
		pitch = header.image_width / header.cell_width;
		cell = Vector2i(header.cell_width, header.cell_height);
		offset.x = (float)cell.x / (float)header.image_width;
		offset.y = (float)cell.y / (float)header.image_height;

		/* read bitmaps */
		const size_t pixel_size = header.bpp / 8;
		const size_t bytes = header.image_width * header.image_height * pixel_size;
		char* data = (char*)malloc(bytes);
		if ( fread(data, 1, bytes, fp) != bytes ){
			fprintf(stderr, "Font `%s' is not a valid font: file truncated\n", real_filename);
			abort();
		}

		/* upload texture */
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, header.image_width, header.image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		free(data);
	}

	virtual ~BitmapFont(){
		glDeleteTextures(1, &texture);
	}

	virtual void  printf(int x, int y, const Color& color, const char* fmt, ...) const {
		va_list ap;
		va_start(ap, fmt);
		vprintf(x, y, color, fmt, ap);
		va_end(ap);
	}

	virtual void vprintf(int x, int y, const Color& color, const char* fmt, va_list ap) const {
		char* str = NULL;
#ifndef WIN32
		vasprintf(&str, fmt, ap);
#else
		str = (char*)malloc(1024);
		vsnprintf(str, 1024, fmt, ap);
#endif

		const size_t num_chars = strlen(str);
		size_t num_vertices = num_chars * 4; /* QUADS */
		static vertex v[1024*4]; /** @todo not reentrant */

		int n = 0;
		int cx = 0;
		for ( unsigned int i = 0; i < num_chars; i++ ){
			unsigned char ch = str[i];

			if ( ch == '\t' ){
				cx += 25 - cx % 25;
				num_vertices -= 4;
				continue;
			}

			int row = (ch-base) / pitch;
			int col = (ch-base) % pitch;
			const int dx = width_lut[ch];

			v[n].x = (float)cx;
			v[n].y = 0.0f;
			v[n].z = 0.0f;
			v[n].s = col * offset.x;
			v[n].t = row * offset.y;
			n++;

			v[n].x = (float)cx;
			v[n].y = (float)cell.y;
			v[n].z = 0.0f;
			v[n].s = col * offset.x;
			v[n].t = row * offset.y + offset.y;
			n++;

			v[n].x = (float)(cx + cell.x);
			v[n].y = (float)cell.y;
			v[n].z = 0.0f;
			v[n].s = col * offset.x + offset.x;
			v[n].t = row * offset.y + offset.y;
			n++;

			v[n].x = (float)(cx + cell.x);
			v[n].y = 0.0f;
			v[n].z = 0.0f;
			v[n].s = col * offset.x + offset.x;
			v[n].t = row * offset.y;
			n++;

			cx += dx;
		}

		if ( x < 0 ) x = size.x + x;
		if ( y < 0 ) y = size.y + y;

		glPushMatrix();
		glPushAttrib(GL_ENABLE_BIT);
		glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

		glLoadIdentity();
		glTranslatef((GLfloat)x, (GLfloat)y, 0.0f);
		glColor4fv(color.value);
		glBindTexture(GL_TEXTURE_2D, texture);
		glVertexPointer  (3, GL_FLOAT, sizeof(float)*5, &v[0].x);
		glTexCoordPointer(2, GL_FLOAT, sizeof(float)*5, &v[0].s);
		glDrawArrays(GL_QUADS, 0, num_vertices);

		glPopClientAttrib();
		glPopAttrib();
		glPopMatrix();

		free(str);
	}

private:
	GLuint texture;
	unsigned char base;
	unsigned char pitch;
	Vector2i cell;
	Vector2f offset;
	unsigned char width_lut[256];
};
