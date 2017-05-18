/******************************************************************
 Project TiltedMaze solves tilted maze problems.

 You might visit http://www.agame.com/game/tilt-maze
 to try yourself solving such problems (use the arrow keys to move)

 The program is able to load the puzzle from text files, but also
 from captured snapshots, which contain various imperfections.
 It is possible to recognize the original maze even when rotating
 or mirroring the snapshot.
 
 Solving the maze is presented as a console animation.

 The project uses Boost, Jpeg, Png, Tiff, Zlib libraries.

 Copyright (c) 2014, 2017 Florin Tulba

*******************************************************************/

#ifndef H_VARIOUS
#define H_VARIOUS

#pragma warning( push, 0 )

#include <Windows.h>
#include <iostream>
#include <sstream>
#include <string>

#pragma warning( pop )



#pragma region TYPES_STUFF

typedef unsigned char			BYTE;

#pragma endregion TYPES_STUFF




#pragma region VARIOUS_MACROS

#define UNREFERENCED_VAR UNREFERENCED_PARAMETER

#define insertCtxt(task, ...) \
	task(__FILE__, __LINE__, __VA_ARGS__)

#define COPY_CONSTR(className) className(const className &other)
#define ASSIGN_OP(className) className& operator=(const className &other)
#define MOVE_CONSTR(className) className(className &&other)
#define MOVE_ASSIGN_OP(className) className& operator=(className &&other)

#define BOUNDS_OF(cont)				std::begin((cont)), std::end((cont))
#define CONST_BOUNDS_OF(cont)		std::cbegin((cont)), std::cend((cont))

#pragma endregion VARIOUS_MACROS



#pragma region DISPLAY_INFO

// Macro-s for displaying an expression and its value together
// The value MUST have a friend operator<< 
#define SPRINT(val, ostr) ostr<<#val " = "<<(val)
#define SPRINTLN(val, ostr) SPRINT(val, ostr)<<std::endl
#define PRINT(val) SPRINT(val, std::cout)
#define PRINTLN(val) SPRINTLN(val, std::cout)

#define WSPRINT(val, ostr) ostr<<L#val L" = "<<(val)
#define WSPRINTLN(val, ostr) WSPRINT(val, ostr)<<std::endl
#define WPRINT(val) WSPRINT(val, std::wcout)
#define WPRINTLN(val) WSPRINTLN(val, std::wcout)

#define TSPRINT(val, ostr) \
	ostr<<_T(#val) _T(" = ") << \
		strToTstr( [&] { \
						std::ostringstream oss; \
						oss<<(val); \
						return oss.str(); \
					} () \
				)
#define TSPRINTLN(val, ostr) TSPRINT(val, ostr)<<std::endl
#define TPRINT(val) TSPRINT(val, TCOUT)
#define TPRINTLN(val) TSPRINTLN(val, TCOUT)


// Macro for declaring & defining the [friend ostream& operator<<(ostream&, const Class &)]
// when Class defines a [virtual string toString() const]
#define opLessLessRef(Class) \
	friend std::ostream& operator<<(std::ostream &os, const Class &obj) { \
		os<<obj.toString(); \
		return os; \
	} \
	\
	friend std::wostream& operator<<(std::wostream &os, const Class &obj) { \
		os<<strToWstr(obj.toString()); \
		return os; \
	}

// Macro for declaring & defining the [friend ostream& operator<<(ostream&, const Class *)]
// when Class defines a [virtual string toString() const]
#define opLessLessPtr(Class) \
	friend std::ostream& operator<<(std::ostream &os, const Class *pObj) { \
		if(pObj!=nullptr) os<<pObj->toString(); \
			else os<<"nullptr " #Class; \
			return os; \
	} \
	\
	friend std::wostream& operator<<(std::wostream &os, const Class *pObj) { \
		if(pObj!=nullptr) os<<strToWstr(pObj->toString()); \
			else os<<L"nullptr " L#Class; \
			return os; \
	}

/// @return the meaning of an error code returned by GetLastError()
const std::string errorCodeMsg(DWORD errCode);

/// Displays a MessageBox
/// @param iconType might be one of: MB_ICONINFORMATION, MB_ICONWARNING, MB_ICONERROR
void popupMessage(const std::string &msg, UINT iconType = MB_ICONINFORMATION);

#pragma endregion DISPLAY_INFO

#endif // H_VARIOUS