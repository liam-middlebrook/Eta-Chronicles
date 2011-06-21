/***************************************************************************
 * turtle.cpp  -  turtle enemy class
 *
 * Copyright (C) 2011 - Cody Van De Mark
 * Copyright (C) 2003 - 2009 Florian Richter (Original)
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../enemies/turtle.h"
#include "../core/game_core.h"
#include "../objects/box.h"
#include "../video/animation.h"
#include "../player/player.h"
#include "../level/level.h"
#include "../gui/hud.h"
#include "../video/gl_surface.h"
#include "../user/savegame.h"
#include "../core/sprite_manager.h"
#include "../core/i18n.h"
// CEGUI
#include "CEGUIXMLAttributes.h"

namespace SMC
{

/* *** *** *** *** *** *** cTurtle *** *** *** *** *** *** *** *** *** *** *** */

cTurtle :: cTurtle( float x, float y )
: cEnemy( x, y )
{
	cTurtle::Init();
}

cTurtle :: cTurtle( CEGUI::XMLAttributes &attributes )
: cEnemy()
{
	cTurtle::Init();
	cTurtle::Create_From_Stream( attributes );
}

cTurtle :: ~cTurtle( void )
{
	//
}

void cTurtle :: Init( void )
{
	m_type = TYPE_TURTLE;
	m_pos_z = 0.091f;

	m_player_counter = 0.0f;
	m_turtle_state = TURTLE_DEAD;
	Set_Turtle_Moving_State( TURTLE_WALK );

	m_color_type = COL_DEFAULT;
	Set_Color( COL_RED );
	Set_Direction( DIR_RIGHT, 1 );

	m_kill_sound = "stomp_4.ogg";
}

cTurtle *cTurtle :: Copy( void )
{
	cTurtle *turtle = new cTurtle( m_start_pos_x, m_start_pos_y );
	turtle->Set_Direction( m_start_direction, 1 );
	turtle->Set_Color( m_color_type );

	return turtle;
}

void cTurtle :: Create_From_Stream( CEGUI::XMLAttributes &attributes )
{
	// position
	Set_Pos( static_cast<float>(attributes.getValueAsInteger( "posx" )), static_cast<float>(attributes.getValueAsInteger( "posy" )), 1 );
	// direction
	Set_Direction( Get_Direction_Id( attributes.getValueAsString( "direction", Get_Direction_Name( m_start_direction ) ).c_str() ), 1 );
	// color
	Set_Color( static_cast<DefaultColor>(Get_Color_Id( attributes.getValueAsString( "color", Get_Color_Name( m_color_type ) ).c_str() )) );
}

void cTurtle :: Save_To_Stream( ofstream &file )
{
	// begin enemy
	file << "\t<enemy>" << std::endl;

	// name
	file << "\t\t<Property name=\"type\" value=\"turtle\" />" << std::endl;
	// position
	file << "\t\t<Property name=\"posx\" value=\"" << static_cast<int>(m_start_pos_x) << "\" />" << std::endl;
	file << "\t\t<Property name=\"posy\" value=\"" << static_cast<int>(m_start_pos_y) << "\" />" << std::endl;
	// color
	file << "\t\t<Property name=\"color\" value=\"" << Get_Color_Name( m_color_type ) << "\" />" << std::endl;
	// direction
	file << "\t\t<Property name=\"direction\" value=\"" << Get_Direction_Name( m_start_direction ) << "\" />" << std::endl;

	// end enemy
	file << "\t</enemy>" << std::endl;
}

void cTurtle :: Load_From_Savegame( cSave_Level_Object *save_object )
{
	cEnemy::Load_From_Savegame( save_object );

	// turtle_state
	if( save_object->exists( "turtle_state" ) )
	{
		Turtle_state mov_state = static_cast<Turtle_state>(string_to_int( save_object->Get_Value( "turtle_state" ) ));

		if( mov_state == TURTLE_SHELL_STAND || mov_state == TURTLE_SHELL_RUN )
		{
			Set_Turtle_Moving_State( mov_state );
			// set shell image without position changes
			cSprite::Set_Image( m_images[5].m_image );
		}
	}
	
	Update_Rotation_Hor_velx();
}

cSave_Level_Object *cTurtle :: Save_To_Savegame( void )
{
	cSave_Level_Object *save_object = cEnemy::Save_To_Savegame();

	// turtle_state ( only save if needed )
	if( m_turtle_state != TURTLE_WALK )
	{
		save_object->m_properties.push_back( cSave_Level_Object_Property( "turtle_state", int_to_string( m_turtle_state ) ) );
	}

	return save_object;
}

void cTurtle :: Set_Direction( const ObjectDirection dir, bool new_start_direction /* = 0 */ )
{
	if( dir != DIR_RIGHT && dir != DIR_LEFT )
	{
		printf( "Warning : Unknown Turtle direction set %s\n", Get_Direction_Name( dir ).c_str() );
		return;
	}

	cEnemy::Set_Direction( dir, new_start_direction );
	Update_Velocity();
	Update_Rotation_Hor_velx( new_start_direction );

	if( new_start_direction )
	{
		Create_Name();
	}
}

void cTurtle :: Set_Color( DefaultColor col )
{
	// already set
	if( m_color_type == col )
	{
		return;
	}

	m_color_type = col;

	std::string filename_dir;

	if( m_color_type == COL_RED )
	{
		filename_dir = "red";

		m_speed_walk = 3.6f;
		m_speed_shell = 14;
		m_kill_points = 50;
		m_life_left = 175;
	}
	else if( m_color_type == COL_GREEN )
	{
		filename_dir = "green";

		m_speed_walk = 4.5f;
		m_speed_shell = 17;
		m_kill_points = 150;
		m_life_left = 200;
	}
	// unknown color
	else
	{
		printf( "Error : Unknown Turtle color : %d\n", m_color_type );
	}

	Update_Velocity();
	Update_Rotation_Hor_velx();

	Clear_Images();

	// Walk
	Add_Image( pVideo->Get_Surface( "enemy/turtle/" + filename_dir + "/walk_0.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/turtle/" + filename_dir + "/walk_1.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/turtle/" + filename_dir + "/walk_2.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/turtle/" + filename_dir + "/walk_1.png" ) );
	// Walk Turn
	//Add_Image( pVideo->Get_Surface( "enemy/turtle/" + filename_dir + "/turn_1.png" ) );
	Add_Image( NULL );
	// Shell
	Add_Image( pVideo->Get_Surface( "enemy/turtle/" + filename_dir + "/shell_front.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/turtle/" + filename_dir + "/shell_move_1.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/turtle/" + filename_dir + "/shell_move_2.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/turtle/" + filename_dir + "/shell_move_3.png" ) );
	Add_Image( pVideo->Get_Surface( "enemy/turtle/" + filename_dir + "/shell_active.png" ) );

	Set_Image_Num( 0, 1 );
	Create_Name();
}

void cTurtle :: Turn_Around( ObjectDirection col_dir /* = DIR_UNDEFINED */ )
{
	cEnemy::Turn_Around( col_dir );

	if( m_turtle_state == TURTLE_WALK )
	{
		// hack : disable turn image
		//Set_Image_Num( 4 );
		//Set_Animation( 0 );
		//Reset_Animation();
	}

	Update_Rotation_Hor_velx();
}

void cTurtle :: DownGrade( bool force /* = 0 */ )
{
	if( !force )
	{
		// normal walking
		if( m_turtle_state == TURTLE_WALK )
		{
			Set_Turtle_Moving_State( TURTLE_SHELL_STAND );
			Col_Move( 0, m_images[0].m_image->m_col_h - m_images[5].m_image->m_col_h, 1, 1 );
		}
		// staying
		else if( m_turtle_state == TURTLE_SHELL_STAND )
		{
			Set_Turtle_Moving_State( TURTLE_SHELL_RUN );
		}
		// running shell
		else if( m_turtle_state == TURTLE_SHELL_RUN )
		{
			Set_Turtle_Moving_State( TURTLE_SHELL_STAND );
		}
	}
	// falling death
	else
	{
		m_counter = 8.0f;
		Set_Dead( 1 );
		m_massive_type = MASS_PASSIVE;
		m_velx = 0.0f;
		m_vely = 0.0f;

		if( m_turtle_state == TURTLE_WALK )
		{
			Move( 0.0f, m_images[0].m_image->m_h - m_images[5].m_image->m_h, 1 );
		}

		Set_Image_Num( 5 );
	}
}

void cTurtle :: Update_Dying( void )
{
	m_counter += pFramerate->m_speed_factor * 0.5f;

	// if not below the screen fall
	if( m_pos_y < game_res_h + m_col_rect.m_h )
	{
		float speed_y = m_counter;

		// first a little bit upwards
		if( speed_y < 10.0f )
		{
			speed_y *= -1;
		}
		// limit falling speed
		else if( speed_y > 25.0f )
		{
			speed_y = 25.0f;
		}

		float speed_x = pFramerate->m_speed_factor;

		if( m_direction == DIR_LEFT )
		{
			speed_x *= -1;
		}

		Add_Rotation_Z( speed_x );

		Move( speed_x * 15.0f, speed_y );
	}
	// if below disable
	else
	{
		Set_Active( 0 );
		m_rot_z = 0.0f;
		m_state = STA_STAY;
		m_turtle_state = TURTLE_DEAD;
	}
}

void cTurtle :: Set_Turtle_Moving_State( Turtle_state new_state )
{
	if( new_state == m_turtle_state )
	{
		return;
	}

	if( new_state == TURTLE_WALK )
	{
		m_state = STA_WALK;
		m_player_range = 1500;

		Set_Animation( 1 );
		Set_Animation_Image_Range( 0, 3 );
		Set_Time_All( 130, 1 );
		Reset_Animation();
		Set_Image_Num( m_anim_img_start );
	}
	else if( new_state == TURTLE_SHELL_STAND )
	{
		m_state = STA_STAY;
		m_player_range = 2000;

		Set_Animation( 0 );
		// set stay image
		Set_Animation_Image_Range( 5, 5 );
		Set_Image_Num( m_anim_img_start );
	}
	else if( new_state == TURTLE_SHELL_RUN )
	{
		m_state = STA_RUN;
		m_player_range = 5000;

		Set_Animation( 1 );
		Set_Animation_Image_Range( 6, 9 );
		Set_Time_All( 80, 1 );
		Reset_Animation();
		Set_Image_Num( m_anim_img_start );
	}

	m_turtle_state = new_state;

	if( m_turtle_state == TURTLE_WALK )
	{
		Update_Velocity();
		Update_Rotation_Hor_velx();
	}
	else if( m_turtle_state == TURTLE_SHELL_RUN )
	{
		Update_Velocity();
		Update_Rotation_Hor_velx();
	}
}

void cTurtle :: Update( void )
{
	cEnemy::Update();

	if( !m_valid_update || !Is_In_Player_Range() )
	{
		return;
	}

	Update_Animation();

	// walking
	if( m_turtle_state == TURTLE_WALK )
	{
		// if turn around image
		if( m_curr_img == 4 )
		{
			m_anim_counter += pFramerate->m_elapsed_ticks;

			// set normal image back
			if( m_anim_counter >= 200 )
			{
				Set_Animation( 1 );
				Reset_Animation();
				Set_Image_Num( m_anim_img_start );
				Update_Rotation_Hor_velx();
			}
			// rotate the turn image
			else if( m_anim_counter >= 100 )
			{
				Update_Rotation_Hor_velx();
			}
		}
	}
	// standing shell
	else if( m_turtle_state == TURTLE_SHELL_STAND )
	{
		m_counter += pFramerate->m_speed_factor;

		// stop waiting
		if( m_counter > 160.0f )
		{
			// animation
			if( m_counter < 192.0f )
			{
				if( static_cast<int>(m_counter) % 5 == 1 )
				{
					Set_Image_Num( 9 ); // active
				}
				else
				{
					Set_Image_Num( 5 ); // front
				}
			}
			// activate
			else
			{
				m_counter = 0.0f;
				Stand_Up();
			}
		}

		// slow down
		if( !Is_Float_Equal( m_velx, 0.0f ) )
		{
			Add_Velocity( -m_velx * 0.2f, 0.0f );

			if( m_velx < 0.3f && m_velx > -0.3f )
			{
				m_velx = 0.0f;
			}
		}
	}
	// moving shell
	else if( m_turtle_state == TURTLE_SHELL_RUN )
	{
		//
	}

	if( m_player_counter > 0.0f )
	{
		m_player_counter -= pFramerate->m_speed_factor;

		if( m_player_counter <= 0.0f )
		{
			// do not start collision detection if colliding with maryo
			if( pPlayer->m_col_rect.Intersects( m_col_rect ) )
			{
				m_player_counter = 5.0f;
			}
			else
			{
				m_player_counter = 0.0f;
			}
		}
	}

	// gravity
	Update_Gravity();
}

void cTurtle :: Stand_Up( void )
{
	if( m_turtle_state != TURTLE_SHELL_STAND && m_turtle_state != TURTLE_SHELL_RUN )
	{
		return;
	}

	// get space needed to stand up
	float move_y = m_image->m_col_h - m_images[0].m_image->m_col_h;

	cObjectCollisionType *col_list = Collision_Check_Relative( 0.0f, move_y, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING );

	// failed to stand up because something is blocking
	if( !col_list->empty() )
	{
		delete col_list;
		return;
	}

	delete col_list;

	pAudio->Play_Sound( "enemy/turtle/stand_up.wav" );
	Col_Move( 0.0f, move_y, 1, 1 );
	Set_Turtle_Moving_State( TURTLE_WALK );
}

bool cTurtle :: Hit_Enemy( cEnemy *enemy )
{
	// invalid
	if( !enemy )
	{
		return 0;
	}

	// don't collide with already dead enemies
	if( enemy->m_dead )
	{
		return 0;
	}
	// shell can not hit it
	if( !enemy->m_can_be_hit_from_shell )
	{
		// enemies that only hit us
		if( enemy->m_type == TYPE_SPIKEBALL )
		{
			DownGrade( 1 );
		}

		return 0;
	}

	// if red shell check if colliding turtle is green and also running
	if( m_color_type == COL_RED && enemy->m_type == TYPE_TURTLE )
	{
		cTurtle *turtle = static_cast<cTurtle *>(enemy);

		// red shell can't kill the green shell
		if( turtle->m_color_type == COL_GREEN && turtle->m_state == STA_RUN )
		{
			return 0;
		}
	}

	// hit enemy
	pAudio->Play_Sound( enemy->m_kill_sound );
	pHud_Points->Add_Points( enemy->m_kill_points, m_pos_x + m_image->m_w / 3, m_pos_y - 5.0f, "", static_cast<Uint8>(255), 1 );
	enemy->DownGrade( 1 );
	pPlayer->Add_Kill_Multiplier();

	// enemies that also hit us
	if( enemy->m_type == TYPE_SPIKA )
	{
		DownGrade( 1 );
	}
	
	return 1;
}

void cTurtle :: Update_Velocity( void )
{
	if( m_direction == DIR_RIGHT )
	{
		if( m_turtle_state == TURTLE_WALK )
		{
			m_velx = m_speed_walk;
		}
		else if( m_turtle_state == TURTLE_SHELL_RUN )
		{
			m_velx = m_speed_shell;
		}
		// shell stay does slow down automatically
	}
	else
	{
		if( m_turtle_state == TURTLE_WALK )
		{
			m_velx = -m_speed_walk;
		}
		else if( m_turtle_state == TURTLE_SHELL_RUN )
		{
			m_velx = -m_speed_shell;
		}
		// shell stay does slow down automatically
	}
}

bool cTurtle :: Is_Update_Valid( void )
{
	if( m_dead || m_freeze_counter || m_state == STA_OBJ_LINKED )
	{
		return 0;
	}

	return 1;
}

Col_Valid_Type cTurtle :: Validate_Collision( cSprite *obj )
{
	// basic validation checking
	Col_Valid_Type basic_valid = Validate_Collision_Ghost( obj );

	// found valid collision
	if( basic_valid != COL_VTYPE_NOT_POSSIBLE )
	{
		return basic_valid;
	}

	if( obj->m_massive_type == MASS_MASSIVE )
	{
		switch( obj->m_type )
		{
			case TYPE_PLAYER:
			{
				// player is invincible
				if( pPlayer->invincible )
				{
					return COL_VTYPE_NOT_VALID;
				}
				// player counter is active
				if( m_turtle_state == TURTLE_SHELL_RUN && m_player_counter > 0.0f )
				{
					return COL_VTYPE_NOT_VALID;
				}

				break;
			}
			case TYPE_FLYON:
			{
				// if walking
				if( m_turtle_state == TURTLE_WALK )
				{
					return COL_VTYPE_NOT_VALID;
				}
				// shell
				else if( m_turtle_state == TURTLE_SHELL_STAND || m_turtle_state == TURTLE_SHELL_RUN )
				{
					return COL_VTYPE_INTERNAL;
				}

				break;
			}
			case TYPE_ROKKO:
			{
				return COL_VTYPE_NOT_VALID;
			}
			case TYPE_GEE:
			{
				if( m_turtle_state == TURTLE_SHELL_STAND || m_turtle_state == TURTLE_SHELL_RUN )
				{
					return COL_VTYPE_INTERNAL;
				}

				return COL_VTYPE_NOT_VALID;
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

		if( obj->m_sprite_array == ARRAY_ENEMY )
		{
			// if moving shell don't collide with enemies
			if( m_turtle_state == TURTLE_SHELL_RUN )
			{
				return COL_VTYPE_INTERNAL;
			}
		}

		return COL_VTYPE_BLOCKING;
	}
	else if( obj->m_massive_type == MASS_HALFMASSIVE )
	{
		// if moving downwards and the object is on bottom
		if( m_vely >= 0.0f && Is_On_Top( obj ) )
		{
			return COL_VTYPE_BLOCKING;
		}
	}
	else if( obj->m_massive_type == MASS_PASSIVE )
	{
		switch( obj->m_type )
		{
			case TYPE_ENEMY_STOPPER:
			{
				if( m_turtle_state == TURTLE_WALK )
				{
					return COL_VTYPE_BLOCKING;
				}

				return COL_VTYPE_NOT_VALID;
			}
			case TYPE_BONUSBOX:
			case TYPE_SPINBOX:
			case TYPE_TEXT_BOX:
			{
				// if shell
				if( m_turtle_state == TURTLE_SHELL_STAND || m_turtle_state == TURTLE_SHELL_RUN )
				{
					cBaseBox *box = static_cast<cBaseBox *>(obj);

					// invisible semi massive
					if( box->box_invisible == BOX_INVISIBLE_SEMI_MASSIVE )
					{
						// if moving upwards and the object is on top
						if( m_vely <= 0.0f && obj->Is_On_Top( this ) )
						{
							return COL_VTYPE_BLOCKING;
						}
					}
				}

				break;
			}
			default:
			{
				break;
			}
		}
	}

	return COL_VTYPE_NOT_VALID;
}

void cTurtle :: Handle_Collision_Player( cObjectCollision *collision )
{
	if( collision->direction == DIR_UNDEFINED || ( m_turtle_state == TURTLE_SHELL_RUN && m_player_counter > 0.0f ) || m_state == STA_OBJ_LINKED )
	{
		return;
	}

	if( collision->direction == DIR_TOP && pPlayer->m_state != STA_FLY )
	{
		if( m_turtle_state == TURTLE_WALK )
		{
			pHud_Points->Add_Points( 25, pPlayer->m_pos_x, pPlayer->m_pos_y );
			pAudio->Play_Sound( "enemy/turtle/hit.ogg" );
		}
		else if( m_turtle_state == TURTLE_SHELL_STAND )
		{
			pHud_Points->Add_Points( 10, pPlayer->m_pos_x, pPlayer->m_pos_y );
			pAudio->Play_Sound( "enemy/turtle/shell/hit.ogg" );
		}
		else if( m_turtle_state == TURTLE_SHELL_RUN )
		{
			pHud_Points->Add_Points( 5, pPlayer->m_pos_x, pPlayer->m_pos_y );
			pAudio->Play_Sound( "enemy/turtle/shell/hit.ogg" );
		}

		// animation
		cParticle_Emitter *anim = new cParticle_Emitter();
		Generate_Hit_Animation( anim );
		anim->Set_Speed( 4.0f, 0.8f );
		anim->Set_Scale( 0.6f );
		// add animation
		pAnimation_Manager->Add( anim );

		DownGrade();

		// if now running
		if( m_turtle_state == TURTLE_SHELL_RUN )
		{
			// if player is on the left side
			if( ( pPlayer->m_col_rect.m_w / 2 ) + pPlayer->m_pos_x < ( m_col_rect.m_w / 2 ) + m_pos_x )
			{
				Set_Direction( DIR_RIGHT );
			}
			// right side
			else
			{
				Set_Direction( DIR_LEFT );
			}

			Update_Direction();
		}

		pPlayer->Action_Jump( 1 );
	}
	else
	{
		if( m_turtle_state == TURTLE_WALK )
		{
			pPlayer->DownGrade_Player();

			if( collision->direction == DIR_RIGHT || collision->direction == DIR_LEFT )
			{
				Turn_Around( collision->direction );
			}
		}
		else if( m_turtle_state == TURTLE_SHELL_STAND )
		{
			pAudio->Play_Sound( "enemy/turtle/shell/hit.ogg" );
			DownGrade();

			cParticle_Emitter *anim = new cParticle_Emitter();
			anim->Set_Image( pVideo->Get_Surface( "animation/particles/light.png" ) );
			anim->Set_Quota( 4 );
			anim->Set_Pos_Z( m_pos_z + 0.0001f );
			anim->Set_Time_to_Live( 0.3f );
			anim->Set_Speed( 4.0f, 0.5f );
			anim->Set_Scale( 0.6f );
			anim->Set_Fading_Size( 1 );
			anim->Set_Color( Color( static_cast<Uint8>(254), 200, 100 ) );

			if( collision->direction == DIR_RIGHT )
			{
				anim->Set_Pos( m_pos_x + m_col_pos.m_x + m_col_rect.m_w, m_pos_y + ( m_col_rect.m_h / 2 ) );
				anim->Set_Direction_Range( 90.0f, 180.0f );
				Set_Direction( DIR_LEFT );
			}
			else if( collision->direction == DIR_LEFT )
			{
				anim->Set_Pos( m_pos_x, m_pos_y + ( m_col_rect.m_h / 2 ) );
				anim->Set_Direction_Range( 270.0f, 180.0f );
				Set_Direction( DIR_RIGHT );
			}
			else
			{
				anim->Set_Pos( m_pos_x + ( m_col_rect.m_w / 2 ), m_pos_y + m_col_pos.m_y + m_col_rect.m_h );
				anim->Set_Direction_Range( 180.0f, 180.0f );

				// if player is on the left side
				if( ( pPlayer->m_col_rect.m_w / 2 ) + pPlayer->m_pos_x < ( m_col_rect.m_w / 2 ) + m_pos_x )
				{
					Set_Direction( DIR_RIGHT );
				}
				// right side
				else
				{
					Set_Direction( DIR_LEFT );
				}

				// small upwards kick
				if( collision->direction == DIR_BOTTOM )
				{
					m_vely = -5.0f + (pPlayer->m_vely * 0.3f);
				}
			}

			pAnimation_Manager->Add( anim );
			m_player_counter = speedfactor_fps * 0.13f;
		}
		else if( m_turtle_state == TURTLE_SHELL_RUN )
		{
			// bottom kicks upwards
			if( collision->direction == DIR_BOTTOM )
			{
				// small upwards kick
				m_vely = -5.0f + (pPlayer->m_vely * 0.3f);
			}
			// other directions downgrade
			else
			{
				pPlayer->DownGrade_Player();

				if( collision->direction == DIR_RIGHT || collision->direction == DIR_LEFT )
				{
					Turn_Around( collision->direction );
				}
			}
		}
	}
}

void cTurtle :: Handle_Collision_Enemy( cObjectCollision *collision )
{
	if( m_turtle_state == TURTLE_SHELL_STAND )
	{
		cEnemy *enemy = static_cast<cEnemy *>(pActive_Sprite_Manager->Get_Pointer( collision->number ));

		// if able to collide
		if( m_state == STA_OBJ_LINKED || m_vely < -5.0f )
		{
			if( Hit_Enemy( enemy ) )
			{
				DownGrade( 1 );
			}
		}
	}
	else if( m_turtle_state == TURTLE_SHELL_RUN )
	{
		cEnemy *enemy = static_cast<cEnemy *>(pActive_Sprite_Manager->Get_Pointer( collision->number ));

		if( Hit_Enemy( enemy ) )
		{
			// create animation
			cParticle_Emitter *anim = new cParticle_Emitter();

			anim->Set_Emitter_Rect( m_col_rect.m_x + ( m_col_rect.m_w * 0.2f ), m_col_rect.m_y + ( m_col_rect.m_h * 0.2f ), m_col_rect.m_w * 0.6f, m_col_rect.m_h * 0.8f );
			anim->Set_Image( pVideo->Get_Surface( "animation/particles/light.png" ) );
			anim->Set_Quota( 5 );
			anim->Set_Pos_Z( m_pos_z + 0.000001f );
			anim->Set_Time_to_Live( 0.3f );
			anim->Set_Speed( 1.2f, 0.8f );
			anim->Set_Scale( 0.7f );
			anim->Set_Fading_Alpha( 1 );
			anim->Set_Blending( BLEND_ADD );

			if( collision->direction == DIR_RIGHT )
			{
				anim->Set_Direction_Range( 0.0f );
			}
			else if( collision->direction == DIR_LEFT )
			{
				anim->Set_Direction_Range( 180.0f );
			}
			
			// add animation
			pAnimation_Manager->Add( anim );
		}
	}
	else if( m_turtle_state == TURTLE_WALK )
	{
		if( collision->direction == DIR_RIGHT || collision->direction == DIR_LEFT )
		{
			Turn_Around( collision->direction );
		}

		Send_Collision( collision );
	}
}

void cTurtle :: Handle_Collision_Massive( cObjectCollision *collision )
{
	if( m_turtle_state == TURTLE_WALK )
	{
		Send_Collision( collision );
	}
	else if( m_turtle_state == TURTLE_SHELL_RUN )
	{
		if( collision->direction == DIR_RIGHT || collision->direction == DIR_LEFT )
		{
			cSprite *col_object = pActive_Sprite_Manager->Get_Pointer( collision->number );

			// animation
			cParticle_Emitter *anim = NULL;
			if( collision->direction == DIR_RIGHT )
			{
				anim = new cParticle_Emitter();
				anim->Set_Pos( col_object->m_pos_x + col_object->m_col_pos.m_x + 4, m_pos_y + ( m_col_rect.m_h / 1.35f ) );
				anim->Set_Direction_Range( 140.0f, 100.0f );
			}
			else
			{
				anim = new cParticle_Emitter();
				anim->Set_Pos( col_object->m_pos_x + col_object->m_col_pos.m_x + col_object->m_col_rect.m_w - 4, m_pos_y + ( m_col_rect.m_h / 1.35f ) );
				anim->Set_Direction_Range( 320.0f, 100.0f );
			}

			anim->Set_Image( pVideo->Get_Surface( "animation/particles/smoke.png" ) );
			anim->Set_Quota( 5 );
			anim->Set_Pos_Z( col_object->m_pos_z - 0.0001f, 0.0002f );
			anim->Set_Time_to_Live( 0.2f, 0.2f );
			anim->Set_Speed( 1.0f, 1.0f );
			anim->Set_Scale( 0.5f, 0.4f );
			// add animation
			pAnimation_Manager->Add( anim );
		}

		// active object collision
		if( collision->m_array == ARRAY_ACTIVE )
		{
			Send_Collision( collision );
		}
	}
	else if( m_turtle_state == TURTLE_SHELL_STAND )
	{
		// if able to collide
		if( m_state == STA_OBJ_LINKED || m_vely < -5.0f )
		{
			// active object box collision
			if( collision->m_array == ARRAY_ACTIVE )
			{
				// get colliding object
				cSprite *col_object = pActive_Sprite_Manager->Get_Pointer( collision->number );

				if( col_object->m_type == TYPE_BONUSBOX || col_object->m_type == TYPE_SPINBOX )
				{
					// get basebox
					cBaseBox *box = static_cast<cBaseBox *>(col_object);

					// if useable
					if( box->useable_count != 0 )
					{
						Send_Collision( collision );

						if( m_state == STA_OBJ_LINKED )
						{
							DownGrade( 1 );
						}
						else
						{
							m_vely = 0.0f;
						}
					}
				}
			}
		}
	}

	if( m_state == STA_OBJ_LINKED )
	{
		return;
	}
	
	if( collision->direction == DIR_TOP )
	{
		if( m_vely < 0.0f )
		{
			m_vely = 0.0f;
		}
	}
	else if( collision->direction == DIR_BOTTOM )
	{
		if( m_vely > 0.0f )
		{
			m_vely = 0.0f;
		}
	}
	else if( collision->direction == DIR_RIGHT || collision->direction == DIR_LEFT )
	{
		Turn_Around( collision->direction );
	}
}

void cTurtle :: Editor_Activate( void )
{
	// get window manager
	CEGUI::WindowManager &wmgr = CEGUI::WindowManager::getSingleton();

	// direction
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox *>(wmgr.createWindow( "TaharezLook/Combobox", "editor_turtle_direction" ));
	Editor_Add( UTF8_("Direction"), UTF8_("Starting direction."), combobox, 100, 75 );

	combobox->addItem( new CEGUI::ListboxTextItem( "left" ) );
	combobox->addItem( new CEGUI::ListboxTextItem( "right" ) );

	combobox->setText( Get_Direction_Name( m_start_direction ) );
	combobox->subscribeEvent( CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber( &cTurtle::Editor_Direction_Select, this ) );

	// init
	Editor_Init();
}

bool cTurtle :: Editor_Direction_Select( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	CEGUI::ListboxItem *item = static_cast<CEGUI::Combobox *>( windowEventArgs.window )->getSelectedItem();

	Set_Direction( Get_Direction_Id( item->getText().c_str() ), 1 );

	return 1;
}

void cTurtle :: Create_Name( void )
{
	m_name = "Turtle ";
	m_name += _(Get_Color_Name( m_color_type ).c_str());
	m_name += " ";
	m_name += _(Get_Direction_Name( m_start_direction ).c_str());
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
