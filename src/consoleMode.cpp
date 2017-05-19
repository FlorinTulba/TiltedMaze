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

#include "consoleMode.h"
#include "problemAdapter.h"

#pragma warning( push, 0 )

#include <conio.h>
#include <iomanip>

#include <boost/icl/closed_interval.hpp>

#pragma warning( pop )

using namespace std;
using namespace boost;
using namespace boost::icl;

int ConsoleUiEngine::displayIndex(unsigned clientIdx, bool isRow) {
	++clientIdx;
	clientIdx <<= 1;
	if(false == isRow)
		clientIdx |= 1U;

	return (int)clientIdx;
}

const ConsoleColor ConsoleUiEngine::remainingColors[] = {ConsoleColor::DarkYellow, ConsoleColor::Gray, ConsoleColor::Green, ConsoleColor::Cyan, ConsoleColor::Red, ConsoleColor::Magenta};
const ConsoleColor ConsoleUiEngine::bgColor = ConsoleColor::Black, ConsoleUiEngine::wallColor = ConsoleColor::White, ConsoleUiEngine::targetColor = ConsoleColor::Yellow;
const char ConsoleUiEngine::traversalCh = '*', ConsoleUiEngine::targetCh = '$';

ConsoleColor ConsoleUiEngine::nextColor() {
	static unsigned idx = 0U, sz = sizeof(remainingColors) / sizeof(ConsoleColor);
	return remainingColors[idx++ % sz];
}

ConsoleUiEngine::ConsoleUiEngine(const Maze &aMaze) : Maze::UiEngine(aMaze) {

	clearConsole();

	getConsoleCursorPos(displayOriginRow, displayOriginCol);
	getConsoleTextColor(origFgColor, origBgColor);

#pragma warning(push)
#pragma warning( disable : TRUNCATION_CASTING )
	static char wallCh = static_cast<char>(219); // full square
#pragma warning(pop)

	const int lastRowIdx = (int)aMaze.rowsCount() - 1;
	const int rowOfTopMargin = displayIndex(0U, true) - 1;
	const int rowOfBottomMargin = displayIndex(aMaze.rowsCount(), true) - 1;
	const int colOfLeftMargin = displayIndex(0, false) - 1;
	const int displayRowSz = (((int)aMaze.columnsCount()) << 1) - 1;

	const string horizMargin(size_t(displayRowSz + 2), wallCh);
	const string horizContent(size_t(displayRowSz), ' ');
	const string horizWallPiece(3, wallCh);

	setConsoleTextColor(wallColor, bgColor);

	// display header of column indexes modulo 10
	setConsoleCursorPos(displayOriginRow, colOfLeftMargin + 1);
	for(unsigned i = 0; i < aMaze.columnsCount(); ++i)
		cout<<(i%10)<<' ';

	// top margin
	setConsoleCursorPos(rowOfTopMargin, colOfLeftMargin);
	cout<<horizMargin;

	// lateral margins preceded & followed by row indexes
	for(int i = 0, row = rowOfTopMargin + 1; i <= lastRowIdx; ++i) {
		setConsoleCursorPos(row++, displayOriginCol);
		cout<<right<<setw(2)<<i<<wallCh<<horizContent<<wallCh<<i;
		setConsoleCursorPos(row++, displayOriginCol + 2);
		cout<<wallCh<<horizContent<<wallCh;
	}

	// bottom margin
	setConsoleCursorPos(rowOfBottomMargin, colOfLeftMargin);
	cout<<horizMargin;

	// display footer of column indexes modulo 10
	setConsoleCursorPos(rowOfBottomMargin + 1, colOfLeftMargin + 1);
	for(unsigned i = 0; i < aMaze.columnsCount(); ++i)
		cout<<(i%10)<<' ';

	// handle vertical segments
	int displayCol = colOfLeftMargin;
	for(const auto &sis : aMaze.columns()) {
		for(const auto &iut : sis) {
			unsigned bottomBound = iut.upper(); // one past the upper end
			if(bottomBound < aMaze.rowsCount()) { // consider only segments not touching the bottom of the maze
				int displayRow = displayIndex(bottomBound, true) - 1;
				setConsoleCursorPos(displayRow, displayCol);
				cout<<horizWallPiece;
			}
		}
		displayCol += 2;
	}

	// handle horizontal segments
	int _displayRow = rowOfTopMargin, displayRow = _displayRow + 1, displayRow_ = displayRow + 1;
	for(const auto &sis : aMaze.rows()) {
		for(const auto &iut : sis) {
			unsigned rightBound = iut.upper(); // one past the upper end
			if(rightBound < aMaze.columnsCount()) { // consider only segments not touching the right margin of the maze
				displayCol = displayIndex(rightBound, false) - 1;
				setConsoleCursorPos(_displayRow, displayCol); cout<<wallCh;
				setConsoleCursorPos(displayRow, displayCol); cout<<wallCh;
				setConsoleCursorPos(displayRow_, displayCol); cout<<wallCh;
			}
		}
		_displayRow += 2;
		displayRow += 2;
		displayRow_ += 2;
	}

	// place the targets
	setConsoleTextColor(targetColor, bgColor);
	for(const auto &t : aMaze.targets()) {
		setConsoleCursorPos(displayIndex(t.row, true), displayIndex(t.col, false));
		cout<<targetCh;
	}

	// position the cursor at the starting location
	setConsoleCursorPos(displayIndex(aMaze.startLocation().row, true), displayIndex(aMaze.startLocation().col, false));
}

ConsoleUiEngine::~ConsoleUiEngine() {
	setConsoleTextColor(origFgColor, origBgColor);
	setConsoleCursorPos(displayIndex(maze.rowsCount(), true) + 1, 0); // after displaying the maze, set the cursor below it
}

void ConsoleUiEngine::drawMove(const Coord &from, const Coord &to) {
	Segment s(from, to);
	int ch = _getch();
	if(ch==0||ch==0xE0) _getch(); // discard any chars left in the console buffer due to pressed function keys

	ConsoleColor c = nextColor();
	setConsoleTextColor(c, bgColor);

	unsigned fi = s.fixedCoord();
	int vl, vu, theRow = 0, theCol = 0;
	Coord lCoord = s.lowerEnd(), uCoord = s.upperEnd();

	if(s.isHorizontal()) {
		theRow = displayIndex(fi, true);
		theCol = displayIndex(lCoord.col, false);
		setConsoleCursorPos(theRow, theCol);
		cout<<string(((uCoord.col-lCoord.col)<<1) + 1, traversalCh);

	} else {
		theCol = displayIndex(fi, false);
		theRow = vl = displayIndex(lCoord.row, true);
		vu = displayIndex(uCoord.row, true);
		for(; vl<=vu; ++vl) {
			setConsoleCursorPos(vl, theCol);
			cout<<traversalCh;
		}
	}

	if(s.isLowerEnd(to))
		setConsoleCursorPos(theRow, theCol);
	else
		cout<<'\b';
}
