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

#ifndef H_MAZE_STRUCT
#define H_MAZE_STRUCT

#include "various.h"
#include "environ.h"

#pragma warning( push, 0 )

#include <string>
#include <vector>

#include <boost/icl/split_interval_set.hpp>

#pragma warning( pop )

/// Maze position
struct Coord {
	unsigned row; ///< row (y) coordinate
	unsigned col; ///< column (x) coordinate
	Coord(unsigned row = UINT_MAX, unsigned col = UINT_MAX) : row(row), col(col) {}

	void reset() { row = col = UINT_MAX; }

	inline bool operator<(const Coord& other) const {
		return (row < other.row) ||
			((row == other.row) && (col < other.col));
	}

	inline bool operator==(const Coord& other) const {
		return (row == other.row) && (col == other.col);
	}

	inline bool operator!=(const Coord& other) const {
		return !operator==(other);
	}

	std::string toString() const;

	opLessLessRef(Coord)
};

typedef std::pair<Coord, Coord> CoordsPair;

class Maze {
	std::string _name;

	unsigned _rowsCount;	///< maze height
	unsigned _columnsCount;	///< maze width

	Coord _startLocation;	///< the game requires starting from this location
	std::vector<Coord> _targets;	///< required targets to be visited

	std::vector<boost::icl::split_interval_set<unsigned>> _rows;	///< limits for the horizontal segments
	std::vector<boost::icl::split_interval_set<unsigned>> _columns;	///< limits for the vertical segments

public:
	Maze(const std::string &mazeFile, bool verbose = false);

	inline const std::string& name() const { return _name; }

	inline unsigned rowsCount() const { return _rowsCount; }
	inline unsigned columnsCount() const { return _columnsCount; }

	inline const Coord& startLocation() const { return _startLocation; }
	inline const std::vector<Coord>& targets() const { return _targets; }

	inline const std::vector<boost::icl::split_interval_set<unsigned>>& rows() const { return _rows; }
	inline const std::vector<boost::icl::split_interval_set<unsigned>>& columns() const { return _columns; }

	/// Draws the maze and animates the moves
	class UiEngine /*abstract*/ {
	protected:
		const Maze &maze;

	public:
		UiEngine(const Maze &aMaze) : maze(aMaze) {}
		virtual ~UiEngine() = 0 {}

		virtual void drawMove(const Coord &from, const Coord &to) = 0;
	};

	std::shared_ptr<UiEngine> display(bool consoleMode = true); ///< Displays the maze in console/graphic mode
};

#endif // H_MAZE_STRUCT