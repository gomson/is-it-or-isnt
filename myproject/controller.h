#include <orbis/Pad.h>
#include <orbis/UserService.h>

#ifndef CONTROLLER_H
#define CONTROLLER_H

class Controller
{
	int pad;
	int userID;
	int prevButtonState;
	int buttonState;
	OrbisPadData padData;
	
	void setButtonState(int state);
	
public:
	Controller();
	~Controller();
	
	bool Init(int controllerUserID);
	int GetUserID();
	
	void UpdateState();
	bool CheckButtonHeld(int stateToCheck);
	bool CheckButtonPressed(int stateToCheck);
	bool CheckButtonReleased(int stateToCheck);
};

#endif

