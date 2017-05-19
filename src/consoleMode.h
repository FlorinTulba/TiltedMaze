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

#ifndef H_CONSOLE_MODE
#define H_CONSOLE_MODE

#include "mazeStruct.h"
#include "consoleOps.h"

#pragma warning( push, 0 )

#include <set>

#pragma warning( pop )

/// Draws the maze and animates the moves in console mode
class ConsoleUiEngine : public Maze::UiEngine {
	static ConsoleColor nextColor();
	static int displayIndex(unsigned clientIdx, bool isRow); /// Helper for computing console screen coordinates

	static const ConsoleColor remainingColors[];
	static const ConsoleColor bgColor;		///< background color
	static const ConsoleColor wallColor;	///< color used for walls
	static const ConsoleColor targetColor;	///< color indicating targets
	static const char traversalCh;			///< symbol indicating visited cells
	static const char targetCh;				///< the symbol used for representing targets within the maze

	ConsoleColor origFgColor;	///< Save initial foreground color, to be able to revert it at exit
	ConsoleColor origBgColor;	///< Save initial background color, to be able to revert it at exit
	int displayOriginRow;		///< Save the initial cursor row to be able to redraw the maze from the same location
	int displayOriginCol;		///< Save the initial cursor column to be able to redraw the maze from the same location


public:
	ConsoleUiEngine(const Maze &aMaze);

	~ConsoleUiEngine();

	void drawMove(const Coord &from, const Coord &to) override;
};

#endif // H_CONSOLE_MODE