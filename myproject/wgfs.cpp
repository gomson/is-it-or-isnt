#include "../../_common/log.h"
#include "wgfs.h"


namespace WGFS
{
	Assets::~Assets()
	{
		this->Files.clear();
		this->Files.resize(0);
		this->Files.shrink_to_fit();
		this->Strings.clear();

		delete this->WGFSData;
	}

	File* Assets::GetFileByIndex(int index)
	{
		return (this->Files[index]).get();
	}

	File* Assets::GetFileByName(const char* name)
	{
		for (int i = 0; i < this->Files.size(); i++)
		{
			auto f = GetFileByIndex(i);
			if (strcmp(name, this->Files[i]->name) == 0) return GetFileByIndex(i);
		}

		DEBUGLOG << "[WGFS|ERROR]: Unable to find file " << name;

		return nullptr;
	}

	size_t Assets::GetFilesAmount()
	{
		return this->Files.size();
	}

	PNG* Assets::MakePNGFromFile(File* f)
	{
		PNG* ret = new PNG(f->size, f->data);
		return ret;
	}

	FT_Face* Assets::MakeFontFromFile(File* file, int fontSize, Scene2D* scene)
	{
		FT_Face* ret = new FT_Face;

		if (!scene->InitMemFont(ret, file->size, file->data, fontSize))
			DEBUGLOG << "font init fail!";
		else
			DEBUGLOG << "font init ok!";

		return ret;
	}

	std::string Assets::GetString(std::string key)
	{
		return this->Strings[key];
	}

	void Assets::LoadFromMem(size_t size, unsigned char* filebuf)
	{
		// copy the filebuf into memory
		this->WGFSData = new unsigned char[size];
		memcpy(this->WGFSData, filebuf, size);

		// load the stuff
		LoadInternal();
	}

	void Assets::LoadFromFile(const char* fname)
	{
		FILE* pFile = fopen(fname, "rb");
		
		// read file into memory
		fseek(pFile, 0, SEEK_END);
		long sFile = ftell(pFile);
		this->WGFSData = new unsigned char[sFile];
		rewind(pFile);
		DEBUGLOG << "Reading " << sFile << " bytes...";
		fread(this->WGFSData, 1, sFile, pFile);
		fclose(pFile);

		// load the actual stuff.
		LoadInternal();
	}

	char Assets::ReadInt8()
	{
		char ret = (this->WGFSData[this->seek]);

		this->seek += sizeof(char);
		return ret;
	}

	short Assets::ReadInt16()
	{
		short ret =
			(this->WGFSData[this->seek    ]     ) |
			(this->WGFSData[this->seek + 1] << 8) ;

		this->seek += sizeof(short);
		return ret;
	}

	int Assets::ReadInt32()
	{
		int ret =
			(this->WGFSData[this->seek    ]      ) |
			(this->WGFSData[this->seek + 1] << 8 ) |
			(this->WGFSData[this->seek + 2] << 16) |
			(this->WGFSData[this->seek + 3] << 24) ;

		this->seek += sizeof(int);
		return ret;
	}

	void* Assets::GetCurDataAddr()
	{
		return (void*)(this->WGFSData + this->seek);
	}

	long Assets::SkipString()
	{
		long oldSeek = this->seek;
		this->seek += strlen((const char*)(this->WGFSData + this->seek)) + 1;
		return oldSeek;
	}

	long Assets::SkipBytes(size_t bytes)
	{
		long oldSeek = this->seek;
		this->seek += bytes;
		return oldSeek;
	}

	void Assets::LoadInternal()
	{
		DEBUGLOG << "[WGFS]: Loading file...";

		int hdr = this->ReadInt32();
		if (hdr != WGFS_HEADER) {
			DEBUGLOG << "[WGFS|ERROR]: Invalid WGFS_HEADER!";
		}

		int ver = this->ReadInt32();
		DEBUGLOG << "[WGFS]: Format version " << ver;

		int filehdr = this->ReadInt32();
		if (filehdr != FILE_HEADER) {
			DEBUGLOG << "[WGFS|ERROR]: Invalid FILE_HEADER!";
		}

		int filecap = this->ReadInt32();
		DEBUGLOG << "[WGFS]: File table contains " << filecap << " items...";

		for (int i = 0; i < filecap; i++) {
			auto f = std::make_unique<File>();

			f->name = (const char*)this->GetCurDataAddr();
			this->SkipString();
			f->size = this->ReadInt32();
			f->data = (unsigned char*)this->GetCurDataAddr();
			this->SkipBytes(f->size);

			DEBUGLOG << "Adding file " << f->name;
			this->Files.push_back(std::move(f));
		}

		int strghdr = this->ReadInt32();
		if (strghdr != STRG_HEADER) {
			DEBUGLOG << "[WGFS|ERROR]: Invalid STRG_HEADER!";
		}

		int strgcap = this->ReadInt32();
		DEBUGLOG << "[WGFS]: String table contains " << strgcap << " items...";

		for (int i = 0; i < strgcap; i++) {
			char* key = (char*)this->GetCurDataAddr();
			this->SkipString();
			char* value = (char*)this->GetCurDataAddr();
			this->SkipString();
			
			this->Strings[key] = value;

			DEBUGLOG << "[WGFS]: " << key << " | " << value;
		}

		int endhdr = this->ReadInt32();
		if (endhdr != WEND_HEADER) {
			DEBUGLOG << "[WGFS|ERROR]: Invalid WEND_HEADER!";
		}

		DEBUGLOG << "[WGFS]: File loaded!";
	}
}