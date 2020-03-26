#include "platform_includes.h"
#include "BoardClient.h"
#include "Shader.h"
#include "Billboard.h"
#include "Whiteboard.h"
#include "Controller.h"
#include "Pointer.h"
#include "Gui.h"
#include "../zbar/zbar.h"
#include <ml_types.h>
#include <ml_logging.h>

class MLCameraOutput;

class App : BoardClient{
	enum {
		STOPPED, PAUSED, RUNNING
	} status;
	
	struct graphics_context_t {
		EGLDisplay egl_display;
		EGLContext egl_context;
		GLuint framebuffer_id;
		MLHandle graphics_client;

		graphics_context_t();
		~graphics_context_t();
		
		MLResult init();
		void destroy();

		void make_current();
		void unmake_current();
	};
	graphics_context_t gfx;
	
	Shader shader3D;
	Shader shadergui;
	Shader shadertex;
	
	volatile enum {
		NOT_CONNECTED, LOOKING_FOR_QRCODE, PROCESSING_IMAGE, CONNECTED
	} connection_status;
	std::string server_uri;
	
	// QR Code scanning stuff
	struct{
		MLHandle camera_capture_handle;
		zbar::zbar_image_scanner_t *scanner;
		zbar::zbar_image_t *image;
	} QRscan;
	
	std::vector<std::string> boards;
	std::vector<std::string> users;
	std::vector<Whiteboard*> content_remote;
	std::vector<Whiteboard*> content_local;
	Whiteboard *active_board; // the board that the user is currently pointing at
	Whiteboard *hovered_board; // the board that is being hovered over in the GUI list

	MLHandle input;
	Controller controller;
	Billboard billboard;
	Pointer pointer;
	Gui gui;
	
	bool marker_mode; // if true, the pointer is stubby, and depressing trigger is not necessary
	
	glm::mat4 controller_pose;
	
	// callbacks for application lifecycle system
	static void on_stop(void* user_data);
	static void on_pause(void* user_data);
	static void on_resume(void* user_data);
	
	static void on_camera_buffer(const MLCameraOutput *output, void *user_data);
	
	void update_board_list();
public:
	App();
	~App();
	
	MLResult init(); // initialize lifecycle system
	MLResult ready(); // set lifecycle to ready state
	
	MLResult perception_init(); // initialize perception sysytem
	void     perception_destroy();
	
	MLResult graphics_init(); // initialize OpenGL
	void     graphics_destroy();
	
	MLResult assets_init();
	void     assets_destroy();
	
	MLResult privileges_init();
	void     privileges_destroy();
	
	void run();
	
	// The main loop calls the following functions in this order:
	void build_gui();
	void process_events();
	void render_scene(const glm::mat4 &projection, const glm::mat4 &modelview);
	
	void QR_scan_begin();
	void QR_scan_poll();
	void QR_scan_end();
	
	// Overrides from BoardClient
	int connect(const std::string &server_uri, const std::string &name);
	int disconnect();
	
	void on_user_connected(const std::string &name);
	void on_user_disconnected(const std::string &name);
	void on_update(board_index iboard, int method, const unsigned char *buffer, unsigned buflen, unsigned x, unsigned y, unsigned w, unsigned h);
};
