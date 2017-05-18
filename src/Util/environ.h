/******************************************************************
 Project TiltedMaze solves tilted maze problems.

 You might visit http://www.agame.com/game/tilt-maze
 to try yourself solving such problems (use the arrow keys to move)

 The program is able to load the puzzle from text files.
 
 Solving the maze is presented as a console animation.

 The project uses Boost.

 Copyright (c) 2014, 2017 Florin Tulba

*******************************************************************/

#ifndef H_ENVIRON
#define H_ENVIRON

#pragma warning( push, 0 )

#include <Windows.h>
#include <string>
#include <iostream>

#pragma warning( pop )

#pragma region PLATFORM_AND_CHAR_SET_RELATED

/// string -> wstring converter
const std::wstring strToWstr(const std::string &str);

/// wstring -> string converter
const std::string wstrToStr(const std::wstring &wstr);


#if defined(_UNICODE) || defined(UNICODE)

#	define TCOUT std::wcout
#	define TCERR std::wcerr
#	define TOSTREAM std::wostream
#	define TOSTRINGSTREAM std::wostringstream
#	define TSTRING std::wstring

/// string -> tstring converter
inline TSTRING strToTstr(const std::string &str) { return strToWstr(str); }

/// sstring -> tstring converter
inline TSTRING wstrToTstr(const std::wstring &wstr) { return wstr; }

/// tstring -> wstring converter
inline std::wstring tstrToWstr(const TSTRING &wstr) { return wstr; }

/// tstring -> string converter
inline std::string tstrToStr(const TSTRING &wstr) {return wstrToStr(wstr);}

#else // NORMAL ASCII CODES

#	define TCOUT std::cout
#	define TCERR std::cerr
#	define TOSTREAM std::ostream
#	define TOSTRINGSTREAM std::ostringstream
#	define TSTRING std::string

/// string -> tstring converter
inline TSTRING strToTstr(const std::string &str) {return str;}

/// wstring -> tstring converter
inline TSTRING wstrToTstr(const std::wstring &wstr) {return wstrToStr(wstr);}

/// tstring -> wstring converter
inline std::wstring tstrToWstr(const TSTRING &str) {return strToWstr(str);}

/// tstring -> string converter
inline std::string tstrToStr(const TSTRING &str) { return str; }

#endif

#pragma endregion PLATFORM_AND_CHAR_SET_RELATED

#endif // H_ENVIRON