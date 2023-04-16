// FBXConverter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <string>
#include <io.h>
#include <assert.h>
#include <stdio.h>
#include "FBX/FbxParser.h"
#include <algorithm>

int main(int argc, char** argv)
{
	std::string strFile;

	if (argc >= 2) {
		std::string input(argv[1]);
		strFile = input;
	}
	else {
		std::string input("../data/Teeths.fbx");
		strFile = input;
	}

	std::string outFile("");
	if (argc >= 3) {
		std::string out(argv[2]);
		outFile = out;
	}


	struct _finddata_t c_file;
	long hFile;
	if ((hFile = _findfirst(strFile.c_str(), &c_file)) == -1L) {
		printf("Cannot find input file %s./n", strFile);
		return -1;
	}

	std::string exstr;
	int idx = strFile.rfind('.');
	if (idx >= 0)
		exstr = strFile.substr(idx + 1);

	std::transform(exstr.begin(), exstr.end(),
		exstr.begin(), ::tolower);

	if (strcmp(exstr.c_str(), "fbx") == 0)
	{
		FbxParser* parser = new FbxParser();
		assert(parser != nullptr);

		if (parser && parser->LoadScene(strFile.c_str()))
		{
			parser->ExtractContent();

			std::string objFile;
			int len = strFile.length() - 4;
			objFile = strFile.substr(0, len) + ".obj";
			parser->ExportOBJ(objFile.c_str());
		}

		if (parser)
			delete parser;
	}
	return 0;
}

