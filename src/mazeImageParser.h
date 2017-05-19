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

#ifndef H_MAZE_IMAGE_PARSER
#define H_MAZE_IMAGE_PARSER

#include "mazeStruct.h"

#pragma warning( push, 0 )

#include <map>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#pragma warning( pop )

/// Reads the maze from an image file
class ImageMazeParser {
	enum { chRed, chGreen, chBlue };
	enum { MAZE_CORNERS = 4, MAZE_SIDE_DEF_SIZE = 400 };

	static const cv::Mat structuralElem;
	static std::map<int, cv::Point2f> cornersIdxMap; ///< mapping between indexes of corners and their correct positions

	bool verbose;

	unsigned &rowsCount, &columnsCount;
	Coord &startLocation;
	std::vector<Coord> &targets;
	std::vector<boost::icl::split_interval_set<unsigned>> &rows, &columns;

	cv::Mat originalImg, straightImg, debugImg;

	typedef bool(*PixCondition)(UINT8, UINT8, UINT8, UINT8);
	static bool redOrBlue(UINT8 red, UINT8 green, UINT8 blue, UINT8 threshold = 128U);
	static bool justRed(UINT8 red, UINT8 green, UINT8 blue, UINT8 threshold = 128U);
	static bool justBlue(UINT8 red, UINT8 green, UINT8 blue, UINT8 threshold = 128U);

	/// SamplingStep allows checking only each n-th pixel on both axes
	static bool findPixel(cv::Mat rgbImg, PixCondition pixCondFun, cv::Point &foundPixel,
						  int samplingStep = 1, UINT8 threshold = 128U);
	static cv::Point2d segCenter(const cv::Point2d &p1, const cv::Point2d &p2);
	static double nthIdealWallCoord(int x0, double delta, int idx);
	static int indexOfWallWithCoord(int x0, double delta, int wallCoord);
	
	/**
	Finds the intersection of two lines, or returns false.
	The lines are defined by (a1, a2) and (b1, b2).
	*/
	static bool linesIntersection(const cv::Point2d &a1, const cv::Point2d &a2,
								  const cv::Point2d &b1, const cv::Point2d &b2,
								  cv::Point2d &result);

	/**
	Based on the nearest segment to the header and the header's slight left position, it's possible to straighten the maze
	by mapping its corners to their expected position
	*/
	static int perspectiveCornerIdxMapping(int idxCorner, int idxNearestSegment, bool flipRequired);
	static void detectMazeCorners(const cv::Mat &img, std::vector<cv::Point> &mazeCorners);

	/// Parsing the integral image and detecting jumps where the walls should be
	static void extractWallsCoords(const cv::Mat &wallsIntegral, bool vertNotHoriz,
								   std::vector<int> &wallsCoords, int &tolerance);

	/**
	Extract just the dark pixels based on a threshold and negative the whole image, so they become white
	Fill the interior of the maze and remove the header of the maze
	Returns the header's center, which is in the original images a bit to the left comparing to the maze's center
	The header's center can't be the mass center of the convex hull of the header,
	as this mass center always gets located near the thicker end of the hull.
	Sometimes this means also too close to the 'Speaker' from the header, and we don't want that at all.
	So, minAreaRect's center was used to compute header's center
	*/
	cv::Mat preprocessImg(cv::Point2d &headerCenter);
	void straightenMaze();
	size_t find1stFeasibleMazeSize(std::vector<int> &hWallsCoords, double &deltaH,
								   std::vector<int> &vWallsCoords, double &deltaV);

	/// Checking a small area around the ideal center of each potential wall to see if there's indeed a wall there
	void isolateWalls(cv::Mat &thickWalls, bool vertNotHoriz, size_t n,
					  std::vector<int> &wallsCoords, double delta, int tolerance,
					  std::vector<int> &idealCentersOfPerpendicularWalls);

	/**
	Can tackle rotated mazes with deformed perspective
	Assumes the original image has a header like:
	Level x: followed by a 'Play' triangle sign and at the end of the row there's also a 'Speaker' symbol
	The center of this header would be slightly to the left from the center of the maze
	*/
	void process(const std::string &fileName);

public:
	ImageMazeParser(const std::string &fileName,
				   unsigned &rowsCount,
				   unsigned &columnsCount,
				   Coord &startLocation,
				   std::vector<Coord> &targets,
				   std::vector<boost::icl::split_interval_set<unsigned>> &rows,
				   std::vector<boost::icl::split_interval_set<unsigned>> &columns,
				   bool verbose = false);
};


#endif // H_MAZE_IMAGE_PARSER