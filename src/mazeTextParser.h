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

#ifndef H_MAZE_TEXT_PARSER
#define H_MAZE_TEXT_PARSER

#include "mazeStruct.h"

/// Loads a maze from text input file
class TextMazeParser {
	unsigned &rowsCount, &columnsCount;

	Coord &startLocation;
	std::vector<Coord> &targets;

	std::vector<boost::icl::split_interval_set<unsigned>> &rows, &columns;

	void process(const std::string &fileName, bool verbose = false);

public:
	TextMazeParser(const std::string &fileName,
				   unsigned &rowsCount,
				   unsigned &columnsCount,
				   Coord &startLocation,
				   std::vector<Coord> &targets,
				   std::vector<boost::icl::split_interval_set<unsigned>> &rows,
				   std::vector<boost::icl::split_interval_set<unsigned>> &columns,
				   bool verbose = false);
};

#endif // H_MAZE_TEXT_PARSER