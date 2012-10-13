#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "freetype.lib")

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

using namespace std;

#pragma pack(push, 1)
struct GlyphHeader
{
	wchar_t code;
	unsigned char advanceX;
	signed char offsetX;
	signed char offsetY;
	unsigned char width;
	unsigned char height;
};
#pragma pack(pop)

FT_Library  library;
FT_Face     face;

vector<wchar_t> charCodes;

void main()
{
	wchar_t addCodes[] = L" !\"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~";
    for (int i = 0; i < sizeof(addCodes) / 2 - 1; i++)
		charCodes.push_back(addCodes[i]);
	for (wchar_t c = L'0'; c <= L'9'; c++)
		charCodes.push_back(c);
	for (wchar_t c = L'a'; c <= L'z'; c++)
		charCodes.push_back(c);
	for (wchar_t c = L'A'; c <= L'Z'; c++)
		charCodes.push_back(c);
	/*
	for (wchar_t c = L'à'; c <= L'ÿ'; c++)
		charCodes.push_back(c);
	for (wchar_t c = L'À'; c <= L'ß'; c++)
		charCodes.push_back(c);
	*/
	sort(charCodes.begin(), charCodes.end());

	FT_Init_FreeType(&library);

	string fontPath = string(getenv("windir")) + "\\Fonts\\OpenSans-Regular.ttf";
	FT_New_Face(library, fontPath.c_str(), 0, &face);

	FT_Set_Char_Size(face, 0, 8 * 64, 96, 96);

	ofstream fontFile;
	unsigned int signature = 'tnfv';
	unsigned char version = 1;
	short charCount = charCodes.size();
	fontFile.open("OpenSans.vfnt", ios_base::binary);
	fontFile.write((char*)&signature, 4);
	fontFile.write((char*)&version, 1);
	fontFile.write((char*)&charCount, 2);
	for (size_t i = 0; i < charCodes.size(); i++)
	{
		FT_UInt glyph_index = FT_Get_Char_Index(face, charCodes[i]);
		FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
		FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

		GlyphHeader gh;
		gh.code = charCodes[i];
		gh.advanceX = (unsigned char)(face->glyph->advance.x >> 6);
		gh.offsetX = face->glyph->bitmap_left;
		gh.offsetY = 11 - face->glyph->bitmap_top;
		gh.width = face->glyph->bitmap.width;
		gh.height = face->glyph->bitmap.rows;
		fontFile.write((char*)&gh, sizeof(GlyphHeader));
		fontFile.write((char*)face->glyph->bitmap.buffer, gh.width * gh.height);
	}
}