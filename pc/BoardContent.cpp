#include "BoardContent.h"
#include "lodepng.h"
#include <iostream>
#include <cstring>
#include <cmath>

BoardContent::BoardContent():drawable_region(0,0,2048,1024){
	width = 2048;
	height = 1024;
	width_height = (float)width / (float)height;
	image.resize(width *height *3);
	memset(&image[0], 0xff, width *height *3);

	drawable_region.x = 0;
	drawable_region.y = 0;
	drawable_region.w = width-64;
	drawable_region.h = height;
	
	pen.color = PenColor(0, 0, 0);
	pen.width = 3.f;
	pen.cursor_prev[0] = 0;
	pen.cursor_prev[1] = 0;
	pen.down = false;

	color_palette.reserve(10);
	color_palette.push_back(PenColor(0, 0, 0));
	color_palette.push_back(PenColor(0.5, 0.5, 0.5));
	color_palette.push_back(PenColor(1, 1, 1));
	color_palette.push_back(PenColor(0.75, 0, 0)); // red
	color_palette.push_back(PenColor(0.9, 0.5, 0)); // orange
	color_palette.push_back(PenColor(0.9, 0.8, 0)); // yellow
	color_palette.push_back(PenColor(0, 0.5, 0)); // green
	color_palette.push_back(PenColor(0, 0.8, 0.8)); // green
	color_palette.push_back(PenColor(0, 0.25, 0.75)); // blue
	color_palette.push_back(PenColor(0.5, 0, 0.8)); // purple
	
	// Set up GUI
	draw_gui(&image[3*(width - 64)], width, 64, height);
}

BoardContent::~BoardContent(){
}

void BoardContent::draw_line(pixel_coord x0, pixel_coord y0, pixel_coord x1, pixel_coord y1, BoardContent::Region *touched) {
	float wd = pen.width;
	//ML_LOG_TAG(Debug, APP_TAG, "drawline %f, %f -> %f, %f : %d, %d -> %d, %d", a[0], a[1], b[0], b[1], x0, y0, x1, y1);

	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx - dy, e2, x2, y2;                          /* error value e_xy */
	float ed = dx + dy == 0 ? 1 : sqrt((float)dx*dx + (float)dy*dy);

	for (wd = (wd + 1) / 2; ; ) {                                   /* pixel loop */
		float val = 1;// std::abs(err - dx + dy) / ed - wd + 1;
		set_pixel(x0, y0, val, touched);
		e2 = err; x2 = x0;
		if (2 * e2 >= -dx) {                                           /* x step */
			for (e2 += dy, y2 = y0; e2 < ed*wd && (y1 != y2 || dx > dy); e2 += dx) {
				float val = 1;// std::abs(e2) / ed - wd + 1;
				set_pixel(x0, y2 += sy, val, touched);
			}
			if (x0 == x1) break;
			e2 = err; err -= dy; x0 += sx;
		}
		if (2 * e2 <= dy) {                                            /* y step */
			for (e2 = dx - e2; e2 < ed*wd && (x1 != x2 || dx < dy); e2 += dy) {
				float val = 1;// std::abs(e2) / ed - wd + 1;
				set_pixel(x2 += sx, y0, val, touched);
			}
			if (y0 == y1) break;
			err += dx; y0 += sy;
		}
	}
}
void BoardContent::set_pixel(pixel_coord x, pixel_coord y, float val, BoardContent::Region *touched) {
	if(!drawable_region.contains(x, y)){ return;  }
	int k = x + y * width;
	image[3 * k + 0] = 255 * (val*pen.color[0]);
	image[3 * k + 1] = 255 * (val*pen.color[1]);
	image[3 * k + 2] = 255 * (val*pen.color[2]);
	touched->expand_to_include(x, y);
}

// Drawing/Pen commands
void BoardContent::pen_set_color(const PenColor &rgb) {
	pen.color = rgb;
}
void BoardContent::pen_get_color(PenColor &rgb) {
	rgb = pen.color;
}
void BoardContent::pen_set_size(float d) {
	pen.width = d;
}
void BoardContent::pen_down(pixel_coord x, pixel_coord y) {
	pen.down = true;
}
void BoardContent::pen_move(pixel_coord x, pixel_coord y) {
	if(!drawable_region.contains(x, y)){ return; }
	if (pen.down && (x != pen.cursor_prev[0] || y != pen.cursor_prev[1])){
		BoardContent::Region touched(width, height, -width, -height);
		draw_line(pen.cursor_prev[0], pen.cursor_prev[1], x, y, &touched);
		on_image_update(&touched);
	}
	pen.cursor_prev[0] = x;
	pen.cursor_prev[1] = y;
}
void BoardContent::pen_up(int x, int y) {
	pen.down = false;
}
void BoardContent::clear(const PenColor &color) {
	for (int j = 0; j < (int)drawable_region.h; ++j) {
		int jy = drawable_region.y + j;
		for (int i = 0; i < (int)drawable_region.w; ++i) {
			int ix = drawable_region.x + i;
			image[3 * (ix + jy * width) + 0] = 255*color[0];
			image[3 * (ix + jy * width) + 1] = 255*color[1];
			image[3 * (ix + jy * width) + 2] = 255*color[2];
		}
	}
	on_image_update(NULL);
}

void BoardContent::get_pen_state(PenColor &color, float &width){
	color = pen.color;
	width = pen.width;
}

void BoardContent::get_size(unsigned int &width_, unsigned int &height_) const{
	width_ = width;
	height_ = height;
}

void BoardContent::paste_image(unsigned int w, unsigned int h, const unsigned char *img, unsigned channels){
	unsigned int stride = w;
	if(w > width){ w = width; }
	if(h > height){ h = height; }
	for(unsigned int j = 0; j < h; ++j){
		if(3 == channels){
			memcpy(&image[3*j*width], &img[3*j*stride], 3*w);
		}else{
			for(unsigned int i = 0; i < w; ++i){
				for(unsigned k = 0; k < 3; ++k){
					image[3*(i+j*width)+k] = img[channels*(i+j*stride)+k];
				}
			}
		}
	}
	BoardContent::Region touched(0, 0, w, h);
	on_image_update(&touched);
}












static void draw_button_border(unsigned char *img, unsigned stride, unsigned width, unsigned height) {
	for (int i = 0; i < width; ++i) {
		img[3 * (i + 0 * stride)+0] = 22;
		img[3 * (i + 0 * stride)+1] = 22;
		img[3 * (i + 0 * stride)+2] = 22;
	}
	for (int i = 0; i < height; ++i) {
		img[3 * (0 + i * stride) + 0] = 22;
		img[3 * (0 + i * stride) + 1] = 22;
		img[3 * (0 + i * stride) + 2] = 22;
		img[3 * (width - 1 + i * stride) + 0] = 22;
		img[3 * (width - 1 + i * stride) + 1] = 22;
		img[3 * (width - 1 + i * stride) + 2] = 22;
	}
	for (int i = 0; i < width; ++i) {
		img[3 * (i + (height-1) * stride) + 0] = 22;
		img[3 * (i + (height-1) * stride) + 1] = 22;
		img[3 * (i + (height-1) * stride) + 2] = 22;
	}
}
static void draw_button_color(unsigned char *img, unsigned stride, unsigned width, unsigned height, const BoardContent::PenColor &color) {
	for (int j = 0; j < height; ++j) {
		for (int i = 0; i < width; ++i) {
			for (int k = 0; k < 3; ++k) {
				img[3 * (i + j * stride) + k] = 255 * color[k];
			}
		}
	}
}
static void draw_button_img(unsigned char *img, unsigned stride, unsigned width, unsigned height, const char *filename) {
	unsigned char *icon = NULL;
	unsigned error;
	unsigned w, h;
	error = lodepng_decode32_file(&icon, &w, &h, filename);
	if (error) {
		//ML_LOG_TAG(Debug, APP_TAG, "lodepng error %u: %s, %s", error, lodepng_error_text(error), filename);
		return;
	}
	for (int j = 0; j < height; ++j) {
		for (int i = 0; i < width; ++i) {
			for (int k = 0; k < 3; ++k) {
				img[3 * (i + j * stride) + k] = icon[4*(i+j*w)+k];
			}
		}
	}
	free(icon);
}

void BoardContent::draw_gui(unsigned char *img, unsigned stride, unsigned width, unsigned height) {
	int i = 0;
	draw_button_img(&img[3 * (64 * i*stride)], stride, width, width, "data/moveb.png"); ++i;
	draw_button_img(&img[3 * (64 * i*stride)], stride, width, width, "data/brush1.png"); ++i;
	draw_button_img(&img[3 * (64 * i*stride)], stride, width, width, "data/brush2.png"); ++i;
	draw_button_img(&img[3 * (64 * i*stride)], stride, width, width, "data/brush3.png"); ++i;
	for (int j = 0; j < color_palette.size(); ++j, ++i) {
		draw_button_color(&img[3 * (64 * i*stride)], stride, width, width, color_palette[j]);
	}
	draw_button_img(&img[3 * (64 * i * stride)], stride, width, width, "data/clearb.png"); ++i;
	draw_button_img(&img[3 * (64 * i * stride)], stride, width, width, "data/clearw.png");
	for (int i = 0; i < 16; ++i) {
		draw_button_border(&img[3 * (64 * i*stride)], stride, width, width);
	}
}
void BoardContent::gui_click(int i) {
printf("click %d\n", i); fflush(stdout);
	if(0 == i){ // move board
		moving = true;
	}
	else if (1 == i) { // brush size 1
		pen.width = 5.f;
	}
	else if (2 == i) { // brush size 2
		pen.width = 10.f;
	}
	else if (3 == i) { // brush size 3
		pen.width = 20.f;
	}
	else if (i >= 4 && i < 4 + color_palette.size()) {
		pen.color = color_palette[i - 4];
	}
	else if (i == 4 + color_palette.size()) { // clear to black
		clear(PenColor(0, 0, 0));
	}
	else if (i == 5 + color_palette.size()) { // clear to white
		clear(PenColor(1, 1, 1));
	}
}
void BoardContent::gui_input(bool pressed, int x, int y){
	if (pressed) {
		if (!pen.down && !drawable_region.contains(x, y)){
			// Clicked a GUI button
			int i = y / 64;
			gui_click(i);
		}
		pen_down(x, y);
	} else {
		pen_up(x, y);
	}
	pen_move(x, y);
}
