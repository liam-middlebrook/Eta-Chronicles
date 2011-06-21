/***************************************************************************
 * world_sprite_manager.h  -  header for the corresponding cpp file
 *
 * Copyright (C) 2008 - 2009 Florian Richter
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SMC_WORLD_SPRITE_MANAGER_H
#define SMC_WORLD_SPRITE_MANAGER_H

#include "../core/globals.h"
#include "../core/sprite_manager.h"
#include "../overworld/overworld.h"

namespace SMC
{

/* *** *** *** *** *** cWorld_Sprite_Manager *** *** *** *** *** *** *** *** *** *** *** *** */

class cWorld_Sprite_Manager : public cSprite_Manager
{
public:
	cWorld_Sprite_Manager( cOverworld *origin );
	virtual ~cWorld_Sprite_Manager( void );

	// Add a sprite
	virtual void Add( cSprite *sprite );

	// origin overworld
	cOverworld *n_origin;
};

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC

#endif
