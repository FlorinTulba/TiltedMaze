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

#ifndef H_PROBLEM_ADAPTER
#define H_PROBLEM_ADAPTER

#include "mazeStruct.h"
#include "consoleMode.h"
#include "graphicalMode.h"

#include "conditions.h"

#pragma warning( push, 0 )

#include <string>
#include <set>
#include <map>
#include <list>

#include <boost/graph/adjacency_list.hpp>

#pragma warning( pop )

typedef std::pair<unsigned, unsigned> UUpair;

// forward declarations
class Segment;
class BranchlessPath;

typedef std::pair<Segment*, Segment*> PSegmentsPair; ///< Pair of pointers to segments

/// The targets are special coordinates
class MazeTarget: public Coord {
	bool _visited;			///< was the target visited
	PSegmentsPair visitors;	///< which of the 2 segments containing this target (horizontal and vertical) has been used to traverse this cell

public:
	MazeTarget(unsigned row = UINT_MAX, unsigned col = UINT_MAX) : Coord(row, col), _visited(false) {}
	MazeTarget(const Coord &c) : Coord(c.row, c.col), _visited(false) {}

	/// Lets the target know which of the horizontal / vertical segments were used when visiting it
	void setVisitors(PSegmentsPair &theVisitors) { visitors = theVisitors; }
	void visit(); ///< Let the visitor (horizontal and/or vertical) segments mark this target as visited
	inline bool visited() const { return _visited; }
	inline bool isShared() const { return (visitors.first != nullptr) && (visitors.second != nullptr); }
};

/// Traversable segment of the maze (wall to wall)
class Segment {
	/// Mapping between segment coordinates (the variable 1D coordinates) and target objects
	typedef std::map<unsigned, MazeTarget*> UTargetMap;
	typedef std::pair<UTargetMap::const_iterator, UTargetMap::const_iterator> RangeUnvisited;

	bool _isHorizontal;		///< is this a horizontal or vertical segment
	unsigned fixedIndex;	///< for horizontal segments, the 'row' coordinate is fixed; for vertical ones, the 'column' is fixed
	boost::icl::closed_interval<unsigned>::type closedInterval;	///< the limit 1D coordinates for the non-fixed part of the 2D coordinate

	std::vector<MazeTarget*> managedTargets;		///< the targets lying on this segment
	mutable UTargetMap varDimUnvisitedTargets;	///< the map between the segment coordinates of unvisited targets and these targets (the key is the variable 1D coordinate of the target on the segment)

	BranchlessPath *parent;	///< the path (graph vertex) containing this segment

	/// @return a map of unvisited targets between the 2 coordinates found on a horizontal / vertical line
	RangeUnvisited unvisitedTargets(const Coord *from = nullptr, const Coord *end = nullptr) const;

public:

	Segment() : _isHorizontal(false), fixedIndex(UINT_MAX), parent(nullptr) {}

	Segment(const Coord &coord1, const Coord &coord2);

	Segment(unsigned fixedIndex, const boost::icl::interval<unsigned>::type &rightOpenInterval, bool isHorizontal = true);

	inline bool operator==(const Segment &other) const {
		return (_isHorizontal == other._isHorizontal) && (fixedIndex == other.fixedIndex) && (closedInterval == other.closedInterval);
	}

	inline unsigned fixedCoord() const { return fixedIndex; }

	inline bool isHorizontal() const { return _isHorizontal; }

	inline Coord lowerEnd() const {
		return (_isHorizontal) ? Coord(fixedIndex, closedInterval.lower()) : Coord(closedInterval.lower(), fixedIndex);
	}

	inline Coord upperEnd() const {
		return (_isHorizontal) ? Coord(fixedIndex, closedInterval.upper()) : Coord(closedInterval.upper(), fixedIndex);
	}

	/// Checks if a coordinate matches one end of this segment
	inline bool isEnd(const Coord &coord) const {
		return (coord == lowerEnd()) || (coord == upperEnd());
	}

	inline bool isLowerEnd(const Coord &end) const {
		bool result = (lowerEnd() == end);
		require(result || (upperEnd() == end), "Provided parameter wasn't an end of this segment!");
		return result;
	}

	/// @return the opposite end of a segment, given one of its ends
	inline Coord otherEnd(const Coord &oneEnd) const {
		return (isLowerEnd(oneEnd)) ? upperEnd() : lowerEnd();
	}

	/// @return both ends of the segment
	inline CoordsPair ends() const {
		return std::make_pair(lowerEnd(), upperEnd());
	}

	inline bool containsCoord(const Coord &coord, bool exceptEnds = false) const {
		return containsCoord(coord.row, coord.col, exceptEnds);
	}

	bool containsCoord(unsigned row, unsigned col, bool exceptEnds = false) const;

	/// @return the neighbor (from inside the segment) of the end of this segment
	Coord nextToEnd(const Coord &end) const;

	bool hasUnvisitedTargets(const Coord *from = nullptr, const Coord *end = nullptr) const;

	std::set<MazeTarget*> getUnvisitedTargets(const Coord *from = nullptr, const Coord *end = nullptr) const;

	/// The segment becomes aware of a certain target found on itself
	void manageTarget(MazeTarget &target);

	/// The segment should know that the target provided as parameter (lying on this segment) was visited
	void ackVisit(const MazeTarget &target);

	/// This segment is visited between from and end. Provide either both or none of these parameters.
	void traverse(const Coord *from = nullptr, const Coord *end = nullptr);

	inline bool hasOwner() const { return nullptr != parent; }
	inline BranchlessPath* owner() const { return parent; }
	void setOwner(BranchlessPath *e) { parent = e; }

	std::string toString() const;

	opLessLessRef(Segment)
};

/**
Several consecutive segments connected at one of their ends. Such 2 consecutive segments
are always perpendicular.

Once the player engages on such a BranchlessPath, he can only leave it through the same
or the opposite end of the path.

There are no alternative branches to follow. This is a simple vertex in the graph corresponding
to this maze.

The 2 ends of the entire path are typically either dead ends or one or both lye
within a segment not covered by the path forming bifurcations.
*/
class BranchlessPath {
	typedef std::list<Segment*>::const_iterator LSI;

	unsigned _id;			///< id of this path (graph vertex)
	CoordsPair ends;		///< coordinates of the limits of the branchlessPath
	PSegmentsPair links;	///< the segments (if any) of other branchlessPaths that contain the ends (bifurcations of the path)
	std::vector<BranchlessPath*> linksOwners; ///< the BranchlessPath to which the links above belong (connected vertices in the graph representing the maze)

	std::list<Segment*> children; ///< all segments forming this path (graph vertex) placed around the path seed - the first child segment
	std::set<Segment*> &_orphanSegments;
	std::map<Coord, PSegmentsPair> &_coordOwners;

	/// Expand the path (graph vertex) adding the new segment from 'anEnd' either to the front, or to the back of children
	void expand(const Coord &anEnd, bool horizDir, bool afterSeed = true);

	/// @return iterator within children pointing to 'seg' segment
	LSI whichSegment(const Segment &seg) const;

	/// Increment / decrement 'it' iterator depending on the 'towardsLowerPartOfBranchlessPath' parameter
	inline void updateLSI(LSI &it, bool towardsLowerPartOfBranchlessPath) const;

	/// @return iterator to the first unvisited target while following in reverse order the path between the provided coordinates
	boost::optional<LSI> lastUnvisited(const Coord *fromCoord = NULL, const Coord *end = NULL) const;

	/// @return iterator within children pointing to the segment containing 'coord'
	LSI locateCoord(const Coord &coord) const;

	/// @return the coordinate of one end of the child segment pointed by 'it'. Which end depends on 'towardsLowerPartOfBranchlessPath'
	Coord segEndWithinBranchlessPath(LSI it, bool towardsLowerPartOfBranchlessPath) const;

	/**
	Traverses the path:
	- starting from coord 'from' (which belongs to the segment pointed by 'itFrom')
	- in the direction specified by 'towardsLowerPartOfBranchlessPath'
	- until reaching the segment pointed by 'itTo'
	*/
	void traverse(const Coord &from, LSI itFrom, LSI itTo, bool towardsLowerPartOfBranchlessPath,
				  std::shared_ptr<Maze::UiEngine> uiEngine);

	/// Discover which other 0..2 paths (graph vertices) are connected to this path (through its 2 ends)
	void setLinksOwners();

public:
	/// Initialize a path with a seed segment which should expand as long as there are no bifurcations
	BranchlessPath(unsigned id, Segment &firstChild, std::set<Segment*> &orphanSegments, std::map<Coord, PSegmentsPair> &coordOwners);

	inline unsigned id() const { return _id; }

	inline Segment* firstLink() const {
		return links.first;
	}

	inline Segment* secondLink() const {
		return links.second;
	}

	inline bool doesLink() const {
		return (nullptr != firstLink()) || (nullptr != secondLink());
	}

	inline bool doubleLinked() const {
		return (nullptr != firstLink()) && (nullptr != secondLink());
	}

	inline PSegmentsPair theLinks() const {
		return links;
	}

	inline const std::vector<BranchlessPath*> theLinksOwners() const {
		if(linksOwners.empty())
			(const_cast<BranchlessPath*>(this))->setLinksOwners();

		return linksOwners;
	}

	inline bool isLink(const Segment &seg) const {
		return (&seg == firstLink()) || (&seg == secondLink());
	}

	inline bool isFirstLink(const Segment &seg) const {
		bool result = (&seg == firstLink());
		require(result || (&seg == secondLink()), "Provided segm isn't a link for this branchlessPath!");
		return result;
	}

	inline Segment* otherLink(const Segment &seg) const {
		return isFirstLink(seg) ? secondLink() : firstLink();
	}

	inline Coord firstEnd() const {
		return ends.first;
	}

	inline Coord secondEnd() const {
		return ends.second;
	}

	inline bool isEnd(const Coord &coord) const {
		return (coord == firstEnd()) || (coord == secondEnd());
	}

	inline bool isFirstEnd(const Coord &coord) const {
		bool result = (coord == firstEnd());
		require(result || (coord == secondEnd()), "Provided coord is not an end of this branchlessPath!");
		return result;
	}

	inline Coord otherEnd(const Coord &anEnd) const {
		return isFirstEnd(anEnd) ? secondEnd() : firstEnd();
	}

	inline CoordsPair theEnds() const {
		return ends;
	}

	/// @return true if there are unvisited targets between the provided coordinates
	inline bool hasUnvisitedTargets(const Coord *fromCoord = NULL, const Coord *end = NULL) const {
		return lastUnvisited(fromCoord, end).is_initialized();
	}

	/// @return the unvisited targets between the provided coordinates
	std::set<MazeTarget*> getUnvisitedTargets(const Coord *fromCoord = NULL, const Coord *end = NULL) const;

	/// @return does 'segToFind' belong to this path (graph vertex)?
	inline bool containsSegment(const Segment &segToFind) const {
		return segToFind.owner() == this;
	}

	/// @return the segment (of this path) containing a certain coordinate
	Segment* containsCoord(const Coord &coord) const;

	/**
	A traversal in one direction might be performed without visiting the targets from the opposite direction
	if this branchlessPath will be revisited. At that time, there'll be fewer targets, so it's possible
	the postponed journey will mean less traveling (fewer segments to traverse).
	It returns immediately after visiting the farthest target that was out of the way.

	Set visitAllTargets to true the last time you visit a BranchlessPath that still has unvisited targets

	The parameter stopAfterLastTarget can be used for the last target to finish the whole traversal.
	Set stopAfterLastTarget to true for the last traversal (with visitAllTargets == true)
	or when detouring to visit all the targets (with visitAllTargets == false)
	*/
	void traverse(const Coord &from, const Coord &end, std::shared_ptr<Maze::UiEngine> uiEngine,
				  bool visitAllTargets = false, bool stopAfterLastTarget = false);

	std::string toString() const;

	opLessLessRef(BranchlessPath)
};

/**
Graph vertex representing a BranchlessPath (BP).
The walk through the graph can expand freely for any BP, except the common auxiliary added sink BP,
who needs all targets be covered before getting to it
*/
class BpVertexProps {
	unsigned _maxUnvisitedTargets; ///< gets set to 0 ONLY for the auxiliary added sink BP
	BranchlessPath *_forTiltedMaze;	///< the BranchlessPath corresponding to this graph vertex

public:
	int num; ///< vertex id

	/// default constructible expected
	BpVertexProps(int n = 0, BranchlessPath *correspondingBP = nullptr, unsigned maxUnvisited = UINT_MAX) :
		num(n), _forTiltedMaze(correspondingBP), _maxUnvisitedTargets(maxUnvisited) {}

	inline unsigned maxUnvisitedTargets() const { return _maxUnvisitedTargets; }

	inline BranchlessPath* forTiltedMaze() const { return _forTiltedMaze; }

	std::string toString() const;

	opLessLessRef(BpVertexProps)
	opLessLessPtr(BpVertexProps)
};

/// Graph edge
struct BpsArcProps {
	int num; // expected

	// default constructible expected
	BpsArcProps(int n = 0) : num(n) {}
};

// Directed Graph, provided as Adjacency List
typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::directedS, BpVertexProps, BpsArcProps >
	BpAdjacencyList;

/**
ProblemAdapter:
- receives a basic Maze object
- transforms it into a graph
- provides the graph to the solver
*/
class ProblemAdapter {
	std::shared_ptr<Maze> maze;

	std::vector<MazeTarget> targets;	///< required targets to be visited

	std::vector<Segment> hSegments;		///< horizontal segments
	std::vector<Segment> vSegments;		///< vertical segments
	std::set<Segment*> orphanSegments; ///< before grouping the segments into paths (without bifurcations) they are considered orphans
	std::map<Coord, PSegmentsPair> coordOwners;	///< 1..2 (horizontal and/or vertical) segments containing a certain coordinate
	std::vector<std::shared_ptr<BranchlessPath>> branchlessPaths; ///< the (non-bifurcated) paths (graph vertices)
	BpAdjacencyList searchGraph;

	void buildGraph(bool verbose = false);

public:
	ProblemAdapter(std::shared_ptr<Maze> aMaze, bool verbose = false);

	inline const std::shared_ptr<Maze>& getMaze() const { return maze; }
	inline const std::vector<MazeTarget>& getTargets() const { return targets; }
	inline const std::vector<std::shared_ptr<BranchlessPath>>& getBranchlessPaths() const { return branchlessPaths; }
	inline const BpAdjacencyList& getSearchGraph() const { return searchGraph; }
};

/// Keeps the visiting evidence for all targets and assesses the effects of some moves
class Targets {
	typedef std::map< MazeTarget*, std::set< BranchlessPath* > >  TargetBpMapping;

	TargetBpMapping initialTargetBpMapping; ///< location of the targets within BranchlessPath-s (graph vertices)

public:
	void clear() { initialTargetBpMapping.clear(); }

	/// Registers a target covered by a single BranchlessPath (graph vertex)
	void addTarget(MazeTarget &t, BranchlessPath &ownerBp);

	/// version ONLY for targets SHARED by idx1stSharer and idx2ndSharer
	void addTarget(MazeTarget &t, BranchlessPath &sharerBp1, BranchlessPath &sharerBp2);

	/// @return the number of unvisited targets remaining after any walk that covers the BPs within traversedBps
	/// If parameter unvisitedTargets != nullptr, it copies the remaining unvisited targets into it
	unsigned unvisited(const std::set<BranchlessPath*> &traversedBps,
					   std::set<MazeTarget*> *unvisitedTargets = nullptr) const;
};

#endif // H_PROBLEM_ADAPTER