/***************************************************************************
 * eato.cpp  -  eating static plant :P
 *
 * Copyright (C) 2011 - Cody Van De Mark
 * Copyright (C) 2006 - 2009 Florian Richter (Original)
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../enemies/eato.h"
#include "../core/game_core.h"
#include "../video/animation.h"
#include "../player/player.h"
#include "../video/gl_surface.h"
#include "../core/i18n.h"
#include "../core/filesystem/filesystem.h"
// boost filesystem
#include "boost/filesystem/convenience.hpp"
namespace fs = boost::filesystem;
// CEGUI
#include "CEGUIXMLAttributes.h"

namespace SMC
{

/* *** *** *** *** *** *** cEato *** *** *** *** *** *** *** *** *** *** *** */

cEato :: cEato( float x, float y )
: cEnemy( x, y )
{
	cEato::Init();
}

cEato :: cEato( CEGUI::XMLAttributes &attributes )
: cEnemy()
{
	cEato::Init();
	cEato::Create_From_Stream( attributes );
}

cEato :: ~cEato( void )
{
	//
}

void cEato :: Init( void )
{
	m_type = TYPE_EATO;
	m_player_range = 1000;
	m_pos_z = 0.087f;
	m_can_be_on_ground = 0;
	Set_Rotation_Affects_Rect( 1 );
	m_fire_resistant = 1;

	m_state = STA_STAY;
	Set_Image_Dir( "enemy/eato/brown/" );
	Set_Direction( DIR_UP_LEFT );

	m_kill_sound = "enemy/eato/die.ogg";
	m_kill_points = 150;
}

cEato *cEato :: Copy( void )
{
	cEato *eato = new cEato( m_start_pos_x, m_start_pos_y );
	eato->Set_Image_Dir( m_img_dir );
	eato->Set_Direction( m_start_direction );

	return eato;
}

void cEato :: Create_From_Stream( CEGUI::XMLAttributes &attributes )
{
	// position
	Set_Pos( static_cast<float>(attributes.getValueAsInteger( "posx" )), static_cast<float>(attributes.getValueAsInteger( "posy" )), 1 );
	// image directory
	Set_Image_Dir( attributes.getValueAsString( "image_dir", m_img_dir ).c_str() );
	// direction
	Set_Direction( Get_Direction_Id( attributes.getValueAsString( "direction", Get_Direction_Name( m_start_direction ) ).c_str() ) );
}

void cEato :: Save_To_Stream( ofstream &file )
{
	// begin enemy
	file << "\t<enemy>" << std::endl;

	// name
	file << "\t\t<Property name=\"type\" value=\"eato\" />" << std::endl;
	// position
	file << "\t\t<Property name=\"posx\" value=\"" << static_cast<int>(m_start_pos_x) << "\" />" << std::endl;
	file << "\t\t<Property name=\"posy\" value=\"" << static_cast<int>(m_start_pos_y) << "\" />" << std::endl;
	// image directory
	file << "\t\t<Property name=\"image_dir\" value=\"" << m_img_dir << "\" />" << std::endl;
	// direction
	file << "\t\t<Property name=\"direction\" value=\"" << Get_Direction_Name( m_start_direction ) << "\" />" << std::endl;

	// end enemy
	file << "\t</enemy>" << std::endl;
}

void cEato :: Set_Image_Dir( std::string dir )
{
	if( dir.empty() )
	{
		return;
	}

	// remove pixmaps dir
	if( dir.find( DATA_DIR "/" GAME_PIXMAPS_DIR "/" ) == 0 )
	{
		dir.erase( 0, strlen( DATA_DIR "/" GAME_PIXMAPS_DIR "/" ) );
	}

	// add trailing slash if missing
	if( *(dir.end() - 1) != '/' )
	{
		dir.insert( dir.length(), "/" );
	}

	// if not image directory
	if( !File_Exists( DATA_DIR "/" GAME_PIXMAPS_DIR "/" + dir + "1.png" ) && !File_Exists( DATA_DIR "/" GAME_PIXMAPS_DIR "/" + dir + "1.settings" ) )
	{
		printf( "Warning : Eato image dir does not exist %s\n", dir.c_str() );
		return;
	}

	m_img_dir = dir;

	// clear images
	Clear_Images();
	// set images
	Add_Image( pVideo->Get_Surface( m_img_dir + "1.png" ) );
	Add_Image( pVideo->Get_Surface( m_img_dir + "2.png" ) );
	Add_Image( pVideo->Get_Surface( m_img_dir + "3.png" ) );
	Add_Image( pVideo->Get_Surface( m_img_dir + "2.png" ) );
	// set start image
	Set_Image_Num( 0, 1 );

	Set_Animation( 1 );
	Set_Animation_Image_Range( 0, 3 );
	Set_Time_All( 180, 1 );
	Reset_Animation();

	Create_Name();
}

void cEato :: Set_Direction( const ObjectDirection dir )
{
	// already set
	if( m_start_direction == dir )
	{
		return;
	}

	cEnemy::Set_Direction( dir, 1 );

	// clear
	Set_Rotation( 0.0f, 0.0f, 0.0f, 1 );

	if( m_start_direction == DIR_UP_LEFT )
	{
		Set_Rotation_Y( 180.0f, 1 );
	}
	else if( m_start_direction == DIR_UP_RIGHT )
	{
		// default
	}
	else if( m_start_direction == DIR_LEFT_UP )
	{
		Set_Rotation_Z( 90.0f, 1 );
		Set_Rotation_X( 180.0f, 1 );
	}
	else if( m_start_direction == DIR_LEFT_DOWN )
	{
		Set_Rotation_Z( 90.0f, 1 );
	}
	else if( m_start_direction == DIR_RIGHT_UP )
	{
		Set_Rotation_Z( 270.0f, 1 );
	}
	else if( m_start_direction == DIR_RIGHT_DOWN )
	{
		Set_Rotation_Z( 270.0f, 1 );
		Set_Rotation_X( 180.0f, 1 );
	}
	else if( m_start_direction == DIR_DOWN_LEFT )
	{
		Set_Rotation_X( 180.0f, 1 );
	}
	else if( m_start_direction == DIR_DOWN_RIGHT )
	{
		Set_Rotation_Z( 180.0f, 1 );
	}

	Create_Name();
}

void cEato :: DownGrade( bool force /* = 0 */ )
{
	Set_Dead( 1 );
	m_massive_type = MASS_PASSIVE;
	m_counter = 0.0f;

	if( !force )
	{
		// animation
		cParticle_Emitter *anim = new cParticle_Emitter();
		Generate_Hit_Animation( anim );

		anim->Set_Scale( 0.8f );
		anim->Set_Direction_Range( 0.0f, 360.0f );
		// add animation
		pAnimation_Manager->Add( anim );
	}
	else
	{
		Set_Rotation_Z( 180.0f );
	}
}

void cEato :: Update_Dying( void )
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

void cEato :: Update( void )
{
	cEnemy::Update();

	if( !m_valid_update || !Is_In_Player_Range() )
	{
		return;
	}

	Update_Animation();
}

bool cEato :: Is_Update_Valid( void )
{
	if( m_dead || m_freeze_counter )
	{
		return 0;
	}

	return 1;
}

Col_Valid_Type cEato :: Validate_Collision( cSprite *obj )
{
	if( obj->m_massive_type == MASS_MASSIVE )
	{
		switch( obj->m_type )
		{
			case TYPE_PLAYER:
			{
				return COL_VTYPE_BLOCKING;
			}
			case TYPE_BALL:
			{
				return COL_VTYPE_BLOCKING;
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

void cEato :: Handle_Collision_Player( cObjectCollision *collision )
{
	// unknown direction
	if( collision->direction == DIR_UNDEFINED )
	{
		return;
	}

	// only if not invincible
	if( pPlayer->invincible <= 0.0f )
	{
		// if player is big and not a bottom collision
		if( pPlayer->maryo_type != MARYO_SMALL && ( collision->direction != DIR_BOTTOM ) )
		{
			// todo : create again
			//pAudio->PlaySound( "player/maryo_au.ogg", RID_MARYO_AU );
			pPlayer->Action_Jump( 1 );
		}

		pPlayer->DownGrade_Player();
	}
}

void cEato :: Editor_Activate( void )
{
	// get window manager
	CEGUI::WindowManager &wmgr = CEGUI::WindowManager::getSingleton();

	// direction
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox *>(wmgr.createWindow( "TaharezLook/Combobox", "editor_eato_direction" ));
	Editor_Add( UTF8_("Direction"), UTF8_("Direction"), combobox, 100, 200 );

	combobox->addItem( new CEGUI::ListboxTextItem( "top_left" ) );
	combobox->addItem( new CEGUI::ListboxTextItem( "top_right" ) );
	combobox->addItem( new CEGUI::ListboxTextItem( "bottom_left" ) );
	combobox->addItem( new CEGUI::ListboxTextItem( "bottom_right" ) );
	combobox->addItem( new CEGUI::ListboxTextItem( "left_top" ) );
	combobox->addItem( new CEGUI::ListboxTextItem( "left_bottom" ) );
	combobox->addItem( new CEGUI::ListboxTextItem( "right_top" ) );
	combobox->addItem( new CEGUI::ListboxTextItem( "right_bottom" ) );

	combobox->setText( Get_Direction_Name( m_start_direction ) );
	combobox->subscribeEvent( CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber( &cEato::Editor_Direction_Select, this ) );

	// image dir
	CEGUI::Editbox *editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "editor_eato_image_dir" ));
	Editor_Add( UTF8_("Image directory"), UTF8_("Directory containing the images"), editbox, 200 );

	editbox->setText( m_img_dir.c_str() );
	editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cEato::Editor_Image_Dir_Text_Changed, this ) );
	// init
	Editor_Init();
}

bool cEato :: Editor_Direction_Select( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	CEGUI::ListboxItem *item = static_cast<CEGUI::Combobox *>( windowEventArgs.window )->getSelectedItem();

	Set_Direction( Get_Direction_Id( item->getText().c_str() ) );

	return 1;
}

bool cEato :: Editor_Image_Dir_Text_Changed( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	std::string str_text = static_cast<CEGUI::Editbox *>( windowEventArgs.window )->getText().c_str();

	Set_Image_Dir( str_text );

	return 1;
}

void cEato :: Create_Name( void )
{
	m_name = "Eato ";
	m_name += _(Get_Direction_Name( m_start_direction ).c_str());

	if( m_start_image && !m_start_image->m_name.empty() )
	{
		m_name += " " + m_start_image->m_name;
	}
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
