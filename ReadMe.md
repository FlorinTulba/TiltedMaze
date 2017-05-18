# TiltedMaze v2.0

This is a program solving tilted maze problems. Here is one such maze:<br>
![](TiltedMaze.jpg)

The **token** (the red circle) needs to travel through the maze so that it visits all the **targets** (blue rectangular spots).
These are the rules about how the token is moved:

- it can move in any direction, but
- it must continue to move in that direction **until reaching a wall** (it cannot stop somewhere on the way)

The game can be played online: [http://www.agame.com/game/tilt-maze](http://www.agame.com/game/tilt-maze).

* * *

The tilted mazes solver uses Boost-Graph library. One graph algorithm from that library needed to be adapted for this problem ([graph_r_c_shortest_paths.h](src/Adapted3rdParty/graph_r_c_shortest_paths.h))

This version of the program:

- reads the maze files selected by the user using the open file dialog. The maze files are either in text format or *directly images*. The image might be *rotated 90 degrees to the right/left*. *Mirroring (reflection)* is also allowed. The only restriction is that the label with the Level of the maze should not get at the bottom of the maze. See [res/](res/) folder for the test mazes
- presents the steps required to solve the maze in a console animation
- is configured for 64-bit Windows (the console animation and the file open dialog are Windows-specific)

The image from below presents an early stage of an animation (left part of the image) and the final of that demonstration (right side of the image). The targets were represented with blue **&#39;$&#39;** signs and each move fills the traversed cells with __&#39;&#42;&#39;__ using a different color than the previous move.<br>
![](doc/TiltedMaze_animation.jpg)

* * *

Building or extending the program requires:

- a **Boost** installation
- configuring the **BoostDir** property in the file [Common.props](Common.props) with the path to the Boost installation. The libraries &#39;_system_&#39; and &#39;_filesystem_&#39; from _Boost_ were compiled and placed in the &#39;**lib.zip**&#39;. You may use the versions from your system by customizing:
	- **AdditionalDependencies** property from [Debug.props](Debug.props) and [Release.props](Release.props)
	- **AdditionalLibraryDirectories** property from [Common.props](Common.props), [Debug.props](Debug.props) or [Release.props](Release.props)
- unpacking &#39;**inc.zip**&#39; and &#39;**lib.zip**&#39; into the same root folder of the project. They contain the headers and libraries from:
	- several image formats: **jpeg, png, tiff**, plus **zlib** libray required by *png*. If you already have them, you may fill their data in [Common.props](Common.props), [Debug.props](Debug.props) or [Release.props](Release.props)
	- an older **extension of Boost-GIL** (library for image manipulation providing support also for .**bmp** images). It was used for identifying the maze elements from the input images

* * *

&copy; 2014, 2017 Florin Tulba
