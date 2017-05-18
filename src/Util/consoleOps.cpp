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

#include "consoleOps.h"
#include "conditions.h"

using namespace std;

static const string consColors[] = {
	"Black",     "Dark Blue", "Dark Green", "Dark Cyan", "Dark Red", "Dark Magenta", "Dark Yellow", "Gray",
	"Dark Gray", "Blue",      "Green",      "Cyan",      "Red",      "Magenta",      "Yellow",      "White"
};

void clearConsole() {
	system("cls");
}

void setConsoleCursorPos(int row, int column) {
	//Initialize the coordinates
	COORD coord = {static_cast<SHORT>(column), static_cast<SHORT>(row)};
	//Set the position
	mtVerify(SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord));
	return;
}

void getConsoleCursorPos(int &row, int &column) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	mtVerify(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi));
	column = csbi.dwCursorPosition.X;
	row = csbi.dwCursorPosition.Y;
}

void getConsoleTextColor(ConsoleColor &fg, ConsoleColor &bg) {
	HANDLE hstdout = nullptr;
	mtVerify(nullptr != (hstdout = GetStdHandle(STD_OUTPUT_HANDLE)));
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	mtVerify(GetConsoleScreenBufferInfo(hstdout, &csbi));
	BYTE colors = static_cast<BYTE>(csbi.wAttributes);
	fg = static_cast<ConsoleColor>(colors&0xF);
	bg = static_cast<ConsoleColor>(colors>>4);
}

void setConsoleTextColor(ConsoleColor fg, ConsoleColor bg) {
	HANDLE hstdout = nullptr;
	mtVerify(nullptr != (hstdout = GetStdHandle(STD_OUTPUT_HANDLE)));
	mtVerify(SetConsoleTextAttribute(hstdout, static_cast<WORD>((static_cast<int>(bg)<<4)|static_cast<int>(fg))));
}
