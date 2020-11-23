#ifndef _WGFS_H_
#define _WGFS_H_

#include <vector>
#include <unordered_map>
#include <string>
#include "../../_common/png.h"

namespace WGFS
{
	// WGFS - Weird Game File System
	// A format used to pack files into a single file for no reason.

	typedef struct _wgfs_file {
		const char *name;
		size_t size;
		unsigned char* data;
	} File;

	class Assets {
		const int WGFS_HEADER = 1397114711;
		const int FILE_HEADER = 1162627398;
		const int STRG_HEADER = 1196577875;
		const int WEND_HEADER = 1145980247;

		std::vector<std::unique_ptr<File>> Files;
		std::unordered_map<std::string, std::string> Strings;
		unsigned char* WGFSData;
		long seek;

		char ReadInt8();
		short ReadInt16();
		int ReadInt32();
		
		void LoadInternal();
		void* GetCurDataAddr();
		long SkipString();
		long SkipBytes(size_t bytes);

	public:
		~Assets(); // free all the files.

		void LoadFromMem(size_t size, unsigned char* filebuf);
		void LoadFromFile(const char* filename);

		File* GetFileByIndex(int index);
		File* GetFileByName(const char* name);
		size_t GetFilesAmount();
		std::string GetString(std::string key);

		// OpenOrbis stuff
		PNG* MakePNGFromFile(File* file);
		FT_Face* MakeFontFromFile(File* file, int fontSize, Scene2D* scene);
	};
}

#endif // _WGFS_H_