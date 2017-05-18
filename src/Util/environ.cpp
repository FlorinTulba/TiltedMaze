/******************************************************************
 Project TiltedMaze solves tilted maze problems.

 You might visit http://www.agame.com/game/tilt-maze
 to try yourself solving such problems (use the arrow keys to move)

 The program is able to load the puzzle from text files.
 
 Solving the maze is presented as a console animation.

 The project uses Boost.

 Copyright (c) 2014, 2017 Florin Tulba

*******************************************************************/

#include "environ.h"
#include "various.h"

#pragma warning( push, 0 )

#include <vector>

#pragma warning( pop )

using namespace std;

const wstring strToWstr(const string &str) {
	size_t len = str.length();
	vector<WCHAR> vectResult(len+1);

	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &vectResult[0], static_cast<int>(len)+1);

	return wstring(&vectResult[0]);
}

const string wstrToStr(const wstring &wstr) {
	size_t len = wstr.length();
	vector<char> vectResult(len+1);

	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &vectResult[0], static_cast<int>(len)+1, nullptr, nullptr);

	return string(&vectResult[0]);
}
