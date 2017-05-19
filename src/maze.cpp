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
#include "environ.h"

#pragma warning( push, 0 )

#include <tchar.h>
#include <iostream>
#include <string>

#include <conio.h>
#include <sstream>

#include <boost/filesystem/operations.hpp>

#pragma warning( pop )

using namespace std;
using namespace boost::filesystem;

namespace {
	enum { TEST_MAZES_COUNT = 11 };

	void pressKeyToContinue(ostream &os) {
		os<<"Press a key to continue ...";
		int ch = _getch();
		if(ch==0||ch==0xE0) _getch(); // discard any chars left in the console buffer due to pressed function keys
		os<<endl;
	}
}

/// Verifying all existing test files
static bool testsOk() {
	bool ok = true;
	cout<<"Ensuring correct parsing & solving of all test files ..."<<endl<<endl;
	const path resFolder("res");
	const vector<const string> knownPrefixes { "maze", "rot_maze", "rot_persp_maze" };
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
						MazeSolver ms(mazePath.string());
						if(!ms.isSolvable()) {
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

void main(int, char *[]) {
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
	ofn.lpstrFilter = _T("All Input Maze Types\0*.txt;*.bmp;*.png;*.tif;*.tiff;*.jpg;*.jpeg\0")
						 _T("Text Input Mazes\0*.txt\0")
						 _T("Image Input Mazes\0*.bmp;*.png;*.tif;*.tiff;*.jpg;*.jpeg\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.lpstrTitle = _T("Please select a maze to solve or close the dialog to quit");

	bool consoleMode = false;

	while(GetOpenFileName(&ofn)) {
		string mazeName = tstrToStr(ofn.lpstrFile);
		if(consoleMode)
			cout<<string(50, '=')<<endl<<"Maze "<<mazeName<<" :"<<endl<<endl;

		try {
			MazeSolver maze(mazeName/*, true*/);
			if(false == maze.solve(consoleMode/*, true*/)) {
				cout<<"Couldn't solve "<<mazeName<<endl;
				pressKeyToContinue(cout);
			}
		} catch(std::exception &e) {
			cerr<<"Error detected in '"<<mazeName<<"': "<<e.what()<<endl;
			pressKeyToContinue(cerr);
		}
	}
}
