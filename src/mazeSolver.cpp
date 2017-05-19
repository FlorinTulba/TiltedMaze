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

#include "mazeSolver.h"
#include "graph_r_c_shortest_paths.h"

#pragma warning( push, 0 )

#include <conio.h>

#pragma warning( pop )

using namespace std;
using namespace boost;
using namespace boost::graph;

extern Targets graphTargets;

/// expected by check_r_c_path
bool operator==(const BpResCont &rc1, const BpResCont &rc2) {
	// the full walk must correspond when verifying with check_r_c_path
	return rc1.walk == rc2.walk;
}


/// Free operator< used by the priority_queue where the LABELS to be handled are inserted
/// Returns true when rc1 is better than rc2 (it should be processed before rc2)
bool operator<(const BpResCont &rc1, const BpResCont &rc2) {
	// Although it discourages determining short solution walks,
	// the function rc1.moreUniqueTraversedBpsThan(rc2)>0 ensures processing as soon as possible
	// longer walks with more unique BPs traversed, as they could probably cover more targets early on.
	return (rc1.moreUniqueTraversedBpsThan(rc2) > 0);
}

MazeSolver::MazeSolver(const std::string &mazeFile, bool verbose/* = false*/) :
	theMaze(make_shared<Maze>(mazeFile, verbose), verbose) {}

bool MazeSolver::isSolvable(vector< vector< graph_traits< BpAdjacencyList >::edge_descriptor > > 
								*pOpt_solutions_spptw/* = nullptr*/,
							vector< BpResCont > *pPareto_opt_rcs_spptw/* = nullptr*/) const {
	const BpAdjacencyList &searchGraph = theMaze.getSearchGraph();
	const size_t idxStartVertex = theMaze.getBranchlessPaths().size(),
		idxEndVertex = idxStartVertex + 1ULL;

	// spptw
	vector< vector< graph_traits< BpAdjacencyList >::edge_descriptor > >  opt_solutions_spptw;
	vector< BpResCont > pareto_opt_rcs_spptw;
	if(pOpt_solutions_spptw == nullptr)
		pOpt_solutions_spptw = &opt_solutions_spptw;
	if(pPareto_opt_rcs_spptw == nullptr)
		pPareto_opt_rcs_spptw = &pareto_opt_rcs_spptw;

	BpResExtensionFn bpRef;
	BpDominanceFn bpDom;

	r_c_shortest_paths_dispatch_adapted(searchGraph,
										get(&BpVertexProps::num, searchGraph),
										get(&BpsArcProps::num, searchGraph),
										idxStartVertex, idxEndVertex,
										*pOpt_solutions_spptw, *pPareto_opt_rcs_spptw, true,
										BpResCont(), // empty uniqueTraversedBps & walk
										bpRef, bpDom,
										allocator< r_c_shortest_paths_label< BpAdjacencyList, BpResCont> >(),
										BpGraphAlgVisitor());
	
	return pOpt_solutions_spptw->size() != 0ULL;
}

bool MazeSolver::solve(bool consoleMode/* = true*/, bool verbose/* = false*/) const {
	const BpAdjacencyList &searchGraph = theMaze.getSearchGraph();

	// spptw
	vector< vector< graph_traits< BpAdjacencyList >::edge_descriptor > >  opt_solutions_spptw;
	vector< BpResCont > pareto_opt_rcs_spptw;

	if(!isSolvable(&opt_solutions_spptw, &pareto_opt_rcs_spptw)) {
		cout<<"There are no solutions for this maze!"<<endl;

		return false;
	}

	size_t solutionsCount = opt_solutions_spptw.size();

	bool b_is_a_path_at_all = false;
	bool b_feasible = false;
	bool b_correctly_extended = false;
	BpResCont actual_final_resource_levels; // update to what's expected
	graph_traits<BpAdjacencyList>::edge_descriptor ed_last_extended_arc;
	check_r_c_path(searchGraph,
				   opt_solutions_spptw[0],
				   BpResCont(),
				   true,
				   pareto_opt_rcs_spptw[0],
				   actual_final_resource_levels,
				   BpResExtensionFn(),
				   b_is_a_path_at_all,
				   b_feasible,
				   b_correctly_extended,
				   ed_last_extended_arc);

	if(graphTargets.unvisited(actual_final_resource_levels.uniqueTraversedBps) > 0U)
		return false;

	if(!b_is_a_path_at_all || !b_feasible || !b_correctly_extended)
		return false;

	list<BranchlessPath*> firstSolution;
	for(size_t j = 0U, walkLen = opt_solutions_spptw[0].size() - 1; j < walkLen; ++j) {
		const BpVertexProps& aVertexProps =
			get(vertex_bundle, searchGraph)[source(opt_solutions_spptw[0][j], searchGraph)];
		firstSolution.push_front(aVertexProps.forTiltedMaze());
	}

	if(verbose) {
		if(solutionsCount == 1U)
			cout<<"Found a solution: ";
		else {
			cout<<solutionsCount<<" solutions were found. Presenting only 1st one: ";
		}
		for(auto bp : firstSolution) {
			cout<<bp->id()<<' ';
		}
		cout<<endl<<endl;

		int ch = _getch();
		if(ch==0||ch==0xE0) _getch(); // discard any chars left in the console buffer due to pressed function keys
	}

	std::shared_ptr<Maze::UiEngine> uiEngine = theMaze.getMaze()->display(consoleMode);

	// Determine the BranchlessPath-s of the walk where all the targets must be visited before going any further
	// This is the case for every BranchlessPath that appears only once within the solution walk
	// For those that reappear, the last visit is the one that must visit every target.
	const size_t solSz = firstSolution.size();
	vector<BranchlessPath*> solAsVector(BOUNDS_OF(firstSolution));
	set<BranchlessPath*> uniqueBpsTraversed(BOUNDS_OF(solAsVector));
	vector<bool> allTargetsMustBeVisited(solSz, false);

	for(auto uniqueBp : uniqueBpsTraversed) {
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
	Coord fromCoord = theMaze.getMaze()->startLocation(), endCoord = solAsVector[0]->firstEnd();
	if(solSz == 1U) {
		solAsVector[0]->traverse(fromCoord, endCoord, uiEngine, true, true); // stops after visiting all targets

	} else { // solution has at least 2 BP-s
		// determine 1st intersection
		if(false == solAsVector[1]->containsCoord(endCoord))
			endCoord = solAsVector[0]->secondEnd();

		// traverse all except last
		size_t idx = 0, lim = solSz - 1U;
		for(;;) {
			solAsVector[idx]->traverse(fromCoord, endCoord, uiEngine, allTargetsMustBeVisited[idx]);
			fromCoord = endCoord;
			endCoord = solAsVector[++idx]->firstEnd();

			if(idx == lim)
				break;

			if(false == solAsVector[idx + 1]->containsCoord(endCoord))
				endCoord = solAsVector[idx]->secondEnd();
		}

		// traverse last
		solAsVector[lim]->traverse(fromCoord, endCoord, uiEngine, true, true); // stops after visiting all targets
	}

	for(const auto &t : theMaze.getTargets()) {
		require(t.visited(), "All targets should have been visited at the end of the walk!");
	}

	return true;
}

BpResCont& BpResCont::operator=(const BpResCont& other) {
	if(this == &other)
		return *this;

	this->~BpResCont();

	new(this) BpResCont(other); // basically transforms assignment operator into copy construction

	return *this;
}

int BpResCont::lessUnvisitedOrAtLeastShorterWalkThan(const BpResCont &other) const {
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

int BpDominanceFn::operator() (const BpResCont &rc1, const BpResCont &rc2) const {
	// The rc1.lessUnvisitedOrAtLeastShorterWalkThan(rc2) from below is the ideal criterion,
	// but unfortunately, the future (unknown) dictates the dominance:
	// The performance of two walks depends on the BPs that they don't share.
	// So, even if now rc1 is better than rc2, the common path they'll share from this BP on is unknown.
	// If this next path covers just BPs that rc1 already traversed and rc2 didn't =>
	// rc2 would have at the end also the rc1's assets they didn't share initially.
	// This would mean that rc1 won't improve, while rc2 would overpass rc1.
	// As a conclusion, we cannot assert which rc is dominant, but we MUST prevent infinite useless loops,
	// which can be detected as follows:

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

int BpResCont::lessUnvisitedThan(const BpResCont &other) const {
	if(uniqueTraversedBps == other.uniqueTraversedBps) // this means also the unvisited targets count is the same
		return 0;

	// Here the unique BPs of this and other differ,
	// but the count of unvisited targets might be the same
	unsigned unvisitedTargets1 = graphTargets.unvisited(uniqueTraversedBps);
	unsigned unvisitedTargets2 = graphTargets.unvisited(other.uniqueTraversedBps);

	return ::compare(unvisitedTargets2, unvisitedTargets1); // returns 1 if this has less unvisited
}

bool BpResExtensionFn::operator() (const BpAdjacencyList& g,
								   BpResCont& new_cont, const BpResCont& old_cont,
								   graph_traits<BpAdjacencyList>::edge_descriptor ed) const {
	int nextBp = (int)target(ed, g);
	const BpVertexProps& vert_prop = get(vertex_bundle, g)[size_t(nextBp)];
	BranchlessPath *tmNextBp = vert_prop.forTiltedMaze();

	new_cont = old_cont;
	new_cont.walk.push_back(tmNextBp);
	new_cont.uniqueTraversedBps.insert(tmNextBp);

	unsigned unvisited4_new_cont = graphTargets.unvisited(new_cont.uniqueTraversedBps);
	unsigned allowedUnvisitedCountByNextBp = vert_prop.maxUnvisitedTargets(); // this is always infinity, except BPend

	return (unvisited4_new_cont <= allowedUnvisitedCountByNextBp);
}
