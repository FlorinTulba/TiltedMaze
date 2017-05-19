/******************************************************************
 Project TiltedMaze solves tilted maze problems.

 You might visit http://www.agame.com/game/tilt-maze
 to try yourself solving such problems (use the arrow keys to move)

 The program is able to load the puzzle from text files, but also
 from captured snapshots, which contain various imperfections.
 It is possible to recognize the original maze even when rotating,
 mirroring the snapshot, or even after applying perspective
 transformations on it.
 
 Solving the maze is presented as an animation, either on console,
 or within a normal window.

 The project uses OpenCV and Boost.

 Copyright (c) 2014, 2017 Florin Tulba

*******************************************************************/

#ifndef H_CONSOLE_OPS
#define H_CONSOLE_OPS

#include "various.h"

#pragma region CONSOLE_OPS

/// List of the colors available within a console
enum struct ConsoleColor {
	Black, DarkBlue, DarkGreen, DarkCyan, DarkRed, DarkMagenta, DarkYellow, Gray,
	DarkGray, Blue, Green, Cyan, Red, Magenta, Yellow, White,
	CONSOLE_COLORS_COUNT
};

/// clears the console
void clearConsole();

/// gets moves the console cursor position
void getConsoleCursorPos(int &row, int &column);

/// moves the console cursor position
void setConsoleCursorPos(int row, int column);

/// sets the next fg & bg colors to be used in console
void setConsoleTextColor(ConsoleColor fg, ConsoleColor bg);

/// gets the fg & bg colors used in console
void getConsoleTextColor(ConsoleColor &fg, ConsoleColor &bg);

#pragma endregion CONSOLE_OPS

#endif // H_CONSOLE_OPS