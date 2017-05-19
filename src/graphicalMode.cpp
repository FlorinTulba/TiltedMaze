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

#include "graphicalMode.h"
#include "problemAdapter.h"

#pragma warning( push, 0 )

#include <boost/filesystem.hpp>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#pragma warning( pop )

using namespace std;
using namespace cv;
using namespace boost;
using namespace boost::icl;
using namespace boost::filesystem;

const int GraphicalUiEngine::fontFace = FONT_HERSHEY_TRIPLEX;
const string GraphicalUiEngine::infoText("Source");
const Size GraphicalUiEngine::infoTextSz = getTextSize(infoText, fontFace, 1., 2, nullptr);

int GraphicalUiEngine::nthIdealCoord(double x0, double delta, int idx) {
	return int(.5 + x0 + idx * delta);
}

GraphicalUiEngine::GraphicalUiEngine(const Maze &aMaze) : Maze::UiEngine(aMaze),
		mazeSide(min((PANEL_WIDTH/2 - PANEL_CONTENT_PADDING), (PANEL_HEIGHT - 2 * PANEL_CONTENT_PADDING))),
		panel(PANEL_HEIGHT, PANEL_WIDTH, CV_8UC3, Scalar(116., 242., 221.)),
		sourceArea(panel, Rect(PANEL_CONTENT_PADDING, 2 * PANEL_CONTENT_PADDING + infoTextSz.height, PANEL_WIDTH / 2 - 2 * PANEL_CONTENT_PADDING, PANEL_HEIGHT - 3 * PANEL_CONTENT_PADDING - infoTextSz.height)),
		solutionArea(panel, Rect((3 * PANEL_WIDTH / 2 - PANEL_CONTENT_PADDING - mazeSide) / 2, (PANEL_HEIGHT - mazeSide) / 2, mazeSide, mazeSide)),
		m(mazeSide, mazeSide, CV_8UC3, Scalar::all(255.)),
		windowName("Solving "+maze.name()),
		centers((size_t)maze.rowsCount()),
		wallWidth(mazeSide*WALLS_WEIGHT_PERCENT/(100 * (1+(int)maze.rowsCount()))),
		cellSz((mazeSide - wallWidth) / (double)maze.rowsCount()),
		circleRadius(int(.5 + (cellSz - wallWidth) * CIRCLE_FILLS_CELL_PERCENT / 200)),
		halfSquareSide(int(.5 + (cellSz - wallWidth) * SQUARE_FILLS_CELL_PERCENT / 200)),
		alpha(CIRCLE_BLEND_PERCENT/100.), oneMinusAlpha(1-alpha),
		oneOverOneMinusAlpha(1/oneMinusAlpha), alphaOverOneMinusAlpha(alpha/oneMinusAlpha) {
	drawSource();
	drawWholeMaze();

	namedWindow(windowName);
	imshow(windowName, panel);
	displayStatusBar(windowName, "Press <Space> for Pause/Resume. Anything else speeds up the demonstration.");
	displayOverlay(windowName, "The status bar contains useful instructions");

	// wait a bit to ensure the starting location is clear during the animated solution
	waitOrHandleKey(10*ANIMATION_DELAY_MS);
}

GraphicalUiEngine::~GraphicalUiEngine() {
	displayStatusBar(windowName, "Press <ESC> to leave this maze!");
	while(27 != waitKey()); // wait the user to press ESC after solving a maze
	destroyAllWindows();
}

void GraphicalUiEngine::waitOrHandleKey(int delayMs) {
	int key = waitKey(delayMs);
	if(key == (int)' ') // Pause request
		while((int)' ' != waitKey()); // Wait for the Resume request
}

void GraphicalUiEngine::drawMove(const Coord &from, const Coord &to) {
	if(from == to)
		return;

	int prevR, prevC, r = (int)from.row, c = (int)from.col, // moving part coordinates
		tr = (int)to.row, tc = (int)to.col, // target coordinates
		dr = tr - r, dc = tc - c; // distances to travel that will be transformed into iteration steps below

	// exactly one of the dr and dc must be 0
	require(0 == dr*dc, "Not a horizontal move!");

	// transforming distances into steps for iteration
	if(dr!=0)
		dr /= abs(dr);  // dr = (int)copysign(1, dr);
	else
		dc /= abs(dc); // dc = (int)copysign(1, dc);

	Mat m_; // the overlay

	do {
		prevR = r; prevC = c;
		r += dr; c += dc;

		waitOrHandleKey(ANIMATION_DELAY_MS);

		// Remove previous blending circle
		m_ = m.clone();
		circle(m_, Point(centers[size_t(prevC)], centers[size_t(prevR)]), circleRadius, Scalar(0., 0., 255.), CV_FILLED);
		m = oneOverOneMinusAlpha * m - alphaOverOneMinusAlpha * m_;

		// Place a tracing red dot instead of the previous circle, to know the cell was visited
		circle(m, Point(centers[size_t(prevC)], centers[size_t(prevR)]), 2, Scalar(0., 0., 255.), CV_FILLED);

		// Add the new blending circle
		m_ = m.clone();
		circle(m_, Point(centers[size_t(c)], centers[size_t(r)]), circleRadius, Scalar(0., 0., 255.), CV_FILLED);
		m = alpha * m_ + oneMinusAlpha * m;

		m.copyTo(solutionArea);
		imshow(windowName, panel);

	} while(Coord((unsigned)r, (unsigned)c) != to);

	waitOrHandleKey(5*ANIMATION_DELAY_MS); // wait longer at the end of the segment
}

void GraphicalUiEngine::drawWholeMaze() {
	int n = (int)maze.rowsCount(), lim = mazeSide - 1;
	double halfWall = wallWidth/2., halfCellSz = cellSz/2., firstCenter = halfWall + halfCellSz;

	for(int i = 0; i<n; ++i)
		centers[size_t(i)] = nthIdealCoord(firstCenter, cellSz, i);

	// display border
	rectangle(m, Point((int)halfWall, (int)halfWall), Point(lim-(int)halfWall, lim-(int)halfWall), Scalar::all(0U), wallWidth);

	// display vertical walls
	int r = 0, c = 0, rDisplay, cDisplay;
	for(const auto &sis : maze.rows()) {
		rDisplay = int(centers[size_t(r)] - halfCellSz - halfWall);
		for(const auto &iut : sis) {
			unsigned rightBound = iut.upper(); // one past the upper end
			if((int)rightBound < n) { // consider only segments not touching the right of the maze
				cDisplay = int(centers[rightBound] - halfCellSz - halfWall);
				rectangle(m, Point(cDisplay, rDisplay), Point(cDisplay+wallWidth, rDisplay+(int)cellSz+wallWidth), Scalar::all(0U), CV_FILLED);
			}
		}
		++r;
	}

	// display horizontal walls
	r = c = 0;
	for(const auto &sis : maze.columns()) {
		cDisplay = int(centers[size_t(c)] - halfCellSz - halfWall);
		for(const auto &iut : sis) {
			unsigned bottomBound = iut.upper(); // one past the upper end
			if((int)bottomBound < n) { // consider only segments not touching the bottom of the maze
				rDisplay = int(centers[bottomBound] - halfCellSz - halfWall);
				rectangle(m, Point(cDisplay, rDisplay), Point(cDisplay+(int)cellSz+wallWidth, rDisplay+wallWidth), Scalar::all(0U), CV_FILLED);
			}
		}
		++c;
	}

	// display the token on the starting location
	circle(m, Point(centers[maze.startLocation().col], centers[maze.startLocation().row]), circleRadius,
		   Scalar((UINT8)(oneMinusAlpha*255U), (UINT8)(oneMinusAlpha*255U), 255U), // display it blending with the background already
		   CV_FILLED);

	// display the targets
	const Point offset(halfSquareSide, halfSquareSide);
	for(const auto &t : maze.targets()) {
		Point center(centers[t.col], centers[t.row]);
		rectangle(m, center-offset, center+offset, Scalar(255U, 0U, 0U), CV_FILLED);
	}

	// put the maze into the displayed panel
	m.copyTo(solutionArea);
}

void GraphicalUiEngine::drawSource() {
	putText(panel, infoText, Point(PANEL_WIDTH / 4 - infoTextSz.width / 2, PANEL_CONTENT_PADDING + infoTextSz.height), fontFace, 1., Scalar::all(60U), 2);

	if(extension(path(maze.name())).compare(".txt") == 0) {
		drawTextContent();
	} else {
		drawSourceImage();
	}
}

void GraphicalUiEngine::drawTextContent() {
	sourceArea = Scalar::all(180U); // change the background

	double fontScale = 1.;
	int thickness = 1;

	string line;
	list<string> fileContent;
	std::ifstream ifs(maze.name());
	size_t linesCount = 0;
	Size textSize;
	int baseline = 0, maxBaseline = 0, maxWidth = 0, height, maxHeight = 0;

	while(getline(ifs, line)) {
		++linesCount;
		if(line.empty())
			line = " "; // getTextSize and putText cannot handle empty strings

		fileContent.push_back(line);

		textSize = getTextSize(line, fontFace, fontScale, thickness, &baseline);
		baseline += thickness;
		if(baseline > maxBaseline)
			maxBaseline = baseline;

		height = textSize.height + baseline;
		if(height > maxHeight)
			maxHeight = height;

		if(textSize.width > maxWidth)
			maxWidth = textSize.width;
	}

	fontScale = min(((double)sourceArea.rows / (linesCount * maxHeight)), ((double)sourceArea.cols / maxWidth));

	double delta = fontScale * maxHeight, y = fontScale * (maxHeight - maxBaseline);
	for(list<string>::const_iterator it = fileContent.cbegin(), itEnd = fileContent.cend(); it!=itEnd; ++it, y += delta) {
		putText(sourceArea, *it, Point(0, (int)(.5+y)), fontFace, fontScale, Scalar::all(40U), thickness);
	}
}

void GraphicalUiEngine::drawSourceImage() {
	Point sourceAreaCenter(sourceArea.cols >> 1, sourceArea.rows >> 1);

	Mat sourceImg(imread(maze.name()));
	double fx = (double)sourceArea.cols / sourceImg.cols, fy = (double)sourceArea.rows / sourceImg.rows, f = min(fx, fy);
	Size srcSz(min(((int)(.5 + f * sourceImg.cols)), sourceArea.cols), min(((int)(.5 + f * sourceImg.rows)), sourceArea.rows));
	Point srcCenter(srcSz.width >> 1, srcSz.height >> 1);
	int interpolation = (fx > 1. && fy > 1.) ? CV_INTER_CUBIC : CV_INTER_AREA;
	resize(sourceImg, sourceImg, srcSz, 0, 0, interpolation);

	Mat relativeSrc(sourceArea, Rect(sourceAreaCenter.x - srcCenter.x, sourceAreaCenter.y - srcCenter.y, srcSz.width, srcSz.height));
	sourceImg.copyTo(relativeSrc);
}
