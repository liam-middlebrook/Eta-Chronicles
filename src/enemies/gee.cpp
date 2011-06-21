/***************************************************************************
 * gee.cpp  -  Electro, Lava or Gift monster
 *
 * Copyright (C) 2006 - 2009 Florian Richter
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../enemies/gee.h"
#include "../core/game_core.h"
#include "../video/animation.h"
#include "../player/player.h"
#include "../gui/hud.h"
#include "../input/mouse.h"
#include "../core/i18n.h"
// CEGUI
#include "CEGUIXMLAttributes.h"

namespace SMC
{

/* *** *** *** *** *** *** cGee *** *** *** *** *** *** *** *** *** *** *** */

cGee :: cGee( float x, float y )
: cEnemy( x, y )
{
	cGee::Init();
}

cGee :: cGee( CEGUI::XMLAttributes &attributes )
: cEnemy()
{
	cGee::Init();
	cGee::Create_From_Stream( attributes );
}

cGee :: ~cGee( void )
{
	//
}

void cGee :: Init( void  )
{
	m_type = TYPE_GEE;
	m_player_range = 1000;
	m_pos_z = 0.088f;
	m_can_be_on_ground = 0;

	m_state = STA_STAY;
	m_speed_fly = 0.0f;
	Set_Max_Distance( 400 );
	m_always_fly = 0;
	m_wait_time = 2.0f;
	m_fly_distance = 400;

	Set_Direction( DIR_HORIZONTAL );
	m_color_type = COL_DEFAULT;
	Set_Color( COL_YELLOW );

	m_kill_sound = "enemy/gee/die.ogg";

	m_wait_time_counter = 0.0f;
	m_fly_distance_counter = 0.0f;
	m_clouds_counter = 0.0f;
}

cGee *cGee :: Copy( void )
{
	cGee *gee = new cGee( m_start_pos_x, m_start_pos_y );
	gee->Set_Direction( m_start_direction );
	gee->Set_Max_Distance( m_max_distance );
	gee->m_always_fly = m_always_fly;
	gee->m_wait_time = m_wait_time;
	gee->m_fly_distance = m_fly_distance;
	gee->Set_Color( m_color_type );

	return gee;
}

void cGee :: Create_From_Stream( CEGUI::XMLAttributes &attributes )
{
	// position
	Set_Pos( static_cast<float>(attributes.getValueAsInteger( "posx" )), static_cast<float>(attributes.getValueAsInteger( "posy" )), 1 );
	// direction
	Set_Direction( Get_Direction_Id( attributes.getValueAsString( "direction", Get_Direction_Name( m_start_direction ) ).c_str() ) );
	// max distance
	Set_Max_Distance( attributes.getValueAsInteger( "max_distance", m_max_distance ) );
	// always fly
	m_always_fly = attributes.getValueAsBool( "always_fly", m_always_fly );
	// wait time
	m_wait_time = attributes.getValueAsFloat( "wait_time", m_wait_time );
	// fly distance
	m_fly_distance = attributes.getValueAsInteger( "fly_distance", m_fly_distance );
	// color
	Set_Color( static_cast<DefaultColor>(Get_Color_Id( attributes.getValueAsString( "color", Get_Color_Name( m_color_type ) ).c_str() )) );
}

void cGee :: Save_To_Stream( ofstream &file )
{
	// begin enemy
	file << "\t<enemy>" << std::endl;

	// name
	file << "\t\t<Property name=\"type\" value=\"gee\" />" << std::endl;
	// position
	file << "\t\t<Property name=\"posx\" value=\"" << static_cast<int>(m_start_pos_x) << "\" />" << std::endl;
	file << "\t\t<Property name=\"posy\" value=\"" << static_cast<int>(m_start_pos_y) << "\" />" << std::endl;
	// direction
	file << "\t\t<Property name=\"direction\" value=\"" << Get_Direction_Name( m_start_direction ) << "\" />" << std::endl;
	// max distance
	file << "\t\t<Property name=\"max_distance\" value=\"" << static_cast<int>(m_max_distance) << "\" />" << std::endl;
	// always fly
	file << "\t\t<Property name=\"always_fly\" value=\"" << m_always_fly << "\" />" << std::endl;
	// wait time
	file << "\t\t<Property name=\"wait_time\" value=\"" << m_wait_time << "\" />" << std::endl;
	// fly distance
	file << "\t\t<Property name=\"fly_distance\" value=\"" << static_cast<int>(m_fly_distance) << "\" />" << std::endl;
	// color
	file << "\t\t<Property name=\"color\" value=\"" << Get_Color_Name( m_color_type ) << "\" />" << std::endl;

	// end enemy
	file << "\t</enemy>" << std::endl;
}

void cGee :: Load_From_Savegame( cSave_Level_Object *save_object )
{
	cEnemy::Load_From_Savegame( save_object );

	Update_Rotation_Hor_velx();
}

void cGee :: Set_Direction( const ObjectDirection dir )
{
	// already set
	if( m_start_direction == dir )
	{
		return;
	}

	// set direction
	cEnemy::Set_Direction( dir, 1 );
	// change left and right to horizontal direction
	if( m_start_direction == DIR_LEFT || m_start_direction == DIR_RIGHT )
	{
		cEnemy::Set_Direction( DIR_HORIZONTAL, 1 );
	}
	// change up and down to vertical direction
	else if( m_start_direction == DIR_UP || m_start_direction == DIR_DOWN )
	{
		cEnemy::Set_Direction( DIR_VERTICAL, 1 );
	}

	if( m_state == STA_FLY )
	{
		// horizontal
		if( m_direction == DIR_HORIZONTAL )
		{
			m_velx = m_speed_fly;
			m_vely = 0.0f;
		}
		// vertical
		else
		{
			m_velx = 0.0f;
			m_vely = m_speed_fly;
		}
	}

	// update direction rotation
	Update_Rotation_Hor( 1 );

	// stop moving
	Stop();
	// set to start position
	Set_Pos( m_start_pos_x, m_start_pos_y );

	Create_Name();
}

void cGee :: Set_Max_Distance( int nmax_distance )
{
	m_max_distance = nmax_distance;

	if( m_max_distance < 0 )
	{
		m_max_distance = 0;
	}
}

void cGee :: Set_Color( DefaultColor col )
{
	// already set
	if( m_color_type == col )
	{
		return;
	}

	// clear old images
	Clear_Images();

	m_color_type = col;

	std::string filename_dir;

	// Electro
	if( m_color_type == COL_YELLOW )
	{
		filename_dir = "electro";

		m_speed_fly = 6.0f;
		m_kill_points = 50;
		m_life_left = 225;

		m_fire_resistant = 0;
	}
	// Lava
	else if( m_color_type == COL_RED )
	{
		filename_dir = "lava";

		m_speed_fly = 8.0f;
		m_kill_points = 300;
		m_life_left = 325;

		m_fire_resistant = 0;
	}
	// Venom
	else if( m_color_type == COL_GREEN )
	{
		filename_dir = "venom";

		m_speed_fly = 10.0f;
		m_kill_points = 200;
		m_life_left = 275;

		m_fire_resistant = 0;
	}

	Create_Name();

	Add_Image( pVideo->Get_Surface( "enemy/gee/" + filename_dir + "/1.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/gee/" + filename_dir + "/2.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/gee/" + filename_dir + "/3.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/gee/" + filename_dir + "/4.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/gee/" + filename_dir + "/5.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/gee/" + filename_dir + "/6.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/gee/" + filename_dir + "/7.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/gee/" + filename_dir + "/8.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/gee/" + filename_dir + "/9.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/gee/" + filename_dir + "/10.png" ) );

	Set_Image_Num( 0, 1 );

	Set_Animation( 1 );
	Set_Animation_Image_Range( 0, 9 );
	Set_Time_All( 140, 1 );
	Reset_Animation();
}

void cGee :: Turn_Around( ObjectDirection col_dir /* = DIR_UNDEFINED */ )
{
	cEnemy::Turn_Around( col_dir );
	// update direction rotation
	Update_Rotation_Hor();
}

void cGee :: DownGrade( bool force /* = 0 */ )
{
	Set_Dead( 1 );
	m_massive_type = MASS_PASSIVE;
	m_counter = 0.0f;
	m_velx = 0.0f;
	m_vely = 0.0f;

	if( !force )
	{
		Generate_Particles( 80 );
	}
	else
	{
		Set_Rotation_Z( 180.0f );
	}
}

void cGee :: Update_Dying( void )
{
	m_counter += pFramerate->m_speed_factor;

	// default death
	if( !Is_Float_Equal( m_rot_z, 180.0f ) )
	{
		Set_Active( 0 );
	}
	// falling death
	else
	{
		// a little bit upwards first
		if( m_counter < 5.0f )
		{
			Move( 0.0f, -5.0f );
		}
		// if not below the screen fall
		else if( m_pos_y < game_res_h + m_col_rect.m_h )
		{
			Move( 0.0f, 20.0f );
		}
		// if below disable
		else
		{
			m_rot_z = 0.0f;
			Set_Active( 0 );
		}
	}
}

void cGee :: Set_Moving_State( Moving_state new_state )
{
	if( new_state == m_state )
	{
		return;
	}

	if( new_state == STA_STAY )
	{
		m_fly_distance_counter = 0.0f;
		m_velx = 0.0f;
		m_vely = 0.0f;
	}
	else if( new_state == STA_FLY )
	{
		m_wait_time_counter = 0.0f;
		// set velocity
		if( m_start_direction == DIR_HORIZONTAL )
		{
			m_velx = m_speed_fly;
			m_vely = 0.0f;
			// randomize direction
			if( rand() % 2 != 1 )
			{
				m_velx *= -1;
			}
		}
		else
		{
			m_velx = 0.0f;
			m_vely = m_speed_fly;
			// randomize direction
			if( rand() % 2 != 1 )
			{
				m_vely *= -1;
			}
		}

		// update rotation
		Update_Direction();
	}

	m_state = new_state;
}

void cGee :: Update( void )
{
	cEnemy::Update();

	if( !m_valid_update || !Is_In_Player_Range() )
	{
		return;
	}

	Update_Animation();

	// staying
	if( m_state == STA_STAY )
	{
		m_wait_time_counter += pFramerate->m_speed_factor;

		// if wait time reached or always fly
		if( m_wait_time_counter > m_wait_time * speedfactor_fps || m_always_fly )
		{
			Activate();
		}
	}
	// moving
	else
	{
		// update fly distance counter
		if( m_velx > 0.0f )
		{
			m_fly_distance_counter += m_velx * pFramerate->m_speed_factor;
		}
		else if( m_velx < 0.0f )
		{
			m_fly_distance_counter -= m_velx * pFramerate->m_speed_factor;
		}
		if( m_vely > 0.0f )
		{
			m_fly_distance_counter += m_vely * pFramerate->m_speed_factor;
		}
		else if( m_vely < 0.0f )
		{
			m_fly_distance_counter -= m_vely * pFramerate->m_speed_factor;
		}

		// walk_distance reached or if beyond max distance
		if( ( !m_always_fly && m_fly_distance_counter > m_fly_distance ) || Is_At_Max_Distance() )
		{
			Stop();
		}

		// generate particle clouds
		m_clouds_counter += pFramerate->m_speed_factor * 0.4f;

		while( m_clouds_counter > 0.0f )
		{
			Generate_Particles();
			m_clouds_counter--;
		}
	}
}

void cGee :: Draw( cSurface_Request *request /* = NULL */ )
{
	if( !m_valid_draw )
	{
		return;
	}

	// draw distance rect
	if( editor_level_enabled )
	{
		if( m_start_direction == DIR_HORIZONTAL )
		{
			pVideo->Draw_Rect( m_start_pos_x - pActive_Camera->x - m_max_distance, m_start_pos_y + ( m_rect.m_h * 0.5f ) - 5.0f - pActive_Camera->y, m_col_rect.m_w + ( m_max_distance * 2.0f ), 10.0f, m_pos_z - 0.000001f, &whitealpha128 );
		}
		else if( m_start_direction == DIR_VERTICAL )
		{
			pVideo->Draw_Rect( m_start_pos_x + ( m_rect.m_w * 0.5f ) - 5.0f - pActive_Camera->x, m_start_pos_y - pActive_Camera->y - m_max_distance, 10.0f, m_col_rect.m_h + ( m_max_distance * 2.0f ), m_pos_z - 0.000001f, &whitealpha128 );
		}
	}

	cEnemy::Draw( request );
}

void cGee :: Activate( void )
{
	// if empty maximum distance or empty walk distance
	if( !m_max_distance || !m_fly_distance )
	{
		return;
	}

	Set_Moving_State( STA_FLY );

	// check max distance
	if( Is_At_Max_Distance() )
	{
		// turn around also updates the direction rotation
		Turn_Around();
	}
	else
	{
		// update direction rotation
		Update_Rotation_Hor();
	}
}

void cGee :: Stop( void )
{
	Set_Moving_State( STA_STAY );
}

void cGee :: Generate_Particles( unsigned int amount /* = 4 */ ) const
{
	cParticle_Emitter *anim = new cParticle_Emitter();
	anim->Set_Emitter_Rect( m_col_rect.m_x + ( m_col_rect.m_w * 0.3f ), m_col_rect.m_y + ( m_col_rect.m_h * 0.2f ), m_col_rect.m_w * 0.4f, m_col_rect.m_h * 0.3f );
	anim->Set_Quota( amount );
	anim->Set_Image( pVideo->Get_Surface( "animation/particles/cloud.png" ) );

	if( !m_dead )
	{
		anim->Set_Speed( 0.0f, 0.5f );

		// direction
		if( m_direction == DIR_LEFT )
		{
			anim->Set_Direction_Range( 90.0f, 180.0f );
		}
		else
		{
			anim->Set_Direction_Range( 270.0f, 180.0f );
		}
	}
	else
	{
		anim->Set_Speed( 0.1f, 0.6f );
	}

	anim->Set_Scale( 0.5f, 0.3f );

	// color
	if( m_color_type == COL_YELLOW )
	{
		anim->Set_Color( yellow );
	}
	else if( m_color_type == COL_RED )
	{
		anim->Set_Color( lightred );
	}
	else if( m_color_type == COL_GREEN )
	{
		anim->Set_Color( lightgreen );
	}
	anim->Set_Time_to_Live( 2.0f );
	anim->Set_Fading_Alpha( 1 );
	anim->Set_Blending( BLEND_ADD );
	pAnimation_Manager->Add( anim );
}

bool cGee :: Is_At_Max_Distance( void ) const
{
	if( m_direction == DIR_UP )
	{
		if( m_pos_y - m_start_pos_y < -m_max_distance )
		{
			return 1;
		}
	}
	else if( m_direction == DIR_DOWN )
	{
		if( m_pos_y - m_start_pos_y > m_max_distance )
		{
			return 1;
		}
	}
	else if( m_direction == DIR_LEFT )
	{
		if( m_pos_x - m_start_pos_x < -m_max_distance )
		{
			return 1;
		}
	}
	else if( m_direction == DIR_RIGHT )
	{
		if( m_pos_x - m_start_pos_x > m_max_distance )
		{
			return 1;
		}
	}

	return 0;
}

bool cGee :: Is_Update_Valid( void )
{
	if( m_dead || m_freeze_counter )
	{
		return 0;
	}

	return 1;
}

bool cGee :: Is_Draw_Valid( void )
{
	bool valid = cEnemy::Is_Draw_Valid();

	// if editor enabled
	if( editor_enabled )
	{
		// if active mouse object
		if( pMouseCursor->m_active_object == this )
		{
			return 1;
		}
	}

	return valid;
}

Col_Valid_Type cGee :: Validate_Collision( cSprite *obj )
{
	if( obj->m_massive_type == MASS_MASSIVE )
	{
		switch( obj->m_type )
		{
			case TYPE_PLAYER:
			{
				return COL_VTYPE_INTERNAL;
			}
			case TYPE_BALL:
			{
				return COL_VTYPE_INTERNAL;
			}
			default:
			{
				break;
			}
		}

		return COL_VTYPE_NOT_VALID;
	}

	return COL_VTYPE_NOT_VALID;
}

void cGee :: Handle_Collision_Player( cObjectCollision *collision )
{
	// unknown direction
	if( collision->direction == DIR_UNDEFINED )
	{
		return;
	}

	if( collision->direction == DIR_TOP && pPlayer->m_state != STA_FLY )
	{
		pAudio->Play_Sound( m_kill_sound );

		DownGrade();
		pPlayer->Action_Jump( 1 );

		pHud_Points->Add_Points( m_kill_points, pPlayer->m_pos_x, pPlayer->m_pos_y, "", static_cast<Uint8>(255), 1 );
		pPlayer->Add_Kill_Multiplier();
	}
	else if( !pPlayer->invincible )
	{
		if( pPlayer->maryo_type != MARYO_SMALL )
		{
			// todo : create again
			//pAudio->PlaySound( "player/maryo_au.ogg", RID_MARYO_AU );

			if( collision->direction == DIR_BOTTOM  )
			{
				pPlayer->Action_Jump( 1 );
			}
			else if( collision->direction == DIR_LEFT )
			{
				pPlayer->m_velx = -7.0f;
			}
			else if( collision->direction == DIR_RIGHT )
			{
				pPlayer->m_velx = 7.0f;
			}
		}

		pPlayer->DownGrade_Player();

		if( collision->direction == DIR_RIGHT || collision->direction == DIR_LEFT )
		{
			Turn_Around( collision->direction );
		}
	}
}

void cGee :: Handle_Collision_Massive( cObjectCollision *collision )
{
	Send_Collision( collision );
}

void cGee :: Editor_Activate( void )
{
	// get window manager
	CEGUI::WindowManager &wmgr = CEGUI::WindowManager::getSingleton();

	// direction
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox *>(wmgr.createWindow( "TaharezLook/Combobox", "editor_gee_direction" ));
	Editor_Add( UTF8_("Direction"), UTF8_("Starting direction."), combobox, 100, 75 );

	combobox->addItem( new CEGUI::ListboxTextItem( "horizontal" ) );
	combobox->addItem( new CEGUI::ListboxTextItem( "vertical" ) );

	combobox->setText( Get_Direction_Name( m_start_direction ) );
	combobox->subscribeEvent( CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber( &cGee::Editor_Direction_Select, this ) );

	// max distance
	CEGUI::Editbox *editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "editor_gee_max_distance" ));
	Editor_Add( UTF8_("Distance"), UTF8_("Movable distance"), editbox, 90 );

	editbox->setValidationString( "^[+]?\\d*$" );
	editbox->setText( int_to_string( m_max_distance ) );
	editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cGee::Editor_Max_Distance_Text_Changed, this ) );

	// always fly
	combobox = static_cast<CEGUI::Combobox *>(wmgr.createWindow( "TaharezLook/Combobox", "editor_gee_always_fly" ));
	Editor_Add( UTF8_("Always fly"), UTF8_("Move without stopping at the fly distance"), combobox, 120, 80 );

	combobox->addItem( new CEGUI::ListboxTextItem( UTF8_("Enabled") ) );
	combobox->addItem( new CEGUI::ListboxTextItem( UTF8_("Disabled") ) );

	if( m_always_fly )
	{
		combobox->setText( UTF8_("Enabled") );
	}
	else
	{
		combobox->setText( UTF8_("Disabled") );
	}

	combobox->subscribeEvent( CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber( &cGee::Editor_Always_Fly_Select, this ) );

	// wait time
	editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "editor_gee_wait_time" ));
	Editor_Add( UTF8_("Wait time"), UTF8_("Time to wait until moving again after a stop"), editbox, 90 );

	editbox->setValidationString( "[+]?[0-9]*\\.?[0-9]*" );
	editbox->setText( float_to_string( m_wait_time ) );
	editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cGee::Editor_Wait_Time_Text_Changed, this ) );

	// fly distance
	editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "editor_gee_fly_distance" ));
	Editor_Add( UTF8_("Fly distance"), UTF8_("The distance to move each time"), editbox, 90 );

	editbox->setValidationString( "^[+]?\\d*$" );
	editbox->setText( int_to_string( m_fly_distance ) );
	editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cGee::Editor_Fly_Distance_Text_Changed, this ) );

	// init
	Editor_Init();
}

bool cGee :: Editor_Direction_Select( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	CEGUI::ListboxItem *item = static_cast<CEGUI::Combobox *>( windowEventArgs.window )->getSelectedItem();

	Set_Direction( Get_Direction_Id( item->getText().c_str() ) );

	return 1;
}

bool cGee :: Editor_Max_Distance_Text_Changed( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	std::string str_text = static_cast<CEGUI::Editbox *>( windowEventArgs.window )->getText().c_str();

	Set_Max_Distance( string_to_int( str_text ) );

	return 1;
}

bool cGee :: Editor_Always_Fly_Select( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	CEGUI::ListboxItem *item = static_cast<CEGUI::Combobox *>( windowEventArgs.window )->getSelectedItem();

	if( item->getText().compare( UTF8_("Enabled") ) == 0 )
	{
		m_always_fly = 1;
	}
	else
	{
		m_always_fly = 0;
	}

	return 1;
}

bool cGee :: Editor_Wait_Time_Text_Changed( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	std::string str_text = static_cast<CEGUI::Editbox *>( windowEventArgs.window )->getText().c_str();

	m_wait_time = string_to_float( str_text );

	return 1;
}

bool cGee :: Editor_Fly_Distance_Text_Changed( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	std::string str_text = static_cast<CEGUI::Editbox *>( windowEventArgs.window )->getText().c_str();

	m_fly_distance = string_to_int( str_text );

	return 1;
}

void cGee :: Create_Name( void )
{
	m_name = "Gee";

	if( m_color_type == COL_YELLOW )
	{
		m_name += "lectro";
	}
	else if( m_color_type == COL_RED )
	{
		m_name += "lava";
	}
	else if( m_color_type == COL_GREEN )
	{
		m_name += "venom";
	}

	if( m_start_direction == DIR_HORIZONTAL )
	{
		m_name += " Hor";
	}
	else if( m_start_direction == DIR_VERTICAL )
	{
		m_name += " Ver";
	}
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
