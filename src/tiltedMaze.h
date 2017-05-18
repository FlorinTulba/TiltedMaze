/******************************************************************
 Project TiltedMaze solves tilted maze problems.

 You might visit http://www.agame.com/game/tilt-maze
 to try yourself solving such problems (use the arrow keys to move)

 The program is able to load the puzzle from text files.
 
 Solving the maze is presented as a console animation.

 The project uses Boost.

 Copyright (c) 2014, 2017 Florin Tulba

*******************************************************************/

#ifndef H_TILTED_MAZE
#define H_TILTED_MAZE

#include "various.h"
#include "environ.h"
#include "conditions.h"
#include "consoleOps.h"
#include "graph_r_c_shortest_paths.h"

#pragma warning ( push, 0 )

#include <fstream>
#include <string>
#include <set>
#include <map>
#include <list>
#include <vector>

#include <boost/icl/split_interval_set.hpp>
#include <boost/optional.hpp>
#include <boost/graph/adjacency_list.hpp>

#pragma warning ( pop )

/// Loader and solver of tilted mazes
class TiltedMaze {
public:
	// forward declaration
	struct Coord;
	class BranchlessPath;
	class Segment;

private:
	typedef std::pair<Coord, Coord> CoordsPair;
	typedef std::pair<unsigned, unsigned> UUpair;
	typedef std::multimap<BranchlessPath*, BranchlessPath*>::value_type BBpair;

public:
	typedef std::pair<Segment*, Segment*> PSegmentsPair; ///< Pair of pointers to segments

	/// Maze position
	struct Coord {
		unsigned row; ///< row (y) coordinate
		unsigned col; ///< column (x) coordinate

		Coord(unsigned row = UINT_MAX, unsigned col = UINT_MAX) : row(row), col(col) {}

		void reset() {row = col = UINT_MAX;}

		inline bool operator<(const Coord& other) const {
			return (row < other.row) ||
				((row == other.row) && (col < other.col));
		}

		inline bool operator==(const Coord& other) const {
			return (row == other.row) && (col == other.col);
		}

		std::string toString() const;

		opLessLessRef(Coord)
	};


	/// Checkpoint within the maze that should be visited when solving the problem
	class Target : public Coord {
		bool _visited;			///< visited already or not
		PSegmentsPair visitors;	///< a target might be visited several times, but only from horizontal or vertical directions

	public:
		Target(unsigned row = UINT_MAX, unsigned col = UINT_MAX) : Coord(row, col), _visited(false) {}
		Target(const Coord &c) : Coord(c.row, c.col), _visited(false) {}

		/// Lets the target know which of the horizontal / vertical segments were used when visiting it
		inline void setVisitors(PSegmentsPair &theVisitors) {visitors = theVisitors;}

		/// Let the visitor (horizontal and/or vertical) segments mark this target as visited
		void visit();
		
		inline bool visited() const {return _visited;}
	};

	/// Traversable segment of the maze (wall to wall)
	class Segment {

		/// Mapping between segment coordinates (the variable 1D coordinates) and target objects
		typedef std::map<unsigned, Target*> UTargetMap;
		typedef std::pair<UTargetMap::const_iterator, UTargetMap::const_iterator> RangeUnvisited;

		bool _isHorizontal;		///< is this a horizontal or vertical segment
		unsigned fixedIndex;	///< for horizontal segments, the 'row' coordinate is fixed; for vertical ones, the 'column' is fixed
		boost::icl::closed_interval<unsigned>::type closedInterval;	///< the limit 1D coordinates for the non-fixed part of the 2D coordinate

		std::vector<Target*> managedTargets;		///< the targets lying on this segment
		mutable UTargetMap varDimUnvisitedTargets;	///< the map between the segment coordinates of unvisited targets and these targets (the key is the variable 1D coordinate of the target on the segment)

		BranchlessPath *parent;	///< the path (graph vertex) containing this segment

		/// @return a map of unvisited targets between the 2 coordinates found on a horizontal / vertical line
		RangeUnvisited unvisitedTargets(const Coord *from = NULL, const Coord *end = NULL) const;

	public:

		Segment() : _isHorizontal(false), fixedIndex(UINT_MAX), parent(NULL) {}

		Segment(const Coord &coord1, const Coord &coord2);

		Segment(unsigned fixedIndex, const boost::icl::interval<unsigned>::type &rightOpenInterval, bool isHorizontal = true);

		inline bool operator==(const Segment &other) const {
			return (_isHorizontal == other._isHorizontal) && (fixedIndex == other.fixedIndex) && (closedInterval == other.closedInterval);
		}

		inline unsigned fixedCoord() const {return fixedIndex;}

		inline bool isHorizontal() const {return _isHorizontal;}

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

		bool hasUnvisitedTargets(const Coord *from = NULL, const Coord *end = NULL) const;

		std::set<Target*> getUnvisitedTargets(const Coord *from = NULL, const Coord *end = NULL) const;

		/// The segment becomes aware of a certain target found on itself
		void manageTarget(Target &target);

		/// The segment should know that the target provided as parameter (lying on this segment) was visited
		void ackVisit(const Target &target);

		/// This segment is visited between from and end. Provide either both or none of these parameters.
		void traverse(const Coord *from = NULL, const Coord *end = NULL);

		inline bool hasOwner() const {return NULL != parent;}
		inline BranchlessPath* owner() const {return parent;}
		void setOwner(BranchlessPath *e) {parent = e;}

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
		TiltedMaze &theMaze;

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
		void traverse(const Coord &from, LSI itFrom, LSI itTo, bool towardsLowerPartOfBranchlessPath);

		/// Discover which other 0..2 paths (graph vertices) are connected to this path (through its 2 ends)
		void setLinksOwners();

	public:
		/// Initialize a path with a seed segment which should expand as long as there are no bifurcations
		BranchlessPath(unsigned id, Segment &firstChild, TiltedMaze &maze);

		inline unsigned id() const {return _id;}

		inline Segment* firstLink() const {
			return links.first;
		}

		inline Segment* secondLink() const {
			return links.second;
		}

		/// Some paths are dead end paths - there is no escape from it and returning isn't an option either
		inline bool doesLink() const {
			return (NULL != firstLink()) || (NULL != secondLink());
		}

		inline bool doubleLinked() const {
			return (NULL != firstLink()) && (NULL != secondLink());
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
		std::set<Target*> getUnvisitedTargets(const Coord *fromCoord = NULL, const Coord *end = NULL) const;

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
		void traverse(const Coord &from, const Coord &end,
						bool visitAllTargets = false, bool stopAfterLastTarget = false);

		std::string toString() const;

		opLessLessRef(BranchlessPath)
	};

private:
	unsigned rowsCount;				///< maze height
	unsigned columnsCount;			///< maze width
	Coord startLocation;			///< the game requires starting from this location
	std::vector<Target> targets;	///< required targets to be visited

	std::vector<boost::icl::split_interval_set<unsigned>> rows;		///< limits for the horizontal segments
	std::vector<boost::icl::split_interval_set<unsigned>> columns;	///< limits for the vertical segments
	std::vector<Segment> hSegments;		///< horizontal segments
	std::vector<Segment> vSegments;		///< vertical segments
	std::map<Coord, PSegmentsPair> coordOwners;	///< 1..2 (horizontal and/or vertical) segments containing a certain coordinate
	std::set<Segment*> orphanSegments;	///< before grouping the segments into paths (without bifurcations) they are considered orphans
	std::vector<std::shared_ptr<BranchlessPath>> branchlessPaths;	///< the (non-bifurcated) paths (graph vertices)

	int displayOriginRow;		///< Save the initial cursor row to be able to redraw the maze from the same location
	int displayOriginCol;		///< Save the initial cursor column to be able to redraw the maze from the same location
	ConsoleColor origFgColor;	///< Save initial foreground color, to be able to revert it at exit
	ConsoleColor origBgColor;	///< Save initial background color, to be able to revert it at exit
	ConsoleColor wallColor;		///< color used for walls
	ConsoleColor targetColor;	///< color indicating targets
	char targetCh;				///< the symbol used for representing targets within the maze

	/// @return next line from a file. At EOF returns none.
	boost::optional<std::string> nextLine(std::ifstream &ifs) const;

	void buildGraph(); ///< Creates the graph corresponding to the maze

	void initDisplaySettings();		///< Prepares the console animation of the solution
	void revertDisplaySettings();	///< Reverts to the settings from before the animation

public:
	/// Helper for computing console screen coordinates
	static inline int displayIndex(unsigned clientIdx, bool isRow);

	class Targets; // forward declaration

private:
	/**
	Graph vertex representing a BranchlessPath (BP).
	The walk through the graph can expand freely for any BP, except the common auxiliary added sink BP,
	who needs all targets be covered before getting to it
	*/
	class BpVertexProps {
		unsigned _maxUnvisitedTargets; ///< gets set to 0 ONLY for the auxiliary added sink BP
		TiltedMaze::BranchlessPath *_forTiltedMaze;	///< the BranchlessPath corresponding to this graph vertex

	public:
		int num; ///< vertex id

		/// default constructible expected
		BpVertexProps(int n = 0, TiltedMaze::BranchlessPath *correspondingBP = NULL, unsigned maxUnvisited = UINT_MAX) :
			num(n), _forTiltedMaze(correspondingBP), _maxUnvisitedTargets(maxUnvisited) {}

		inline unsigned maxUnvisitedTargets() const {return _maxUnvisitedTargets;}

		inline TiltedMaze::BranchlessPath* forTiltedMaze() const {return _forTiltedMaze;}

		std::string toString() const;

		opLessLessRef(BpVertexProps)
		opLessLessPtr(BpVertexProps)
	};

	/// Graph edge
	struct BpsArcProps {
		int num; ///< edge id

		/// default constructible expected
		BpsArcProps(int n = 0) : num(n) {}
	};

	/// Directed Graph, provided as Adjacency List
	typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::directedS, BpVertexProps, BpsArcProps >
		BpAdjacencyList;

	BpAdjacencyList searchGraph; ///< the graph as adjacency list

public:
	/// Keeps the visiting evidence for all targets and assesses the effects of some moves
	class Targets {
		typedef std::map< TiltedMaze::Target*, std::set< TiltedMaze::BranchlessPath* > >  TargetBpMapping;

		TargetBpMapping initialTargetBpMapping; ///< location of the targets within BranchlessPath-s (graph vertices)

	public:
		void clear() {initialTargetBpMapping.clear();}

		/// Registers a target covered by a single BranchlessPath (graph vertex)
		void addTarget(TiltedMaze::Target &t, TiltedMaze::BranchlessPath &ownerBp);

		/// version ONLY for targets SHARED by idx1stSharer and idx2ndSharer
		void addTarget(TiltedMaze::Target &t,
					   TiltedMaze::BranchlessPath &sharerBp1, TiltedMaze::BranchlessPath &sharerBp2);

		/// @return the number of unvisited targets remaining after any walk that covers the BPs within traversedBps
		/// If parameter unvisitedTargets != NULL, it copies the remaining unvisited targets into it
		unsigned unvisited(const std::set<TiltedMaze::BranchlessPath*> &traversedBps,
						   std::set<TiltedMaze::Target*> *unvisitedTargets = NULL) const;
	};

private:
	/**
	ResourceContainer
	Each LABEL manipulated by the algorithm holds such a ResourceContainer.
	All decision making is based on this structure:
	- all edges get validated (in BpResExtensionFn) except for the ones towards the sink BP,
			where all targets must be already visited
	- LABELS associated with a given BP during an inferior distinct walk get marked as dominated (BpDominanceFn)
			and they get removed
	*/
	struct BpResCont {
		std::list< TiltedMaze::BranchlessPath* >  walk; ///< the current path; its length matters for BpDominanceFn
		std::set< TiltedMaze::BranchlessPath* >  uniqueTraversedBps; ///< the unique BPs within the walk

		BpResCont& operator=(const BpResCont& other);

		/// for shorter this.walk => 1
		inline int shorterWalkThan(const BpResCont &other) const {
			return ::compare(other.walk.size(), walk.size());
		}

		/**
		A walk (represented through BpResCont) is:
		- superior to another if it covered more targets, or the same count, but within less steps (a shorter walk)
		- equal to another if both, the targets count and the walk's length are the same, even for different walks

		Returns:
				-1 if this is worse than other ; 0 if equal ; 1 if this is better than other
		justEquality parameter speeds up the control for equality
		*/
		int lessUnvisitedOrAtLeastShorterWalkThan(const BpResCont &other) const;

		/// Compare version that ignores the walk length
		int lessUnvisitedThan(const BpResCont &other) const;

		/// @return true if current walk contains more unique BranchlessPath-s (graph vertices) than the walk performed by other
		inline int moreUniqueTraversedBpsThan(const BpResCont &other) const {
			return ::compare(uniqueTraversedBps.size(), other.uniqueTraversedBps.size());
		}
	};

	/// DominanceFunction model
	struct BpDominanceFn {
		/**
		Compares for dominance 2 BpResCont-s
		Returns:
		 1 if rc1 dominates over rc2
		 0 if rc1 and rc2 don't dominate each other
		-1 if rc1 is dominated by rc2
		*/
		int operator() (const BpResCont &rc1, const BpResCont &rc2) const;
	};

	/// ResourceExtensionFunction model
	struct BpResExtensionFn {
		/// Tackles the feasibility of a new edge and fills in the required data for the reached BP
		bool operator() (const BpAdjacencyList& g, BpResCont& new_cont, const BpResCont& old_cont, 
			boost::graph_traits<BpAdjacencyList>::edge_descriptor ed) const;
	};

	/// A visitor that might consult the graph at some key points during the algorithm
	struct BpGraphAlgVisitor {
		template<class Label, class Graph>
		inline void on_label_popped(const Label &/*l*/, const Graph &/*g*/) {
/*
			std::cout<<"    Popped label: ";
			for(const BranchlessPath *bp : l.cumulated_resource_consumption.walk) {
				if(NULL != bp)
					cout<<bp->id()<<' ';
			}
			std::cout<<std::endl;
*/
		}

		template<class Label, class Graph>
		inline void on_label_feasible( const Label&, const Graph& ) {}

		template<class Label, class Graph>
		inline void on_label_not_feasible( const Label&, const Graph& ) {}

		template<class Label, class Graph>
		inline void on_label_dominated( const Label &/*l*/, const Graph &) {
/*
			std::cout<<"    Dominated label: ";
			for(const BranchlessPath *bp : l.cumulated_resource_consumption.walk) {
				if(NULL != bp)
					cout<<bp->id()<<' ';
			}
			std::cout<<std::endl;
*/
		}

		template<class Label, class Graph>
		inline void on_label_not_dominated( const Label &/*l*/, const Graph &) {
/*
			std::cout<<"NOT Dominated label: ";
			for(const BranchlessPath *bp : l.cumulated_resource_consumption.walk) {
				if(NULL != bp)
					cout<<bp->id()<<' ';
			}
			std::cout<<std::endl;
*/
		}

		template<class Queue, class Graph>             
		inline bool on_enter_loop(const Queue&, const Graph&) {return true;}
	};

public:
	TiltedMaze(const std::string &mazeFile);

	void displayMaze(); ///< Shows the initial frame of the console animation

	bool solve(bool graphicMode = true); ///< Computes a solution and optionally animates it within the console

	/// expected by check_r_c_path
	friend inline bool operator==(const TiltedMaze::BpResCont &rc1, const TiltedMaze::BpResCont &rc2) {
		// the full walk must correspond when verifying with check_r_c_path
		return rc1.walk == rc2.walk;
	}

	/// Free operator< used by the priority_queue where the LABELS to be handled are inserted
	/// Returns true when rc1 is better than rc2 (it should be processed before rc2)
	friend inline bool operator<(const TiltedMaze::BpResCont &rc1, const TiltedMaze::BpResCont &rc2) {
		// Although it discourages determining short solution walks,
		// the function rc1.moreUniqueTraversedBpsThan(rc2)>0 ensures processing as soon as possible
		// longer walks with more unique BPs traversed, as they could probably cover more targets early on.
		return (rc1.moreUniqueTraversedBpsThan(rc2) > 0);
	}
};

#endif // H_TILTED_MAZE