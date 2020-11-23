#include <sstream>
#include <orbis/SystemService.h>

#include "../../_common/png.h"
#include "../../_common/log.h"
#include "../../_common/graphics.h"
#include "controller.h"
#include "game.h"

// Logging
std::stringstream debugLogStream;

int main(void) {

    // No buffering
    setvbuf(stdout, NULL, _IONBF, 0);

	DEBUGLOG << "--> Can you read this?";
    
    // Create a 2D scene
    DEBUGLOG << "Creating a scene";
    
    auto scene = new Scene2D(FRAME_WIDTH, FRAME_HEIGHT, FRAME_DEPTH);
	int frameID = 0;
    
    if(!scene->Init(0xC000000, FRAME_NUMBUFFERS))
    {
    	DEBUGLOG << "[DEBUG] [ERROR] Failed to initialize 2D scene";
    	for(;;);
    }
    
    // Create a controller
    DEBUGLOG << "Initializing controller";
    
    auto controller = new Controller();
    
    if(!controller->Init(CONTROLLER_ANY_USER))
    {
    	DEBUGLOG << "[DEBUG] [ERROR] Failed to initialize controller";
    	for(;;);
    }

	// make the game class.
	auto gameObject = new Game();
	gameObject->SetObjects(controller, scene);
    
    // Load textures...
	gameObject->Load();
    
    // Main loop
	DEBUGLOG << "--> Entering main loop...";
	sceSystemServiceHideSplashScreen(); // we've loaded everything, can hide the splash.
	
    for (;;)
    {
		// Game logic goes here.
		gameObject->GameFrame();

        // Submit the frame buffer
        scene->SubmitFlip(frameID);
        scene->FrameWait(frameID);

        // Swap to the next buffer
        scene->FrameBufferSwap();
        frameID++;
    }

	delete gameObject;
	delete controller;
	delete scene;

	DEBUGLOG << "[DEBUG] Going to return with EXIT_FAILURE!";

	return EXIT_FAILURE; // you're never supposed to return from main() on a PS4.
}
