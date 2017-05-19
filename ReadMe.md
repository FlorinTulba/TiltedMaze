# TiltedMaze

This is a program solving tilted maze problems. Here is one such maze:<br>
<table style="width:100%; margin-left:0; margin-right:0" border="0" cellpadding="0" cellspacing="0">
	<tr valign="top" style="vertical-align:top">
		<td width="23%">
			<img src="TiltedMaze.jpg">
		</td>
		<td align="justify" style="text-align:justify; padding-left:0; padding-right:0">
The <b>token</b> (the red circle) needs to travel through the maze so that it visits all the <b>targets</b> (blue rectangular spots).<br>
These are the rules about how the token is moved:
<ul>
	<li>it can move in any direction, but</li>
	<li>it must continue to move in that direction <b>until reaching a wall</b> (it cannot stop somewhere on the way)</li>
</ul>
The game can be played online: <a href="http://www.agame.com/game/tilt-maze">http://www.agame.com/game/tilt-maze</a>.
		</td>
	</tr>
</table>

* * *

The tilted mazes solver uses the [Boost-Graph](http://www.boost.org/doc/libs/release/libs/graph/) library. One graph algorithm from that library needed to be adapted for this problem ([graph_r_c_shortest_paths.h](src/Adapted3rdParty/graph_r_c_shortest_paths.h))

This version of the program:

- reads the maze files selected by the user using the _open file dialog_. The maze files are either in _text format_ or directly _images_. Such an image might be **rotated, mirrored (reflected)** and even affected by **perspective transformations**. See the example from below and the [res/](res/) folder for the test mazes
- presents the steps required to solve the maze in a **console / graphic animation**
- is configured for **64-bit Windows** (the file open dialog is Windows-specific)

<table style="width:100%; margin-left:0; margin-right:0" border="0" cellpadding="0" cellspacing="0">
	<tr valign="top" style="vertical-align:top">
		<td style="padding-left:0; padding-right:0">
<p>These are a couple of (<b>graphic</b>) animations caught in early stages:<p>
The &#39;<b>Source</b>&#39; panels present the files containing the mazes
(text files or images with various imperfections also subjected to several transformations).<p>
The panels from the right show the <b>interpreted mazes</b>.<p>
The mazes loaded from images undo any image transformation (rotations, reflections, perspective transformations) - reorienting
the mazes to have the &#39;<b>Level</b>&#39; label positioned like in the original images, that is on top of the mazes to the left.<p>
The label is omitted in the reconstructed mazes.<p>
The moving red <i>token</i> leaves behind a trail of <i>small red diamonds</i>.
		</td>
		<td width="75%">
      <img src="doc/TiltedMaze_animation.jpg">
		</td>
	</tr>
</table>

<table style="width:100%; margin-left:0; margin-right:0" border="0" cellpadding="0" cellspacing="0">
	<tr valign="middle" style="vertical-align:middle">
		<td align="justify" style="text-align:justify; padding-left:0; padding-right:0">
If you consider using the <b>console-mode animation</b>, the following image shows how it looks like.
It presents an early stage of such an animation (left part of the image) and the final of that demonstration (right side of the image).
The targets were represented with blue <b>&#39;$&#39;</b> signs and each move fills the traversed cells with <b>&#39;&#42;&#39;</b>
using a different color than the previous move.
		</td>
		<td width="67%">
      <img src="../version_1.0/doc/TiltedMaze_animation.jpg">
		</td>
	</tr>
</table>

* * *

Building or extending the program requires:

- **OpenCV** and **Boost** installations
- configuring **BoostDir** and **OpenCvDir** properties in the file [Common.props](Common.props) with the path to the Boost and OpenCV installations
- configuring **AdditionalIncludeDirectories**, **AdditionalLibraryDirectories** and **AdditionalDependencies** from the files [Common.props](Common.props), [Debug.props](Debug.props) and [Release.props](Release.props) to point to the Boost and OpenCV headers or libraries from the target machine

* * *

&copy; 2014, 2017 Florin Tulba
