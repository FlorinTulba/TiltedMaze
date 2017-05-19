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

#include "mazeTextParser.h"
#include "mazeImageParser.h"
#include "consoleMode.h"
#include "graphicalMode.h"

#pragma warning( push, 0 )

#include <boost/filesystem.hpp>

#pragma warning( pop )

using namespace std;
using namespace boost::filesystem;

string Coord::toString() const {
	ostringstream oss;
	oss<<"( "<<row<<" , "<<col<<" )";
	return oss.str();
}

Maze::Maze(const string &mazeFile, bool verbose/* = false*/) :
		_name(mazeFile), _rowsCount(0U), _columnsCount(0U), _rows(), _columns(),
		_startLocation(), _targets() {
	path mazeNameAsPath(mazeFile);
	if(false == mazeNameAsPath.has_extension())
		throw invalid_argument("The image provided as maze source has no extension!");

	string imgType(extension(mazeNameAsPath));
	if(imgType.compare(".txt") == 0)
		TextMazeParser(mazeFile, _rowsCount, _columnsCount, _startLocation, _targets, _rows, _columns, verbose);
	else {
#pragma warning ( disable: THREAD_UNSAFE_CONSTRUCTION )
		static string supportedExtensions = ".bmp.jpg.jpeg.png.tif.tiff";
#pragma warning ( default: THREAD_UNSAFE_CONSTRUCTION )
		if(string::npos == supportedExtensions.find(imgType))
			throw invalid_argument("Unsupported image type!");

		ImageMazeParser(mazeFile, _rowsCount, _columnsCount, _startLocation, _targets, _rows, _columns, verbose);
	}
}

std::shared_ptr<Maze::UiEngine> Maze::display(bool consoleMode/* = true*/) {
	if(consoleMode)
		return make_shared<ConsoleUiEngine>(*this);

	return make_shared<GraphicalUiEngine>(*this);
}
