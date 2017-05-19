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

#ifndef H_FORCED_INCLUDE
#define H_FORCED_INCLUDE

// Necessary define for using math constants like M_PI.
// See below why:
// It must be defined before math.h / cmath .
// But the file using the math constants might be compiled at a stage after compiling
// some other file that included already the math.h / cmath . So the define should be
// already performed, no matter the compilation order.
#define _USE_MATH_DEFINES

// Necessary define for using rand_s function from stdlib
// This define has to be placed before the first inclusion of <cstdlib>
#define _CRT_RAND_S


// Signaled warnings not related to code generation (< 4700)
#define	ENUM_ITEM_COVERED_BY_DEFAULT_IN_SWITCH	4061
#define	UNREFERENCED_FORMAL_PARAMETER			4100
#define CONST_TEST_CONDITION					4127
#define UNSAFE_CONVERTING_TO_OTHER_TYPE			4191
#define NONSTD_EXT_MODIFY_TMP_OBJ				4239
#define VIRTUALS_HIDDEN_OR_NOT_OVERRIDEN		4263 4264 4266
#define THROW_LIST_IGNORED_IN_VC				4290
#define THROWS_WHEN_NOT_SUPPOSED_TO				4297
#define TRUNCATION_CASTING						4302 4305 4309
#define THIS_USED_IN_INIT_LIST					4355
#define	SIGNED_UNSIGNED_MISMATCH				4365
#define NONSTD_EXT_ENUM							4482
#define DECORATED_NAME_LENGTH_EXCEEDED			4503
#define DEFAULT_CONSTR_GEN_FAILED				4510
#define ASSIGN_OP_GEN_FAILED					4512 4626
#define	REMOVED_NOT_CALLED_FUNCTION				4514
#define POOR_COMMA_OP_BEHAVIOUR					4548
#define USER_DEFINED_CONSTR_REQUIRED			4610
#define NO_SUCH_WARNING_CODE					4619
#define COPY_CONSTR_GEN_FAILED					4625
#define THREAD_UNSAFE_CONSTRUCTION				4640
#define	UNDEFINED_MACRO							4668

// Signaled warnings related to code generation (>= 4700)
// They can be disabled only together with the whole function inside which they happen !!!!
#define UNREACHABLE_CODE						4702
#define	INLINE_REQ_FAILED						4710
#define	INLINED_BUT_NOT_REQUESTED				4711
#define DESTRUCTOR_NOT_RETURNING				4722
#define POTENTIAL_DIVISION_BY_ZERO				4723
#define	WASTEFUL_MEM_ACCESS_BESIDES_REGS		4738
#define	PADDING_AFTER_FIELD						4820
#define ONLY_INLINE_DEFINITION_POSSIBLE			4822
#define	MISPLACED_GUID							4917
#define	CONFLICTING_DECLARATIONS				4986
#define NONSTD_EXT_THROW_ELLIPSIS				4987
#define REINTERPRET_CAST_INSTEAD_OF_STATIC		4946
#define UNSAFE_USE_OR_DEPRECATED_FUNCTION		4996

// !! The compiler only supports up to 56 #pragma warning statements in a compiland. !!
// Globally set the warning level to All Warnings
// See Project Properties -> Config.Properties -> C/C++ -> General -> Warning Level (/W0-4)
#pragma warning( push, 4 )

// !! The compiler only supports up to 56 #pragma warning statements in a compiland. !!
// Disabled warnings for the project
#pragma warning( disable: \
	THROW_LIST_IGNORED_IN_VC  NONSTD_EXT_THROW_ELLIPSIS \
	REMOVED_NOT_CALLED_FUNCTION \
	NO_SUCH_WARNING_CODE \
	UNDEFINED_MACRO \
	INLINE_REQ_FAILED   INLINED_BUT_NOT_REQUESTED \
	PADDING_AFTER_FIELD \
	ONLY_INLINE_DEFINITION_POSSIBLE \
	CONFLICTING_DECLARATIONS \
	DECORATED_NAME_LENGTH_EXCEEDED \
	ASSIGN_OP_GEN_FAILED)

// The following warnings would be useful to be signaled perhaps even as errors. Make sure to capture and handle them anyway!
// 		4239			NONSTD_EXT_MODIFY_TMP_OBJ
// 		4297			THROWS_WHEN_NOT_SUPPOSED_TO
// 		4302,4305,4309	TRUNCATION_CASTING
// 		4355			THIS_USED_IN_INIT_LIST
// 		4365			SIGNED_UNSIGNED_MISMATCH
// 		4625			COPY_CONSTR_GEN_FAILED
// 		4640			THREAD_UNSAFE_CONSTRUCTION
// 		4702			UNREACHABLE_CODE
// 		4722			DESTRUCTOR_NOT_RETURNING
// 		4723			POTENTIAL_DIVISION_BY_ZERO
// 		4738			WASTEFUL_MEM_ACCESS_BESIDES_REGS

#endif // H_FORCED_INCLUDE