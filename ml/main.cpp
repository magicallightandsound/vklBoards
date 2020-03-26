#define MLSDK20

#include "App.h"

int main() {
	App app;
	MLResult result;

	MLLoggingEnableLogLevel(MLLogLevel_Debug);

	if(MLResult_Ok != (result = app.init())){ return result; }
	if(MLResult_Ok != (result = app.perception_init())){ return result; }
	if(MLResult_Ok != (result = app.graphics_init())){ return result; }

	if(MLResult_Ok != (result = app.ready())){ return result; }

	if(MLResult_Ok != (result = app.assets_init())){ return result; }
	if(!app.privileges_init()){ return -1; }

	app.run();

	app.assets_destroy();
	app.privileges_destroy();
	app.graphics_destroy();
	app.perception_destroy();

	return 0;
}
