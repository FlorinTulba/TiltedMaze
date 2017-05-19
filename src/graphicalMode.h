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

#ifndef H_GRAPHICAL_MODE
#define H_GRAPHICAL_MODE

#include "mazeStruct.h"

#pragma warning( push, 0 )

#include <vector>

#include <opencv2/core/core.hpp>

#pragma warning( pop )

/// Draws the maze and animates the moves in graphic mode
class GraphicalUiEngine : public Maze::UiEngine {
	enum { PANEL_WIDTH = 790, PANEL_HEIGHT = 450, // the size of the window where the input maze and the solution is displayed
		PANEL_CONTENT_PADDING = 15, // margin around the content
		WALLS_WEIGHT_PERCENT = 15, // (100 - WALLS_WEIGHT_PERCENT)% from mazeSide = free space for drawing the interior of the cells of a row
		CIRCLE_BLEND_PERCENT = 70, // how much emphasis has the circle (moving token) compared to the background (where can be targets, as well)
		CIRCLE_FILLS_CELL_PERCENT = 75, // how close to the walls the circle (moving token) is
		SQUARE_FILLS_CELL_PERCENT = 30, // how close to the walls each square (target) is
		ANIMATION_DELAY_MS = 75}; // how many ms to wait before drawing the next frame

	static int nthIdealCoord(double x0, double delta, int idx); ///< helper for computing maze cell centers
	static const int fontFace;			///< id of font to use when drawing provided input text maze files
	static const std::string infoText;	///< title of the panel showing the input maze file
	static const cv::Size infoTextSz;	///< font size used for the title of the panel showing the input maze file

	const int mazeSide; ///< Side of the drawn maze in pixels; Keep this before the other fields, to be initialized first
	const std::string windowName;	///< title of the window: Solving maze ...
	cv::Mat panel;			///< the whole content displayed in the window (source area + solution area)
	cv::Mat sourceArea;		///< the left zone from the window - showing the input maze
	cv::Mat solutionArea;	///< the zone from the right - showing the animated solution
	cv::Mat m; ///< matrix for various computations
	std::vector<int> centers; ///< coordinates of the centers of the cells
	const int wallWidth; ///< width of walls in pixels; Keep it before cellSz
	const double cellSz; ///< cell width in pixels; Keep it below wallWidth (it depends on its prior evaluation during construction)
	const int circleRadius;	///< radius of the token in pixels; Keep it below cellSz (they depend on its prior evaluation during construction)
	const int halfSquareSide; ///< half the side of the blue squares representing a target; Keep it below cellSz (they depend on its prior evaluation during construction)
	const double alpha, oneMinusAlpha, oneOverOneMinusAlpha, alphaOverOneMinusAlpha;

	void waitOrHandleKey(int delayMs);
	void drawTextContent();
	void drawSourceImage();
	void drawSource();
	void drawWholeMaze();

public:
	GraphicalUiEngine(const Maze &aMaze);
	~GraphicalUiEngine();

	void drawMove(const Coord &from, const Coord &to) override;
};

#endif // H_GRAPHICAL_MODE