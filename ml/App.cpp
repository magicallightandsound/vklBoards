#define MLSDK20

#include "App.h"
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <cmath>
#include <sstream>

#include <ml_logging.h>
#include <ml_graphics.h>
#include <ml_lifecycle.h>
#include <ml_perception.h>
#include <ml_head_tracking.h>
#include <ml_input.h>
#include <ml_camera.h>
#include <ml_privileges.h>

#include "Shader.h"
#include "Billboard.h"
#include "Whiteboard.h"
#include "Pointer.h"
#include "../zbar/zbar.h"
#include "Controller.h"
#include "Gui.h"
#include "../imgui/imgui.h"

#include "BoardClient.h"
#include "ImageCoder.h"

#include "platform_includes.h"
#include <ml_types.h>

#define MLRESULT_EXPECT(result, expected, string_fn, level)                                                \
  do {                                                                                                     \
    MLResult _evaluated_result = result;                                                                   \
    ML_LOG_IF(level, (expected) != _evaluated_result, "%s:%d %s (%X)%s", __FILE__, __LINE__, __FUNCTION__, \
              _evaluated_result, (string_fn)(_evaluated_result));                                          \
  } while (0)

#define UNWRAP_MLRESULT(result) MLRESULT_EXPECT((result), MLResult_Ok, MLGetResultString, Error)
#define UNWRAP_MLRESULT_FATAL(result) MLRESULT_EXPECT((result), MLResult_Ok, MLGetResultString, Fatal)


const char APP_TAG[] = "vklBoards";
	
void App::on_stop(void* user_data){
	App *app = (App*)user_data;
	app->status = app->STOPPED;
}

void App::on_pause(void* user_data){
	App *app = (App*)user_data;
	app->status = app->PAUSED;
}

void App::on_resume(void* user_data){
	App *app = (App*)user_data;
	app->status = app->RUNNING;
}

App::App(){
	status = RUNNING;
	connection_status = NOT_CONNECTED;
	QRscan.camera_capture_handle = ML_INVALID_HANDLE;
	QRscan.scanner = NULL;
	QRscan.image = NULL;
	active_board = NULL;
	hovered_board = NULL;
	marker_mode = false;
	hide_splash = false;
}
App::~App(){
}

MLResult App::init(){
	MLResult result;
	// Assign call application lifecycle callback functions
	MLLifecycleCallbacksEx lifecycle_callbacks = {};
	MLLifecycleCallbacksExInit(&lifecycle_callbacks);
	lifecycle_callbacks.on_stop = &on_stop;
	lifecycle_callbacks.on_pause = &on_pause;
	lifecycle_callbacks.on_resume = &on_resume;

	// Initialize application lifecycle
	result = MLLifecycleInitEx(&lifecycle_callbacks, this);
	if(result != MLResult_Ok){
		ML_LOG_TAG(Error, APP_TAG, "Failed to initialize lifecycle system");
		return -1;
	}
	return MLResult_Ok;
}
MLResult App::ready(){
	MLResult result;
	result = MLLifecycleSetReadyIndication();
	// Ready for application lifecycle
	if(MLResult_Ok != result){
		ML_LOG_TAG(Error, APP_TAG, "Failed to indicate lifecycle readyness");
		return result;
	}
	return MLResult_Ok;
}

MLResult App::perception_init(){
	// Initialize perception system
	MLPerceptionSettings perception_settings;
	if (MLResult_Ok != MLPerceptionInitSettings(&perception_settings)) {
		ML_LOG_TAG(Error, APP_TAG, "Failed to initialize perception");
		return -1;
	}

	if (MLResult_Ok != MLPerceptionStartup(&perception_settings)) {
		ML_LOG_TAG(Error, APP_TAG, "Failed to startup perception");
		return -1;
	}
	return MLResult_Ok;
}
void App::perception_destroy(){
	MLPerceptionShutdown();
}

MLResult App::graphics_init(){
	return gfx.init();
}
void App::graphics_destroy(){
	gfx.destroy();
}

MLResult App::assets_init(){
	ML_LOG_TAG(Debug, APP_TAG, "Create input system (controller)");
	input = controller.handle;

	shader3D.load_shaders("shader/standard3D.vert", "shader/standard.frag");
	shadertex.load_shaders("shader/tex.vert", "shader/tex.frag");
	shadergui.load_shaders("shader/gui.vert", "shader/gui.frag");

	billboard.load("data/qr.png");
	billboard.ApplyShader(shadertex);
	pointer.ApplyShader(shader3D);
	
	gui.init(&controller);
	gui.ApplyShader(shadertex);
	
	splash.load("data/splash.png");
	splash.ApplyShader(shadertex);
	
	return MLResult_Ok;
}

void App::assets_destroy(){
}

MLResult App::privileges_init(){
	MLResult result;
	UNWRAP_MLRESULT(MLPrivilegesStartup());
	result = MLPrivilegesRequestPrivilege(MLPrivilegeID_CameraCapture);
	if(MLPrivilegesResult_Granted != result){
		ML_LOG(Error, "ImageCapture: Error requesting MLPrivilegeID_CameraCapture privilege, error: %s", MLPrivilegesGetResultString(result));
		return false;
	}
	result = MLPrivilegesRequestPrivilege(MLPrivilegeID_LocalAreaNetwork);
	if(MLPrivilegesResult_Granted != result){
		ML_LOG(Error, "LocalAreaNetwork: Error requesting MLPrivilegeID_LocalAreaNetwork privilege, error: %s", MLPrivilegesGetResultString(result));
		return false;
	}
	return true;
}
void App::privileges_destroy(){
	UNWRAP_MLRESULT(MLPrivilegesShutdown());
}

void App::run(){
	// The main loop
	while(status != STOPPED) {
		if(status == PAUSED){ continue; }
		
		// Render gui
		gui.update_begin();
		build_gui();
		gui.update_end();
		
		process_events();

		// Initialize a frame
		MLGraphicsFrameInfo frame_info;
		
		MLGraphicsFrameParamsEx frame_params;
		MLGraphicsFrameParamsExInit(&frame_params);

		frame_params.surface_scale = 1.0f;
		frame_params.projection_type = MLGraphicsProjectionType_ReversedInfiniteZ;
		frame_params.near_clip = 0.38;
		frame_params.focus_distance = 1.0f;
		
		MLGraphicsFrameInfoInit(&frame_info);
		MLResult frame_result = MLGraphicsBeginFrameEx(gfx.graphics_client, &frame_params, &frame_info);

		if(frame_result == MLResult_Ok){
			// Prepare rendering for each camera/eye
			for (int camera = 0; camera < frame_info.num_virtual_cameras; ++camera) {
				glBindFramebuffer(GL_FRAMEBUFFER, gfx.framebuffer_id);
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, frame_info.color_id, 0, camera);
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, frame_info.depth_id, 0, camera);

				const MLRectf& viewport = frame_info.viewport;
				glViewport((GLint)viewport.x, (GLint)viewport.y, (GLsizei)viewport.w, (GLsizei)viewport.h);

				glClearColor(0.0, 0.0, 0.0, 0.0);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				// Obtain headpose
				MLSnapshot *snapshot = nullptr;
				UNWRAP_MLRESULT(MLPerceptionGetSnapshot(&snapshot));
				MLTransform head_transform = {};
				UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &gfx.head_static_data.coord_frame_head, &head_transform));
				UNWRAP_MLRESULT(MLPerceptionReleaseSnapshot(snapshot));
				glm::mat4 headpose;
				{
					glm::vec3 trans = glm::make_vec3(head_transform.position.values);
					glm::mat4 rotMat = glm::mat4_cast(glm::make_quat(head_transform.rotation.values));
					glm::mat4 transMat = glm::translate(glm::mat4(1.0f), trans);
					headpose = transMat * rotMat;
				}

				// Get the projection matrix
				MLGraphicsVirtualCameraInfo &current_camera = frame_info.virtual_cameras[camera];
				
				glm::mat4 projection(glm::make_mat4(current_camera.projection.matrix_colmajor));
				glm::mat4 modelview;
				{
					glm::vec3 trans = glm::make_vec3(current_camera.transform.position.values);
					glm::mat4 rotMat = glm::mat4_cast(glm::make_quat(current_camera.transform.rotation.values));
					glm::mat4 transMat = glm::translate(glm::mat4(1.0f), trans);
					glm::mat4 worldFromCamera = transMat * rotMat;
					modelview = (glm::inverse(worldFromCamera));
				}
				
				render_scene(projection, modelview, headpose);

				// Bind the frame buffer
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				MLGraphicsSignalSyncObjectGL(gfx.graphics_client, frame_info.virtual_cameras[camera].sync_object);
			}

			// Finish the frame
			MLResult result = MLGraphicsEndFrame(gfx.graphics_client, frame_info.handle);

			if(MLResult_Ok != result){
				ML_LOG_TAG(Error, APP_TAG, "MLGraphicsEndFrame() error: %d", result);
			}
		}else if(frame_result != MLResult_Timeout){
			// Sometimes it fails with timeout when device is busy
			ML_LOG_TAG(Error, APP_TAG, "MLGraphicsBeginFrame() error: %d", frame_result);
		}
	}
	gui.cleanup();
}

void App::update_board_list(){
	std::vector<std::string> board_names;
	if(CONNECTED == connection_status && is_connected()){
		get_boards(board_names);
		get_users(users);
		int n = board_names.size();
		content_remote.resize(n);
		for(int i = 0; i < n; ++i){
			if(NULL == content_remote[i]){
				content_remote[i] = new Whiteboard(this, true, i, board_names[i]);
				content_remote[i]->ApplyShader(shadertex);
			}
		}
	}
}

void App::render_scene(const glm::mat4 &projection, const glm::mat4 &modelview, const glm::mat4 &headpose){
	glm::mat4 projectionMatrix = projection * modelview;

	// Render objects
	for(int i = 0; i < content_remote.size(); ++i){
		content_remote[i]->Render(projectionMatrix);
	}
	for(int i = 0; i < content_local.size(); ++i){
		content_local[i]->Render(projectionMatrix);
	}
	
	// Render head-locked QR overlay
	if(connection_status == LOOKING_FOR_QRCODE){
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		billboard.Render(projection * modelview * headpose);
		glDisable(GL_BLEND);
	}
	
	// Render pointer
	if(!gui.is_visible() && active_board){
		BoardContent::PenColor color;
		float width;
		active_board->get_pen_state(color, width);
		pointer.SetColorAndRadius(color.rgb, 0.5*width*(active_board->position.scale/1024));
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		pointer.Render(projectionMatrix * controller_pose);
		glDisable(GL_BLEND);
	}
	
	if(!hide_splash){
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		splash.Render(projection * modelview * headpose);
		glDisable(GL_BLEND);
	}
	
	// Render GUI
	gui.Render(projectionMatrix);
}

void App::build_gui(){
	if (ImGui::ListBoxHeader("Boards")) {
		for(int i = 0; i < content_remote.size() + content_local.size(); ++i){
			std::stringstream title;
			Whiteboard *board;
			if(i < content_remote.size()){
				board = content_remote[i];
				title << "[shared] ";
			}else{
				int ib = i - content_remote.size();
				board = content_local[ib];
				title << "[local] ";
			}
			title << board->get_name();
			bool is_visible = board->get_visibility();
			board->set_highlight(false);
			if(ImGui::Selectable(title.str().c_str(), is_visible)){
				if(!is_visible && !board->has_placement){
					glm::vec3 pos;
					glm::quat rot;
					gui.get_placement_pose(pos, rot);
					board->set_position(pos, rot);
				}
				board->set_visibility(!is_visible);
				if(!is_visible){
					if(i < content_remote.size() && connection_status == CONNECTED){
						request_update(i);
					}
				}
			}
			board->set_highlight(gui.is_visible() && ImGui::IsItemHovered());
		}
		ImGui::ListBoxFooter();
	}
	if(ImGui::Button("New board")){
		std::stringstream title;
		if(connection_status == CONNECTED){ // call to is_connected possibly crashing here
			title << "Board " << content_remote.size();
			new_board(title.str(), 2048, 1024);
			update_board_list();
		}else{
			title << "Board " << content_local.size();
			content_local.push_back(new Whiteboard(NULL, false, content_local.size(), title.str()));
			content_local.back()->ApplyShader(shadertex);
		}
	}
	if(ImGui::Button("Refresh")){
		if(connection_status == CONNECTED){
			update_board_list();
		}
	}
	ImGui::Checkbox("Marker mode", &marker_mode);
	
	
	if(connection_status == NOT_CONNECTED){
		if (ImGui::Button("Connect")) {
			QR_scan_begin();
		}
	}else{
		if (ImGui::Button("Disconnect")) {
			disconnect();
		}
	}
	if (ImGui::Button("Exit")) {
		if(connection_status == CONNECTED){
			disconnect();
		}
		status = STOPPED;
	}
}
void App::process_events(){
	// Update controller
	controller.poll();
	if(controller.connected()) {
		controller.get_pose(controller_pose);
	}
	
	if(controller.get_bumper()){ hide_splash = true; }
	
	// Start image capture if needed
	if(connection_status == PROCESSING_IMAGE){
		QR_scan_end();
		std::string name("<magic leap device>");
		if(0 == connect(server_uri, name)){
			ML_LOG_TAG(Error, APP_TAG, "Connected!\n");
			update_board_list();
		}else{
			connection_status = NOT_CONNECTED;
			ML_LOG_TAG(Error, APP_TAG, "Connection failed\n");
		}
	}else if(connection_status == CONNECTED){
		BoardClient::poll();
	}

	// Determine if user is pointing at a board
	Whiteboard *prev_active_board = active_board;
	active_board = NULL;
	{
		glm::vec4 z = controller_pose[2];
		glm::vec4 pos = controller_pose[3];
		float tmin = std::numeric_limits<float>::max();
		for(int i = 0; i < content_remote.size() + content_local.size(); ++i){
			int ib = i;
			bool remote = true;
			if(i >= content_remote.size()){
				ib = i - content_remote.size();
				remote = false;
			}
			Whiteboard *board = (remote ? content_remote[ib] : content_local[ib]);
			
			glm::vec2 p;
			float t;
			int x = board->RayIntersection(glm::vec3(pos[0], pos[1], pos[2]), glm::vec3(-z[0], -z[1], -z[2]), p, t);
			if(0 == x || t <= 0){ continue; }
			if(t >= tmin){ continue; } // not closest
			tmin = t;
			active_board = board;
			glm::vec3 pos;
			glm::quat rot;
			controller.get_pose(pos, rot);
			glm::vec3 touch_delta;
			controller.get_touch(touch_delta);
			if(active_board != prev_active_board){
				active_board->redraw_gui();
			}
			bool is_marking = false;
			if(marker_mode){
				static const float stub_length = 0.05f; // in meters
				float actual_length = stub_length;
				if(t < stub_length){
					is_marking = true;
					actual_length = t;
				}
				pointer.SetLength(actual_length, t - actual_length);
				active_board->ReceiveInput(pos, rot, is_marking && !gui.is_visible(), p, touch_delta);
			}else{
				static const float trigger_threshold = 0.1f; // 0 = no actuation, 1 = full actuation
				pointer.SetLength(t);
				is_marking = controller.get_trigger() > trigger_threshold;
			}
			active_board->ReceiveInput(pos, rot, is_marking && !gui.is_visible(), p, touch_delta);
			break;
		}
	}
}

int App::connect(const std::string &server_uri, const std::string &name){
	int ret = BoardClient::connect(server_uri, name);
	if(0 != ret){
		ML_LOG_TAG(Error, APP_TAG, "BoardClient::connect returned %d\n", ret);
		return ret;
	}
	connection_status = CONNECTED;
	get_boards(boards);
	
	return ret;
}
int App::disconnect(){
	connection_status = NOT_CONNECTED;
	for(size_t i = 0; i < content_remote.size(); ++i){
		delete content_remote[i];
	}
	content_remote.clear();
	return BoardClient::disconnect();
}
void App::on_user_connected(const std::string &name){
	users.push_back(name);
}
void App::on_user_disconnected(const std::string &name){
	std::vector<std::string>::iterator it = std::find(users.begin(), users.end(), name);
	if(it != users.end()){
		users.erase(it);
	}
}
void App::on_update(board_index iboard, int method, const unsigned char *buffer, unsigned buflen, unsigned x, unsigned y, unsigned w, unsigned h){
	if(!(0 <= iboard && iboard < content_remote.size())){ return; }
	ML_LOG_TAG(Debug, APP_TAG, "on_update %d\n", iboard);
	
	Whiteboard *board = content_remote[iboard];
	unsigned width = 2048;
	ImageCoder::decode(method,
		buffer, buflen,
		&board->image[3*(x+y*width)], width, w, h
	);
	board->UpdateTexture(&board->image[0], width, x, y, w, h);
}
void App::on_board_list_update(const std::vector<std::string> &boards_){
	boards = boards_;
	int n = boards.size();
	content_remote.resize(n);
	for(int i = 0; i < n; ++i){
		if(NULL == content_remote[i]){
			content_remote[i] = new Whiteboard(this, true, i, boards[i]);
			content_remote[i]->ApplyShader(shadertex);
		}
	}
}


// -----------------------------------------------------------------------------
// 2. OpenGL context functions

App::graphics_context_t::graphics_context_t():
	graphics_client(ML_INVALID_HANDLE)
{
	egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	EGLint major = 4;
	EGLint minor = 0;
	eglInitialize(egl_display, &major, &minor);
	eglBindAPI(EGL_OPENGL_API);

	EGLint config_attribs[] = {
		EGL_RED_SIZE, 5,
		EGL_GREEN_SIZE, 6,
		EGL_BLUE_SIZE, 5,
		EGL_ALPHA_SIZE, 0,
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};

	EGLConfig egl_config = nullptr;
	EGLint config_size = 0;
	eglChooseConfig(egl_display, config_attribs, &egl_config, 1, &config_size);

	EGLint context_attribs[] = {
		EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
		EGL_CONTEXT_MINOR_VERSION_KHR, 0,
		EGL_NONE
	};

	egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
}

void App::graphics_context_t::make_current() {
	eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_context);
}

void App::graphics_context_t::unmake_current() {
	eglMakeCurrent(NULL, EGL_NO_SURFACE, EGL_NO_SURFACE, NULL);
}

App::graphics_context_t::~graphics_context_t() {
	eglDestroyContext(egl_display, egl_context);
	eglTerminate(egl_display);
}

MLResult App::graphics_context_t::init(){
	// Create OpenGL context and create framebuffer
	make_current();
	glGenFramebuffers(1, &framebuffer_id);

	MLGraphicsOptions graphics_options = { 0, MLSurfaceFormat_RGBA8UNorm, MLSurfaceFormat_D32Float };
	MLHandle opengl_context = reinterpret_cast<MLHandle>(egl_context);
	graphics_client = ML_INVALID_HANDLE;
	MLGraphicsCreateClientGL(&graphics_options, opengl_context, &graphics_client);

	// Set up the head tracker
	MLResult head_track_result = MLHeadTrackingCreate(&head_tracker);

	if(MLResult_Ok == head_track_result && MLHandleIsValid(head_tracker)){
		MLHeadTrackingGetStaticData(head_tracker, &head_static_data);
	}else{
		ML_LOG_TAG(Error, APP_TAG, "Failed to create head tracker");
	}
	return MLResult_Ok;
}
void App::graphics_context_t::destroy(){
	unmake_current();
	glDeleteFramebuffers(1, &framebuffer_id);
	MLGraphicsDestroyClient(&graphics_client);
}


void App::on_video_buffer(
	const MLCameraOutput *output,
	const MLCameraResultExtras *extra,
	const MLCameraFrameMetadata *frame_metadata,
	void *user_data
){
	App *app = (App*)user_data;
	if(app->connection_status != app->LOOKING_FOR_QRCODE){ return; }
	if(0 != (extra->frame_number & 0x7)){ return; } // only analyze every 8th frame

	ML_LOG_TAG(Debug, APP_TAG, "Video buffer received\n");
	
	zbar::zbar_image_set_size(app->QRscan.image, output->planes[0].stride, output->planes[0].height);
	zbar::zbar_image_set_data(app->QRscan.image, output->planes[0].data, output->planes[0].stride * output->planes[0].height, NULL);
	
	ML_LOG_TAG(Debug, APP_TAG, "Scanning for QR codes\n");
	
	int n = zbar::zbar_scan_image(app->QRscan.scanner, app->QRscan.image);
	const zbar::zbar_symbol_t *symbol = zbar::zbar_image_first_symbol(app->QRscan.image);
	for (; symbol; symbol = zbar::zbar_symbol_next(symbol)) {
		zbar::zbar_symbol_type_t typ = zbar::zbar_symbol_get_type(symbol);
		const char *data = zbar::zbar_symbol_get_data(symbol);
		ML_LOG_TAG(Error, APP_TAG, "decoded %s symbol \"%s\"\n", zbar::zbar_get_symbol_name(typ), data);
		
		app->connection_status = app->PROCESSING_IMAGE; // At this point, flag as not connected and stop camera captures.
		app->server_uri = data;
		break;
	}
}

void App::QR_scan_begin(){
	UNWRAP_MLRESULT_FATAL(MLCameraConnect());
	
	ML_LOG_TAG(Debug, APP_TAG, "Installing callbacks\n");
		
	MLCameraCaptureCallbacksEx camcb = { 0 };
	MLCameraCaptureCallbacksExInit(&camcb);
	camcb.on_video_buffer_available = &on_video_buffer;
	UNWRAP_MLRESULT_FATAL(MLCameraSetCaptureCallbacksEx(&camcb, this));

	ML_LOG_TAG(Debug, APP_TAG, "Preparing capture\n");
	
	UNWRAP_MLRESULT(MLCameraPrepareCapture(MLCameraCaptureType_VideoRaw, &QRscan.camera_capture_handle));
	
	ML_LOG_TAG(Debug, APP_TAG, "Initializing zbar\n");

	QRscan.scanner = zbar::zbar_image_scanner_create();
	zbar::zbar_image_scanner_set_config(QRscan.scanner, zbar::ZBAR_QRCODE, zbar::ZBAR_CFG_ENABLE, 1);
	QRscan.image = zbar::zbar_image_create();
	zbar::zbar_image_set_format(QRscan.image, *(int*)"Y800");
	
	ML_LOG_TAG(Debug, APP_TAG, "Starting video\n");
	UNWRAP_MLRESULT(MLCameraCaptureRawVideoStart()); // this is blocking
	ML_LOG_TAG(Debug, APP_TAG, "Started video\n");
	
	connection_status = LOOKING_FOR_QRCODE;
}
void App::QR_scan_end(){
	UNWRAP_MLRESULT(MLCameraCaptureVideoStop());
	UNWRAP_MLRESULT(MLCameraDisconnect());
	zbar::zbar_image_destroy(QRscan.image);
	zbar::zbar_image_scanner_destroy(QRscan.scanner);
}
