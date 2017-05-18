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

#include "tiltedMaze.h"

#pragma warning ( push, 0 )

#include <tchar.h>
#include <conio.h>
#include <iomanip>
#include <strstream>
#include <queue>
#include <algorithm>

#include <boost/filesystem/convenience.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/assign/std/map.hpp>
#include <boost/assign/std/list.hpp>
#include <boost/assign/std/set.hpp>
#include <boost/icl/closed_interval.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include <boost/gil/extension/io/jpeg_all.hpp>
#include <boost/gil/extension/io/png_all.hpp>
#include <boost/gil/extension/io/bmp_all.hpp>
#include <boost/gil/extension/io/tiff_all.hpp>

#pragma warning ( pop )

using namespace std;
using namespace boost;
using namespace boost::icl;
using namespace boost::assign;
using namespace boost::gil;
using namespace boost::filesystem;

namespace {
	enum { TEST_MAZES_COUNT = 11 };

	set<ConsoleColor> remainingColors;
	ConsoleColor bgColor;
	bool solveGraphically = true;
	char traversalCh = '*';

	inline ConsoleColor nextColor() {
#pragma warning ( disable: THREAD_UNSAFE_CONSTRUCTION )
		static set<ConsoleColor>::iterator it = remainingColors.begin(), itEnd = remainingColors.end();
#pragma warning ( default: THREAD_UNSAFE_CONSTRUCTION )
		if(it == itEnd)
			it = remainingColors.begin();

		return *it++;
	}

	void displaySeg(const TiltedMaze::Segment &s, const TiltedMaze::Coord &towardsEnd) {
		int ch = _getch();
		if(ch==0||ch==0xE0) _getch(); // discard any chars left in the console buffer due to pressed function keys

		ConsoleColor c = nextColor();
		setConsoleTextColor(c, bgColor);

		unsigned fi = s.fixedCoord();
		int vl, vu, theRow = 0, theCol = 0;
		TiltedMaze::Coord lCoord = s.lowerEnd(), uCoord = s.upperEnd();

		if(s.isHorizontal()) {
			theRow = TiltedMaze::displayIndex(fi, true);
			theCol = TiltedMaze::displayIndex(lCoord.col, false);
			setConsoleCursorPos(theRow, theCol);
			cout<<string(((uCoord.col-lCoord.col)<<1) + 1, traversalCh);
		
		} else {
			theCol = TiltedMaze::displayIndex(fi, false);
			theRow = vl = TiltedMaze::displayIndex(lCoord.row, true);
			vu = TiltedMaze::displayIndex(uCoord.row, true);
			for(; vl<=vu; ++vl) {
				setConsoleCursorPos(vl, theCol);
				cout<<traversalCh;
			}
		}

		if(s.isLowerEnd(towardsEnd))
			setConsoleCursorPos(theRow, theCol);
		else
			cout<<'\b';
	}

	void pressKeyToContinue(ostream &os) {
		os<<"Press a key to continue ...";
		int ch = _getch();
		if(ch==0||ch==0xE0) _getch(); // discard any chars left in the console buffer due to pressed function keys
		os<<endl;
	}

	/// Verifying all existing test files
	bool testsOk() {
		bool ok = true;
		cout<<"Ensuring correct parsing & solving of all test files ..."<<endl<<endl;
		const path resFolder("res");
		const vector<const string> knownPrefixes { "maze", "rot_maze" };
		vector<string> knownSuffixes((size_t)TEST_MAZES_COUNT);
		for(int suffix = 1; suffix <= TEST_MAZES_COUNT; ++suffix)
			knownSuffixes[(size_t)(suffix - 1)] = to_string(suffix);
		const vector<const string> knownExtensions { ".bmp", ".jpg", ".jpeg", ".png", ".tif", ".tiff", ".txt" };

		path mazePrefix, mazePathNoExt, mazePath;

		for(const auto &prefix : knownPrefixes) {
			mazePrefix = path(resFolder).append(prefix);
			for(const auto &suffix : knownSuffixes) {
				mazePathNoExt = path(mazePrefix).concat(suffix);
				for(const auto &extension : knownExtensions) {
					mazePath = path(mazePathNoExt).concat(extension);
					if(exists(mazePath)) {
						try {
							TiltedMaze tm(mazePath.string());
							if(!tm.solve(false)) {
								cerr<<"Maze "<<mazePath<<" couldn't be solved!"<<endl;
								ok = false;
							}
						} catch(std::exception &e) {
							cerr<<"There were problems parsing "<<mazePath<<" : "<<endl<<'\t'<<e.what()<<endl<<endl;
							ok = false;
						}
					}
				}
			}
		}

		return ok;
	}
}

TiltedMaze::Targets graphTargets;
rgb8_pixel_t TiltedMaze::ImageTiltedMazeReader::greenPixel(0, 255, 0);

string TiltedMaze::Coord::toString() const {
	ostringstream oss;
	oss<<"( "<<row<<" , "<<col<<" )";
	return oss.str();
}

void TiltedMaze::Target::visit() {
	_visited = true;

	if(visitors.first != NULL)
		visitors.first->ackVisit(*this);

	if(visitors.second != NULL)
		visitors.second->ackVisit(*this);
}

TiltedMaze::Segment::Segment(const Coord &coord1, const Coord &coord2) : parent(NULL) {
	unsigned nfi1 = coord1.row, fi1 = coord1.col, nfi2 = coord2.row, fi2 = coord2.col;
	_isHorizontal = (nfi1 == nfi2);
	require(_isHorizontal || (fi1 == fi2), "Provided coords don't express an horizontal / vertical segment!");
	if(_isHorizontal) {
		swap(fi1, nfi1);
		swap(fi2, nfi2);
	}
	fixedIndex = fi1;
	UUpair pairMinMax;
	pairMinMax = minmax(nfi1, nfi2);
	closedInterval = closed_interval<unsigned>(pairMinMax.first, pairMinMax.second);
}

TiltedMaze::Segment::Segment(unsigned fixedIndex, const interval<unsigned>::type &rightOpenInterval, bool isHorizontal/* = true*/) :
		_isHorizontal(isHorizontal), fixedIndex(fixedIndex), parent(NULL),
		closedInterval(closed_interval<unsigned>(rightOpenInterval.lower(), rightOpenInterval.upper() - 1U)) {
}

bool TiltedMaze::Segment::containsCoord(unsigned row, unsigned col, bool exceptEnds/* = false*/) const {
	unsigned fi = col, nfi = row;
	if(_isHorizontal)
		swap(fi, nfi);

	if(fi != fixedIndex)
		return false;

	if(exceptEnds)
		return contains(open_interval<unsigned>(closedInterval.lower(), closedInterval.upper()), nfi);
					
	return  contains(closedInterval, nfi);
}

TiltedMaze::Segment::RangeUnvisited TiltedMaze::Segment::unvisitedTargets(const Coord *from /* = NULL*/, const Coord *end /* = NULL*/) const {
	UTargetMap::const_iterator itEnd = varDimUnvisitedTargets.end();
	if(varDimUnvisitedTargets.empty())
		return make_pair(itEnd, itEnd);

	// varDimUnvisitedTargets is here non-empty

	bool limitsProvided = (from != NULL);
	require(((end != NULL) == limitsProvided), "Either both parameter or none must be NULL!");

	if(false == limitsProvided)
		return make_pair(varDimUnvisitedTargets.begin(), itEnd);

	// limitsProvided is here true

	bool isEndTheLowerEnd = isLowerEnd(*end);
	UNREFERENCED_VAR(isEndTheLowerEnd);
	require(containsCoord(*from), "From doesn't belong to this segment!");


	const unsigned *varFrom = NULL;
	const unsigned *varEnd = NULL;
	if(_isHorizontal) {
		varFrom = &from->col;
		varEnd = &end->col;
	} else {
		varFrom = &from->row;
		varEnd = &end->row;
	}

	UUpair limitsMinMax;
	limitsMinMax = minmax(*varFrom, *varEnd);
	UTargetMap::const_iterator
		itLimMin = varDimUnvisitedTargets.lower_bound(limitsMinMax.first),
		itLimMax = varDimUnvisitedTargets.upper_bound(limitsMinMax.second);

	return make_pair(itLimMin, itLimMax);
}

TiltedMaze::Coord TiltedMaze::Segment::nextToEnd(const Coord &end) const {
	bool is1stEnd = isLowerEnd(end);
	unsigned varDim = (is1stEnd ? (closedInterval.lower() + 1U) : (closedInterval.upper() - 1U));
	if(_isHorizontal)
		return Coord(fixedIndex, varDim);

	return Coord(varDim, fixedIndex);
}

bool TiltedMaze::Segment::hasUnvisitedTargets(const Coord *from/* = NULL*/, const Coord *end/* = NULL*/) const {
	RangeUnvisited rangeUnvisited = unvisitedTargets(from, end);
	return distance(rangeUnvisited.first, rangeUnvisited.second) > 0;
}

set<TiltedMaze::Target*> TiltedMaze::Segment::getUnvisitedTargets(const Coord *from/* = NULL*/, const Coord *end/* = NULL*/) const {
	set<Target*> result;
	RangeUnvisited rangeUnvisited = unvisitedTargets(from, end);
	UTargetMap::const_iterator it = rangeUnvisited.first, itEnd = rangeUnvisited.second;
	for(; it != itEnd; ++it) {
		result.insert(it->second);
	}
	return result;
}

void TiltedMaze::Segment::manageTarget(Target &target) {
	require(containsCoord(target), "Provided Target can't be on this segment!");
	managedTargets.push_back( & target);
	unsigned varDim = (_isHorizontal ? target.col : target.row);
	varDimUnvisitedTargets[varDim] = & target;
}

void TiltedMaze::Segment::ackVisit(const Target &target) {
	require(containsCoord(target), "Provided Target can't be on this segment!");
	const unsigned varDim = (_isHorizontal ? target.col : target.row);
	require(1U == varDimUnvisitedTargets.erase(varDim), "No such target was found!");
}

void TiltedMaze::Segment::traverse(const Coord *from/* = NULL*/, const Coord *end/* = NULL*/) {
	RangeUnvisited rangeUnvisited = unvisitedTargets(from, end);
	while(rangeUnvisited.first != rangeUnvisited.second) {
		Target *target = rangeUnvisited.first->second;
		require(NULL != target, "Found NULL target!");
		require(false == target->visited(), "This target should have been not visited yet");
		++rangeUnvisited.first; // to escape the invalidation triggered by next call
		target->visit();
		if(false == solveGraphically)
			cout<<" $ ";
	}
}

string TiltedMaze::Segment::toString() const {
	ostringstream oss;
	oss<<"[ ";
	if(_isHorizontal)
		oss<<fixedIndex<<","<<closedInterval.lower()<<" - "<<fixedIndex<<","<<closedInterval.upper();
	else
		oss<<closedInterval.lower()<<","<<fixedIndex<<" - "<<closedInterval.upper()<<","<<fixedIndex;
	oss<<" ]";
	return oss.str();
}

void TiltedMaze::BranchlessPath::expand(const Coord &anEnd, bool horizDir, bool afterSeed/* = true*/) {
	PSegmentsPair hvSegments = theMaze.coordOwners[anEnd];
	Segment *seg = (horizDir ? hvSegments.first : hvSegments.second);
	if(NULL == seg)
		return;

	void (list<Segment*>::*listInsertMethod)(Segment * const &);
	Coord *endToUpdate = NULL;
	Segment **linkToUpdate = NULL;
	if(afterSeed) {
		endToUpdate = & ends.second;
		linkToUpdate = & links.second;
		listInsertMethod = &list<Segment*>::push_back;
	} else {
		endToUpdate = & ends.first;
		linkToUpdate = & links.first;
		listInsertMethod = &list<Segment*>::push_front;
	}

	// anEnd belongs for sure to *seg
	if(false == seg->containsCoord(anEnd, true)) { // and one of its ends meets the provided anEnd
		if((theMaze.orphanSegments.find(seg) != theMaze.orphanSegments.end())) {
			// we found a new child:
			seg->setOwner(this);
			theMaze.orphanSegments.erase(seg);
			(children.*listInsertMethod)(seg);
			*endToUpdate = seg->otherEnd(anEnd);
			expand(*endToUpdate, ! horizDir, afterSeed);
		}
	} else { // anEnd falls within seg and doesn't meet its ends
		*linkToUpdate = seg;
	}
}

TiltedMaze::BranchlessPath::LSI TiltedMaze::BranchlessPath::whichSegment(const Segment &seg) const {
	require(seg.owner() == this, "The provided segment doesn't belong to this branchlessPath!");
	LSI itEnd = children.end(), it = children.begin();
	for(; it != itEnd; ++it)
		if(*it == &seg)
			return it;

	return itEnd; // shouldn't be reachable
}

TiltedMaze::BranchlessPath::BranchlessPath(unsigned id, Segment &firstChild, TiltedMaze &maze) : theMaze(maze), _id(id) {
	require(firstChild.hasOwner() == false, "Cannot create an BranchlessPath using a Segment that already belongs to an BranchlessPath!");
	firstChild.setOwner(this);
	ends = firstChild.ends();
	theMaze.orphanSegments.erase(&firstChild);
	children.push_back(&firstChild);
	bool is1stChildHorizontal = firstChild.isHorizontal();
	expand(ends.first, ! is1stChildHorizontal, false);
	expand(ends.second, ! is1stChildHorizontal);
}

optional<TiltedMaze::BranchlessPath::LSI> TiltedMaze::BranchlessPath::lastUnvisited(const Coord *fromCoord/* = NULL*/, const Coord *end/* = NULL*/) const {
	bool nonNullFrom = (NULL != fromCoord);
	require((end != NULL) == nonNullFrom, "fromCoord and end should be both either NULL or valid pointers!");
	if(false == nonNullFrom) {
		fromCoord = & ends.first;
		end = & ends.second;
	}

	bool secondEndAsEnd = ! isFirstEnd(*end);
			
	LSI itFrom = locateCoord(*fromCoord),
		itEnd = (secondEndAsEnd ? (--children.end()) : (children.begin()));
	Segment *seg = *itEnd;
	Coord segBegin = segEndWithinBranchlessPath(itEnd, secondEndAsEnd),
		segCoordNext2Begin,
		segEnd = seg->otherEnd(segBegin);

	// itEnd is the one that moves
	for(; itFrom != itEnd;
							segEnd = segBegin,
							updateLSI(itEnd, secondEndAsEnd),
							seg = *itEnd,
							segBegin = seg->otherEnd(segEnd)) {

		segCoordNext2Begin = seg->nextToEnd(segBegin);
		if(seg->hasUnvisitedTargets(&segCoordNext2Begin, &segEnd))
			return itEnd;
	}

	if(seg->hasUnvisitedTargets(fromCoord, &segEnd))
		return itEnd;

	return optional<LSI>();
}

TiltedMaze::Segment* TiltedMaze::BranchlessPath::containsCoord(const Coord &coord) const {
	PSegmentsPair hvSegments = theMaze.coordOwners[coord];
	Segment *hSeg = hvSegments.first, *vSeg = hvSegments.second;
	bool hFound = (NULL != hSeg), vFound = (NULL != vSeg);
	require(hFound || vFound, "At least one segment should cover each Coord!");
	if(hFound && containsSegment(*hSeg))
		return hSeg;

	if(vFound && containsSegment(*vSeg))
		return vSeg;

	return NULL;
}

TiltedMaze::BranchlessPath::LSI TiltedMaze::BranchlessPath::locateCoord(const Coord &coord) const {
	Segment *seg = containsCoord(coord);
	require(NULL != seg, "Coord must belong to this branchlessPath!");
	return whichSegment(*seg);
}

TiltedMaze::Coord TiltedMaze::BranchlessPath::segEndWithinBranchlessPath(LSI it, bool towardsLowerPartOfBranchlessPath) const {
	if((it == children.cbegin()) && towardsLowerPartOfBranchlessPath)
		return firstEnd();

	if((it == --children.cend()) && (false == towardsLowerPartOfBranchlessPath))
		return secondEnd();

	Segment *neighbour = NULL, *self = *it;
	updateLSI(it, towardsLowerPartOfBranchlessPath);
	neighbour = *it;

	Coord lowerEnd = self->lowerEnd();

	return (neighbour->containsCoord(lowerEnd) ? lowerEnd : self->upperEnd());
}

set<TiltedMaze::Target*> TiltedMaze::BranchlessPath::getUnvisitedTargets(const Coord *fromCoord/* = NULL*/, const Coord *end/* = NULL*/) const {
	bool nonNullFrom = (NULL != fromCoord);
	require((end != NULL) == nonNullFrom, "From and end should be both either NULL or valid pointers!");

	if(false == nonNullFrom) {
		fromCoord = &ends.first;
		end = &ends.second;
	}

	bool firstEndAsEnd = isFirstEnd(*end);

	if(firstEndAsEnd) {
		swap(fromCoord, end);
	}

	set<Target*> result;
	Coord segEnd;
	LSI it = locateCoord(*fromCoord), itEnd = locateCoord(*end);

	if(it == itEnd) { // if the path of interest contains only one segment
		segEnd = segEndWithinBranchlessPath(it, firstEndAsEnd);
		result = (*it)->getUnvisitedTargets(fromCoord, &segEnd);

		return result;
	}

	set<Target*> segTargets;
	segEnd = segEndWithinBranchlessPath(it, false);
	segTargets = (*it)->getUnvisitedTargets(fromCoord, &segEnd);
	result.insert(BOUNDS_OF(segTargets));

	for(++it; it != itEnd; ++it) {
		segTargets = (*it)->getUnvisitedTargets();
		result.insert(BOUNDS_OF(segTargets));
	}

	segEnd = segEndWithinBranchlessPath(itEnd, true);
	segTargets = (*it)->getUnvisitedTargets(end, &segEnd);
	result.insert(BOUNDS_OF(segTargets));

	return result;
}

void TiltedMaze::BranchlessPath::updateLSI(LSI &it, bool towardsLowerPartOfBranchlessPath) const {
	if(towardsLowerPartOfBranchlessPath)
		--it;
	else
		++it;
}

void TiltedMaze::BranchlessPath::setLinksOwners() {
	bool has1stLinkSeg = (links.first != NULL);
	bool has2ndLinkSeg = (links.second != NULL);

	linksOwners.reserve((has1stLinkSeg && has2ndLinkSeg) ? 2ULL : 1ULL);

	if(has1stLinkSeg)
		linksOwners.push_back(links.first->owner());

	if(has2ndLinkSeg)
		linksOwners.push_back(links.second->owner());
}

void TiltedMaze::BranchlessPath::traverse(const Coord &from, LSI itFrom, LSI itTo, bool towardsLowerPartOfBranchlessPath) {
	Segment *seg = *itFrom;
	Coord segEnd = segEndWithinBranchlessPath(itFrom, towardsLowerPartOfBranchlessPath);
	if(from != segEnd) {
		seg->traverse(&from, &segEnd);
		if(solveGraphically)
			displaySeg(Segment(from, segEnd), segEnd);
		else
			cout<<segEnd;
	}

	if(itFrom != itTo) {
		for(updateLSI(itFrom, towardsLowerPartOfBranchlessPath);
				itTo != itFrom;
				updateLSI(itFrom, towardsLowerPartOfBranchlessPath)) {
			seg = *itFrom;
			seg->traverse();
			segEnd = seg->otherEnd(segEnd);
			if(solveGraphically)
				displaySeg(*seg, segEnd);
			else
				cout<<segEnd;
		}

		seg = *itTo;
		seg->traverse();
		segEnd = seg->otherEnd(segEnd);
		if(solveGraphically)
			displaySeg(*seg, segEnd);
		else
			cout<<segEnd;
	}
}

void TiltedMaze::BranchlessPath::traverse(const Coord &from, const Coord &end,
										  bool visitAllTargets/* = false*/,
										  bool stopAfterLastTarget/* = false*/) {

	if(false == solveGraphically) {
// 				cout<<endl;
// 				cout<<"Traversing BranchlessPath "<<_id<<" between "<<from<<" -> "<<end<<boolalpha<<"  (";
// 				PRINT(visitAllTargets);
// 				cout<<" ; ";
// 				PRINT(stopAfterLastTarget);
// 				cout<<") :"<<endl;
		cout<<from; // the start coord is expected to be displayed before calling traverse
	}

	Coord start = from, finish = end, theOtherEnd = otherEnd(finish);
	LSI itStart = locateCoord(from), itFinish = locateCoord(finish);
	optional<LSI> oItLastUnvisitedEnd, oItLastUnvisitedOtherEnd;
	bool isEndThe1stEnd = isFirstEnd(finish);

	if(visitAllTargets)
		oItLastUnvisitedOtherEnd = lastUnvisited(&start, &theOtherEnd);

	if(stopAfterLastTarget)
		oItLastUnvisitedEnd = lastUnvisited(&start, &finish);

	if(stopAfterLastTarget && visitAllTargets) { // lastTraversal - provided end might be suboptimal
		// traversedSegments = 2 * distanceToFarthestTargetOutOfTheWay + distanceToFarthestTargetOnTheWay
		ptrdiff_t distanceToFarthestTargetOutOfTheWay, distanceToFarthestTargetOnTheWay;

		if(isEndThe1stEnd) {
			distanceToFarthestTargetOutOfTheWay = (oItLastUnvisitedOtherEnd) ?
				(std::distance(itStart, *oItLastUnvisitedOtherEnd) + 1) : 0;

			distanceToFarthestTargetOnTheWay = (oItLastUnvisitedEnd) ?
				(std::distance(*oItLastUnvisitedEnd, itStart) + 1) : 0;

		} else {
			distanceToFarthestTargetOutOfTheWay = (oItLastUnvisitedOtherEnd) ?
				(std::distance(*oItLastUnvisitedOtherEnd, itStart) + 1) : 0;

			distanceToFarthestTargetOnTheWay = (oItLastUnvisitedEnd) ?
				(std::distance(itStart, *oItLastUnvisitedEnd) + 1) : 0;
		}

		// Optimal choice is to choose the end towards the farthest target
		if(distanceToFarthestTargetOutOfTheWay > distanceToFarthestTargetOnTheWay) {
			swap(finish, theOtherEnd);
			swap(oItLastUnvisitedEnd, oItLastUnvisitedOtherEnd);
			isEndThe1stEnd = ! isEndThe1stEnd;
		}
	}

	if(visitAllTargets && oItLastUnvisitedOtherEnd) {
		traverse(from, itStart, *oItLastUnvisitedOtherEnd, ! isEndThe1stEnd);

		itStart = *oItLastUnvisitedOtherEnd;
		start = segEndWithinBranchlessPath(itStart, ! isEndThe1stEnd);
	}

	if(stopAfterLastTarget && oItLastUnvisitedEnd) {
		itFinish = *oItLastUnvisitedEnd;
		finish = segEndWithinBranchlessPath(itFinish, isEndThe1stEnd);
	}

	traverse(start, itStart, itFinish, isEndThe1stEnd);

	if(false == solveGraphically)
		cout<<endl;
}

string TiltedMaze::BranchlessPath::toString() const {
	ostringstream oss;
	oss<<"BranchlessPath "<<_id;
	set<Target*> unvisitedTargets = getUnvisitedTargets();
	if(unvisitedTargets.empty())
		oss<<" (that has no unvisited targets)";
	else {
		oss<<" (which contains "<<unvisitedTargets.size()<<" targets not visited yet: ";
		for(const Target *t : unvisitedTargets) {
			oss<<(Coord)*t<<" ; ";
		}
		oss<<"\b\b)";
	}
	oss<<" :"<<endl
		<<"		from "<<ends.first;
	if(links.first != NULL)
		oss<<" where it meets BranchlessPath "<<links.first->owner()->id();
	oss<<endl
		<<"		to "<<ends.second;
	if(links.second != NULL)
		oss<<" where it meets BranchlessPath "<<links.second->owner()->id();
	oss<<endl
		<<string(50, '-')<<endl;

	oss<<'{';
	for(const Segment *seg : children) {
		oss<<' '<<*seg<<" ;";
	}
	oss<<"\b}"<<endl;

	return oss.str();
}

TiltedMaze::ImageTiltedMazeReader
	::FeasibleMazeConfig::FeasibleMazeConfig(const rgb8_view_t &theInnerMazeView,
											ptrdiff_t theWallWidth,
											ptrdiff_t feasibleMazeSz ) :
		innerMazeView(theInnerMazeView), wallWidth(theWallWidth),
		mazeSz(feasibleMazeSz), _foundWallsCount(0U) {
	require(mazeSz > 1, "The maze should be at least 2x2!");

	hFoundWalls.resize((size_t)mazeSz);
	vFoundWalls.resize((size_t)mazeSz);

	rowCenters.resize((size_t)mazeSz);
	idealWallPositions.resize(size_t(mazeSz - 1LL));

	// compute rowCenters & idealWallPositions:
	ptrdiff_t handyAvailSpace = innerMazeView.width() + wallWidth;
	ptrdiff_t x = rowCenters[0] = handyAvailSpace - mazeSz * wallWidth, xNext;
	handyAvailSpace <<= 1;
	double mazeSzDoubled = (double)(mazeSz << 1);

	for(ptrdiff_t i=1; i<mazeSz; ++i) {
		x = rowCenters[(size_t)i] = x + handyAvailSpace;
	}

	x = rowCenters[0ULL] = (ptrdiff_t)boost::math::llround( rowCenters[0ULL] / mazeSzDoubled );
	for(ptrdiff_t i=1; i<mazeSz; ++i) {
		xNext = rowCenters[(size_t)i] = (ptrdiff_t)boost::math::llround(rowCenters[(size_t)i] / mazeSzDoubled);
		idealWallPositions[size_t(i-1LL)] = (x + xNext + 1) >> 1; // rounded arithmetic mean
		x = xNext;
	}

	// Examine the maze:
	detectWallsOnRows(innerMazeView, vFoundWalls); // traverse each row detecting vertical walls

	// use detectWallsOnRows on the rotated clockwise view of the flipped up-down view of the innerMazeView
	detectWallsOnRows( rotated90cw_view( flipped_up_down_view( innerMazeView )),
		hFoundWalls); // traverses each original column detecting horizontal walls
}

#pragma warning ( disable : CONST_TEST_CONDITION )
TiltedMaze::ImageTiltedMazeReader::ImageTiltedMazeReader(const string &theMazeName,
														 const string &outDir/* = ""*/,
														 bool verbose/* = false*/) :
				mazeName(theMazeName),
				monitorOn(outDir.length() > 0),
				mazeOutName((outDir.length()==0) ? "" : (outDir + "/processedMaze.bmp")), verbose(verbose) {
		string imgType(extension(path(mazeName)));

		if(imgType.compare(".bmp") == 0) {
			read_image(mazeName.c_str(), img, bmp_tag());

		} else if(imgType.compare(".png") == 0) {
			read_image(mazeName.c_str(), img, png_tag());

		} else if(imgType.compare(".jpg") == 0 || imgType.compare(".jpeg") == 0) {
			read_image(mazeName.c_str(), img, jpeg_tag());

		} else if(imgType.compare(".tif") == 0 || imgType.compare(".tiff") == 0) {
			read_image(mazeName.c_str(), img, tiff_tag());

		} else {
			require(false, "Unsupported image type!");
		}

		imgView = view(img);

		if(monitorOn) {
			imgOut = img;
			imgOutView = view(imgOut);
		}

		examineMazeBorder();

		// LOOKING 1ST FOR THE CIRCLE, AS IT'S QUITE LARGE AND PROVIDES A HINT ABOUT THE MAZE SIZE
		list<ptrdiff_t> feasibleMazeSizes = circleProcessing();

		if(verbose)
			cout<<endl;

		unsigned maxWallsFound = 0U;
		for(ptrdiff_t feasibleMazeSz : feasibleMazeSizes) {
			std::shared_ptr<FeasibleMazeConfig> cfg =
				std::make_shared<FeasibleMazeConfig>(innerMazeView, wallWidth, feasibleMazeSz);

			unsigned foundWalls = cfg->foundWallsCount();
			if(verbose)
				cout<<"The configuration considering the maze size as "<<cfg->mazeSz<<" found "<<foundWalls<<" walls!"<<endl;

			if(foundWalls > maxWallsFound) {
				maxWallsFound = foundWalls;
				mostLikelyMazeConfig = cfg;
			}
		}
		if(verbose)
			cout<<endl<<"Deduced that the maze size is "<<mostLikelyMazeConfig->mazeSz<<endl;

		if(monitorOn)
			mostLikelyMazeConfig->displayFoundWalls(innerMazeOutView); // display the identified walls of mostLikelyMazeConfig

		// update the circle center to denote which column (x), row (y) within the maze the circle resides
		double handyCellSz = (innerMazeView.width() + wallWidth) / (double)mostLikelyMazeConfig->mazeSz;
		circleCenter.x = (ptrdiff_t)boost::math::lltrunc((circleCenter.x + wallWidth_2) / handyCellSz);
		circleCenter.y = (ptrdiff_t)boost::math::lltrunc((circleCenter.y + wallWidth_2) / handyCellSz);

		// TACKLING THE BLUE SQUARES:
		squaresProcessing();
		if(verbose)
			cout<<endl<<squares.size()<<" squares were found."<<endl;

		if(monitorOn) {
			// SHOW THE IDENTIFIED ENTITIES:
			write_view(mazeOutName.c_str(), imgOutView, bmp_tag());
			std::system(mazeOutName.c_str());
		}
}
#pragma warning ( default : CONST_TEST_CONDITION )

void TiltedMaze::ImageTiltedMazeReader::squaresProcessing() {
	for(ptrdiff_t i=0; i<mostLikelyMazeConfig->mazeSz; ++i) {
		ptrdiff_t r = mostLikelyMazeConfig->getRowCenters()[(size_t)i];
		rgb8_view_t::x_iterator 
			rowBegin = innerMazeView.row_begin(r),
			rowBeginOut = innerMazeOutView.row_begin(r);

		for(ptrdiff_t j=0; j<mostLikelyMazeConfig->mazeSz; ++j) {
			ptrdiff_t c = mostLikelyMazeConfig->getRowCenters()[(size_t)j];
			rgb8_view_t::x_iterator
				cellCenter = rowBegin + c,
				cellCenterOut = rowBeginOut + c;

			if(isFit(*(cellCenter), BLUE_CHECK)) {
				squares.emplace_back(point_t(j, i));

				if(monitorOn)
					*cellCenterOut = greenPixel;
			}
		}
	}
}

list<ptrdiff_t> TiltedMaze::ImageTiltedMazeReader::circleProcessing() {
	pair<point_t, point_t> enclosedCircle = encloseCircle();
	ptrdiff_t circleDiam = enclosedCircle.second.x - enclosedCircle.first.x;
	circleCenter = rectCenter(enclosedCircle.first, enclosedCircle.second);

	if(monitorOn)
		drawRect(innerMazeOutView, enclosedCircle.first, enclosedCircle.second);

	/*
	BASED ON THE CIRCLE DIAMETER WE ESTIMATE THE MAZE ROWS/COLUMNS COUNT (n) AS A RANGE:
	innerAvailSpace = n * innerCellSz + (n-1) * wallWidth =>
	innerCellSz + wallWidth = (innerAvailSpace + wallWidth) / n =>
	n = (innerAvailSpace + wallWidth) / (innerCellSz + wallWidth)

	Empirically, innerCellSz may be between [circleDiam+4, 2*circleDiam] =>

	n falls within:
	min: (innerAvailSpace + wallWidth) / (2*circleDiam + wallWidth) - 1st int value larger or equal to it
	max: (innerAvailSpace + wallWidth) / (circleDiam+4 + wallWidth) - last int value smaller or equal to it
	*/
	ptrdiff_t innerAvailSpace = ( std::min )(innerMazeView.width(), innerMazeView.height());
	ptrdiff_t handyAvailSpace = innerAvailSpace + wallWidth, temp = (circleDiam << 1) + wallWidth;
	ptrdiff_t nMax = handyAvailSpace / (circleDiam+4 + wallWidth); // correct 'rounding' by default
	ptrdiff_t nMin = (handyAvailSpace + temp - 1) / temp; // helped rounding by adding (temp-1) above

	list<ptrdiff_t> possibleNs;
	for(ptrdiff_t m = nMin; m <= nMax; ++m)
		possibleNs.push_back(m);

	if(verbose)
		cout<<"The maze size is within: "<<nMin<<" .. "<<nMax<<endl<<endl;

	// TRYING TO EXCLUDE ALL n FROM [nMin, nMax] THAT WOULD PUT WALLS ON THE CIRCLE - NOT ALLOWED
	for(ptrdiff_t n = nMin; n <= nMax; ++n) {
		// determine to which cell the (center of the) circle would belong:
		double 
			handyCellSz = handyAvailSpace / (double)n,
			halfInnerCellSz = (handyCellSz - wallWidth) / 2;
		ptrdiff_t innerCellSz_2 = (ptrdiff_t)halfInnerCellSz;

		ptrdiff_t yCircleCenter = (ptrdiff_t)boost::math::lltrunc((circleCenter.y + wallWidth_2) / handyCellSz);
		ptrdiff_t xCircleCenter = (ptrdiff_t)boost::math::lltrunc((circleCenter.x + wallWidth_2) / handyCellSz);

		ptrdiff_t xCellCenter = 
			(ptrdiff_t)boost::math::lltrunc(halfInnerCellSz + (xCircleCenter * handyAvailSpace) / (double)n);
		ptrdiff_t yCellCenter = 
			(ptrdiff_t)boost::math::lltrunc(halfInnerCellSz + (yCircleCenter * handyAvailSpace) / (double)n);

		bool allowed = true;
		if(xCellCenter - innerCellSz_2 > enclosedCircle.first.x)
			allowed = false;
		else if(xCellCenter + innerCellSz_2 < enclosedCircle.second.x)
			allowed = false;
		else if(yCellCenter - innerCellSz_2 > enclosedCircle.first.y)
			allowed = false;
		else if(yCellCenter + innerCellSz_2 < enclosedCircle.second.y)
			allowed = false;

		if(false == allowed) {
			if(verbose)
				cout<<"The maze can't have "<<n<<" rows/columns, as in that case the walls will cross the Start Circle!"<<endl;
			possibleNs.remove(n);
		}
	}
	require(possibleNs.empty()==false, "Faulty algorithm, as this maze is correct!");

	return possibleNs;
}

void TiltedMaze::ImageTiltedMazeReader::examineMazeBorder() {
	// FIND THE BOTTOM-LEFT CORNER OF THE MAZE:
	mazeCornerBL = findBottomLeftOfMaze(imgView);

	// FIND THE BOTTOM-RIGHT CORNER OF THE MAZE:
	mazeCornerBR = findBottomLeftOfMaze(
		flipped_left_right_view(
		subimage_view(imgView, 0, 0, (int)imgView.width(), (int)(mazeCornerBL.y + 1))));
	mazeCornerBR.x = imgView.width() - mazeCornerBR.x - 1;

	// FIND THE TOP-RIGHT CORNER OF THE MAZE:
	mazeCornerTR = findBottomLeftOfMaze(
		rotated90cw_view(
		flipped_up_down_view(
		subimage_view(imgView, 0, 0, (int)(mazeCornerBR.x + 1), (int)imgView.height()))));
	mazeCornerTR.y = mazeCornerTR.x;
	mazeCornerTR.x = mazeCornerBR.x;

	require(abs((mazeCornerBR.y-mazeCornerTR.y) - (mazeCornerBR.x-mazeCornerBL.x)) < 15,
		"We work for now only with square mazes. This is not a square one.");

	// DEDUCE TOP-LEFT CORNER OF THE MAZE:
	mazeCornerTL = point_t(mazeCornerBL.x, mazeCornerTR.y);

	// DETERMINE WALL WIDTH:
	wallWidth = diagonalFillLength(imgView, mazeCornerBL, BLACK_CHECK, MIN_THICKNESS_BORDER);
	wallWidth_2 = wallWidth >> 1;

	if(monitorOn)
		drawRect(imgOutView, mazeCornerTL, mazeCornerBR);

	// MAZE INTERIOR:
	tl = point_t(mazeCornerTL.x + wallWidth, mazeCornerTL.y + wallWidth);
	tr = point_t(mazeCornerTR.x - wallWidth, mazeCornerTR.y + wallWidth);
	bl = point_t(mazeCornerBL.x + wallWidth, mazeCornerBL.y - wallWidth);
	br = point_t(mazeCornerBR.x - wallWidth, mazeCornerBR.y - wallWidth);

	if(monitorOn)
		drawRect(imgOutView, tl, br);

	point_t viewSize(tr.x - tl.x + 1, br.y - tr.y + 1);
	innerMazeView = subimage_view(imgView, tl, viewSize);
	innerMazeOutView = subimage_view(imgOutView, tl, viewSize);
}

pair<point_t, point_t> TiltedMaze::ImageTiltedMazeReader::encloseCircle() {
	point_t circleTop = findCircleTop();
	ptrdiff_t radius = diagonalFillLength(innerMazeView, circleTop, RED_CHECK, MIN_DIAMETER_CIRCLE>>1);
	return make_pair(point_t(circleTop.x - radius, circleTop.y), point_t(circleTop.x + radius, circleTop.y + (radius << 1)));
}

point_t TiltedMaze::ImageTiltedMazeReader::findCircleTop() {
	static unsigned circleSamplingStepSize = static_cast<unsigned>(MIN_DIAMETER_CIRCLE * M_SQRT1_2);

	pair<point_t, point_t> firstSubsamplingSegment, firstRealSegment;
	ptrdiff_t actualX, actualY;

	// subsampled view (MIN_THICKNESS_BORDER) 
	dynamic_xy_step_type<rgb8_view_t>::type
		ssv = subsampled_view(innerMazeView, circleSamplingStepSize, circleSamplingStepSize);

	// perform the subsampled search for the 1st red segment
	firstSubsamplingSegment = find1stSegment(ssv, RED_CHECK);

	// compute the original coordinates of the found pixel + defining the narrow search area
	ptrdiff_t xLeft = (firstSubsamplingSegment.first.x - 1) * circleSamplingStepSize + 1;
	ptrdiff_t yBottom = firstSubsamplingSegment.first.y * circleSamplingStepSize;
	ptrdiff_t yTop = (firstSubsamplingSegment.first.y - 1) * circleSamplingStepSize + 1;
	ptrdiff_t xRight = (firstSubsamplingSegment.second.x + 1) * circleSamplingStepSize - 1;

	// the circle's top should be found traversing downwards and left->right
	// the rectangle with height (circleSamplingStepSize - 1) right under line yTop
	// and that extends from xLeft to xRight

	// narrowing the search space for the thorough searching
	rgb8_view_t
		si = subimage_view(innerMazeView, (int)xLeft, (int)yTop, (int)(1 + xRight - xLeft), (int)(1 + yBottom - yTop));

	// performing the concluding search: (find a 1st red top line and return its center)
	firstRealSegment = find1stSegment(si, RED_CHECK);

	// Computing the actual coordinates of the top of the circle
	actualX = xLeft + ((firstRealSegment.first.x + firstRealSegment.second.x + 1) >> 1); // the center of the top red line of the circle
	actualY = yTop + firstRealSegment.first.y;
	require(isFit(innerMazeView(actualX, actualY), RED_CHECK), "Wrong coords computation!");

	return point_t(actualX, actualY);
}

bool TiltedMaze::ImageTiltedMazeReader::isFit(const rgb8_pixel_t &pix, ColorCheckType checkType) {
	bool result = false;
	int channelValue = get_color(pix, green_t());

	// all checked colors don't allow green to be very high
	if(false == isDim(channelValue))
		return false;

	switch(checkType) {
	case BLACK_CHECK:
		return true;

	case BLUE_CHECK:
		return 
			isDim(get_color(pix, red_t())) &&
			(get_color(pix, blue_t()) > HIGH_THRESHOLD);

	case RED_CHECK:
		return 
			isDim(get_color(pix, blue_t())) &&
			(get_color(pix, red_t()) > HIGH_THRESHOLD);

	default:
		;
	}

	return result;
}

optional<string> TiltedMaze::nextLine(std::ifstream &ifs) const {
	string line;
	for(;;) {
		if( ! getline(ifs, line))
			return optional<string>(); // EOF or error

		// skip empty lines or comments
		if((0 == line.compare("")) || (';' == line[0]))
			continue;

		return line;
	}
}

void TiltedMaze::buildGraph() {
	// determine the number of horizontal segments
	unsigned hSegmentsCount = 0U, vSegmentsCount = 0U, i;
	for(const split_interval_set<unsigned> &sis : rows) {
		hSegmentsCount += (unsigned)sis.iterative_size();
	}
	hSegments.reserve(hSegmentsCount);

	i = 0U;
	for(const split_interval_set<unsigned> &sis : rows) {
		for(const interval<unsigned>::type &iut : sis) {
			if(iut.upper() - iut.lower() > 1U) { // single cells don't constitute segments
				hSegments.emplace_back(Segment(i, iut));
				Segment &seg = hSegments.back();
				orphanSegments.insert(&seg);
				for(unsigned j = iut.lower(), jLim = iut.upper(); j < jLim; ++j)
					coordOwners[Coord(i, j)].first = & seg;
			}
		}
		++i;
	}

	// determine the number of vertical segments
	for(const split_interval_set<unsigned> &sis : columns) {
		vSegmentsCount += (unsigned)sis.iterative_size();
	}
	vSegments.reserve(vSegmentsCount);

	i = 0U;
	for(const split_interval_set<unsigned> &sis : columns) {
		for(const interval<unsigned>::type &iut : sis) {
			if(iut.upper() - iut.lower() > 1U) { // single cells don't constitute segments
				vSegments.emplace_back(Segment(i, iut, false));
				Segment &seg = vSegments.back();
				orphanSegments.insert(&seg);
				for(unsigned j = iut.lower(), jLim = iut.upper(); j < jLim; ++j)
					coordOwners[Coord(j, i)].second = & seg;
			}
		}
		++i;
	}

	// placing the targets on the appropriate segments
	for(Target &targetCoord : targets) {
		PSegmentsPair hvSegments = coordOwners[targetCoord];
		Segment *hSeg = hvSegments.first, *vSeg = hvSegments.second;
		bool hFound = (NULL != hSeg), vFound = (NULL != vSeg);
		require(hFound || vFound, "At least one segment should cover each Coord!");
		targetCoord.setVisitors(hvSegments);
		if(hFound)
			hSeg->manageTarget(targetCoord);
		if(vFound)
			vSeg->manageTarget(targetCoord);
	}


	// creating the branchlessPaths
	for(i = 0U; false == orphanSegments.empty(); ++i) {
		set<Segment*>::iterator it = orphanSegments.begin();
		std::shared_ptr<BranchlessPath> pBranchlessPath = std::make_shared<BranchlessPath>(i, *(*it), *this);
		branchlessPaths.push_back(pBranchlessPath);
	}

/*
	for(std::shared_ptr<BranchlessPath> e : branchlessPaths) {
		cout<<string(60, '=')<<endl;
		cout<<*e<<endl;
	}
*/

	// introducing the targets into the required structure
	graphTargets.clear();
	for(Target &targetCoord : targets) {
		BranchlessPath *bp = NULL;
		PSegmentsPair hvSegments = coordOwners[targetCoord];
		Segment *seg = hvSegments.first;
		if(NULL != seg) {
			bp = seg->owner();
			graphTargets.addTarget(targetCoord, *bp);
		}
		seg = hvSegments.second;
		if(NULL != seg) {
			bp = seg->owner();
			graphTargets.addTarget(targetCoord, *bp);
		}
	}


	// CREATE THE GRAPH
	// THE VERTICES
	for(std::shared_ptr<BranchlessPath> pVertex : branchlessPaths) {
		add_vertex(BpVertexProps((int)pVertex->id(), pVertex.get()), searchGraph);
	}

	// Introducing a virtual (auxiliary) start vertex (idx -1) that doesn't correspond to any BP
	// Used just to create a single start point instead of 2, as the start position may belong to 2 BPs
	const int idxStartVertex = (int)branchlessPaths.size();
	const int idxEndVertex = idxStartVertex + 1;
	add_vertex(BpVertexProps(idxStartVertex), searchGraph);

	// Introducing also a virtual (auxiliary) end vertex (last idx)
	// Excepting the other virtual vertex, all other vertices will have a direct edge towards this End Vertex.
	// This vertex allows strictly 0 unvisited targets in order to be included in the walk
	add_vertex(BpVertexProps(idxEndVertex, NULL, 0U), searchGraph); 

	// THE EDGES
	int edgeIdx = 0;

	// there are 1/2 BP-s to start from (the Start coordinate belongs both to 1 or 2 BPs)
	PSegmentsPair hvSegsForStart = coordOwners[startLocation];
	Segment *seg = hvSegsForStart.first;
	if(NULL != seg) {
		add_edge((size_t)idxStartVertex, (size_t)seg->owner()->id(), BpsArcProps(edgeIdx++), searchGraph);
	}
	seg = hvSegsForStart.second;
	if(NULL != seg) {
		add_edge((size_t)idxStartVertex, (size_t)seg->owner()->id(), BpsArcProps(edgeIdx++), searchGraph);
	}

	for(std::shared_ptr<BranchlessPath> pVertex : branchlessPaths) {
		int fromIdx = (int)pVertex->id();
		for(const BranchlessPath *bp : pVertex->theLinksOwners()) {
			add_edge((size_t)fromIdx, (size_t)bp->id(), BpsArcProps(edgeIdx++), searchGraph);
		}

		// Adding the virtual (auxiliary) edge towards the End vertex
		add_edge((size_t)fromIdx, (size_t)idxEndVertex, BpsArcProps(edgeIdx++), searchGraph);
	}
}

TiltedMaze::TiltedMaze(const string &mazeFile) :
				rowsCount(0U), columnsCount(0U) {

	path mazeNameAsPath(mazeFile);
	require(mazeNameAsPath.has_extension(), "The image provided as maze source has no extension!");

	string imgType(extension(mazeNameAsPath));
	if(imgType.compare(".txt") == 0)
		initFromTextFile(mazeFile);
	else
		initFromImageFile(mazeFile);
}

void TiltedMaze::initFromTextFile(const string &mazeFile) {
	std::ifstream ifs(mazeFile);
	optional<string> line;

	// Reading the size of the maze
	{
		line = nextLine(ifs); require(line.is_initialized(), "Error while reading the maze!");
		istringstream iss(*line);
		iss>>rowsCount>>columnsCount;
		require((rowsCount>0U) && (columnsCount>0U), "Read invalid maze size!");

// 		PRINTLN(rowsCount);
// 		PRINTLN(columnsCount);

		unsigned i;
		rows.reserve(rowsCount);
		for(i=0U; i<rowsCount; ++i) {
			split_interval_set<unsigned> sis;
			sis += interval<unsigned>::type(0U, columnsCount);
			rows.push_back(sis);
		}

		columns.reserve(columnsCount);
		for(i=0U; i<columnsCount; ++i) {
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
		line = nextLine(ifs); require(line.is_initialized(), "Error while reading the maze!");
		istringstream iss(*line);
		iss>>type;
		if((0 == type.compare("row"))) isRowInterval = true;
		else if((0 == type.compare("column"))) isRowInterval = false;
		else
			break; // no more rows, nor columns - just read the start location

		iss>>index; require(index < (isRowInterval ? rowsCount : columnsCount), "Invalid row / column index within the maze matrix!");
		iss>>colonCh; require(':' == colonCh, "Expected :");

// 		cout<<type<<' '<<index<<':';
		for(unsigned wallIndex = UINT_MAX; ; wallIndex = UINT_MAX) {
			iss>>wallIndex;
			if(UINT_MAX == wallIndex)
				break; // read last wall index from the line

			++wallIndex; // The read value still falls within the previous interval
			require(wallIndex < (isRowInterval ? columnsCount : rowsCount), "Invalid wall index within the maze matrix!");
// 			cout<<' '<<wallIndex;
			if(isRowInterval) {
				rows[index] += interval<unsigned>::type(wallIndex, columnsCount);
			} else {
				columns[index] += interval<unsigned>::type(wallIndex, rowsCount);
			}
		}
// 		cout<<endl;
	}

	// Parsing the Starting location from the last read line
	{
		istringstream iss(*line);
		startLocation.reset();
		iss>>startLocation.row>>startLocation.col;
		require((startLocation.row < rowsCount) && (startLocation.col < columnsCount), "Invalid Starting Location within the maze!");
// 		cout<<"startLocation coords: "<<startLocation.row<<','<<startLocation.col<<endl;
	}

	// Reading the targets
	for(bool targetsFound = false; ;) {
		line = nextLine(ifs);
		if( ! line) {
			require(targetsFound, "No targets were found!");
			break;
		}
		istringstream iss(*line);
		Target target;
		iss>>target.row>>target.col;
		require((target.row < rowsCount) && (target.col < columnsCount), "Invalid Target coords!");
// 		cout<<"target coords: "<<target.row<<','<<target.col<<endl;
		targetsFound = true;
		targets.push_back(target);
	}

// 	cout<<"File was correct!"<<endl;

	buildGraph();
}

void TiltedMaze::initFromImageFile(const string &mazeFile) {
	ImageTiltedMazeReader mazeReader(mazeFile/*, "", true*/);

	rowsCount = columnsCount = (unsigned)mazeReader.mazeSize();

	unsigned i;
	rows.reserve(rowsCount);
	for(i=0U; i<rowsCount; ++i) {
		split_interval_set<unsigned> sis;
		sis += interval<unsigned>::type(0U, columnsCount);
		rows.push_back(sis);
	}

	columns.reserve(columnsCount);
	for(i=0U; i<columnsCount; ++i) {
		split_interval_set<unsigned> sis;
		sis += interval<unsigned>::type(0U, rowsCount);
		columns.push_back(sis);
	}

	const vector<vector<unsigned>> &theVerticalWalls = mazeReader.getVfoundWalls();
	const vector<vector<unsigned>> &theHorizontalWalls = mazeReader.getHfoundWalls();

	for(i=0U; i<rowsCount; ++i) {
		for(unsigned wallIdx : theVerticalWalls[i])  {
			rows[i] += interval<unsigned>::type(++wallIdx, columnsCount);
		}
	}

	for(i=0U; i<columnsCount; ++i) {
		for(unsigned wallIdx : theHorizontalWalls[i])  {
			columns[i] += interval<unsigned>::type(++wallIdx, rowsCount);
		}
	}

	point_t theCircleLocation = mazeReader.circleLocation();
	startLocation.row = (unsigned)theCircleLocation.y;
	startLocation.col = (unsigned)theCircleLocation.x;

	const list<point_t> &theSquares = mazeReader.squaresLocations();
	for(const point_t &targetCoord : theSquares) {
		targets.emplace_back(Target((unsigned)targetCoord.y, (unsigned)targetCoord.x));
	}

	buildGraph();
}

void TiltedMaze::initDisplaySettings() {
	clearConsole();
	bgColor = ConsoleColor::Black;
	wallColor = ConsoleColor::White;
	targetColor = ConsoleColor::Yellow;
			
	remainingColors += ConsoleColor::DarkYellow, ConsoleColor::Gray, ConsoleColor::DarkGray,
		ConsoleColor::Blue, ConsoleColor::Green, ConsoleColor::Cyan, ConsoleColor::Red,
		ConsoleColor::Magenta, ConsoleColor::Yellow, ConsoleColor::White;
	remainingColors.erase(bgColor);
	remainingColors.erase(wallColor);
	remainingColors.erase(targetColor);

	getConsoleCursorPos(displayOriginRow, displayOriginCol);
	getConsoleTextColor(origFgColor, origBgColor);
	targetCh = '$';
}

void TiltedMaze::revertDisplaySettings() {
	setConsoleTextColor(origFgColor, origBgColor);
	setConsoleCursorPos(displayIndex(rowsCount, true) + 1, 0); // after displaying the maze, set the cursor below it
}

int TiltedMaze::displayIndex(unsigned clientIdx, bool isRow) {
	++clientIdx;
	clientIdx <<= 1;
	if(false == isRow)
		clientIdx |= 1U;

	return (int)clientIdx;
}

void TiltedMaze::displayMaze() {
	initDisplaySettings();
#pragma warning(push)
#pragma warning( disable : TRUNCATION_CASTING THREAD_UNSAFE_CONSTRUCTION )
	static char wallCh = static_cast<char>(219); // full square
#pragma warning(pop)
			
	const int lastRowIdx = (int)rowsCount - 1;
	const int rowOfTopMargin = displayIndex(0U, true) - 1;
	const int rowOfBottomMargin = displayIndex(rowsCount, true) - 1;
	const int colOfLeftMargin = displayIndex(0, false) - 1;
	const int displayRowSz = (((int)columnsCount) << 1) - 1;

	const string horizMargin(size_t(displayRowSz + 2), wallCh);
	const string horizContent((size_t)displayRowSz, ' ');
	const string horizWallPiece(3, wallCh);

	setConsoleTextColor(wallColor, bgColor);

	// display header of column indices modulo 10
	setConsoleCursorPos(displayOriginRow, colOfLeftMargin + 1);
	for(unsigned i = 0; i < columnsCount; ++i)
		cout<<(i%10)<<' ';

	// top margin
	setConsoleCursorPos(rowOfTopMargin, colOfLeftMargin);
	cout<<horizMargin;

	// lateral margins preceded & followed by row indices
	for(int i = 0, row = rowOfTopMargin + 1; i <= lastRowIdx; ++i) {
		setConsoleCursorPos(row++, displayOriginCol);
		cout<<right<<setw(2)<<i<<wallCh<<horizContent<<wallCh<<i;
		setConsoleCursorPos(row++, displayOriginCol + 2);
		cout<<wallCh<<horizContent<<wallCh;
	}

	// bottom margin
	setConsoleCursorPos(rowOfBottomMargin, colOfLeftMargin);
	cout<<horizMargin;

	// display footer of column indices modulo 10
	setConsoleCursorPos(rowOfBottomMargin + 1, colOfLeftMargin + 1);
	for(unsigned i = 0; i < columnsCount; ++i)
		cout<<(i%10)<<' ';

	// handle vertical segments
	int displayCol = colOfLeftMargin;
	for(const split_interval_set<unsigned> &sis : columns) {
		for(const interval<unsigned>::type &iut : sis) {
			unsigned bottomBound = iut.upper(); // one past the upper end
			if(bottomBound < rowsCount) { // consider only segments not touching the bottom of the maze
				int displayRow = displayIndex(bottomBound, true) - 1;
				setConsoleCursorPos(displayRow, displayCol);
				cout<<horizWallPiece;
			}
		}
		displayCol += 2;
	}

	// handle horizontal segments
	int _displayRow = rowOfTopMargin, displayRow = _displayRow + 1, displayRow_ = displayRow + 1;
	for(const split_interval_set<unsigned> &sis : rows) {
		for(const interval<unsigned>::type &iut : sis) {
			unsigned rightBound = iut.upper(); // one past the upper end
			if(rightBound < columnsCount) { // consider only segments not touching the right margin of the maze
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
	for(const Target &t : targets) {
		setConsoleCursorPos(displayIndex(t.row, true), displayIndex(t.col, false));
		cout<<targetCh;
	}

	// position the cursor at the starting location
	setConsoleCursorPos(displayIndex(startLocation.row, true), displayIndex(startLocation.col, false));
}

bool TiltedMaze::solve(bool graphicMode/* = true*/) {
	solveGraphically = graphicMode;

	const size_t idxStartVertex = branchlessPaths.size(),
		idxEndVertex = idxStartVertex + 1ULL;

	// spptw
	vector< vector< graph_traits< BpAdjacencyList >::edge_descriptor > >  opt_solutions_spptw;
	vector< BpResCont > pareto_opt_rcs_spptw;
	BpResExtensionFn bpRef;
	BpDominanceFn bpDom;

	r_c_shortest_paths_dispatch_adapted(searchGraph,
										get(&BpVertexProps::num, searchGraph),
										get(&BpsArcProps::num, searchGraph),
										idxStartVertex, idxEndVertex, 
										opt_solutions_spptw, pareto_opt_rcs_spptw, true,
										BpResCont(), // empty uniqueTraversedBps & walk
										bpRef, bpDom, 
										allocator< r_c_shortest_paths_label< BpAdjacencyList, BpResCont> >(), 
										BpGraphAlgVisitor());

	size_t solutionsCount = opt_solutions_spptw.size();
	if(solutionsCount == 0ULL)
		return false;
			
	bool b_is_a_path_at_all = false;
	bool b_feasible = false; 
	bool b_correctly_extended = false;
	BpResCont actual_final_resource_levels; // update to what's expected
	graph_traits<BpAdjacencyList>::edge_descriptor ed_last_extended_edge;
	check_r_c_path( searchGraph, 
					opt_solutions_spptw[0], 
					BpResCont(), 
					true, 
					pareto_opt_rcs_spptw[0], 
					actual_final_resource_levels, 
					BpResExtensionFn(), 
					b_is_a_path_at_all, 
					b_feasible, 
					b_correctly_extended, 
					ed_last_extended_edge );

	if(graphTargets.unvisited(actual_final_resource_levels.uniqueTraversedBps) > 0U)
		return false;

	if( !b_is_a_path_at_all || !b_feasible || !b_correctly_extended )
		return false;

	list<BranchlessPath*> firstSolution;
	for(size_t j = 0U, walkLen = opt_solutions_spptw[0].size() - 1; j < walkLen ; ++j) {
		const BpVertexProps& aVertexProps =
			get(vertex_bundle, searchGraph)[source( opt_solutions_spptw[0][j], searchGraph )];
		firstSolution.push_front(aVertexProps.forTiltedMaze());
	}

	if(solveGraphically) {
		initDisplaySettings();
		displayMaze();
			
	} else {
		if(solutionsCount == 1U)
			cout<<"Found a solution: ";
		else {
			cout<<solutionsCount<<" solutions were found. Presenting only 1st one: ";
		}
		for(const BranchlessPath *bp : firstSolution) {
			cout<<bp->id()<<' ';
		}
		cout<<endl<<endl<<endl;
	}

	// Determine the BranchlessPath-s of the walk where all the targets must be visited before going any further
	// This is the case for every BranchlessPath that appears only once within the solution walk
	// For those that reappear, the last visit is the one that must visit every target.
	const size_t solSz = firstSolution.size();
	vector<BranchlessPath*> solAsVector(BOUNDS_OF(firstSolution));
	set<BranchlessPath*> uniqueBpsTraversed(BOUNDS_OF(solAsVector));
	vector<bool> allTargetsMustBeVisited(solSz, false);

	for(const BranchlessPath *uniqueBp : uniqueBpsTraversed) {
		// find the position of the last uniqueBp within solAsVector
		size_t idx = solSz-1;
		vector<BranchlessPath*>::reverse_iterator rit = solAsVector.rbegin(), ritEnd = solAsVector.rend();
		for(; rit != ritEnd; ++rit, --idx) {
			if(uniqueBp == *rit) {
				allTargetsMustBeVisited[idx] = true;
				break;
			}
		}
	}

	// perform the traversal
	Coord fromCoord = startLocation, endCoord = solAsVector[0]->firstEnd();
	if(solSz == 1U) {
		solAsVector[0]->traverse(fromCoord, endCoord, true, true); // stops after visiting all targets
			
	} else { // solution has at least 2 BP-s
		// determine 1st intersection
		if(false == solAsVector[1]->containsCoord(endCoord))
			endCoord = solAsVector[0]->secondEnd();

		// traverse all except last
		size_t idx = 0, lim = solSz - 1U;
		for(;;) {
			solAsVector[idx]->traverse(fromCoord, endCoord, allTargetsMustBeVisited[idx]);
			fromCoord = endCoord;
			endCoord = solAsVector[++idx]->firstEnd();
					
			if(idx == lim)
				break;

			if(false == solAsVector[idx + 1]->containsCoord(endCoord))
				endCoord = solAsVector[idx]->secondEnd();
		}

		// traverse last
		solAsVector[lim]->traverse(fromCoord, endCoord, true, true); // stops after visiting all targets
	}

	for(const Target &t : targets) {
		require(t.visited(), "All targets should have been visited at the end of the walk!");
	}

	if(solveGraphically)
		revertDisplaySettings();

	return true;
}

string TiltedMaze::BpVertexProps::toString() const {
	ostringstream oss;
	if(NULL == _forTiltedMaze) {
		oss<<"*";
	} else {
		oss<<_forTiltedMaze->id();
	}
	return oss.str();
}

void TiltedMaze::Targets::addTarget(TiltedMaze::Target &t, TiltedMaze::BranchlessPath &ownerBp) {
	initialTargetBpMapping[&t].insert(&ownerBp);
}

void TiltedMaze::Targets::addTarget(TiltedMaze::Target &t,
			TiltedMaze::BranchlessPath &sharerBp1, TiltedMaze::BranchlessPath &sharerBp2) {
	initialTargetBpMapping[&t] = set< TiltedMaze::BranchlessPath* >(list_of(&sharerBp1)(&sharerBp2));
}

unsigned TiltedMaze::Targets::unvisited(const set<TiltedMaze::BranchlessPath*> &traversedBps,
										set<TiltedMaze::Target*> *unvisitedTargets/* = NULL*/) const {
	bool reportsTargetsToo = (unvisitedTargets != NULL);
	unsigned result = 0U;

	set<TiltedMaze::BranchlessPath*> relevantBps;
	for(TargetBpMapping::value_type tBpM : initialTargetBpMapping) {
		set_intersection(BOUNDS_OF(tBpM.second),
						BOUNDS_OF(traversedBps),
						inserter(relevantBps, relevantBps.begin()));
		if(relevantBps.empty()) {
			++result;

			if(reportsTargetsToo)
				unvisitedTargets->insert(tBpM.first);

		} else {
			relevantBps.clear();
		}
	}

	return result;
}

TiltedMaze::BpResCont& TiltedMaze::BpResCont::operator=(const BpResCont& other) {
	if(this == &other)
		return *this;

	this->~BpResCont();

	new(this) BpResCont(other); // basically transforms assignment operator into copy construction

	return *this;
}

int TiltedMaze::BpResCont::lessUnvisitedOrAtLeastShorterWalkThan(const BpResCont &other) const {
	int walkSzCompare = ::compare(other.walk.size(), walk.size()); // for shorter this.walk => 1

	if(uniqueTraversedBps == other.uniqueTraversedBps) // this means also the unvisited targets count is the same
		return walkSzCompare;

	// Here the unique BPs of this and other differ,
	// but the count of unvisited targets might be the same
	unsigned unvisitedTargets1 = graphTargets.unvisited(uniqueTraversedBps);
	unsigned unvisitedTargets2 = graphTargets.unvisited(other.uniqueTraversedBps);

	int unvisitedTargetsCompare = ::compare(unvisitedTargets2, unvisitedTargets1); // returns 1 if this has less unvisited

	if(0 == unvisitedTargetsCompare)
		return walkSzCompare;

	return unvisitedTargetsCompare;
}

int TiltedMaze::BpDominanceFn::operator() (const BpResCont &rc1, const BpResCont &rc2) const {
	/*
	The rc1.lessUnvisitedOrAtLeastShorterWalkThan(rc2) from below is the ideal criterion,
	but unfortunately, the future (unknown) dictates the dominance:
	The performance of two walks depends on the BPs that they don't share.
	So, even if now rc1 is better than rc2, the common path they'll share from this BP on is unknown.
	If this next path covers just BPs that rc1 already traversed and rc2 didn't =>
	rc2 would have at the end also the rc1's assets they didn't share initially.
	This would mean that rc1 won't improve, while rc2 would overpass rc1.
	As a conclusion, we cannot assert which rc is dominant, but we MUST prevent infinite useless loops,
	which can be detected as follows:
	*/

	bool sameUniqueBPsTraversed = (rc1.uniqueTraversedBps == rc2.uniqueTraversedBps);
	int rigurousCompareOfExistingWalks = rc1.lessUnvisitedOrAtLeastShorterWalkThan(rc2);

	if(0 == rigurousCompareOfExistingWalks) { // same unvisited count & walk length
		if(sameUniqueBPsTraversed) // rc2 is just a permutation of rc2
			return 1; // rc2 can be removed safely

		return 0; // may contain a different set of BPs and the future outcome isn't clear
	}

	// Here they have a different count of unvisited BPs or else a different walk length

	int walkLengthCompare = rc1.shorterWalkThan(rc2);

	if(sameUniqueBPsTraversed) { // => they have the same count of unvisited BPs, so only the walk length is different
		// checking for bad loops (extending through the same BPs)
		if(walkLengthCompare > 0) // rc1 is shorter => rc2 is dominated
			return 1; // rc2 can be removed safely

		// Here walkLengthCompare < 0 (rc2 is shorter => rc1 is dominated)
		return -1; // rc1 can be removed safely
	}

	// Here they have a different count of unvisited BPs or else a different walk length
	// In any case, their sets of unique BPs traversed are different
	return 0; // ambiguous, better preserve both
}

int TiltedMaze::BpResCont::lessUnvisitedThan(const BpResCont &other) const {
	if(uniqueTraversedBps == other.uniqueTraversedBps) // this means also the unvisited targets count is the same
		return 0;

	// Here the unique BPs of this and other differ,
	// but the count of unvisited targets might be the same
	unsigned unvisitedTargets1 = graphTargets.unvisited(uniqueTraversedBps);
	unsigned unvisitedTargets2 = graphTargets.unvisited(other.uniqueTraversedBps);

	return ::compare(unvisitedTargets2, unvisitedTargets1); // returns 1 if this has less unvisited
}

bool TiltedMaze::BpResExtensionFn::operator() (const BpAdjacencyList& g,
	BpResCont& new_cont, const BpResCont& old_cont, 
	graph_traits<BpAdjacencyList>::edge_descriptor ed) const {
		int nextBp = (int)target(ed, g);
		const BpVertexProps& vert_prop = get(vertex_bundle, g)[(size_t)nextBp];
		TiltedMaze::BranchlessPath *tmNextBp = vert_prop.forTiltedMaze();

		new_cont = old_cont;
		new_cont.walk += tmNextBp;
		new_cont.uniqueTraversedBps += tmNextBp;

		unsigned unvisited4_new_cont = graphTargets.unvisited(new_cont.uniqueTraversedBps);
		unsigned allowedUnvisitedCountByNextBp = vert_prop.maxUnvisitedTargets(); // this is always infinity, except BPend

		return (unvisited4_new_cont <= allowedUnvisitedCountByNextBp);
}

void main(int, char*[]) {
	if(testsOk())
		cout<<"All tests were ok."<<endl<<"Entering interactive demonstration mode ..."<<endl;
	else {
		cerr<<"Found problems while performing the tests! Leaving ..."<<endl;
		return;
	}

	OPENFILENAME ofn;       // common dialog box structure
	TCHAR szFile[260];       // buffer for file name

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr; // no owner
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = _T("All Input Maze Types\0*.txt;*.bmp;*.png;*.tif;*.tiff;")
						 _T("*.jpg;*.jpeg\0Text Input Mazes\0*.txt\0Image Input Mazes\0")
						 _T("*.bmp;*.png;*.tif;*.tiff;*.jpg;*.jpeg\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.lpstrTitle = _T("Please select a maze to solve or close the dialog to quit");

	while(GetOpenFileName(&ofn)) {
		string mazeName = tstrToStr(ofn.lpstrFile);

		try {
			TiltedMaze tm(mazeName);
			if(false == tm.solve()) {
				cout<<"Couldn't solve "<<mazeName<<endl;
				pressKeyToContinue(cout);
			}
		} catch(std::exception &e) {
			cerr<<"Error detected in '"<<mazeName<<"': "<<e.what()<<endl;
			pressKeyToContinue(cerr);
		}
	}
}
