#ifndef _GAME_H_
#define _GAME_H_

#include <stdint.h>
#include <math.h>
#include <vector>
#include <thread>
#include <mutex>
#include "../../_common/graphics.h"
#include "../../_common/png.h"

#include "controller.h"
#include "wgfs.h"

// Header library for decoding wav files
#include "dr_wav.h"

// FB Dimensions
#define FRAME_WIDTH     1920
#define FRAME_HEIGHT    1080
#define FRAME_DEPTH        4
#define FRAME_NUMBUFFERS   2

// Other defines
#define CONTROLLER_ANY_USER -1

// a simple enum cast macro.
// Si - Silicon & "signed int"
#define Si(x) (static_cast<signed int>(x))

enum class GameStrings : int {
	TITLE,
	UNDER_TITLE,
	START_TEXT,
	QUESTION_FORMAT,
	QUESTION_ALTVERB_FORMAT,
	UNDER_PICTURE,
	HUD_TEXT,
	LOST_TEXT,
	IDIOT_PNG_TEXT,
	VERSION_TEXT
};

enum class GameState : int {
	MENU,
	LOST,
	PLAY
};

enum class GameHAlign : int {
	LEFT,
	CENTER,
	RIGHT
};

enum class GameVAlign : int {
	TOP,
	MIDDLE,
	BOTTOM
};

class Game {
	Controller *con;
	Scene2D *scene;
	std::vector<PNG*> sprites;
	std::vector<std::string> lookup;
	std::unique_ptr<WGFS::Assets> assets;

	std::vector<FT_Face*> fonts;

	GameState state;

	std::vector<std::string> strings;

	std::string Question;

	int PLAYimageindex;
	bool AnswerIsYes;

	int Score;

	unsigned long long Time;
	bool Count;

	bool StopAudioThread;
	std::mutex StopMutex;

	int32_t AudioHandle;
	std::thread AudioThread;
	size_t SampleCount;
	bool Playing;

	drwav DrWav;
	drwav_int16 *SampleData;

	void StateMenu();
	void StatePlay();
	void StateLost();
	void SimpleAudioThread();
	void HandleAudio();
	void StopAudio();

	void DrawTextAlign(GameHAlign ha, GameVAlign va, char* string, int fontIndex, int x, int y, Color col, TextDimm *out);
	void DrawSpriteAlign(GameHAlign ha, GameVAlign va, int sprite, int x, int y, PNG_INFO* out);

	void ChangeState(GameState s);

public:
	void SetObjects(Controller* const& c, Scene2D* const& sc);
	void GameFrame();
	void Load();

	const char* ToString(GameState v);
	const char* ToString(GameHAlign v);
	const char* ToString(GameVAlign v);
	const char* ToString(bool v);
};

#endif // _GAME_H_