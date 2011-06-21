/***************************************************************************
 * powerup.h  -  header for the corresponding cpp file
 *
 * Copyright (C) 2003 - 2009 Florian Richter
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SMC_POWERUP_H
#define SMC_POWERUP_H

#include "../core/globals.h"
#include "../objects/animated_sprite.h"

namespace SMC
{

/* *** *** *** *** *** cPowerUp *** *** *** *** *** *** *** *** *** *** *** *** */

class cPowerUp : public cAnimated_Sprite
{
public:
	// constructor
	cPowerUp( float x = 0, float y = 0 );
	// destructor
	virtual ~cPowerUp( void );

	// load from savegame
	virtual void Load_From_Savegame( cSave_Level_Object *save_object );
	// save to savegame
	virtual cSave_Level_Object *Save_To_Savegame( void );

	/* draw
	 * a spawned powerup doesn't draw in editor mode
	*/
	virtual void Draw( cSurface_Request *request = NULL );

	// if update is valid for the current state
	virtual bool Is_Update_Valid( void );

	/* Validate the given collision object
	 * returns 0 if not valid
	 * returns 1 if an internal collision with this object is valid
	 * returns 2 if the given object collides with this object (blocking)
	*/
	virtual Col_Valid_Type Validate_Collision( cSprite *obj );
	// handle moved out of Level event
	virtual void Handle_out_of_Level( ObjectDirection dir );

	float counter;
};

/* *** *** *** *** *** cMushroom *** *** *** *** *** *** *** *** *** *** *** *** */

class cMushroom : public cPowerUp
{
public:
	// constructor
	cMushroom( float x, float y );
	// create from stream
	cMushroom( CEGUI::XMLAttributes &attributes );
	// destructor
	virtual ~cMushroom( void );

	// init defaults
	void Init( void );

	// copy
	virtual cMushroom *Copy( void );

	// create from stream
	virtual void Create_From_Stream( CEGUI::XMLAttributes &attributes );
	// save to stream
	virtual void Save_To_Stream( ofstream &file );

	// Set the Mushroom Type
	void Set_Type( SpriteType ntype );

	// Activates the item
	virtual void Activate( void );

	// update
	virtual void Update( void );

	// collision from player
	virtual void Handle_Collision_Player( cObjectCollision *collision );
	// collision with massive
	virtual void Handle_Collision_Massive( cObjectCollision *collision );
	// collision from a box
	virtual void Handle_Collision_Box( ObjectDirection cdirection, GL_rect *r2 );

	// glim animation modifier
	bool glim_mod;
};

/* *** *** *** *** *** cFirePlant *** *** *** *** *** *** *** *** *** *** *** *** */

class cFirePlant : public cPowerUp
{
public:
	// constructor
	cFirePlant( float x, float y );
	// create from stream
	cFirePlant( CEGUI::XMLAttributes &attributes );
	// destructor
	virtual ~cFirePlant( void );

	// init defaults
	void Init( void );
	// copy
	virtual cFirePlant *Copy( void );

	// create from stream
	virtual void Create_From_Stream( CEGUI::XMLAttributes &attributes );
	// save to stream
	virtual void Save_To_Stream( ofstream &file );

	// Activates the item
	virtual void Activate( void );

	// update
	virtual void Update( void );

	// collision from player
	virtual void Handle_Collision_Player( cObjectCollision *collision );

	float particle_counter;
};

/* *** *** *** *** *** cMoon *** *** *** *** *** *** *** *** *** *** *** *** */

class cMoon : public cPowerUp
{
public:
	// constructor
	cMoon( float x, float y );
	// create from stream
	cMoon( CEGUI::XMLAttributes &attributes );
	// destructor
	virtual ~cMoon( void );

	// init defaults
	void Init( void );

	// copy
	virtual cMoon *Copy( void );

	// create from stream
	virtual void Create_From_Stream( CEGUI::XMLAttributes &attributes );
	// save to stream
	virtual void Save_To_Stream( ofstream &file );

	// Activates the item
	virtual void Activate( void );

	// update
	virtual void Update( void );

	// collision from player
	virtual void Handle_Collision_Player( cObjectCollision *collision );

	float particle_counter;
};

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC

#endif
