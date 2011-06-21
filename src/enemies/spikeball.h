/***************************************************************************
 * spikeball.h  -  headers for the corresponding cpp file
 *
 * Copyright (C) 2011 - Cody Van De Mark 
 * Copyright (C) 2009 Florian Richter (Original)
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SMC_SPIKEBALL_H
#define SMC_SPIKEBALL_H

#include "../enemies/enemy.h"

namespace SMC
{

/* *** *** *** *** *** *** cSpikeball *** *** *** *** *** *** *** *** *** *** *** */

class cSpikeball : public cEnemy 
{
/* Spikeball
 * 
 */
public:
	// constructor
	cSpikeball( float x, float y );
	// create from stream
	cSpikeball( CEGUI::XMLAttributes &attributes );
	// destructor
	virtual ~cSpikeball( void );

	// init defaults
	void Init( void );
	// copy
	virtual cSpikeball *Copy( void );

	// create from stream
	virtual void Create_From_Stream( CEGUI::XMLAttributes &attributes );
	// save to stream
	virtual void Save_To_Stream( ofstream &file );

	// load from savegame
	virtual void Load_From_Savegame( cSave_Level_Object *save_object );

	// Set Direction
	void Set_Direction( const ObjectDirection dir );
	/* set color
	 * black = normal
	*/
	void Set_Color( const DefaultColor &col );

	/* Move into the opposite Direction
	 * if col_dir is given only turns around if the collision direction is in front
	 */
	virtual void Turn_Around( ObjectDirection col_dir = DIR_UNDEFINED );

	/* downgrade state ( if already weakest state : dies )
	 * force : usually dies or a complete downgrade
	*/
	virtual void DownGrade( bool force = 0 );
	// dying animation update
	virtual void Update_Dying( void );

	// set the moving state
	void Set_Moving_State( Moving_state new_state );

	// update
	virtual void Update( void );

	// update the current velocity
	void Update_Velocity( void );

	// if update is valid for the current state
	virtual bool Is_Update_Valid( void );

	/* Validate the given collision object
	 * returns 0 if not valid
	 * returns 1 if an internal collision with this object is valid
	 * returns 2 if the given object collides with this object (blocking)
	*/
	virtual Col_Valid_Type Validate_Collision( cSprite *obj );
	// collision from player
	virtual void Handle_Collision_Player( cObjectCollision *collision );
	// collision from an enemy
	virtual void Handle_Collision_Enemy( cObjectCollision *collision );
	// collision with massive
	virtual void Handle_Collision_Massive( cObjectCollision *collision );
	// collision from a box
	virtual void Handle_Collision_Box( ObjectDirection cdirection, GL_rect *r2 );

	// editor activation
	virtual void Editor_Activate( void );
	// editor direction option selected event
	bool Editor_Direction_Select( const CEGUI::EventArgs &event );

	// Color
	DefaultColor m_color_type;

private:
	// Create the Name from the current settings
	void Create_Name( void );

	// walking speed
	float m_speed_walk;
	// counter if staying
	float m_counter_stay;
	// counter if walking
	float m_counter_walk;
	// counter if running
	float m_counter_running;
	// particle counter if running
	float m_running_particle_counter;
};

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC

#endif
