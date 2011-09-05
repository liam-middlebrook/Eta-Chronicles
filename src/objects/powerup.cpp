/***************************************************************************
 * powerup.cpp  -  powerup classes
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

#include "../objects/powerup.h"
#include "../core/game_core.h"
#include "../player/player.h"
#include "../gui/hud.h"
#include "../core/framerate.h"
#include "../video/animation.h"
#include "../video/gl_surface.h"
#include "../user/savegame.h"
#include "../core/math/utilities.h"
#include "../core/i18n.h"

namespace SMC
{

/* *** *** *** *** *** *** cPowerUp *** *** *** *** *** *** *** *** *** *** *** */

cPowerUp :: cPowerUp( float x, float y )
: cAnimated_Sprite( x, y )
{
	m_sprite_array = ARRAY_ACTIVE;
	m_massive_type = MASS_PASSIVE;
	m_type = TYPE_POWERUP;
	m_pos_z = 0.05f;

	counter = 0;
}

cPowerUp :: ~cPowerUp( void )
{
	//
}

void cPowerUp :: Load_From_Savegame( cSave_Level_Object *save_object )
{
	// active
	bool save_visible = string_to_int( save_object->Get_Value( "active" ) ) > 0;
	Set_Active( save_visible );
}

cSave_Level_Object *cPowerUp :: Save_To_Savegame( void )
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

	// visible
	save_object->m_properties.push_back( cSave_Level_Object_Property( "active", int_to_string( m_active ) ) );

	return save_object;
}

void cPowerUp :: Draw( cSurface_Request *request /* = NULL */ )
{
	if( !m_valid_draw )
	{
		return;
	}

	// don't draw in editor mode if spawned
	if( m_spawned && editor_level_enabled )
	{
		return;
	}

	cAnimated_Sprite::Draw( request );
}

bool cPowerUp :: Is_Update_Valid( void )
{
	// if not visible
	if( !m_active )
	{
		return 0;
	}

	return 1;
}

Col_Valid_Type cPowerUp :: Validate_Collision( cSprite *obj )
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

void cPowerUp :: Handle_out_of_Level( ObjectDirection dir )
{
	if( dir == DIR_LEFT )
	{
		m_pos_x = pActive_Camera->limit_rect.m_x;
	}
	else if( dir == DIR_RIGHT )
	{
		m_pos_x = pActive_Camera->limit_rect.m_x + pActive_Camera->limit_rect.m_w - m_col_pos.m_x - m_col_rect.m_w - 0.01f;
	}

	Turn_Around( dir );
}

/* *** *** *** *** *** *** cMushroom *** *** *** *** *** *** *** *** *** *** *** */

cMushroom :: cMushroom( float x, float y )
: cPowerUp( x, y )
{
	cMushroom::Init();
}

cMushroom :: cMushroom( CEGUI::XMLAttributes &attributes )
: cPowerUp()
{
	cMushroom::Init();
	cMushroom::Create_From_Stream( attributes );
}

cMushroom :: ~cMushroom( void )
{
	//
}

void cMushroom :: Init( void )
{
	m_velx = 3;
	m_player_range = 5000;

	m_type = TYPE_UNDEFINED;
	Set_Type( TYPE_MUSHROOM_DEFAULT );

	Update_Direction();

	glim_mod = 1;
}

cMushroom *cMushroom :: Copy( void )
{
	cMushroom *mushroom = new cMushroom( m_start_pos_x, m_start_pos_y );
	mushroom->Set_Type( m_type );

	return mushroom;
}

void cMushroom :: Create_From_Stream( CEGUI::XMLAttributes &attributes )
{
	// position
	Set_Pos( static_cast<float>(attributes.getValueAsInteger( "posx" )), static_cast<float>(attributes.getValueAsInteger( "posy" )) );
	// type
	Set_Type( static_cast<SpriteType>(attributes.getValueAsInteger( "mushroom_type", TYPE_MUSHROOM_DEFAULT )) );
}

void cMushroom :: Save_To_Stream( ofstream &file )
{
	// begin item
	file << "\t<item>" << std::endl;

	// type
	file << "\t\t<Property name=\"type\" value=\"mushroom\" />" << std::endl;
	// mushroom type
	file << "\t\t<Property name=\"mushroom_type\" value=\"" << m_type << "\" />" << std::endl;
	// position
	file << "\t\t<Property name=\"posx\" value=\"" << static_cast<int>(m_start_pos_x) << "\" />" << std::endl;
	file << "\t\t<Property name=\"posy\" value=\"" << static_cast<int>(m_start_pos_y) << "\" />" << std::endl;

	// end item
	file << "\t</item>" << std::endl;
}

void cMushroom :: Set_Type( SpriteType ntype )
{
	// already set
	if( m_type == ntype )
	{
		return;
	}

	Set_Color_Combine( 0, 0, 0, 0 );
	Clear_Images();

	if( ntype == TYPE_MUSHROOM_DEFAULT )
	{
		Add_Image( pVideo->Get_Surface( "game/items/mushroom_red.png" ) );
		m_name = _("Mushroom Red");
	}
	else if( ntype == TYPE_MUSHROOM_LIVE_1 )
	{
		Add_Image( pVideo->Get_Surface( "game/items/mushroom_green.png" ) );
		m_name = _("Mushroom 1-UP");
	}
	else if( ntype == TYPE_MUSHROOM_POISON )
	{
		Add_Image( pVideo->Get_Surface( "game/items/mushroom_poison.png" ) );
		m_name = _("Mushroom Poison");
	}
	else if( ntype == TYPE_MUSHROOM_BLUE )
	{
		Add_Image( pVideo->Get_Surface( "game/items/mushroom_blue.png" ) );
		m_name = _("Mushroom Blue");
	}
	else if( ntype == TYPE_MUSHROOM_GHOST )
	{
		Add_Image( pVideo->Get_Surface( "game/items/mushroom_ghost.png" ) );
		m_name = _("Mushroom Ghost");
	}
	else
	{
		printf( "Warning Unknown Mushroom type : %d\n", ntype );
		return;
	}
	
	m_type = ntype;
	
	Set_Image_Num( 0, 1, 0 );
}

void cMushroom :: Activate( void )
{
	if( !m_active )
	{
		return;
	}

	pPlayer->Get_Item( m_type );

	if( m_type == TYPE_MUSHROOM_DEFAULT )
	{
		pHud_Points->Add_Points( 500, m_pos_x + (m_rect.m_w * 0.5f), m_pos_y + 3 );
	}
	else if( m_type == TYPE_MUSHROOM_LIVE_1 )
	{
		pHud_Points->Add_Points( 1000, m_pos_x + (m_rect.m_w * 0.5f), m_pos_y + 3 );
	}
	else if( m_type == TYPE_MUSHROOM_POISON )
	{
		// nothing
	}
	else if( m_type == TYPE_MUSHROOM_BLUE )
	{
		pHud_Points->Add_Points( 700, m_pos_x + (m_rect.m_w * 0.5f), m_pos_y + 3 );
	}
	else if( m_type == TYPE_MUSHROOM_GHOST )
	{
		pHud_Points->Add_Points( 800, m_pos_x + (m_rect.m_w * 0.5f), m_pos_y + 3 );
	}

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

void cMushroom :: Update( void )
{
	if( !m_valid_update || !Is_In_Player_Range() )
	{
		return;
	}

	if( !m_ground_object )
	{
		if( m_vely < 25.0f )
		{
			Add_Velocity( 0.0f, 1.5f );
		}
	}
	else
	{
		m_vely = 0.0f;
	}

	// particles
	if( m_type == TYPE_MUSHROOM_LIVE_1 || m_type == TYPE_MUSHROOM_BLUE )
	{
		counter += pFramerate->m_speed_factor * 0.8f;

		while( counter > 1.0f )
		{
			cParticle_Emitter *anim = new cParticle_Emitter();
			anim->Set_Pos( m_col_rect.m_x + Get_Random_Float( 0, m_col_rect.m_w ), m_col_rect.m_y + Get_Random_Float( 0, m_col_rect.m_h ) );
			anim->Set_Pos_Z( m_pos_z - 0.000001f );
			anim->Set_Direction_Range( 180, 180 );
			anim->Set_Scale( 0.4f, 0.1f );
			// 1-UP
			if( m_type ==  TYPE_MUSHROOM_LIVE_1 )
			{
				anim->Set_Time_to_Live( 0.4f );
				anim->Set_Color( Color( static_cast<Uint8>(rand() % 30), 100 + ( rand() % 150 ), rand() % 30 ) );
				anim->Set_Speed( 0.5f, 0.5f );
				anim->Set_Blending( BLEND_ADD );
			}
			// Ice
			else if( m_type == TYPE_MUSHROOM_BLUE )
			{
				anim->Set_Time_to_Live( 0.6f );
				anim->Set_Color( Color( static_cast<Uint8>(180 + ( rand() % 50 ) ), 180 + ( rand() % 50 ), 255, 128 ) );
				anim->Set_Speed( 0.2f, 0.1f );
			}
			
			anim->Set_Image( pVideo->Get_Surface( "animation/particles/light.png" ) );

			pAnimation_Manager->Add( anim );

			counter--;
		}

		if( m_type == TYPE_MUSHROOM_BLUE )
		{
			// glimming
			float new_glim = m_combine_color[2]; 

			if( glim_mod )
			{
				new_glim += pFramerate->m_speed_factor * 0.02f;

				if( new_glim > 0.5f )
				{
					new_glim = 1.0f;
					glim_mod = 0;
				}
			}
			else
			{
				new_glim -= pFramerate->m_speed_factor * 0.02f;

				if( new_glim < 0.0f )
				{
					new_glim = 0.0f;
					glim_mod = 1;
				}
			}

			Set_Color_Combine( new_glim * 0.1f, new_glim * 0.1f, new_glim, GL_ADD );
		}
	}
	// Poison
	else if( m_type == TYPE_MUSHROOM_POISON )
	{
		// animation only if on ground
		if( m_ground_object )
		{
			counter += pFramerate->m_speed_factor * 0.7f;

			while( counter > 1.0f )
			{
				cParticle_Emitter *anim = new cParticle_Emitter();
				anim->Set_Emitter_Rect( m_col_rect.m_x + (m_col_rect.m_w * 0.3f), m_col_rect.m_y + (m_col_rect.m_h * 0.91f), Get_Random_Float( 0, m_col_rect.m_w * 0.4f ), 0 );
				anim->Set_Time_to_Live( 1.4f, 0.4f );
				anim->Set_Scale( 0.7f, 0.2f );
				anim->Set_Color( Color( static_cast<Uint8>( 120 + rand() % 40 ), 190 + ( rand() % 60 ), 0 + rand() % 10 ) );
				anim->Set_Blending( BLEND_ADD );
				anim->Set_Speed( 0.0f, 0.0f );
				anim->Set_Image( pVideo->Get_Surface( "animation/particles/slime_1.png" ) );

				pAnimation_Manager->Add( anim );

				counter--;
			}
		}
	}
}

void cMushroom :: Handle_Collision_Player( cObjectCollision *collision )
{
	// invalid
	if( collision->direction == DIR_UNDEFINED )
	{
		return;
	}

	Activate();
}

void cMushroom :: Handle_Collision_Massive( cObjectCollision *collision )
{
	Turn_Around( collision->direction );
}

void cMushroom :: Handle_Collision_Box( ObjectDirection cdirection, GL_rect *r2 )
{
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
	// unsupported collision direction
	else
	{
		return;
	}

	Reset_On_Ground();
}

/* *** *** *** *** *** *** cFirePlant *** *** *** *** *** *** *** *** *** *** *** */

cFirePlant :: cFirePlant( float x, float y )
: cPowerUp( x, y )
{
	cFirePlant::Init();
}

cFirePlant :: cFirePlant( CEGUI::XMLAttributes &attributes )
: cPowerUp()
{
	cFirePlant::Init();
	cFirePlant::Create_From_Stream( attributes );
}

cFirePlant :: ~cFirePlant( void )
{
	//
}

void cFirePlant :: Init( void )
{
	m_type = TYPE_FIREPLANT;
	m_can_be_on_ground = 0;

	Add_Image( pVideo->Get_Surface( "game/items/fireplant.png" ) );
	Add_Image( pVideo->Get_Surface( "game/items/fireplant_left.png" ) );
	Add_Image( pVideo->Get_Surface( "game/items/fireplant_right.png" ) );

	Set_Image_Num( 0, 1, 0 );

	m_name = _("Fireplant");

	particle_counter = 0.0f;
}

cFirePlant *cFirePlant :: Copy( void )
{
	cFirePlant *fireplant = new cFirePlant( m_start_pos_x, m_start_pos_y );

	return fireplant;
}

void cFirePlant :: Create_From_Stream( CEGUI::XMLAttributes &attributes )
{
	// position
	Set_Pos( static_cast<float>(attributes.getValueAsInteger( "posx" )), static_cast<float>(attributes.getValueAsInteger( "posy" )), 1 );
}

void cFirePlant :: Save_To_Stream( ofstream &file )
{
	// begin item
	file << "\t<item>" << std::endl;

	// type
	file << "\t\t<Property name=\"type\" value=\"fireplant\" />" << std::endl;
	// position
	file << "\t\t<Property name=\"posx\" value=\"" << static_cast<int>(m_start_pos_x) << "\" />" << std::endl;
	file << "\t\t<Property name=\"posy\" value=\"" << static_cast<int>(m_start_pos_y) << "\" />" << std::endl;

	// end item
	file << "\t</item>" << std::endl;
}

void cFirePlant :: Activate( void )
{
	if( !m_active )
	{
		return;
	}

	pPlayer->Get_Item( TYPE_FIREPLANT );

	pHud_Points->Add_Points( 700, m_pos_x + m_image->m_w / 2, m_pos_y );
	
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

void cFirePlant :: Update( void )
{
	if( !m_valid_update || !Is_Visible_On_Screen() )
	{
		return;
	}

	counter += pFramerate->m_speed_factor;

	if( counter > speedfactor_fps * 2.5f )
	{
		// if no direction image set
		if( m_curr_img == 0 )
		{
			Set_Image_Num( 1 + ( rand() % 2 ), 0, 0 );
		}
		// direction image is set
		else
		{
			Set_Image_Num( 0, 0, 0 );
		}

		counter = 0.0f;
	}

	// particle animation
	particle_counter += pFramerate->m_speed_factor * 0.8f;

	while( particle_counter > 1 )
	{
		cParticle_Emitter *anim = new cParticle_Emitter();
		anim->Set_Pos( m_col_rect.m_x + Get_Random_Float( 0.0f, m_col_rect.m_w ), m_col_rect.m_y + Get_Random_Float( 0.0f, m_col_rect.m_h * 0.5f ) );
		anim->Set_Pos_Z( m_pos_z + 0.000001f );
		anim->Set_Direction_Range( 180.0f, 180.0f );
		anim->Set_Scale( 0.4f, 0.1f );
		anim->Set_Time_to_Live( 0.4f );
		anim->Set_Color( Color( static_cast<Uint8>(255), 50 + ( rand() % 50 ), 0 ) );
		anim->Set_Speed( 0.2f, 0.1f );
		anim->Set_Blending( BLEND_ADD );
		
		anim->Set_Image( pVideo->Get_Surface( "animation/particles/light.png" ) );

		pAnimation_Manager->Add( anim );

		particle_counter--;
	}
}

void cFirePlant :: Handle_Collision_Player( cObjectCollision *collision )
{
	// invalid
	if( collision->direction == DIR_UNDEFINED )
	{
		return;
	}

	Activate();
}

/* *** *** *** *** *** *** *** cMoon *** *** *** *** *** *** *** *** *** *** */

cMoon :: cMoon( float x, float y )
: cPowerUp( x, y )
{
	cMoon::Init();
}

cMoon :: cMoon( CEGUI::XMLAttributes &attributes )
: cPowerUp()
{
	cMoon::Init();
	cMoon::Create_From_Stream( attributes );
}

cMoon :: ~cMoon( void )
{
	//
}

void cMoon :: Init( void )
{
	m_type = TYPE_MOON;
	m_can_be_on_ground = 0;

	Add_Image( pVideo->Get_Surface( "game/items/moon_1.png" ), 4800 );
	Add_Image( pVideo->Get_Surface( "game/items/moon_2.png" ), 150 );

	Set_Image_Num( 0, 1 );
	Set_Animation( 1 );
	Set_Animation_Image_Range( 0, 1 );
	Reset_Animation();

	m_name = _("Moon (3-UP)");
}

cMoon *cMoon :: Copy( void )
{
	cMoon *moon = new cMoon( m_start_pos_x, m_start_pos_y );

	return moon;
}

void cMoon :: Create_From_Stream( CEGUI::XMLAttributes &attributes )
{
	// position
	Set_Pos( static_cast<float>(attributes.getValueAsInteger( "posx" )), static_cast<float>(attributes.getValueAsInteger( "posy" )) );
}

void cMoon :: Save_To_Stream( ofstream &file )
{
	// begin item
	file << "\t<item>" << std::endl;

	// type
	file << "\t\t<Property name=\"type\" value=\"moon\" />" << std::endl;
	// position
	file << "\t\t<Property name=\"posx\" value=\"" << static_cast<int>(m_start_pos_x) << "\" />" << std::endl;
	file << "\t\t<Property name=\"posy\" value=\"" << static_cast<int>(m_start_pos_y) << "\" />" << std::endl;

	// end item
	file << "\t</item>" << std::endl;
}

void cMoon :: Activate( void )
{
	if( !m_active )
	{
		return;
	}

	pPlayer->Get_Item( TYPE_MOON );

	pHud_Points->Add_Points( 4000, m_pos_x + m_image->m_w / 2, m_pos_y );

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

void cMoon :: Update( void )
{
	if( !m_valid_update || !Is_Visible_On_Screen() )
	{
		return;
	}

	Update_Animation();

	particle_counter += pFramerate->m_speed_factor;

	// particles
	if( particle_counter > 15.0f )
	{
		cParticle_Emitter *anim = new cParticle_Emitter();
		anim->Set_Pos( m_pos_x + Get_Random_Float( 28.0f, 40.0f ), m_pos_y + Get_Random_Float( 10.0f, 30.0f ) );
		anim->Set_Pos_Z( m_pos_z - 0.0001f );
		anim->Set_Image( pVideo->Get_Surface( "animation/particles/light.png" ) );
		anim->Set_Speed( 0.2f, 0.2f );
		anim->Set_Scale( 0.2f );
		anim->Set_Time_to_Live( 0.9f );
		anim->Set_Color( Color( static_cast<Uint8>(200), 200, 0 ) );
		anim->Set_Blending( BLEND_ADD );

		pAnimation_Manager->Add( anim );

		particle_counter = 0.0f;
	}
}

void cMoon :: Handle_Collision_Player( cObjectCollision *collision )
{
	// invalid
	if( collision->direction == DIR_UNDEFINED )
	{
		return;
	}

	Activate();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
