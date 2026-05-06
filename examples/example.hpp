/*******************************************************************************************
*
*   rhio example - shared example entry points
*
*   Copyright (c) 2026 SOHNE, Leandro Peres (@zschzen) and contributors
*
********************************************************************************************/

#pragma once

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

struct ExampleInfo
{
    int          screenWidth  = 0;
    int          screenHeight = 0;
    const char * appName      = nullptr;
};

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

extern const ExampleInfo exampleInfo;

bool InitExample();
void OnFramebufferResize( int width, int height );
void OnKey( int key, int action );
void ShutdownExample();
