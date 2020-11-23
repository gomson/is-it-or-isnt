#include "controller.h"
#include "../../_common/log.h"

Controller::Controller()
{
	
}

Controller::~Controller()
{
	scePadClose(this->pad);
	DEBUGLOG << "[DEBUG] Controller freed!";
}

bool Controller::Init(int controllerUserID)
{
    // Initialize the Pad library
    if (scePadInit() != 0)
    {
        DEBUGLOG << "[DEBUG] [ERROR] Failed to initialize pad library!";
        return false;
    }

    // Get the user ID
    if (controllerUserID < 0)
    {
    	OrbisUserServiceInitializeParams param;
    	param.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    	sceUserServiceInitialize(&param);
    	sceUserServiceGetInitialUser(&this->userID);
    }
    else
    {
    	this->userID = controllerUserID;
    }

    // Open a handle for the controller
    this->pad = scePadOpen(this->userID, ORBIS_PAD_PORT_TYPE_STANDARD, 0, NULL);

    if (this->pad < 0)
    {
        DEBUGLOG << "[DEBUG] [ERROR] Failed to open pad!";
        return false;
    }
    
    return true;
}

int Controller::GetUserID() {
	return this->userID;
}

void Controller::setButtonState(int state)
{
	this->prevButtonState = this->buttonState;
	this->buttonState = state;
}

void Controller::UpdateState()
{
	scePadReadState(this->pad, &this->padData);
	setButtonState(this->padData.buttons);
}

bool Controller::CheckButtonHeld(int stateToCheck)
{
	return (stateToCheck & this->buttonState);
}

bool Controller::CheckButtonPressed(int stateToCheck)
{
	return (stateToCheck & this->buttonState && !(stateToCheck & this->prevButtonState));
}

bool Controller::CheckButtonReleased(int stateToCheck)
{
	return (!(stateToCheck & this->buttonState) && (stateToCheck & this->prevButtonState));
}
