/******************************************************************
 Project TiltedMaze solves tilted maze problems.

 You might visit http://www.agame.com/game/tilt-maze
 to try yourself solving such problems (use the arrow keys to move)

 The program is able to load the puzzle from text files.
 
 Solving the maze is presented as a console animation.

 The project uses Boost.

 Copyright (c) 2014, 2017 Florin Tulba

*******************************************************************/

#include "various.h"
#include "environ.h"

#pragma warning( push, 0 )

#include <iomanip>

#pragma warning( pop )

using namespace std;

const string errorCodeMsg(DWORD errCode) {
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, nullptr);

	ostringstream oss;
	oss<<"(ErrCode=0x"<<hex<<setfill('0')<<setw(6)<<errCode<<dec<<") - "<<tstrToStr((TCHAR*)lpMsgBuf);

	LocalFree(lpMsgBuf);
	return oss.str();
}

void popupMessage(const string &msg, UINT iconType/* = MB_ICONINFORMATION*/) {
	TCHAR moduleName[MAX_PATH];
	GetModuleFileName(nullptr, moduleName, MAX_PATH);
	MessageBox(nullptr, strToWstr(msg).c_str(), moduleName, iconType | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
}
