/******************************************************************
 Project TiltedMaze solves tilted maze problems.

 You might visit http://www.agame.com/game/tilt-maze
 to try yourself solving such problems (use the arrow keys to move)

 The program is able to load the puzzle from text files.
 
 Solving the maze is presented as a console animation.

 The project uses Boost.

 Copyright (c) 2014, 2017 Florin Tulba

*******************************************************************/

#include "conditions.h"
#include "environ.h"

#pragma warning( push, 0 )

#include <cassert>

#pragma warning( pop )

using namespace std;

void printError(LPSTR causingCode, LPSTR fileName, int lineNo, DWORD errNo) {
	string errMsg = errorCodeMsg(errNo);

	ostringstream oss;
	oss<<endl
		<<"The following call failed at line "<<lineNo<<" in "<<fileName<<":"<<endl
		<<endl
		<<"\t"<<causingCode<<endl
		<<endl
		<<"Reason: "<<errMsg<<endl;

	errMsg = oss.str();

	cerr<<errMsg;
	popupMessage(errMsg, MB_ICONERROR);
}

void excCausedBy(const char* errFile, int errLineNo, const string &errMsg) {
#ifdef _DEBUG // debug version
	// It appears that the Debug version for Win32 generates warning 4365 (long->unsigned signed/unsigned mismatch)
	// Here's a solution: converting to unsigned the __LINE__ which was provided as long.
	wstring msg = strToWstr(errMsg), file = strToWstr(errFile);
	_wassert(msg.c_str(),  file.c_str(), (unsigned)errLineNo);

#else // delivery version

	fprintf(stderr, "%s(%d): %s\n", errFile, errLineNo, errMsg.c_str());
	throw FatalError();

#endif
}

void assure(const ifstream &in, const string &filename/* = ""*/) {
	if( ! in) {
		fprintf(stderr, "Could not open file %s\n", filename.c_str());
		throw FatalError();
	}
}

