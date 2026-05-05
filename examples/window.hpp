/*******************************************************************************************
*
*   rhio example - GLFW window module
*
*   Copyright (c) 2026 SOHNE, Leandro Peres (@zschzen) and contributors
*
********************************************************************************************/

#pragma once

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

struct GLFWwindow;

struct WindowInfo
{
    int          screenWidth  = 0;
    int          screenHeight = 0;
    const char * title        = nullptr;
};

using WindowResizeCallback = void ( * )( int width, int height );

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

bool         InitWindow( const WindowInfo & info );
void         CloseWindow();
bool         WindowShouldClose();
void         BeginWindowFrame();
void         EndWindowFrame();
void         SetWindowResizeCallback( WindowResizeCallback callback );
GLFWwindow * GetWindowHandle();
