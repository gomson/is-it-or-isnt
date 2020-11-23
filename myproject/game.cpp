#include <stdint.h>
#include "controller.h"
#include "../../_common/graphics.h"
#include "../../_common/log.h"
#include "game.h"
#include <time.h>
#include <list>
#include <cctype>

#include <orbis/AudioOut.h>
#define PARAMS16 ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#define FONT_MENU 0
#define FONT_HELP 1

#define FONT_MENU_SIZE 48 /* default font used in the menu */
#define FONT_HELP_SIZE 24 /* a small font for tips & helps */

#define GAME_VERSION "1.0"

const char* Game::ToString(GameHAlign v) {
	switch (v) {
		case GameHAlign::CENTER: return "CENTER";
		case GameHAlign::LEFT: return "LEFT";
		case GameHAlign::RIGHT: return "RIGHT";
		default: return "[UNKNOWN]";
	}
}

const char* Game::ToString(GameVAlign v) {
	switch (v) {
		case GameVAlign::BOTTOM: return "BOTTOM";
		case GameVAlign::MIDDLE: return "MIDDLE";
		case GameVAlign::TOP: return "TOP";
		default: return "[UNKNOWN]";
	}
}

const char* Game::ToString(GameState v) {
	switch (v) {
		case GameState::MENU: return "MENU";
		case GameState::LOST: return "LOST";
		case GameState::PLAY: return "PLAY";
		default: return "[UNKNOWN]";
	}
}

const char* Game::ToString(bool v) {
	return v ? "True" : "False";
}

void Game::SetObjects(Controller* const& c, Scene2D* const& sc) {
	this->con = c;
	this->scene = sc;
}

void Game::DrawTextAlign(GameHAlign ha, GameVAlign va, char* string, int fontIndex, int x, int y, Color col, TextDimm *out) {
	TextDimm myDimm = { 0, 0 };
	FT_Face font = *(this->fonts[fontIndex]);
	this->scene->CalcTextDimm(string, font, &myDimm);

	switch (ha) {
		case GameHAlign::CENTER: {
			x -= (myDimm.w / 2);
			break;
		}

		case GameHAlign::RIGHT: {
			x -= (myDimm.w);
			break;
		}

		case GameHAlign::LEFT: {
			// do nothing.
			break;
		}
	}

	switch (va) {
		case GameVAlign::MIDDLE: {
			y -= (myDimm.h / 2);
			break;
		}

		case GameVAlign::BOTTOM: {
			y -= (myDimm.h);
			break;
		}

		case GameVAlign::TOP: {
			// default???
			break;
		}
	}

	// always assume black background color because I am lazy.
	this->scene->DrawText(string, font, x, y, { 0, 0, 0 }, col);

	if (out != nullptr) {
		out->w = myDimm.w;
		out->h = myDimm.h;
	}
}

void Game::DrawSpriteAlign(GameHAlign ha, GameVAlign va, int sprite, int x, int y, PNG_INFO* out) {
	auto spr = this->sprites[sprite];
	PNG_INFO info = { 0, 0, 0 };
	spr->GetInfo(&info);

	switch (ha) {
		case GameHAlign::LEFT: {
			// the default.
			break;
		}

		case GameHAlign::CENTER: {
			x -= (info.w / 2);
			break;
		}

		case GameHAlign::RIGHT: {
			x -= (info.w);
			break;
		}
	}

	switch (va) {
		case GameVAlign::TOP: {
			// the default.
			break;
		}

		case GameVAlign::MIDDLE: {
			y -= (info.h / 2);
			break;
		}

		case GameVAlign::BOTTOM: {
			y -= (info.h);
			break;
		}
	}

	spr->Draw(this->scene, x, y);

	if (out != nullptr) {
		out->w = info.w;
		out->h = info.h;
		out->channels = info.channels;
	}
}

bool RandomBool() {
	return ((rand() % 2) == 1 ? true : false);
}

bool IsAnVerb(std::string const& str) {
	// a e i o u
	switch (std::tolower(str.at(0))) {
		case 'a':
		case 'e':
		case 'i':
		case 'o':
		case 'u':
			return true;
		default:
			return false;
	}
}

void Game::SimpleAudioThread() {
	int32_t sOffs = 0;
	drwav_int16 *pSample = nullptr;

	for (;;) {
		this->StopMutex.lock();


		if (this->StopAudioThread) {
			this->StopAudioThread = false;
			DEBUGLOG << "[AUDIO]: STOPPING AUDIO THREAD...";
			this->StopMutex.unlock();
			break;
		}

		this->StopMutex.unlock();

		// the sample data is never accessed after ::Load so we don't need to do the mutex magic here...
		// the StopAudioThread variable is, so we use a mutex for it.

		pSample = &(this->SampleData[sOffs]);
		// Output audio.
		sceAudioOutOutput(this->AudioHandle, NULL);	// NULL: wait for completion

		if (sceAudioOutOutput(this->AudioHandle, pSample) < 0) {
			DEBUGLOG << "[AUDIO]: Failed to output audio!";
		}

		sOffs += 256 * sizeof(drwav_int16);
		if (sOffs >= this->SampleCount) sOffs = 0; // start again.
	}
}

void Game::HandleAudio() {
	if (this->SampleData != nullptr && !this->Playing) {
		this->AudioThread = std::thread(&Game::SimpleAudioThread, this);
		this->Playing = true;

		DEBUGLOG << "[GAME]: Starting audio thread!";
	}
}

void Game::StopAudio() {
	if (this->Playing) {
		this->Playing = false;

		this->StopMutex.lock();
		this->StopAudioThread = true;
		this->StopMutex.unlock();

		this->AudioThread.join();

		DEBUGLOG << "[GAME]: Stopped audio thread!";
	}
}

void Game::ChangeState(GameState s) {
	DEBUGLOG << "[GAME]: Setting new state to " << this->ToString(s);

	this->state = s;

	// you can preload some stuff for the game state here...
	switch (s) {
		case GameState::PLAY: {
			this->Count = true;

			this->HandleAudio();

			// the new sprite must not be equal to the current sprite!
			// otherwise the game would be boring.
			int oldSprite = this->PLAYimageindex;
			do {
				this->PLAYimageindex = (rand() % this->sprites.size());
			} while (oldSprite == this->PLAYimageindex);

			this->AnswerIsYes = RandomBool();

			// question index is equal to our picture.
			int qIndex = this->PLAYimageindex;

			// if the answer should be "No"
			if (!this->AnswerIsYes) {
				// find any other name that's not this file.
				do {
					qIndex = (rand() % this->sprites.size());
				} while (qIndex == this->PLAYimageindex);
			}

			DEBUGLOG <<
				"playindex " << this->PLAYimageindex <<
				" isyes " << this->ToString(this->AnswerIsYes) <<
				" qindex " << qIndex;

			std::string filename = this->lookup[qIndex];
			filename = filename.substr(0, filename.find('.', 0));

			DEBUGLOG << "filename " << filename;

			if (IsAnVerb(filename)) // 'is an '
				this->Question = this->strings[Si(GameStrings::QUESTION_ALTVERB_FORMAT)];
			else // 'is a '
				this->Question = this->strings[Si(GameStrings::QUESTION_FORMAT)];

			char fmt_question[128] = { };
			snprintf(fmt_question, sizeof(fmt_question), this->Question.c_str(), filename.c_str());

			this->Question = fmt_question;

			DEBUGLOG << "formatted question " << fmt_question;

			break;
		}

		case GameState::LOST: {
			this->Count = false;
			this->StopAudio();
			DEBUGLOG << "[LOST]: time --> " << this->Time;
			break;
		}

		default: break;
	}
}

void Game::StateMenu() {
	Color white = { 255, 255, 255 };

	// title text.
	char* titleString = (char*)this->strings[Si(GameStrings::TITLE)].c_str();
	char* underString = (char*)this->strings[Si(GameStrings::UNDER_TITLE)].c_str();
	char* startString = (char*)this->strings[Si(GameStrings::START_TEXT)].c_str();
	int titleMargin = FONT_MENU_SIZE;
	int centerX = FRAME_WIDTH / 2;
	int centerY = FRAME_HEIGHT / 2;

	GameHAlign h = GameHAlign::CENTER;
	GameVAlign v = GameVAlign::MIDDLE;

	TextDimm dimm = { 0, 0 };

	this->DrawTextAlign(h, v, titleString, FONT_MENU, centerX, centerY - titleMargin, white, &dimm);
	this->DrawTextAlign(h, v, underString, FONT_HELP, centerX, centerY - titleMargin + dimm.h, white, nullptr);

	this->DrawTextAlign(h, v, startString, FONT_MENU, centerX, centerY + (titleMargin * 4), white, nullptr);

	char versionString[128] = { };
	snprintf(versionString, sizeof(versionString), this->strings[Si(GameStrings::VERSION_TEXT)].c_str(), GAME_VERSION);

	this->DrawTextAlign(GameHAlign::LEFT, GameVAlign::TOP, versionString, FONT_HELP, 64, 64, white, nullptr);

	/*
	this->scene->DrawText((char*)(std::string("Halign: ") + std::string(this->ToString(this->hal))).c_str(), *this->fonts[0], 64, 64, black, white);
	this->scene->DrawText((char*)(std::string("Valign: ") + std::string(this->ToString(this->al))).c_str(), *this->fonts[0], 64, 128, black, white);
	this->scene->DrawText((char*)(std::string("string height: ") + std::to_string(dimm.h)).c_str(), *this->fonts[0], 64, 256, black, white);
	*/

	if (this->con->CheckButtonPressed(ORBIS_PAD_BUTTON_CROSS) || this->con->CheckButtonPressed(ORBIS_PAD_BUTTON_OPTIONS)) {
		DEBUGLOG << "[MENU]: Start button pressed!";
		this->ChangeState(GameState::PLAY);
	}
}

void Game::StatePlay() {
	Color white = { 255, 255, 255 };

	int centerX = FRAME_WIDTH / 2;
	int centerY = FRAME_HEIGHT / 2;
	int margin = FONT_MENU_SIZE;

	PNG_INFO pI = { 0, 0, 0 };

	char* underPicture = (char*)this->strings[Si(GameStrings::UNDER_PICTURE)].c_str();

	this->DrawSpriteAlign(GameHAlign::CENTER, GameVAlign::MIDDLE, this->PLAYimageindex, centerX, centerY, &pI);

	this->DrawTextAlign(GameHAlign::CENTER, GameVAlign::BOTTOM, (char*)this->Question.c_str(), FONT_MENU, centerX, centerY - (pI.h / 2) - margin/2, white, nullptr);
	this->DrawTextAlign(GameHAlign::CENTER, GameVAlign::TOP, underPicture, FONT_MENU, centerX, centerY + (pI.h / 2) + margin, white, nullptr);

	char scorestr[128] = { };
	snprintf(scorestr, sizeof(scorestr), this->strings[Si(GameStrings::HUD_TEXT)].c_str(), this->Score);

	this->DrawTextAlign(GameHAlign::LEFT, GameVAlign::TOP, scorestr, FONT_MENU, 64, 64, white, nullptr);

	int input = this->con->CheckButtonPressed(ORBIS_PAD_BUTTON_CROSS) - this->con->CheckButtonPressed(ORBIS_PAD_BUTTON_CIRCLE);
	if (input != 0) {
		bool correct = false;
		if (this->AnswerIsYes)
			correct = input > 0;
		else
			correct = input < 0;

		if (correct) {
			DEBUGLOG << "[PLAY]: answer correct!";
			this->Score++;
			ChangeState(GameState::PLAY); // restart the state.
		}
		else {
			DEBUGLOG << "[PLAY]: answer not correct!";
			ChangeState(GameState::LOST);
		}
	}
}

void Game::StateLost() {
	char* lostString = (char*)this->strings[Si(GameStrings::LOST_TEXT)].c_str();
	Color white = { 255, 255, 255 };

	int lostX = FRAME_WIDTH / 2;
	int lostY = (FRAME_HEIGHT / 2) - FONT_MENU_SIZE;
	int scoreY = (FRAME_HEIGHT / 2) + FONT_MENU_SIZE;
	this->DrawTextAlign(GameHAlign::CENTER, GameVAlign::BOTTOM, lostString, FONT_MENU, lostX, lostY, white, nullptr);

	char scorestr[128] = { };
	snprintf(scorestr, sizeof(scorestr), this->strings[Si(GameStrings::HUD_TEXT)].c_str(), this->Score);

	this->DrawTextAlign(GameHAlign::CENTER, GameVAlign::TOP, scorestr, FONT_MENU, lostX, scoreY, white, nullptr);

	if (strcmp(this->lookup[this->PLAYimageindex].c_str(), "idiot.png") == 0) {
		char* nGonIsAnIdiot = (char*)this->strings[Si(GameStrings::IDIOT_PNG_TEXT)].c_str();

		//Color dkwhite = { 250, 250, 250 };

		this->DrawTextAlign(GameHAlign::LEFT, GameVAlign::TOP, nGonIsAnIdiot, FONT_HELP, 64, 64, white, nullptr);
	}

	if (this->con->CheckButtonPressed(ORBIS_PAD_BUTTON_CROSS) || this->con->CheckButtonPressed(ORBIS_PAD_BUTTON_OPTIONS)) {
		DEBUGLOG << "[LOST]: Start button pressed!";
		this->Score = 0;
		this->ChangeState(GameState::PLAY);
	}
}

void Game::GameFrame() {

	this->con->UpdateState(); // update the dualshock's state.
	this->scene->FrameBufferClear(); // clear the frame buffer.

	/*
		The game is supposed to run at 30 FPS BUT due to the weird way how
		SceVideoOut and VSync works

		I still couldn't figure out how to properly measure game time :\

		Maybe =you= can figure it out?
		
	*/

	if (this->Count) this->Time++; // ""timer""

	// process the current game state.
	switch (this->state) {
		case GameState::MENU: {
			StateMenu();
			break;
		}

		case GameState::PLAY: {
			StatePlay();
			break;
		}

		case GameState::LOST: {
			StateLost();
			break;
		}
	}

}

void Game::Load() {
	// load sum assets here lol
	DEBUGLOG << "Game::Load()!";
	this->assets = std::make_unique<WGFS::Assets>();
	this->assets->LoadFromFile("/app0/assets/data.dat");

	this->state = GameState::MENU;
	this->Count = false;
	this->PLAYimageindex = -1;

	unsigned int seed = ((unsigned int)(time(NULL) & UINT32_MAX))/* / 2U*/; // this is bad but it was 1 AM.
	srand(seed);
	DEBUGLOG << "srand seed " << seed;

	DEBUGLOG << "init font!";
	auto font = this->assets->GetFileByName("font.ttf");
	this->fonts.push_back(this->assets->MakeFontFromFile(font, FONT_MENU_SIZE, this->scene));
	this->fonts.push_back(this->assets->MakeFontFromFile(font, FONT_HELP_SIZE, this->scene));

	DEBUGLOG << "init strings!";
	std::list<std::string> stringids{ "title", "under_title", "start_text", "question_format", "question_altverb_format", "under_picture", "hud_text", "lost_text", "idiot_png_text", "version_text" };
	
	for (auto& name : stringids) {
		DEBUGLOG << "loading string " << name;
		this->strings.push_back(this->assets->GetString(name));
	}

	DEBUGLOG << "init sprites!";
	std::list<std::string> spritenames{ "banana.png", "cat.png", "idiot.png", "pug.png", "router.png", "opossum.png", "rat.png", "mirror.png", "fox.png", "pen.png", "flashdrive.png" };

	for (auto& name : spritenames) {
		DEBUGLOG << "loading sprite " << name;
		auto file = this->assets->GetFileByName(name.c_str()); // get the sprite's file struct
		if (file == nullptr) continue;

		this->lookup.push_back(name); // push the name of the sprite to a SPRITE|NAME look up table.
		this->sprites.push_back(this->assets->MakePNGFromFile(file)); // make an openorbis PNG struct from the file struct.
	}

	DEBUGLOG << "init audio!";
	auto sound = this->assets->GetFileByName("audio.wav");
	
	this->AudioHandle = -1;
	this->Playing = false;
	this->SampleData = nullptr;

	int err = sceAudioOutInit();
	if (err == 0 && sound != nullptr) {
		DEBUGLOG << "sceAudio init ok";

		this->AudioHandle = sceAudioOutOpen(ORBIS_USER_SERVICE_USER_ID_SYSTEM, ORBIS_AUDIO_OUT_PORT_TYPE_MAIN, 0, 256, 48000, PARAMS16);

		if (this->AudioHandle <= 0) {
			DEBUGLOG << "sceAudioOut open fail";
		}
		else {
			if (!drwav_init_memory(&this->DrWav, sound->data, sound->size, NULL)) {
				DEBUGLOG << "drwav init fail :(";
			}
			else {
				// Calculate the sample count and allocate a buffer for the sample data accordingly
				size_t sampleCount = this->DrWav.totalPCMFrameCount * this->DrWav.channels;
				this->SampleData = (drwav_int16 *)malloc(sampleCount * sizeof(drwav_int16));

				// Decode the wav into pSampleData
				drwav_read_pcm_frames_s16(&this->DrWav, this->DrWav.totalPCMFrameCount, this->SampleData);

				this->SampleCount = sampleCount;

				DEBUGLOG << ".wav decoded and loaded!";
			}
		}
	}
}