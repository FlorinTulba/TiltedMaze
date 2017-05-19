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

#include "problemAdapter.h"

#pragma warning( push, 0 )

#include <iterator>

#pragma warning( pop )

using namespace std;
using namespace boost;
using namespace boost::icl;
using namespace boost::graph;

Targets graphTargets;

ProblemAdapter::ProblemAdapter(std::shared_ptr<Maze> aMaze, bool verbose/* = false*/) :
		maze(aMaze),
		hSegments(), vSegments(),
		orphanSegments(),
		coordOwners(),
		branchlessPaths(),
		targets(aMaze->targets().begin(), aMaze->targets().end()),
		searchGraph() {
	buildGraph(verbose);
}

void ProblemAdapter::buildGraph(bool verbose/* = false*/) {
	// determine the number of horizontal segments
	unsigned hSegmentsCount = 0U, vSegmentsCount = 0U, i;
	for(const auto &sis : maze->rows()) {
		hSegmentsCount += (unsigned)sis.iterative_size();
	}
	hSegments.reserve(hSegmentsCount);

	i = 0U;
	for(const auto &sis : maze->rows()) {
		for(const auto &iut : sis) {
			if(iut.upper() - iut.lower() > 1U) { // single cells don't constitute segments
				hSegments.emplace_back(i, iut);
				Segment &seg = hSegments.back();
				orphanSegments.insert(&seg);
				for(unsigned j = iut.lower(), jLim = iut.upper(); j < jLim; ++j)
					coordOwners[Coord(i, j)].first = &seg;
			}
		}
		++i;
	}

	if(verbose) {
		cout<<"custom_delims<ContDelims<>>(hSegments) = ";
		copy(CONST_BOUNDS_OF(hSegments), ostream_iterator<Segment>(cout, ", "));
		cout<<endl;
	}

	// determine the number of vertical segments
	for(const auto &sis : maze->columns()) {
		vSegmentsCount += (unsigned)sis.iterative_size();
	}
	vSegments.reserve(vSegmentsCount);

	i = 0U;
	for(const auto &sis : maze->columns()) {
		for(const auto &iut : sis) {
			if(iut.upper() - iut.lower() > 1U) { // single cells don't constitute segments
				vSegments.emplace_back(i, iut, false);
				Segment &seg = vSegments.back();
				orphanSegments.insert(&seg);
				for(unsigned j = iut.lower(), jLim = iut.upper(); j < jLim; ++j)
					coordOwners[Coord(j, i)].second = &seg;
			}
		}
		++i;
	}

	if(verbose) {
		cout<<"custom_delims<ContDelims<>>(vSegments) = ";
		copy(CONST_BOUNDS_OF(vSegments), ostream_iterator<Segment>(cout, ", "));
		cout<<endl;
	}


	// placing the targets on the appropriate segments
	for(auto &targetCoord : targets) {
		PSegmentsPair hvSegments = coordOwners[targetCoord];
		Segment *hSeg = hvSegments.first, *vSeg = hvSegments.second;
		bool hFound = (nullptr != hSeg), vFound = (nullptr != vSeg);
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
		std::shared_ptr<BranchlessPath> pBranchlessPath = make_shared<BranchlessPath>(i, *(*it), orphanSegments, coordOwners);
		branchlessPaths.push_back(pBranchlessPath);
	}

	if(verbose) {
		for(auto e : branchlessPaths) {
			cout<<string(60, '=')<<endl;
			cout<<*e<<endl;
		}
	}

	// introducing the targets into the required structure
	graphTargets.clear();
	for(auto &targetCoord : targets) {
		BranchlessPath *bp = nullptr;
		PSegmentsPair hvSegments = coordOwners[targetCoord];
		Segment *seg = hvSegments.first;
		if(nullptr != seg) {
			bp = seg->owner();
			graphTargets.addTarget(targetCoord, *bp);
		}
		seg = hvSegments.second;
		if(nullptr != seg) {
			bp = seg->owner();
			graphTargets.addTarget(targetCoord, *bp);
		}
	}


	// CREATE THE GRAPH
	// THE VERTICES
	for(auto pVertex : branchlessPaths) {
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
	add_vertex(BpVertexProps(idxEndVertex, nullptr, 0U), searchGraph);

	// THE EDGES
	int edgeIdx = 0;

	// there are 1/2 BP-s to start from (the Start coordinate belongs both to 1 or 2 BPs)
	PSegmentsPair hvSegsForStart = coordOwners[maze->startLocation()];
	Segment *seg = hvSegsForStart.first;
	if(nullptr != seg) {
		add_edge((size_t)idxStartVertex, (size_t)seg->owner()->id(), BpsArcProps(edgeIdx++), searchGraph);
	}
	seg = hvSegsForStart.second;
	if(nullptr != seg) {
		add_edge((size_t)idxStartVertex, (size_t)seg->owner()->id(), BpsArcProps(edgeIdx++), searchGraph);
	}

	for(auto pVertex : branchlessPaths) {
		int fromIdx = (int)pVertex->id();
		for(auto bp : pVertex->theLinksOwners()) {
			add_edge((size_t)fromIdx, (size_t)bp->id(), BpsArcProps(edgeIdx++), searchGraph);
		}

		// Adding the virtual (auxiliary) edge towards the End vertex
		add_edge((size_t)fromIdx, (size_t)idxEndVertex, BpsArcProps(edgeIdx++), searchGraph);
	}

	if(verbose)
		cout<<"Graph built!"<<endl;
}

void Targets::addTarget(MazeTarget &t, BranchlessPath &ownerBp) {
	initialTargetBpMapping[&t].insert(&ownerBp);
}

// version ONLY for targets SHARED by idx1stSharer and idx2ndSharer
void Targets::addTarget(MazeTarget &t,
									BranchlessPath &sharerBp1, BranchlessPath &sharerBp2) {
	initialTargetBpMapping[&t] = set< BranchlessPath* >{&sharerBp1, &sharerBp2};
}

// Returns the number of unvisited targets remaining after any walk that covers the BPs within traversedBps
// If parameter unvisitedTargets != nullptr, it copies the remaining unvisited targets into it
unsigned Targets::unvisited(const set<BranchlessPath*> &traversedBps,
										set<MazeTarget*> *unvisitedTargets/* = nullptr*/) const {
	bool reportsTargetsToo = (unvisitedTargets != nullptr);
	unsigned result = 0U;

	set<BranchlessPath*> relevantBps;
	for(auto tBpM : initialTargetBpMapping) {
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

void MazeTarget::visit() {
	_visited = true;

	if(visitors.first != nullptr)
		visitors.first->ackVisit(*this);

	if(visitors.second != nullptr)
		visitors.second->ackVisit(*this);
}

Segment::Segment(const Coord &coord1, const Coord &coord2) : parent(nullptr) {
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

Segment::Segment(unsigned fixedIndex, const interval<unsigned>::type &rightOpenInterval, bool isHorizontal/* = true*/) :
_isHorizontal(isHorizontal), fixedIndex(fixedIndex), parent(nullptr),
closedInterval(closed_interval<unsigned>(rightOpenInterval.lower(), rightOpenInterval.upper() - 1U)) {}

bool Segment::containsCoord(unsigned row, unsigned col, bool exceptEnds/* = false*/) const {
	unsigned fi = col, nfi = row;
	if(_isHorizontal)
		swap(fi, nfi);

	if(fi != fixedIndex)
		return false;

	if(exceptEnds)
		return contains(open_interval<unsigned>(closedInterval.lower(), closedInterval.upper()), nfi);

	return  contains(closedInterval, nfi);
}

Segment::RangeUnvisited Segment::unvisitedTargets(const Coord *from /* = nullptr*/, const Coord *end /* = nullptr*/) const {
	UTargetMap::const_iterator itEnd = varDimUnvisitedTargets.end();
	if(varDimUnvisitedTargets.empty())
		return make_pair(itEnd, itEnd);

	// varDimUnvisitedTargets is here non-empty

	bool limitsProvided = (from != nullptr);
	require(((end != nullptr) == limitsProvided), "Either both parameter or none must be nullptr!");

	if(false == limitsProvided)
		return make_pair(varDimUnvisitedTargets.begin(), itEnd);

	// limitsProvided is here true

	bool isEndTheLowerEnd = isLowerEnd(*end);
	UNREFERENCED_VAR(isEndTheLowerEnd);
	require(containsCoord(*from), "From doesn't belong to this segment!");


	const unsigned *varFrom = nullptr;
	const unsigned *varEnd = nullptr;
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

Coord Segment::nextToEnd(const Coord &end) const {
	bool is1stEnd = isLowerEnd(end);
	unsigned varDim = (is1stEnd ? (closedInterval.lower() + 1U) : (closedInterval.upper() - 1U));
	if(_isHorizontal)
		return Coord(fixedIndex, varDim);

	return Coord(varDim, fixedIndex);
}

bool Segment::hasUnvisitedTargets(const Coord *from/* = nullptr*/, const Coord *end/* = nullptr*/) const {
	RangeUnvisited rangeUnvisited = unvisitedTargets(from, end);
	return distance(rangeUnvisited.first, rangeUnvisited.second) > 0;
}

set<MazeTarget*> Segment::getUnvisitedTargets(const Coord *from/* = nullptr*/, const Coord *end/* = nullptr*/) const {
	set<MazeTarget*> result;
	RangeUnvisited rangeUnvisited = unvisitedTargets(from, end);
	UTargetMap::const_iterator it = rangeUnvisited.first, itEnd = rangeUnvisited.second;
	for(; it != itEnd; ++it) {
		result.insert(it->second);
	}
	return result;
}

void Segment::manageTarget(MazeTarget &target) {
	require(containsCoord(target), "Provided MazeTarget can't be on this segment!");
	managedTargets.push_back(&target);
	unsigned varDim = (_isHorizontal ? target.col : target.row);
	varDimUnvisitedTargets[varDim] = &target;
}

void Segment::ackVisit(const MazeTarget &target) {
	require(containsCoord(target), "Provided MazeTarget can't be on this segment!");
	const unsigned varDim = (_isHorizontal ? target.col : target.row);
	require(1U == varDimUnvisitedTargets.erase(varDim), "No such target was found!");
}

void Segment::traverse(const Coord *from/* = nullptr*/, const Coord *end/* = nullptr*/) {
	RangeUnvisited rangeUnvisited = unvisitedTargets(from, end);
	while(rangeUnvisited.first != rangeUnvisited.second) {
		MazeTarget *target = rangeUnvisited.first->second;
		require(nullptr != target, "Found nullptr target!");
		require(false == target->visited(), "This target should have been not visited yet");
		++rangeUnvisited.first; // to escape the invalidation triggered by next call
		target->visit();
	}
}

string Segment::toString() const {
	ostringstream oss;
	oss<<"[ ";
	if(_isHorizontal)
		oss<<fixedIndex<<","<<closedInterval.lower()<<" - "<<fixedIndex<<","<<closedInterval.upper();
	else
		oss<<closedInterval.lower()<<","<<fixedIndex<<" - "<<closedInterval.upper()<<","<<fixedIndex;
	oss<<" ]";
	return oss.str();
}

void BranchlessPath::expand(const Coord &anEnd, bool horizDir, bool afterSeed/* = true*/) {
	PSegmentsPair hvSegments = _coordOwners[anEnd];
	Segment *seg = (horizDir ? hvSegments.first : hvSegments.second);
	if(nullptr == seg)
		return;

	void (list<Segment*>::*listInsertMethod)(Segment * const &);
	Coord *endToUpdate = nullptr;
	Segment **linkToUpdate = nullptr;
	if(afterSeed) {
		endToUpdate = &ends.second;
		linkToUpdate = &links.second;
		listInsertMethod = &list<Segment*>::push_back;
	} else {
		endToUpdate = &ends.first;
		linkToUpdate = &links.first;
		listInsertMethod = &list<Segment*>::push_front;
	}

	// anEnd belongs for sure to *seg
	if(false == seg->containsCoord(anEnd, true)) { // and one of its ends meets the provided anEnd
		if(_orphanSegments.find(seg) != _orphanSegments.end()) {
			// we found a new child:
			seg->setOwner(this);
			_orphanSegments.erase(seg);
			(children.*listInsertMethod)(seg);
			*endToUpdate = seg->otherEnd(anEnd);
			expand(*endToUpdate, !horizDir, afterSeed);
		}
	} else { // anEnd falls within seg and doesn't meet its ends
		*linkToUpdate = seg;
	}
}

BranchlessPath::LSI BranchlessPath::whichSegment(const Segment &seg) const {
	require(seg.owner() == this, "The provided segment doesn't belong to this branchlessPath!");
	LSI itEnd = children.end(), it = children.begin();
	for(; it != itEnd; ++it)
		if(*it == &seg)
			return it;

	return itEnd; // shouldn't be reachable
}

BranchlessPath::BranchlessPath(unsigned id, Segment &firstChild, set<Segment*> &orphanSegments, map<Coord, PSegmentsPair> &coordOwners) : _id(id), _orphanSegments(orphanSegments), _coordOwners(coordOwners) {
	require(firstChild.hasOwner() == false, "Cannot create an BranchlessPath using a Segment that already belongs to an BranchlessPath!");
	firstChild.setOwner(this);
	ends = firstChild.ends();
	_orphanSegments.erase(&firstChild);
	children.push_back(&firstChild);
	bool is1stChildHorizontal = firstChild.isHorizontal();
	expand(ends.first, !is1stChildHorizontal, false);
	expand(ends.second, !is1stChildHorizontal);
}

optional<BranchlessPath::LSI> BranchlessPath::lastUnvisited(const Coord *fromCoord/* = nullptr*/, const Coord *end/* = nullptr*/) const {
	bool nonNullFrom = (nullptr != fromCoord);
	require((end != nullptr) == nonNullFrom, "From and end should be both either nullptr or valid pointers!");
	if(false == nonNullFrom) {
		fromCoord = &ends.first;
		end = &ends.second;
	}

	bool secondEndAsEnd = !isFirstEnd(*end);

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

Segment* BranchlessPath::containsCoord(const Coord &coord) const {
	PSegmentsPair hvSegments = _coordOwners[coord];
	Segment *hSeg = hvSegments.first, *vSeg = hvSegments.second;
	bool hFound = (nullptr != hSeg), vFound = (nullptr != vSeg);
	require(hFound || vFound, "At least one segment should cover each Coord!");
	if(hFound && containsSegment(*hSeg))
		return hSeg;

	if(vFound && containsSegment(*vSeg))
		return vSeg;

	return nullptr;
}

BranchlessPath::LSI BranchlessPath::locateCoord(const Coord &coord) const {
	Segment *seg = containsCoord(coord);
	require(nullptr != seg, "Coord must belong to this branchlessPath!");
	return whichSegment(*seg);
}

Coord BranchlessPath::segEndWithinBranchlessPath(LSI it, bool towardsLowerPartOfBranchlessPath) const {
	if((it == children.cbegin()) && towardsLowerPartOfBranchlessPath)
		return firstEnd();

	if((it == --children.cend()) && (false == towardsLowerPartOfBranchlessPath))
		return secondEnd();

	Segment *neighbour = nullptr, *self = *it;
	updateLSI(it, towardsLowerPartOfBranchlessPath);
	neighbour = *it;

	Coord lowerEnd = self->lowerEnd();

	return (neighbour->containsCoord(lowerEnd) ? lowerEnd : self->upperEnd());
}

set<MazeTarget*> BranchlessPath::getUnvisitedTargets(const Coord *fromCoord/* = nullptr*/, const Coord *end/* = nullptr*/) const {
	bool nonNullFrom = (nullptr != fromCoord);
	require((end != nullptr) == nonNullFrom, "From and end should be both either nullptr or valid pointers!");

	if(false == nonNullFrom) {
		fromCoord = &ends.first;
		end = &ends.second;
	}

	bool firstEndAsEnd = isFirstEnd(*end);

	if(firstEndAsEnd) {
		swap(fromCoord, end);
	}

	set<MazeTarget*> result;
	Coord segEnd;
	LSI it = locateCoord(*fromCoord), itEnd = locateCoord(*end);

	if(it == itEnd) { // if the path of interest contains only one segment
		segEnd = segEndWithinBranchlessPath(it, firstEndAsEnd);
		result = (*it)->getUnvisitedTargets(fromCoord, &segEnd);

		return result;
	}

	set<MazeTarget*> segTargets;
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

void BranchlessPath::updateLSI(LSI &it, bool towardsLowerPartOfBranchlessPath) const {
	if(towardsLowerPartOfBranchlessPath)
		--it;
	else
		++it;
}

void BranchlessPath::setLinksOwners() {
	bool has1stLinkSeg = (links.first != nullptr);
	bool has2ndLinkSeg = (links.second != nullptr);

	linksOwners.reserve((has1stLinkSeg && has2ndLinkSeg) ? 2ULL : 1ULL);

	if(has1stLinkSeg)
		linksOwners.push_back(links.first->owner());

	if(has2ndLinkSeg)
		linksOwners.push_back(links.second->owner());
}

void BranchlessPath::traverse(const Coord &from, LSI itFrom, LSI itTo, bool towardsLowerPartOfBranchlessPath, std::shared_ptr<Maze::UiEngine> uiEngine) {
	Segment *seg = *itFrom;
	Coord segEnd = segEndWithinBranchlessPath(itFrom, towardsLowerPartOfBranchlessPath);
	if(from != segEnd) {
		seg->traverse(&from, &segEnd);
		uiEngine->drawMove(from, segEnd);
	}

	if(itFrom != itTo) {
		for(updateLSI(itFrom, towardsLowerPartOfBranchlessPath);
			itTo != itFrom;
			updateLSI(itFrom, towardsLowerPartOfBranchlessPath)) {
			seg = *itFrom;
			seg->traverse();
			segEnd = seg->otherEnd(segEnd);
			uiEngine->drawMove(seg->otherEnd(segEnd), segEnd);
		}

		seg = *itTo;
		seg->traverse();
		segEnd = seg->otherEnd(segEnd);
		uiEngine->drawMove(seg->otherEnd(segEnd), segEnd);
	}
}

// A traversal in one direction might be performed without visiting the targets from the opposite direction
// if this branchlessPath will be revisited. At that time, there'll be fewer targets, so it's possible
// the postponed journey will mean less traveling (fewer segments to traverse).
// It returns immediately after visiting the farthest target that was out of the way.
// 
// The parameter stopAfterLastTarget can be used for the last target to finish the whole traversal
// or when detouring to visit all the targets
void BranchlessPath::traverse(const Coord &from, const Coord &end,

							  std::shared_ptr<Maze::UiEngine> uiEngine,

							  // set visitAllTargets to true the last time you visit a BranchlessPath
							  // that still has unvisited targets
							  bool visitAllTargets/* = false*/,

							  // set stopAfterLastTarget to true for the last traversal (with visitAllTargets == true)
							  // or when detouring to visit all the targets (with visitAllTargets == false)
							  bool stopAfterLastTarget/* = false*/) {

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
			isEndThe1stEnd = !isEndThe1stEnd;
		}
	}

	if(visitAllTargets && oItLastUnvisitedOtherEnd) {
		traverse(from, itStart, *oItLastUnvisitedOtherEnd, !isEndThe1stEnd, uiEngine);

		itStart = *oItLastUnvisitedOtherEnd;
		start = segEndWithinBranchlessPath(itStart, !isEndThe1stEnd);
	}

	if(stopAfterLastTarget && oItLastUnvisitedEnd) {
		itFinish = *oItLastUnvisitedEnd;
		finish = segEndWithinBranchlessPath(itFinish, isEndThe1stEnd);
	}

	traverse(start, itStart, itFinish, isEndThe1stEnd, uiEngine);
}

string BranchlessPath::toString() const {
	ostringstream oss;
	oss<<"BranchlessPath "<<_id;
	set<MazeTarget*> unvisitedTargets = getUnvisitedTargets();
	if(unvisitedTargets.empty())
		oss<<" (that has no unvisited targets)";
	else {
		oss<<" (which contains "<<unvisitedTargets.size()<<" targets not visited yet: ";
		for(auto t : unvisitedTargets) {
			oss<<(Coord)*t<<" ; ";
		}
		oss<<"\b\b)";
	}
	oss<<" :"<<endl
		<<"		from "<<ends.first;
	if(links.first != nullptr)
		oss<<" where it meets BranchlessPath "<<links.first->owner()->id();
	oss<<endl
		<<"		to "<<ends.second;
	if(links.second != nullptr)
		oss<<" where it meets BranchlessPath "<<links.second->owner()->id();
	oss<<endl
		<<string(50, '-')<<endl;

	oss<<'{';
	for(const auto seg : children) {
		oss<<' '<<*seg<<" ;";
	}
	oss<<"\b}"<<endl;

	return oss.str();
}

string BpVertexProps::toString() const {
	ostringstream oss;
	if(nullptr == _forTiltedMaze) {
		oss<<"*";
	} else {
		oss<<_forTiltedMaze->id();
	}
	return oss.str();
}

