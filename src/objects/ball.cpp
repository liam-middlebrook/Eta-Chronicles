/***************************************************************************
 * ball.cpp  -  ball class
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

#include "../objects/ball.h"
#include "../core/game_core.h"
#include "../enemies/enemy.h"
#include "../objects/goldpiece.h"
#include "../enemies/gee.h"
#include "../enemies/spika.h"
#include "../player/player.h"
#include "../video/animation.h"
#include "../level/level.h"
#include "../gui/hud.h"
#include "../core/sprite_manager.h"

namespace SMC
{

/* *** *** *** *** *** *** cBall *** *** *** *** *** *** *** *** *** *** *** */

cBall :: cBall( float x, float y, const cSprite *origin_object /* = NULL */, ball_effect btype /* = FIREBALL_DEFAULT */  )
: cMovingSprite( NULL, x, y )
{
	m_sprite_array = ARRAY_ACTIVE;
	m_type = TYPE_BALL;
	m_pos_z = 0.095f;

	m_spawned = 1;
	m_player_range = 2000;

	m_massive_type = MASS_MASSIVE;

	m_glim_mod = 1;
	m_glim_counter = 0.0f;
	m_fire_counter = 0.0f;

	m_life_left = 0;

	if( btype == FIREBALL_DEFAULT || btype == FIREBALL_EXPLOSION )
	{
		Set_Image( pVideo->Get_Surface( "animation/fireball/1.png" ) );
		m_ball_type = FIREBALL_DEFAULT;
	}
	else if( btype == ICEBALL_DEFAULT || btype == ICEBALL_EXPLOSION )
	{
		Set_Image( pVideo->Get_Surface( "animation/iceball/1.png" ) );
		m_ball_type = ICEBALL_DEFAULT;
	}
	else
	{
		printf( "Warning : Ball unknown type %d\n", btype );
		cMovingSprite::Destroy();
		return;
	}

	if( origin_object )
	{
		m_origin_array = origin_object->m_sprite_array;
		m_origin_type = origin_object->m_type;

		if( m_origin_type == TYPE_PLAYER )
		{
			if( m_ball_type == FIREBALL_DEFAULT || m_ball_type == ICEBALL_DEFAULT )
			{
				pPlayer->shoot_counter = speedfactor_fps;
			}
		}
	}
	// if origin not set
	else
	{
		printf( "Warning : Ball origin not set\n" );
		m_origin_array = ARRAY_UNDEFINED;
		m_origin_type = TYPE_UNDEFINED;
	}
}

cBall :: ~cBall( void )
{
	// always destroy
	if( !m_auto_destroy )
	{
		cBall::Destroy();
	}
}

void cBall :: Destroy_Ball( bool with_sound /* = 0 */ )
{
	Set_Velocity(0,0);

	if( with_sound )
	{
		if( m_ball_type == FIREBALL_DEFAULT )
		{
			pAudio->Play_Sound( "item/fireball_explode.wav" );
		}
	}

	Destroy();
}

void cBall :: Destroy( void )
{
	if( m_auto_destroy )
	{
		return;
	}

	if( m_ball_type == FIREBALL_DEFAULT )
	{
		pAnimation_Manager->Add( new cAnimation_Fireball( m_pos_x + m_col_rect.m_w / 2, m_pos_y + m_col_rect.m_h / 2 ) );
	}
	else if( m_ball_type == ICEBALL_DEFAULT )
	{
		// create animation
		cParticle_Emitter *anim = new cParticle_Emitter();
		Generate_Particles( anim );
		anim->Set_Quota( 15 );
		// add animation
		pAnimation_Manager->Add( anim );
	}

	cMovingSprite::Destroy();
}

void cBall :: Update( void )
{
	if( !m_valid_update )
	{
		return;
	}

	// right
	if( m_velx > 0.0f )
	{
		Set_Velocity(20,0);
		m_rot_z += pFramerate->m_speed_factor * 40;
	}
	// left
	else
	{
		Set_Velocity(-20,0);
		m_rot_z -= pFramerate->m_speed_factor * 40;
	}


	// if ball is out of Player Range
	if( !Is_In_Player_Range() )
	{
		Destroy();
	}

	// glim animation
	if( m_glim_mod )
	{
		m_glim_counter += pFramerate->m_speed_factor * 0.1f;
		//m_glim_counter += pFramerate->m_speed_factor * 0.0f;

		if( m_glim_counter > 1.0f )
		{
			m_glim_counter = 1.0f;
			//m_glim_counter = 0.0f;
			m_glim_mod = 0;
		}
	}
	else
	{
		m_glim_counter -= pFramerate->m_speed_factor * 0.1f;
		//m_glim_counter -= pFramerate->m_speed_factor * 0.0f;

		if( m_glim_counter < 0.0f )
		{
			m_glim_counter = 0.0f;
			m_glim_mod = 1;
		}
	}

	// generate fire particle animation
	m_fire_counter += pFramerate->m_speed_factor;
	while( m_fire_counter > 1 )
	{
		Generate_Particles();
		m_fire_counter -= 1;
	}
}

void cBall :: Draw( cSurface_Request *request /* = NULL */ )
{
	if( !m_valid_draw )
	{
		return;
	}

	// don't draw if leveleditor mode
	if( editor_level_enabled )
	{
		return;
	}

	if( m_ball_type == FIREBALL_DEFAULT )
	{
		Set_Color_Combine( m_glim_counter, m_glim_counter / 1.2f, m_glim_counter / 2, GL_ADD );
	}
	else if( m_ball_type == ICEBALL_DEFAULT )
	{
		Set_Color_Combine( m_glim_counter / 2.5f, m_glim_counter / 2.5f, m_glim_counter / 1.7f, GL_ADD );
	}

	cMovingSprite::Draw( request );
}

void cBall :: Generate_Particles( cParticle_Emitter *anim /* = NULL */ ) const
{
	bool create_anim = 0;

	if( !anim )
	{
		create_anim = 1;
		// create animation
		anim = new cParticle_Emitter();
	}

	anim->Set_Emitter_Rect( m_col_rect );
	anim->Set_Image( pVideo->Get_Surface( "animation/particles/light.png" ) );
	anim->Set_Pos_Z( m_pos_z + 0.0001f );
	if( m_ball_type == FIREBALL_DEFAULT )
	{
		anim->Set_Time_to_Live( 0.2f );
		anim->Set_Color( Color( static_cast<Uint8>(250), 140, 90 ) );
	}
	else if( m_ball_type == ICEBALL_DEFAULT )
	{
		anim->Set_Time_to_Live( 0.5f );
		anim->Set_Color( Color( static_cast<Uint8>(90), 90, 255 ) );
	}
	anim->Set_Blending( BLEND_ADD );
	anim->Set_Speed( 0.35f, 0.3f );
	anim->Set_Scale( 0.4f, 0.2f );
	
	if( create_anim )
	{
		// add animation
		pAnimation_Manager->Add( anim );
	}
}

Col_Valid_Type cBall :: Validate_Collision( cSprite *obj )
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
			if( m_origin_type != TYPE_PLAYER )
			{
				return COL_VTYPE_INTERNAL;
			}

			return COL_VTYPE_NOT_VALID;
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
		if( obj->m_sprite_array == ARRAY_ENEMY && m_origin_array == ARRAY_ENEMY )
		{
			return COL_VTYPE_NOT_VALID;
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

	return COL_VTYPE_NOT_VALID;
}

void cBall :: Handle_Collision( cObjectCollision *collision )
{
	// already destroyed
	if( m_auto_destroy )
	{
		return;
	}

	cMovingSprite::Handle_Collision( collision );
}

void cBall :: Handle_Collision_Player( cObjectCollision *collision )
{
	// velocity hit
	if( collision->direction == DIR_LEFT )
	{
		if( pPlayer->m_velx > 0.0f )
		{
			pPlayer->m_velx *= 0.3f;
		}
	}
	else if( collision->direction == DIR_RIGHT )
	{
		if( pPlayer->m_velx < 0.0f )
		{
			pPlayer->m_velx *= 0.3f;
		}
	}
	else if( collision->direction == DIR_UP )
	{
		if( pPlayer->m_vely > 0.0f )
		{
			pPlayer->m_vely *= 0.2f;
		}
	}
	else if( collision->direction == DIR_DOWN )
	{
		if( pPlayer->m_vely < 0.0f )
		{
			pPlayer->m_vely *= 0.4f;
		}
	}

	pAudio->Play_Sound( "item/fireball_repelled.wav" );
	Destroy();
}

void cBall :: Handle_Collision_Enemy( cObjectCollision *collision )
{
	cEnemy *enemy = static_cast<cEnemy *>(pActive_Sprite_Manager->Get_Pointer( collision->number ));

	// if enemy is not destroyable
	if( ( m_ball_type == FIREBALL_DEFAULT && enemy->m_fire_resistant ) || ( m_ball_type == ICEBALL_DEFAULT && enemy->m_ice_resistance >= 1 ) )
	{
		pAudio->Play_Sound( "item/fireball_repelled.wav" );
	}
	// destroy enemy
	else
	{
		// enemy rect particle animation
		for( unsigned int w = 0; w < enemy->m_col_rect.m_w; w += 15 )
		{
			for( unsigned int h = 0; h < enemy->m_col_rect.m_h; h += 15 )
			{
				// animation
				cParticle_Emitter *anim = new cParticle_Emitter();
				anim->Set_Pos( enemy->m_pos_x + w, enemy->m_pos_y + h );

				anim->Set_Image( pVideo->Get_Surface( "animation/particles/light.png" ) );
				anim->Set_Time_to_Live( 0.2f, 0.4f );
				Color anim_color, anim_color_rand;
				if( m_ball_type == FIREBALL_DEFAULT )
				{
					anim_color = Color( static_cast<Uint8>(250), 170, 150 );
					anim_color_rand = Color( static_cast<Uint8>( rand() % 5 ), rand() % 85, rand() % 25, 0 );
				}
				else
				{
					anim_color = Color( static_cast<Uint8>(150), 150, 240 );
					anim_color_rand = Color( static_cast<Uint8>( rand() % 80 ), rand() % 80, rand() % 10, 0 );
				}
				anim->Set_Color( anim_color, anim_color_rand );
				anim->Set_Fading_Alpha( 1 );
				anim->Set_Fading_Size( 1 );
				anim->Set_Speed( 0.5f, 2.2f );
				anim->Set_Blending( BLEND_DRIVE );
				// add animation
				pAnimation_Manager->Add( anim );
			}
		}

		// play enemy kill sound
		pAudio->Play_Sound( enemy->m_kill_sound );

		if( m_ball_type == FIREBALL_DEFAULT )
		{

			enemy->set_life_left(1);

			enemy->return_life_left();

			//if (m_life_left < 1)
			if (enemy->return_life_left() < 0)
			{
				// get points
				pHud_Points->Add_Points( enemy->m_kill_points, m_pos_x, m_pos_y, "", static_cast<Uint8>(255), 1 );

				// create goldpiece
				cMovingSprite *goldpiece = new cFGoldpiece( enemy->m_col_rect.m_x, enemy->m_col_rect.m_y + enemy->m_col_rect.m_h, collision->direction );
				// set optimal position
				goldpiece->Col_Move( -( ( goldpiece->m_col_rect.m_w - enemy->m_col_rect.m_w ) / 2 ), -( goldpiece->m_col_pos.m_y + goldpiece->m_col_rect.m_h ), 1, 1 );
				// add goldpiece
				pActive_Sprite_Manager->Add( goldpiece );

				enemy->Set_Active( 0 );
				enemy->DownGrade( 1 );
				pPlayer->Add_Kill_Multiplier();
			}
		}
		else if( m_ball_type == ICEBALL_DEFAULT )
		{
			enemy->Freeze();
		}
	}

	Destroy();
}

void cBall :: Handle_Collision_Massive( cObjectCollision *collision )
{
	if( collision->direction == DIR_DOWN )
	{
		// if directly hitting the ground
		if( m_velx < 0.1f && m_velx > -0.1f )
		{
			Destroy_Ball( 1 );
			return;
		}

		if( m_ball_type == FIREBALL_DEFAULT )
		{
			m_vely = 0.0f;	
			
			// create animation
			cAnimation_Fireball *anim = new cAnimation_Fireball( m_pos_x + m_col_rect.m_w / 2, m_pos_y + m_col_rect.m_h / 2 );
			anim->Set_Fading_Speed( 3.0f );
			pAnimation_Manager->Add( anim );
		}
		else if( m_ball_type == ICEBALL_DEFAULT )
		{
			m_vely = -5.0f;

			// create animation
			cParticle_Emitter *anim = new cParticle_Emitter();
			anim->Set_Pos( m_pos_x + m_col_rect.m_w / 2, m_pos_y + m_col_rect.m_h / 2 );
			anim->Set_Image( pVideo->Get_Surface( "animation/particles/cloud.png" ) );
			anim->Set_Direction_Range( 0, 180 );
			anim->Set_Quota( 3 );
			anim->Set_Time_to_Live( 0.8f );
			anim->Set_Pos_Z( m_pos_z + 0.0001f );
			anim->Set_Color( Color( static_cast<Uint8>(50), 50, 250 ) );
			anim->Set_Blending( BLEND_ADD );
			anim->Set_Speed( 0.5f, 0.4f );
			anim->Set_Scale( 0.3f, 0.4f );
			// add animation
			pAnimation_Manager->Add( anim );
		}
	}
	// other directions
	else
	{
		Destroy_Ball( 1 );
	}
}

void cBall :: Handle_out_of_Level( ObjectDirection dir )
{
	// ignore top
	if( dir == DIR_TOP )
	{
		return;
	}

	Destroy_Ball( 1 );
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
