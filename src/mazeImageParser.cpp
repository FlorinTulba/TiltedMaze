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

#include "mazeImageParser.h"

using namespace std;
using namespace cv;
using namespace boost;
using namespace boost::icl;

const Mat ImageMazeParser::structuralElem = getStructuringElement(MORPH_RECT, Size(3, 3));

map<int, Point2f> ImageMazeParser::cornersIdxMap;

// Based on the nearest segment to the header and the header's slight left position, it's possible to straighten the maze
// by mapping its corners to their expected position
int ImageMazeParser::perspectiveCornerIdxMapping(int idxCorner, int idxNearestSegment, bool flipRequired) {
	if(flipRequired)
		return (MAZE_CORNERS + 1 + idxNearestSegment - idxCorner) % MAZE_CORNERS;

	return (MAZE_CORNERS + idxCorner - idxNearestSegment) % MAZE_CORNERS;
}

Point2d ImageMazeParser::segCenter(const Point2d &p1, const Point2d &p2) {
	return Point2d((p1.x+p2.x)/2., (p1.y+p2.y)/2.);
}

// Finds the intersection of two lines, or returns false.
// The lines are defined by (a1, a2) and (b1, b2).
bool ImageMazeParser::linesIntersection(const Point2d &a1, const Point2d &a2, const Point2d &b1, const Point2d &b2, Point2d &result) {
	Point2d deltaA = a2 - a1;
	Point2d deltaB = b2 - b1;

	double divider = deltaA.x * deltaB.y - deltaA.y * deltaB.x;
	if(abs(divider) < 1e-8)
		return false;

	Point2d diffTails = b1 - a1;
	double multiplier = (diffTails.x * deltaB.y - diffTails.y * deltaB.x) / divider;
	result = a1 + deltaA * multiplier;

	return true;
}

bool ImageMazeParser::redOrBlue(UINT8 red, UINT8 green, UINT8 blue, UINT8 threshold/* = 128U*/) {
	return (green < threshold) && (min(red, blue) < threshold) && (max(red, blue) >= threshold);
}

bool ImageMazeParser::justRed(UINT8 red, UINT8 green, UINT8 blue, UINT8 threshold/* = 128U*/) {
	return (red > threshold) && (blue <= threshold) && (green <= threshold);
}

bool ImageMazeParser::justBlue(UINT8 red, UINT8 green, UINT8 blue, UINT8 threshold/* = 128U*/) {
	return (blue > threshold) && (red <= threshold) && (green <= threshold);
}

bool ImageMazeParser::findPixel(Mat rgbImg, PixCondition pixCondFun, Point &foundPixel, int samplingStep/* = 1*/, UINT8 threshold/* = 128U*/) {
	for(int r = 0; r<rgbImg.rows; r += samplingStep) {
		for(int c = 0; c<rgbImg.cols; c += samplingStep) {

			Vec3b pixel = rgbImg.at<Vec3b>(r, c);

			if(pixCondFun(pixel[chRed], pixel[chGreen], pixel[chBlue], threshold)) {
				foundPixel = Point(c, r); // Point holds the flipped coordinates
				return true;
			}
		}
	}

	return false;
}

void ImageMazeParser::process(const string &fileName) {
	originalImg = imread(fileName);
	if(originalImg.data == nullptr)
		throw invalid_argument("The provided file isn't a valid image!");

	if(originalImg.type() != CV_8UC3)
		throw invalid_argument("The image isn't a standard RGB image!");

	straightenMaze();

	enum { MAX_BLACK_THRESHOLD = 80, MIN_BLUE_THRESHOLD = 110 }; // adjusted to suit the provided mazes

	vector<Mat> channels;
	split(straightImg, channels);
	Mat wallsGross = (max(max(channels[chRed], channels[chBlue]), channels[chGreen]) < MAX_BLACK_THRESHOLD);

	Mat walls4BetterDetection, walls4Integration, wallsIntegral; // wallsIntegral will have an extra line and column compared to wallsGross

	// Preparing for integration and better detection
	dilate(wallsGross, walls4BetterDetection, structuralElem); // fills the teeth of comb-like walls => sharpens the integral's slope when meeting the segment

	debugImg = walls4BetterDetection.clone();

	erode(walls4BetterDetection, walls4Integration, structuralElem, Point(-1, -1), 2); // reduces the count of segment end pixels that are counted on perpendicular direction => less interference produced by perpendicular segments

	walls4Integration.convertTo(walls4Integration, CV_64F,
								1./(255 * countNonZero(walls4Integration))); // ensures the integrals bottom right corner is 1
	integral(walls4Integration, wallsIntegral);

	// Traversing the base of the wallsIntegral and marking the walls
	vector<int> vWallsCoords, hWallsCoords;
	int vTol = 0, hTol = 0; // tolerance for vertical / horizontal walls, considering also misplaced (shifted) walls

	extractWallsCoords(wallsIntegral, true, vWallsCoords, vTol);
	extractWallsCoords(wallsIntegral, false, hWallsCoords, hTol);

	double deltaH = 0., deltaV = 0.;
	size_t n = rowsCount = columnsCount = (unsigned)find1stFeasibleMazeSize(hWallsCoords, deltaH, vWallsCoords, deltaV);

	int lim = (int)n, offsetCenterH = (int)(hWallsCoords[0] + deltaH/2 + .5), offsetCenterV = (int)(vWallsCoords[0] + deltaV/2 + .5);
	vector<int> idealCentersH((size_t)lim), idealCentersV((size_t)lim);
	for(int i = 0; i < lim; ++i) {
		idealCentersV[(size_t)i] = (int)(.5 + nthIdealWallCoord(offsetCenterV, deltaV, i));
		idealCentersH[(size_t)i] = (int)(.5 + nthIdealWallCoord(offsetCenterH, deltaH, i));
	}

	isolateWalls(walls4BetterDetection, true, n, vWallsCoords, deltaV, vTol, idealCentersH);
	isolateWalls(walls4BetterDetection, false, n, hWallsCoords, deltaH, hTol, idealCentersV);

	// detecting the start location and the targets by checking the pixels under the ideal centers of each cell
	bool circleFound = false;
	for(unsigned r = 0U; r<n; ++r) {
		for(unsigned c = 0U; c<n; ++c) {
			Point idealCellCenter(idealCentersV[c], idealCentersH[r]);
			Vec3b pixel = straightImg.at<Vec3b>(idealCellCenter.y, idealCellCenter.x);
			UINT8 red = pixel[2], green = pixel[1], blue = pixel[0];
			
			if(false == circleFound) {
				if(false == redOrBlue(red, green, blue, MIN_BLUE_THRESHOLD))
					continue;

				if(justRed(red, green, blue, MIN_BLUE_THRESHOLD)) {
					circleFound = true;
					circle(debugImg, idealCellCenter, 15, 128U, CV_FILLED);
					startLocation = Coord(r, c);
					continue;
				}
			}

			if(justBlue(red, green, blue, MIN_BLUE_THRESHOLD)) {
				targets.emplace_back(r, c);

				Point p(4, 4);
				rectangle(debugImg, idealCellCenter - p, idealCellCenter + p, 128U, CV_FILLED);
			}
		}
	}
}

Mat ImageMazeParser::preprocessImg(Point2d &headerCenter) {
	enum { MAX_BLACK_THRESHOLD = 210, MAX_R_B_DIFF = 55 }; // adjusted to suit the provided mazes
	vector<Mat> channels;

	split(originalImg, channels);
	Mat img = (max(max(channels[chRed], channels[chBlue]), channels[chGreen]) < MAX_BLACK_THRESHOLD) &
		(abs(channels[chBlue] - channels[chRed]) < MAX_R_B_DIFF);

	// We want to flood the interior of the maze and we need a point from its interior
	// The starting circle and the targets squares have some colors that appear strictly within the maze
	// So, if we know a point of any of these squares or of the circle, then this point will be for sure within the maze's interior
	Point firstRedOrBlue;
	if(false == findPixel(originalImg, redOrBlue, firstRedOrBlue, 3/*, 128U*/)) // Find 1st red or blue pixel while searching each 4rd pixel
		throw domain_error("No red / blue pixel was found in this image! Please adjust the sampling and/or the threshold if the image is correct!");

	// Below using 4-vicinity to be able to tackle really thin maze borders.
	// However, 4-vicinity leaves some isolated pixels unfilled, which are removed with a Close operation below
	img.at<UINT8>(firstRedOrBlue.y, firstRedOrBlue.x) = 0U; // Ensure the seed isn't White
	floodFill(img, firstRedOrBlue, 255U, nullptr, Scalar(), Scalar(), FLOODFILL_FIXED_RANGE | 4); // Point takes flipped coordinates

	// Header Maze does bother some processing stages, so it's useful to remove it:
	Mat header = img.clone();
	floodFill(header, firstRedOrBlue, 0U, nullptr, Scalar(), Scalar(), FLOODFILL_FIXED_RANGE | 4); // hiding the flooded maze; Point takes flipped coordinates
	// temp contains now the header

	img ^= header; // preserve only the flooded maze

	// the Close operation (dilation followed by erosion) to remove the isolated unflooded pixels
	dilate(img, img, structuralElem);

	header &= ~img; // ensures temp doesn't contain maze pixels that couldn't be flooded during the last floodFill

	erode(img, img, structuralElem); // the postponed erosion to complete the Close operation


	// Below comes a good estimation of the center of the header using minimum enclosing rectangle.
	vector<Point> headerPixels; // required by minAreaRect
	findNonZero(header, headerPixels);
	try {
		headerCenter = minAreaRect(headerPixels).center; // throws when the image is completely black
	} catch(cv::Exception&) {
		throw domain_error("Couldn't isolate the 'header' of the maze within the provided image! Please adjust thresholds if the image is correct!");
	}

	return img;
}

void ImageMazeParser::detectMazeCorners(const Mat &img, vector<Point> &mazeCorners) {
	vector<vector<Point>> contours;
	findContours(img, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
	if(contours.size() != 1) // There should be only 1 shape and therefore a single contour!
		throw domain_error("Couldn't isolate the maze within the provided image! Please adjust thresholds if the image is correct!");

	approxPolyDP(contours[0], mazeCorners, arcLength(contours[0], true) * .02, true);
	if(mazeCorners.size() != MAZE_CORNERS) // There should be only 4 corners for the maze, since it's a quadrilateral!
		throw domain_error("Wrongfully isolated a non-quadrilateral shape, while looking for the maze!");
}

void ImageMazeParser::straightenMaze() {
	Point2d headerCenter;
	Mat img = preprocessImg(headerCenter);
// 	circle(colorImg, headerCenter, 1, Scalar(0U, 0U, 255U), 1, 8, 0); // red dot marking the center of the header

	vector<Point> mazeCorners;
	vector<Point2d> sidesCenters(MAZE_CORNERS);
	detectMazeCorners(img, mazeCorners);

	// We have the header's center and we need to know what's the nearest side of the maze, to establish maze's top
	// We just compute the L1 distance between header's center and each side's center and find the minimum
	for(int i = 0; i<MAZE_CORNERS; ++i) {
// 		circle(colorImg, mazeCorners[i], 1, Scalar(0U, 0U, 255U), 2, 8, 0);
		sidesCenters[(size_t)i] = segCenter(mazeCorners[(size_t)i], mazeCorners[size_t((i+1) % MAZE_CORNERS)]);
	}

// 	line(colorImg, sidesCenters[0], sidesCenters[2], Scalar(0U, 255U, 0U));
// 	line(colorImg, sidesCenters[1], sidesCenters[3], Scalar(0U, 255U, 0U));

	int idxNearestMazeSide = -1, idxFarthestMazeSide = -1;
	double minL1Dist = numeric_limits<double>::max();
	for(int i = 0; i<MAZE_CORNERS; ++i) {
		double dist = abs(sidesCenters[(size_t)i].x-headerCenter.x) + abs(sidesCenters[(size_t)i].y-headerCenter.y); // fast & safe enough
		if(dist < minL1Dist) {
			minL1Dist = dist;
			idxNearestMazeSide = i;
		}
	}

	// In order to see if the image was also flipped, we check on which side the header's center falls
	// when considering a line crossing the centers of the maze's bottom and top sides
	// If it's on the right, it needs flipping
	// It was more convenient to intersect the top side of the bottom with a line
	// between the center of the maze's bottom side and the header's center:
	// If the intersection was on the right side of the center of the maze's top side, then a flipping is required
	idxFarthestMazeSide = (idxNearestMazeSide + 2) % MAZE_CORNERS;

	bool flipRequired = false;
	Point2d headerCenterProjection;
	if(false == linesIntersection(headerCenter, sidesCenters[(size_t)idxFarthestMazeSide], // 1st line
								mazeCorners[(size_t)idxNearestMazeSide], mazeCorners[size_t((idxNearestMazeSide + 1) % MAZE_CORNERS)], // 2nd line
								headerCenterProjection)) // There should be an intersection!
		throw domain_error("The isolated maze and its 'header' are badly positioned!");

// 	circle(colorImg, headerCenterProjection, 1, Scalar(255U, 0U, 0U), 2, 8, 0);

	// checking where the intersection happens to be and if needed, demanding a flip
	if(norm(Point2d(mazeCorners[(size_t)idxNearestMazeSide]) - headerCenterProjection) <
				norm(headerCenterProjection - Point2d(mazeCorners[size_t((idxNearestMazeSide + 1) % MAZE_CORNERS)])))
	   flipRequired = true;

	// mapping the corners to their appropriate position, by expressing which index will be each corner
	vector<Point2f> quadPts(MAZE_CORNERS), mazeCornersFp(MAZE_CORNERS);
	for(int i = 0; i<MAZE_CORNERS; ++i) {
		int idx = perspectiveCornerIdxMapping(i, idxNearestMazeSide, flipRequired);
		quadPts[(size_t)i] = cornersIdxMap[idx];
		mazeCornersFp[(size_t)i] = mazeCorners[(size_t)i];
	}

	// Get transformation matrix
	Mat transformM = getPerspectiveTransform(mazeCornersFp, quadPts);

	// Apply perspective transformation
	warpPerspective(originalImg, straightImg, transformM, Size(MAZE_SIDE_DEF_SIZE, MAZE_SIDE_DEF_SIZE));
}

double ImageMazeParser::nthIdealWallCoord(int x0, double delta, int idx) {
	return x0 + idx * delta;
}

int ImageMazeParser::indexOfWallWithCoord(int x0, double delta, int wallCoord) {
	return (int)(floor((wallCoord - x0) / delta + .5));
}

size_t ImageMazeParser::find1stFeasibleMazeSize(vector<int> &hWallsCoords, double &deltaH, vector<int> &vWallsCoords, double &deltaV) {
	enum { MAX_DIFF_BETWEEN_MAZE_SIDES = 10, ERROR_MULTIPLIER_THRESHOLD = 10 }; // adjusted to suit the provided mazes
	size_t n = max(vWallsCoords.size(), hWallsCoords.size()) - 1U; // n >= maximum from the sizes of the 2 vectors of coordinates - 1

	int idx, v0 = vWallsCoords.front(), h0 = hWallsCoords.front();
	if(abs(vWallsCoords.back() - v0 - hWallsCoords.back() + h0) > MAX_DIFF_BETWEEN_MAZE_SIDES)
		throw domain_error("Maze borders don't seem to be of a square maze! Please check interpolation and thresholding if the image is correct!");

	// Increasing n until the real segments are close enough to the ideal ones for the checked n
	for(;; ++n) {
		double error = 0;
		deltaV = (vWallsCoords.back() - v0 + 1.) / n;
		deltaH = (hWallsCoords.back() - h0 + 1.) / n;

		for(auto coord : vWallsCoords) {
			idx = indexOfWallWithCoord(v0, deltaV, coord);
			error += abs(nthIdealWallCoord(v0, deltaV, idx) - coord);
		}

		for(auto coord : hWallsCoords) {
			idx = indexOfWallWithCoord(h0, deltaH, coord);
			error += abs(nthIdealWallCoord(h0, deltaH, idx) - coord);
		}

		if(error < ERROR_MULTIPLIER_THRESHOLD * n)
			break;

		if(verbose) {
			cout<<"Checked "<<n<<endl;
			PRINTLN(error);
		}
	}

	if(verbose) {
		cout<<"vWallsIndexes: ";
		for(auto coord : vWallsCoords) {
			cout<<indexOfWallWithCoord(v0, deltaV, coord)<<", ";
		}
		cout<<endl<<endl;

		cout<<"hWallsIndexes: ";
		for(auto coord : hWallsCoords) {
			cout<<indexOfWallWithCoord(h0, deltaH, coord)<<", ";
		}
		cout<<endl;
	}

	return n;
}

void ImageMazeParser::extractWallsCoords(const Mat &wallsIntegral, bool vertNotHoriz, vector<int> &wallsCoords, int &tolerance) {
	enum { MIN_CELL_SIDE = 20 }; // adjusted to suit the provided mazes

	Mat toProcess = vertNotHoriz ? wallsIntegral : wallsIntegral.t();

	int lastRow = toProcess.rows-1, lastCol = toProcess.cols - 1, lastWall = -MIN_CELL_SIDE;
	int tol = 0; // tolerance for walls ignoring misplaced walls

	double lastVal = 0, newVal, diff, threshold = 1./255; // threshold of a single pixel + or -
	bool wallMode = false;
	int wallStart = -1, wallCenter;
	for(int i = 0; i<=lastCol; ++i) {
		newVal = toProcess.at<double>(lastRow, i);
		diff = newVal-lastVal;
		if(wallMode) {
			if(diff < threshold) {
				wallMode = false;
				wallCenter = (wallStart + i - 2) >> 1;
				tol = max(tol, i - 2 - wallCenter);

				if(wallCenter - lastWall < MIN_CELL_SIDE) { // misplaced wall (a bit shifted)
					int avgWallPos = (lastWall + wallCenter) >> 1;

					wallsCoords.back() = avgWallPos; // replace previous wall with one inbetween these 2 'twin' walls

					tolerance = max(tolerance, wallCenter - avgWallPos);
				} else {
					wallsCoords.push_back(wallCenter);
				}

				lastWall = wallCenter;
			}
		} else { // cell mode
			if(diff > threshold) {
				wallMode = true;
				wallStart = i-1;
			}
		}

		lastVal = newVal;
	}

	if(wallMode) {
		wallCenter = (wallStart + lastCol) >> 1;
		wallsCoords.push_back(wallCenter);
	}

	tolerance += tol;
}

void ImageMazeParser::isolateWalls(Mat &thickWalls, bool vertNotHoriz, size_t n, vector<int> &wallsCoords, double delta, int tolerance, vector<int> &idealCentersOfPerpendicularWalls) {
	Mat walls = vertNotHoriz ? thickWalls : thickWalls.t();
	int x0 = wallsCoords[0], lim = (int)n;

	if(vertNotHoriz) {
		rows.reserve(rowsCount);
		for(unsigned i = 0U; i<rowsCount; ++i) {
			split_interval_set<unsigned> sis;
			sis += interval<unsigned>::type(0U, (unsigned)n);
			rows.push_back(sis);
		}
	} else {
		columns.reserve(columnsCount);
		for(unsigned i = 0U; i<columnsCount; ++i) {
			split_interval_set<unsigned> sis;
			sis += interval<unsigned>::type(0U, (unsigned)n);
			columns.push_back(sis);
		}
	}

	for(vector<int>::iterator it = ++wallsCoords.begin(), itEnd = --wallsCoords.end(); it != itEnd; ++it) {
		int coord = *it;
		int idxWall = indexOfWallWithCoord(x0, delta, coord);
		for(int i = 0; i < lim; ++i) {
			int center = idealCentersOfPerpendicularWalls[(size_t)i];

			if(countNonZero(walls.row(center).colRange(coord-tolerance, coord+tolerance)) > 0) {
				if(vertNotHoriz) {
					rows[(size_t)i] += interval<unsigned>::type((unsigned)idxWall, (unsigned)n);
					line(debugImg, Point(coord - tolerance, center), Point(coord + tolerance, center), 128U);
				} else {
					columns[(size_t)i] += interval<unsigned>::type((unsigned)idxWall, (unsigned)n);
					line(debugImg, Point(center, coord - tolerance), Point(center, coord + tolerance), 128U);
				}
			}
		}
	}
}

ImageMazeParser::ImageMazeParser(const string &fileName,
							   unsigned &rowsCount,
							   unsigned &columnsCount,
							   Coord &startLocation,
							   vector<Coord> &targets,
							   vector<split_interval_set<unsigned>> &rows,
							   vector<split_interval_set<unsigned>> &columns,
							   bool Verbose/* = false*/) :
		rowsCount(rowsCount), columnsCount(columnsCount), startLocation(startLocation), targets(targets), rows(rows), columns(columns),
		verbose(Verbose) {

	if(cornersIdxMap.empty()) { // making sure a static is initialized
		cornersIdxMap[0] = Point2f(MAZE_SIDE_DEF_SIZE-1, 0);
		cornersIdxMap[1] = Point2f(0, 0);
		cornersIdxMap[2] = Point2f(0, MAZE_SIDE_DEF_SIZE-1);
		cornersIdxMap[3] = Point2f(MAZE_SIDE_DEF_SIZE-1, MAZE_SIDE_DEF_SIZE-1);
	}

	process(fileName);
}
