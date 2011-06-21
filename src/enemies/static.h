/***************************************************************************
 * static.h  -  headers for the corresponding cpp file
 *
 * Copyright (C) 2011 - Cody Van De Mark
 * Copyright (C) 2007 - 2009 Florian Richter (Original)
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SMC_STATIC_ENEMY_H
#define SMC_STATIC_ENEMY_H

#include "../enemies/enemy.h"
#include "../objects/path.h"

namespace SMC
{

/* *** *** *** *** *** *** cStaticEnemy *** *** *** *** *** *** *** *** *** *** *** */

class cStaticEnemy : public cEnemy 
{
/* Static enemies don't move but will
 * hit you if you touch them.
 */
public:
	// constructor
	cStaticEnemy( float x, float y );
	// create from stream
	cStaticEnemy( CEGUI::XMLAttributes &attributes );
	// destructor
	virtual ~cStaticEnemy( void );

	// init defaults
	void Init( void );
	/* late initialization
	 * this needs linked objects to be already loaded
	*/
	virtual void Init_Links( void );
	// copy
	virtual cStaticEnemy *Copy( void );

	// create from stream
	virtual void Create_From_Stream( CEGUI::XMLAttributes &attributes );
	// save to stream
	virtual void Save_To_Stream( ofstream &file );

	// load from savegame
	virtual void Load_From_Savegame( cSave_Level_Object *save_object );
	// save to savegame
	virtual cSave_Level_Object *Save_To_Savegame( void );

	// Set the static image
	void Set_Static_Image( const std::string &filename );
	// Set the rotation speed
	void Set_Rotation_Speed( float speed );
    // Set the movement speed
    void Set_Speed( float speed );
    // Set the path identifier
    void Set_Path_Identifier( const std::string &path );

	/* downgrade state ( if already weakest state : dies )
	 * force : usually dies or a complete downgrade
	*/
	virtual void DownGrade( bool force = 0 );
	// dying animation update
	virtual void Update_Dying( void );

	// update
	virtual void Update( void );
	// draw
	virtual void Draw( cSurface_Request *request /* = NULL */ );

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

	// leveleditor activation
	virtual void Editor_Activate( void );
	// editor image text changed event
	bool Editor_Image_Text_Changed( const CEGUI::EventArgs &event );
	// editor rotation speed text changed event
	bool Editor_Rotation_Speed_Text_Changed( const CEGUI::EventArgs &event );
    // editor path identifier text changed event
	bool Editor_Path_Identifier_Text_Changed( const CEGUI::EventArgs &event );
    // editor speed text changed event
	bool Editor_Speed_Text_Changed( const CEGUI::EventArgs &event );
	// editor fire resistant option selected event
	bool Editor_Fire_Resistant_Select( const CEGUI::EventArgs &event );
	// editor ice resistance text changed event
	bool Editor_Ice_Resistance_Text_Changed( const CEGUI::EventArgs &event );

	// image filename
	std::string m_img_filename;
	// rotation speed
	float m_rotation_speed;
	// movement speed if using path
	float m_speed;

	// path state if linked to a path
	cPath_State m_path_state;
private:
	// Create the Name from the current settings
	void Create_Name( void );
};

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC

#endif
