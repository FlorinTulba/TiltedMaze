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

#ifndef H_MAZE_SOLVER
#define H_MAZE_SOLVER

#include "problemAdapter.h"

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
	std::list< BranchlessPath* >  walk; ///< the current path; its length matters for BpDominanceFn
	std::set< BranchlessPath* >  uniqueTraversedBps; ///< the unique BPs within the walk

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
	inline void on_label_feasible(const Label&, const Graph&) {}

	template<class Label, class Graph>
	inline void on_label_not_feasible(const Label&, const Graph&) {}

	template<class Label, class Graph>
	inline void on_label_dominated(const Label &/*l*/, const Graph &) {
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
	inline void on_label_not_dominated(const Label &/*l*/, const Graph &) {
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
	inline bool on_enter_loop(const Queue&, const Graph&) { return true; }
};

/// Loads and solves a maze
class MazeSolver {
	ProblemAdapter theMaze;

public:
	/// Loads the mazeFile and adapts it for the Boost graph algorithms
	MazeSolver(const std::string &mazeFile, bool verbose = false);

	/// Checks if the maze is solvable. Both parameters might be nullptr (default) when called by foreign code
	bool isSolvable(std::vector< std::vector< boost::graph_traits< BpAdjacencyList >::edge_descriptor > >
						*pOpt_solutions_spptw = nullptr,
					std::vector< BpResCont > *pPareto_opt_rcs_spptw = nullptr) const;

	bool solve(bool consoleMode = true, bool verbose = false) const;
};

#endif // H_MAZE_SOLVER