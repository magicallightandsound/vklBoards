#ifndef BOARD_CONTENT_H_INCLUDED
#define BOARD_CONTENT_H_INCLUDED

#include <vector>

class BoardContent{
public:
	typedef int pixel_coord;
	struct PenColor{
		float rgb[3];
		PenColor(){}
		PenColor(float r, float g, float b){
			rgb[0] = r;
			rgb[1] = g;
			rgb[2] = b;
		}
		float operator[](unsigned i) const{ return rgb[i]; }
		float &operator[](unsigned i){ return rgb[i]; }
	};
	struct Region{
		pixel_coord x, y, w, h;
		Region(pixel_coord x_, pixel_coord y_, pixel_coord w_, pixel_coord h_):x(x_),y(y_),w(w_),h(h_){}
		void expand_to_include(int tx, int ty){
			if(tx < x){ w += (x - tx); x = tx; }
			else if(tx >= x+w){ w = tx+1 - x; }
			if(ty < y){ h += (y - ty); y = ty; }
			else if(ty >= y+h){ h = ty+1 - y; }
		}
		bool contains(int tx, int ty) const{
			return (x <= tx && tx < x+w && y <= ty && ty < y+h);
		}
	};

	BoardContent();
	~BoardContent();
public:
	// Drawing/Pen commands
	void pen_set_color(const PenColor &color);
	void pen_get_color(PenColor &color);
	void pen_set_size(float d); // diameter of pen
	void pen_down(pixel_coord x, pixel_coord y);
	void pen_move(pixel_coord x, pixel_coord y);
	void pen_up(pixel_coord x, pixel_coord y);
	void clear(const PenColor &color);

	void get_pen_state(PenColor &color, float &width);
	
	void get_size(unsigned int &width, unsigned int &height) const;
	enum PasteLocation{
		PASTE_LOC_CENTERED = 0x00,
		PASTE_LOC_LEFT     = 0x01,
		PASTE_LOC_RIGHT    = 0x02,
		PASTE_LOC_TOP      = 0x04,
		PASTE_LOC_BOTTOM   = 0x08
	};
	enum PasteFormat{
		PASTE_FORMAT_RGBA    = 0x00,
		PASTE_FORMAT_BGR     = 0x01,
		PASTE_FORMAT_BW      = 0x02
	};
	void paste_image(
		unsigned int location_flags, unsigned int format,
		const unsigned char *img, unsigned row_stride_bytes, unsigned bytes_per_pixel,
		unsigned width, unsigned height
	);
	
	void draw_gui(unsigned char *img, unsigned stride, unsigned width, unsigned height);
	void redraw_gui();
	void gui_click(int i);
	virtual void gui_input(bool pressed, int x, int y);
public:
	unsigned int width, height;
	std::vector<unsigned char> image; // size 3*width*height
	float width_height; // width/height
	Region drawable_region;
	bool moving;

	struct PenState{
		PenColor color;
		float width;
		int cursor_prev[2];
		bool down;
		PenState():
			color(0,0,0),
			width(5.f),
			down(false)
		{
			cursor_prev[0] = 0;
			cursor_prev[1] = 0;
		}
	};
	PenState pen;

	std::vector<PenColor> color_palette;

	void draw_line(pixel_coord x0, pixel_coord y0, pixel_coord x1, pixel_coord y1, Region *touched);
	void set_pixel(pixel_coord x, pixel_coord y, float val, Region *touched);
	virtual void on_image_update(Region *touched = NULL){}
};

#endif // BOARD_CONTENT_H_INCLUDED
