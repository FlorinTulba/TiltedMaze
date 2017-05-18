/******************************************************************
 Project TiltedMaze solves tilted maze problems.

 You might visit http://www.agame.com/game/tilt-maze
 to try yourself solving such problems (use the arrow keys to move)

 The program is able to load the puzzle from text files.
 
 Solving the maze is presented as a console animation.

 The project uses Boost.

 Copyright (c) 2014, 2017 Florin Tulba

*******************************************************************/

#ifndef H_CONDITIONS
#define H_CONDITIONS

#include "various.h"

#pragma warning( push, 0 )

#include <crtdbg.h>
#include <fstream>

#pragma warning( pop )

/// Exception to be thrown instead of calling exit
struct FatalError : std::exception {};

/// multithreading assert
#define mtAssert(a) _ASSERTE(a)

/// multithreading verify
#define mtVerify(a) if (!(a)) { \
	printError(#a, __FILE__, __LINE__, GetLastError()); \
	throw FatalError(); \
}

/// Displays an error plus its occuring context
void printError(LPSTR causingCode, LPSTR fileName, int lineNo, DWORD errNum);

/// Provides an explanation for an exception
void excCausedBy(const char* errFile, int errLineNo, const std::string &errMsg);
// Throws FatalError if reqCond isn't met and displays a provided message
// require(mandatoryCondition, messageIfConditionFailed)
//
// The debug version provides the opportunity to perform debug at the error spot
// The delivery version displays messageIfConditionFailed and the file & line number
// where the error occurred and throws FatalError
#ifdef _DEBUG // debug version

#	define require(mandatoryCondition, messageIfConditionFailed) \
		if(!(mandatoryCondition)) \
			insertCtxt(excCausedBy, std::string(#mandatoryCondition " - ") + std::string(messageIfConditionFailed))

#else // delivery version

#	define require(mandatoryCondition, messageIfConditionFailed) \
		if(false==(mandatoryCondition)) \
			insertCtxt(excCausedBy, (messageIfConditionFailed))

#endif


/// Makes sure the file pointed by 'in' exists and can be open
void assure(const std::ifstream &in, const std::string &filename = "");


/**
Compare(v1, v2) => 1/0/-1
Sometimes we need to provide the result of the comparison of 2 integral values, especially for unsigned types.
For unsigned values it's problematic to just pass (v1-v2) instead of calling such a function, 
as negative results cannot be represented.

 v1>v2 => 1; v1==v2 => 0; v1<v2 => -1
*/
template<typename T>
	// valid only for integral T classes
	std::enable_if_t< std::is_integral< T >::value,
int>
/*int */compare(const T v1, const T v2) {
	if(v1 == v2)
		return 0;

	return (v1 > v2) ? 1 : -1;
}

#endif // H_CONDITIONS