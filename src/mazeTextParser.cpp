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

#pragma warning( push, 0 )

#include <fstream>

#include <boost/optional.hpp>

#pragma warning( pop )

using namespace std;
using namespace boost;
using namespace boost::icl;

optional<string> nextRelevantLine(ifstream &ifs) {
	string line;
	for(;;) {
		if(!getline(ifs, line))
			return optional<string>(); // EOF or error

		// skip empty lines or comments
		if((0 == line.compare("")) || (';' == line[0]))
			continue;

		return line;
	}
}

void TextMazeParser::process(const string &fileName, bool verbose/* = false*/) {
	ifstream ifs(fileName);
	optional<string> line;

	// Reading the size of the maze
	{
		if( ! (line = nextRelevantLine(ifs)) )
			throw runtime_error("The provided maze file ended before specifying the maze size!");

		istringstream iss(*line);
		iss>>rowsCount>>columnsCount;
		if(rowsCount==0U || columnsCount==0U)
			throw out_of_range("Read invalid maze size!");

		if(verbose) {
			PRINTLN(rowsCount);
			PRINTLN(columnsCount);
		}

		unsigned i;
		rows.reserve(rowsCount);
		for(i = 0U; i<rowsCount; ++i) {
			split_interval_set<unsigned> sis;
			sis += interval<unsigned>::type(0U, columnsCount);
			rows.push_back(sis);
		}

		columns.reserve(columnsCount);
		for(i = 0U; i<columnsCount; ++i) {
			split_interval_set<unsigned> sis;
			sis += interval<unsigned>::type(0U, rowsCount);
			columns.push_back(sis);
		}
	}

	// Reading the rows & columns intervals
	for(;;) {
		string type;
		unsigned index = UINT_MAX;
		bool isRowInterval = false;
		char colonCh;
		if( ! (line = nextRelevantLine(ifs)) )
			throw domain_error("The provided maze file doesn't specify neither a start location, nor any target!");

		istringstream iss(*line);
		iss>>type;
		if((0 == type.compare("row"))) isRowInterval = true;
		else if((0 == type.compare("column"))) isRowInterval = false;
		else
			break; // no more rows, nor columns - just read the start location

		iss>>index;
		if(index >= (isRowInterval ? rowsCount : columnsCount))
			throw out_of_range("The provided maze file refers to an invalid row/column based on the specified maze dimensions!");

		iss>>colonCh;
		if(colonCh != ':')
			throw invalid_argument("Expected ':' before the walls' indexes while parsing a row/column declaration.");

		if(verbose)
	 		cout<<type<<' '<<index<<':';

		for(unsigned wallIndex = UINT_MAX;; wallIndex = UINT_MAX) {
			iss>>wallIndex;
			if(UINT_MAX == wallIndex)
				break; // read last wall index from the line

			++wallIndex; // The read value still falls within the previous interval
			if(wallIndex >= (isRowInterval ? columnsCount : rowsCount))
				throw out_of_range("The provided maze file refers to an invalid wall index given the specified maze dimensions!");

			if(verbose)
	 			cout<<' '<<wallIndex;

			if(isRowInterval) {
				rows[index] += interval<unsigned>::type(wallIndex, columnsCount);
			} else {
				columns[index] += interval<unsigned>::type(wallIndex, rowsCount);
			}
		}
		if(verbose)
	 		cout<<endl;
	}

	if(verbose) {
#pragma warning ( disable : THREAD_UNSAFE_CONSTRUCTION )
		static const auto showRanges = [] (const vector<split_interval_set<unsigned>> &rowsOrColumns) {
			for(const auto &sis : rowsOrColumns) {
				cout<<'{';
				for(const auto &limPair : sis) {
					cout<<'['<<limPair.lower()<<','<<limPair.upper()<<')';
				}
				cout<<"}, ";
			}
		};
#pragma warning ( default : THREAD_UNSAFE_CONSTRUCTION )
		cout<<"custom_delims<ContDelims<>>(rows) = "; showRanges(rows); cout<<endl;
		cout<<"custom_delims<ContDelims<>>(columns) = "; showRanges(columns); cout<<endl;
	}

	// Parsing the Starting location from the last read line
	{
		istringstream iss(*line);
		startLocation.reset();
		iss>>startLocation.row>>startLocation.col;
		if(startLocation.row>=rowsCount || startLocation.col>=columnsCount)
			throw out_of_range("The provided maze file specifies an invalid starting location given the configured maze dimensions!");

		if(verbose)
	 		cout<<"startLocation coords: "<<startLocation.row<<','<<startLocation.col<<endl;
	}

	// Reading the targets
	for(bool targetsFound = false;;) {
		if( ! (line = nextRelevantLine(ifs)) ) {
			if(false == targetsFound)
				throw domain_error("The provided maze file doesn't specify any targets!");

			break;
		}

		istringstream iss(*line);
		Coord target;
		iss>>target.row>>target.col;
		if(target.row>=rowsCount || target.col>=columnsCount)
			throw out_of_range("The provided maze file specifies an invalid target given the configured maze dimensions!");

		if(verbose)
	 		cout<<"target coords: "<<target.row<<','<<target.col<<endl;

		targetsFound = true;
		targets.push_back(target);
	}

	if(verbose)
	 	cout<<"File was correct!"<<endl;
}

TextMazeParser::TextMazeParser(const string &fileName,
				   unsigned &rowsCount,
				   unsigned &columnsCount,
				   Coord &startLocation,
				   vector<Coord> &targets,
				   vector<split_interval_set<unsigned>> &rows,
				   vector<split_interval_set<unsigned>> &columns,
				   bool verbose/* = false*/) :
		rowsCount(rowsCount), columnsCount(columnsCount), startLocation(startLocation), targets(targets), rows(rows), columns(columns) {
	process(fileName, verbose);
}
