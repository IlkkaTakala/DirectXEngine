#include "stdafx.h"
#include "SpriteFontLoader.h"

#pragma pack(1)
struct FontCommon
{
	unsigned short lineHeight;
	unsigned short base;
	unsigned short scaleW;
	unsigned short scaleH;
	unsigned short pages;
	unsigned char bitField;
	unsigned char alphaChnl;
	unsigned char redChnl;
	unsigned char greenChnl;
	unsigned char blueChnl;
};

#pragma pack(1)
struct FontChars
{
	unsigned int id;
	unsigned short x;
	unsigned short y;
	unsigned short width;
	unsigned short height;
	short xoffset;
	short yoffset;
	short xadvance;
	unsigned char page;
	unsigned char chnl;
};

SpriteFont* SpriteFontLoader::LoadContent(const ContentLoadInfo& loadInfo)
{
	const auto pReader = new BinaryReader();
	pReader->Open(loadInfo.assetFullPath);

	if (!pReader->Exists())
	{
		Logger::LogError(L"Failed to read the assetFile!\nPath: \'{}\'", loadInfo.assetSubPath);
		return nullptr;
	}

	//See BMFont Documentation for Binary Layout

	char bmf[4] = { 0 };
	bmf[0] = pReader->Read<char>();
	bmf[1] = pReader->Read<char>();
	bmf[2] = pReader->Read<char>();
	if (strcmp(bmf, "BMF") != 0) {
		Logger::LogError(L"SpriteFontLoader::LoadContent > Not a valid.fnt font");
		return nullptr;
	}

	//Parse the version (version 3 required)
	char version = pReader->Read<char>();
	if (version != 3) {
		Logger::LogError(L"SpriteFontLoader::LoadContent > Only.fnt version 3 is supported");
		return nullptr;
	}

	//Valid .fnt file >> Start Parsing!
	//use this SpriteFontDesc to store all relevant information (used to initialize a SpriteFont object)
	SpriteFontDesc fontDesc{};
	char blockId = 0;
	unsigned int size = 0;
	//**********
	// BLOCK 0 *
	//**********
	blockId = pReader->Read<char>();
	size = pReader->Read<unsigned int>();

	fontDesc.fontSize = pReader->Read<short>();
	pReader->MoveBufferPosition(12);
	fontDesc.fontName = pReader->ReadNullString();

	//**********
	// BLOCK 1 *
	//**********
	blockId = pReader->Read<char>();
	size = pReader->Read<unsigned int>();
	FontCommon common = pReader->Read<FontCommon>();
	fontDesc.textureHeight = common.scaleH;
	fontDesc.textureWidth = common.scaleW;
	if (common.pages > 1) {
		Logger::LogError(L"Only one texture per font is allowed!");
		return nullptr; 
	}

	//**********
	// BLOCK 2 *
	//**********
	blockId = pReader->Read<char>();
	size = pReader->Read<unsigned int>();
	std::wstring name = pReader->ReadNullString();
	auto path = loadInfo.assetFullPath.parent_path().append(name);
	fontDesc.pTexture = ContentManager::Load<TextureData>(path);
	
	//**********
	// BLOCK 3 *
	//**********
	blockId = pReader->Read<char>();
	size = pReader->Read<unsigned int>();
	int numChars = size / 20;
	for (int i = 0; i < numChars; ++i) {
		FontChars c = pReader->Read<FontChars>();
		FontMetric m{};

		m.character = static_cast<wchar_t>(c.id);
		m.width = c.width;
		m.height = c.height;
		m.offsetX = c.xoffset;
		m.offsetY = c.yoffset;
		m.advanceX = c.xadvance;
		m.page = static_cast<unsigned char>(c.page);

		m.channel = c.chnl & 0b0001 ? 2 : c.chnl & 0b0010 ? 1 : c.chnl & 0b0100 ? 0 : 3;
		m.texCoord.x = (float)c.x / (float)fontDesc.textureWidth;
		m.texCoord.y = (float)c.y / (float)fontDesc.textureHeight;

		fontDesc.metrics.emplace(m.character, m);
	}
	
	delete pReader;
	return new SpriteFont(fontDesc);
}

void SpriteFontLoader::Destroy(SpriteFont* objToDestroy)
{
	SafeDelete(objToDestroy);
}
