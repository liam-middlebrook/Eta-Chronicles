/***************************************************************************
 * goldpiece.cpp  -  goldpiece class
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

#include "../objects/goldpiece.h"
#include "../core/game_core.h"
#include "../player/player.h"
#include "../audio/audio.h"
#include "../core/framerate.h"
#include "../video/animation.h"
#include "../gui/hud.h"
#include "../user/savegame.h"
#include "../core/math/utilities.h"
#include "../core/i18n.h"

namespace SMC
{

/* *** *** *** *** *** *** cGoldpiece *** *** *** *** *** *** *** *** *** *** *** */

cGoldpiece :: cGoldpiece( float x, float y )
: cAnimated_Sprite( x, y )
{
	cGoldpiece::Init();
}

cGoldpiece :: cGoldpiece( CEGUI::XMLAttributes &attributes )
: cAnimated_Sprite()
{
	cGoldpiece::Init();
	cGoldpiece::Create_From_Stream( attributes );
}

cGoldpiece :: ~cGoldpiece( void )
{
	//
}

void cGoldpiece :: Init( void )
{
	m_sprite_array = ARRAY_ACTIVE;
	m_massive_type = MASS_PASSIVE;
	m_type = TYPE_GOLDPIECE;
	m_pos_z = 0.041f;
	m_can_be_on_ground = 0;

	Set_Gold_Color( COL_YELLOW );
}

cGoldpiece *cGoldpiece :: Copy( void )
{
	cGoldpiece *goldpiece = new cGoldpiece( m_start_pos_x, m_start_pos_y );
	goldpiece->Set_Gold_Color( color_type );

	return goldpiece;
}

void cGoldpiece :: Create_From_Stream( CEGUI::XMLAttributes &attributes )
{
	// position
	Set_Pos( static_cast<float>(attributes.getValueAsInteger( "posx" )), static_cast<float>(attributes.getValueAsInteger( "posy" )), 1 );
	// gold color
	Set_Gold_Color( Get_Color_Id( attributes.getValueAsString( "color", Get_Color_Name( color_type ) ).c_str() ) );
}

void cGoldpiece :: Save_To_Stream( ofstream &file )
{
	// begin item
	file << "\t<item>" << std::endl;

	// type
	file << "\t\t<Property name=\"type\" value=\"goldpiece\" />" << std::endl;
	// position
	file << "\t\t<Property name=\"posx\" value=\"" << static_cast<int>(m_start_pos_x) << "\" />" << std::endl;
	file << "\t\t<Property name=\"posy\" value=\"" << static_cast<int>(m_start_pos_y) << "\" />" << std::endl;
	// color
	file << "\t\t<Property name=\"color\" value=\"" << Get_Color_Name( color_type ) << "\" />" << std::endl;

	// end item
	file << "\t</item>" << std::endl;
}

void cGoldpiece :: Load_From_Savegame( cSave_Level_Object *save_object )
{
	// active
	if( save_object->exists( "active" ) )
	{
		Set_Active( string_to_int( save_object->Get_Value( "active" ) ) > 0 );
	}
}

cSave_Level_Object *cGoldpiece :: Save_To_Savegame( void )
{
	// only save if needed
	if( m_active )
	{
		return NULL;
	}

	cSave_Level_Object *save_object = new cSave_Level_Object();

	// default values
	save_object->m_type = m_type;
	save_object->m_properties.push_back( cSave_Level_Object_Property( "posx", int_to_string( static_cast<int>(m_start_pos_x) ) ) );
	save_object->m_properties.push_back( cSave_Level_Object_Property( "posy", int_to_string( static_cast<int>(m_start_pos_y) ) ) );

	// active
	save_object->m_properties.push_back( cSave_Level_Object_Property( "active", int_to_string( m_active ) ) );

	return save_object;
}

void cGoldpiece :: Set_Gold_Color( DefaultColor ncolor )
{
	color_type = ncolor;

	// clear images
	Clear_Images();

	if( m_type == TYPE_FGOLDPIECE )
	{
		if( color_type == COL_RED )
		{
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/1_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/2_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/3_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/4_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/5_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/6_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/7_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/8_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/9_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/10_falling.png" ) );

			m_name = _("Red Falling Goldpiece");
		}
		// default is yellow
		else
		{
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/1_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/2_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/3_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/4_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/5_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/6_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/7_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/8_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/9_falling.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/10_falling.png" ) );

			m_name = _("Falling Goldpiece");
		}
	}
	else
	{
		if( color_type == COL_RED )
		{
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/1.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/2.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/3.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/4.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/5.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/6.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/7.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/8.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/9.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/red/10.png" ) );

			m_name = _("Red Goldpiece");
		}
		// default is yellow
		else
		{
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/1.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/2.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/3.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/4.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/5.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/6.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/7.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/8.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/9.png" ) );
			Add_Image( pVideo->Get_Surface( "game/items/goldpiece/yellow/10.png" ) );

			m_name = _("Goldpiece");
		}
	}

	Set_Image_Num( 0, 1 );
	Set_Animation( 1 );
	Set_Animation_Image_Range( 0, 9 );

	if( m_type == TYPE_JGOLDPIECE || m_type == TYPE_FGOLDPIECE )
	{
		Set_Time_All( 70, 1 );
	}
	else
	{
		Set_Time_All( 80, 1 );
	}

	Reset_Animation();
}

void cGoldpiece :: Activate( void )
{
	if( !m_active )
	{
		return;
	}

	// animation
	cAnimation_Goldpiece *anim = new cAnimation_Goldpiece( m_pos_x + ( m_col_rect.m_w / 10 ), m_pos_y + ( m_col_rect.m_h / 10 ) );

	// gold
	unsigned int points = 0;

	if( color_type == COL_RED )
	{
		pHud_Goldpieces->Add_Gold( 5 );
		points = 100;

		anim->Set_Scale( 1.2f, 1 );
	}
	else
	{
		pHud_Goldpieces->Add_Gold( 1 );
		points = 5;
	}

	pAnimation_Manager->Add( anim );

	// if jumping double the points
	if( m_type == TYPE_JGOLDPIECE )
	{
		points *= 2;
	}
	else
	{
		if( color_type == COL_RED )
		{
			pAudio->Play_Sound( "item/goldpiece_red.wav" );
		}
		else
		{
			pAudio->Play_Sound( "item/goldpiece_1.ogg" );
		}
	}

	pHud_Points->Add_Points( points, m_pos_x + m_col_rect.m_w / 2, m_pos_y + 2 );

	// if spawned destroy
	if( m_spawned )
	{
		Destroy();
	}
	// hide
	else
	{
		Set_Active( 0 );
	}
}

void cGoldpiece :: Update( void )
{
	if( !m_valid_update || !Is_Visible_On_Screen() )
	{
		return;
	}

	Update_Animation();
}

void cGoldpiece :: Draw( cSurface_Request *request /* = NULL */ )
{
	if( !m_valid_draw )
	{
		return;
	}

	// don't draw in leveleditor if spawned ingame
	if( editor_level_enabled && m_spawned )
	{
		return;
	}

	cAnimated_Sprite::Draw( request );
}

bool cGoldpiece :: Is_Update_Valid( void )
{
	// if not visible
	if( !m_active )
	{
		return 0;
	}

	return 1;
}

void cGoldpiece :: Handle_Collision_Player( cObjectCollision *collision )
{
	// invalid
	if( collision->direction == DIR_UNDEFINED )
	{
		return;
	}

	Activate();
}

/* *** *** *** *** *** *** cJGoldpiecee *** *** *** *** *** *** *** *** *** *** *** */

cJGoldpiece :: cJGoldpiece( float x, float y )
: cGoldpiece( x, y )
{
	m_type = TYPE_JGOLDPIECE;
	m_spawned = 1;

	cJGoldpiece::Set_Gold_Color( COL_YELLOW );

	m_vely = -18.0f;
}

cJGoldpiece :: ~cJGoldpiece( void )
{
	//
}

void cJGoldpiece :: Update( void )
{
	if( !m_active )
	{
		return;
	}

	Update_Animation();

	// add velocity downwards
	if( m_vely < 8.0f )
	{
		Add_Velocity( 0, 1.62f );
	}
	// finished animation
	else
	{
		Activate();
	}
}

Col_Valid_Type cJGoldpiece :: Validate_Collision( cSprite *obj )
{
	return COL_VTYPE_NOT_VALID;
}

/* *** *** *** *** *** *** cFGoldpiecee *** *** *** *** *** *** *** *** *** *** *** */

cFGoldpiece :: cFGoldpiece( float x, float y, ObjectDirection dir /* = DIR_NOTHING */ )
: cGoldpiece( x, y )
{
	m_type = TYPE_FGOLDPIECE;
	m_spawned = 1;
	m_player_range = 2000;
	m_can_be_on_ground = 1;

	// direction set
	if( dir == DIR_LEFT || dir == DIR_RIGHT )
	{
		m_direction = dir;
	}
	// set a random direction
	else
	{
		if( rand() % 2 != 1 )
		{
			m_direction = DIR_LEFT;
		}
		else
		{
			m_direction = DIR_RIGHT;
		}
	}

	if( m_direction == DIR_RIGHT )
	{
		m_velx = 5.0f;
	}
	else
	{
		m_velx = -5.0f;
	}

	cFGoldpiece::Set_Gold_Color( COL_YELLOW );
}

cFGoldpiece :: ~cFGoldpiece( void )
{
	//
}

void cFGoldpiece :: Update( void )
{
	if( !m_valid_update || !Is_In_Player_Range() )
	{
		return;
	}

	Update_Animation();

	// Add Gravitation
	if( !m_ground_object && m_vely < 25.0f )
	{
		Add_Velocity( 0, 1.2f );
	}
}

Col_Valid_Type cFGoldpiece :: Validate_Collision( cSprite *obj )
{
	// basic validation checking
	Col_Valid_Type basic_valid = Validate_Collision_Ghost( obj );

	// found valid collision
	if( basic_valid != COL_VTYPE_NOT_POSSIBLE )
	{
		return basic_valid;
	}

	switch( obj->m_type )
	{
		case TYPE_PLAYER:
		{
			return COL_VTYPE_INTERNAL;
		}
		case TYPE_BALL:
		{
			return COL_VTYPE_NOT_VALID;
		}
		default:
		{
			break;
		}
	}

	if( obj->m_massive_type == MASS_MASSIVE )
	{
		if( obj->m_sprite_array == ARRAY_ENEMY )
		{
			return COL_VTYPE_NOT_VALID;
		}

		return COL_VTYPE_BLOCKING;
	}
	if( obj->m_massive_type == MASS_HALFMASSIVE )
	{
		// if moving downwards and the object is on bottom
		if( m_vely >= 0.0f && Is_On_Top( obj ) )
		{
			return COL_VTYPE_BLOCKING;
		}
	}

	return COL_VTYPE_NOT_VALID;
}

void cFGoldpiece :: Handle_Collision_Massive( cObjectCollision *collision )
{
	if( collision->direction == DIR_RIGHT || collision->direction == DIR_LEFT )
	{
		Turn_Around( collision->direction );
	}
	else if( collision->direction == DIR_UP )
	{
		m_vely = -( m_vely * 0.3f );
	}
	else if( collision->direction == DIR_DOWN )
	{
		// minimal value for a jump
		if( m_vely > 0.5f )
		{
			m_vely = -( m_vely * 0.5f );

			// maximum value for a jump
			if( m_vely > 10.0f )
			{
				m_vely = 10.0f;
			}
		}
		else
		{
			m_vely = 0.0f;
		}
	}
}

void cFGoldpiece :: Handle_Collision_Box( ObjectDirection cdirection, GL_rect *r2 )
{
	// if unsupported collision direction
	if( cdirection != DIR_DOWN && cdirection != DIR_LEFT && cdirection != DIR_RIGHT )
	{
		return;
	}

	if( cdirection == DIR_DOWN )
	{
		m_vely = -30.0f;

		// left
		if( m_pos_x > r2->m_x && m_velx < 0.0f )
		{
			Turn_Around( DIR_LEFT );
		}
		// right
		else if( m_pos_x < r2->m_x && m_velx > 0.0f )
		{
			Turn_Around( DIR_RIGHT );
		}
	}
	else if( cdirection == DIR_LEFT || cdirection == DIR_RIGHT )
	{
		m_vely = -13.0f;
		Turn_Around( cdirection );
	}

	Reset_On_Ground();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
