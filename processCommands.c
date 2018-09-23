/*
 *   Copyright (C) 2017,2018 by Lieven De Samblanx ON7LDS
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "NextionDriver.h"
#include "basicFunctions.h"
#include "helpers.h"


//============================================================================
//
//   If needed, put your code in the subroutine below
//
//============================================================================
void processCommands() {

//-------------------------------
//       process commands
//-------------------------------

//---------------------------------------------------------------------
//
//   You can study the examples in basicFunctions.c to get an idea
//    of how to program some extra functionality
//
//---------------------------------------------------------------------
//
//   When using colors, you can use the function
//    RGBtoNextionColor( RGBcolor)
//
//   The 'page' variable (int) holds the last selected page number
//    (0=MMDVM, 1=Dstar, 2=DMR, ...)
//
//---------------------------------------------------------------------

char last_time[512];

if( page == 0 )
	{
	if( strstr( TXbuffer, "t2.txt=" ) > 0 )
		{
		if( strcmp( TXbuffer, last_time ) == 0 )
			{
			TXbuffer[0] = 0;
			}
		else
			{
			sprintf( last_time, "%s", TXbuffer );
			}
		}


	}
}

