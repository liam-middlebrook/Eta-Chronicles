/***************************************************************************
 * player.cpp  -  Level player class
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

#include "../core/globals.h"
#include "../player/player.h"
#include "../core/main.h"
#include "../video/animation.h"
#include "../core/game_core.h"
#include "../user/preferences.h"
#include "../input/joystick.h"
#include "../core/sprite_manager.h"
#include "../core/framerate.h"
#include "../audio/audio.h"
#include "../enemies/turtle.h"
#include "../overworld/overworld.h"
#include "../level/level.h"
#include "../gui/menu.h"
#include "../objects/level_exit.h"
#include "../objects/box.h"
#include "../input/keyboard.h"
#include "../core/math/utilities.h"
#include "../core/i18n.h"
#include "../objects/path.h"
#include "../video/gl_surface.h"
#include "../core/filesystem/filesystem.h"

namespace SMC
{

// Milliseconds to enable power jump when ducking
const int power_jump_delta = 1000;

const float cPlayer::m_default_pos_x = 200;
const float cPlayer::m_default_pos_y = 50;
const float MAX_LIFE = 3;

/* *** *** *** *** *** *** *** *** cPlayer *** *** *** *** *** *** *** *** *** */

cPlayer :: cPlayer( float x /* = m_default_pos_x */, float y /* = m_default_pos_y */ )
: cAnimated_Sprite( x, y )
{
	m_sprite_array = ARRAY_PLAYER;
	m_type = TYPE_PLAYER;
	m_massive_type = MASS_MASSIVE;
	m_state = STA_FALL;

	maryo_type = MARYO_SMALL;
	maryo_type_temp_power = MARYO_DEAD;
	m_name = "Maryo";	

	m_images.reserve( 20 );
	m_pos_z = 0.0999f;
	ducked = 0;
	ducked_animation_counter = 0;
	parachute = 0;
	throwing_counter = 0;

	// invincible data
	invincible = 0;
	invincible_star = 0;
	invincible_mod = 0;
	invincible_starcounter = 0;

	walk_count = 0;
	running_particle_counter = 0;

	godmode = 0;

	walk_time = 0;
	ghost_time = 0;
	ghost_time_mod = 0;

	// Starting with 3 lives 
	lives = 3;
	goldpieces = 0;
	points = 0;
	kill_multiplier = 1;
	last_kill_counter = 0;

	// jump data
	UpKeytime = 0;
	force_jump = 0;
	next_jump_sound = 1;
	next_jump_power = 17;
	next_jump_accel = 4;
	jump_power = 0;
	jump_accel_up = 4.5f;
	jump_vel_deaccel = 0.06f;

	// no movement timer
	no_velx_counter = 0;
	no_vely_counter = 0;

	shoot_counter = 0;
	num_shots = 0;
	life_bar = MAX_LIFE;
	active_object = NULL;
	duck_direction = DIR_UNDEFINED;
}

cPlayer :: ~cPlayer( void )
{
	Ball_Clear();
}

cPlayer *cPlayer :: Copy( void )
{
	// non copyable
	return NULL;
}

void cPlayer :: Init( void )
{
	Load_Images();
	// default direction : right
	Set_Direction( DIR_RIGHT, 1 );
}

void cPlayer :: Hold( void )
{
	if( !m_ground_object || ( m_state != STA_WALK && m_state != STA_RUN ) )
	{
		return;
	}
	
	Set_Moving_State( STA_STAY );
}

void cPlayer :: Set_Direction( const ObjectDirection dir, bool new_start_direction /* = 0 */ )
{
	// set start image
	if( new_start_direction )
	{
		cMovingSprite::Set_Image( pVideo->Get_Surface( "maryo/small/stand_" + Get_Direction_Name( dir ) + ".png" ), 1 );

		// set back current image
		m_curr_img = -1;
		Set_Image_Num( Get_Image() + m_direction );
	}

	cAnimated_Sprite::Set_Direction( dir, new_start_direction );
}

bool cPlayer :: Set_On_Ground( cSprite *obj, bool set_on_top /* = 1 */ )
{
	bool valid = cMovingSprite::Set_On_Ground( obj, set_on_top );

	// if valid ground
	if( valid )
	{
		// if moving
		if( m_velx )
		{
			if( m_state != STA_STAY && m_state != STA_RUN )
			{
				Set_Moving_State( STA_WALK );
			}
		}
		// not moving
		else
		{
			Set_Moving_State( STA_STAY );
		}

		// if massive ground and ducking key is pressed
		if( m_ground_object->m_massive_type == MASS_MASSIVE && pKeyboard->keys[pPreferences->m_key_down] )
		{
			Start_Ducking();
		}
	}

	return valid;
}

void cPlayer :: DownGrade( bool force /* = 0 */ )
{
	DownGrade_Player( 1, force );
}

void cPlayer :: DownGrade_Player( bool delayed /* = 1 */, bool force /* = 0 */ )
{
	if( godmode || invincible )
	{
		return;
	}

	// already dead
	if( maryo_type == MARYO_DEAD )
	{
		return;
	}

	if( delayed )
	{
		Game_Action = GA_DOWNGRADE_PLAYER;
		if( force )
		{
			Game_Action_Data.add( "force", "1" );
		}

		return;
	}

	// if not weakest state or not forced 
	if( maryo_type != MARYO_SMALL && !force )
	{
		if (life_bar > 0)
		{
			pAudio->Play_Sound( "player/powerdown.ogg", RID_MARYO_POWERDOWN );

			// power down
			//Set_Type( MARYO_SMALL );

			invincible = speedfactor_fps * 2.5f;
			invincible_mod = 0;

			life_bar--;
			pHud_LifeDisplay->Decrease_Life(1);
		
			pHud_Itembox->Request_Item();
		
			return;
		}
	}
	
	if (maryo_type == MARYO_SMALL && !force)
	{		
		if (life_bar > 0)
		{
			life_bar--;
			pAudio->Play_Sound( "player/powerdown.ogg", RID_MARYO_POWERDOWN );

			pHud_LifeDisplay->Decrease_Life(1);

			invincible = speedfactor_fps * 2.5f;
			invincible_mod = 0;
			
			return;
		}
	}

	pHud_LifeDisplay->Decrease_Life(1);
	life_bar = MAX_LIFE;
	pHud_LifeDisplay->Reset();
	pHud_LifeDisplay->Set_Life(4);
	Set_Type( MARYO_DEAD, 0, 0 );
	pHud_Points->Clear();
	Ball_Clear();
	pHud_Lives->Add_Lives( -1 );
	pAudio->Fadeout_Music( 1700 );

	// lost a live
	if( lives >= 0 )
	{
		pAudio->Play_Sound( "player/dead.ogg", RID_MARYO_DEATH );
	}
	// game over
	else
	{
		pAudio->Play_Sound( DATA_DIR "/" GAME_MUSIC_DIR "/game/lost_1.ogg", RID_MARYO_DEATH );
	}
	
	// dying animation
	Set_Image_Num( MARYO_IMG_DEAD + 1 );

	// draw
	Draw_Game();
	// render
	pVideo->Render();

	// wait
	SDL_Delay( 500 );
	pFramerate->Reset();
	
	Set_Image_Num( MARYO_IMG_DEAD );

	float i;

	for( i = 0.0f; i < 7.0f; i += pFramerate->m_speed_factor )
	{
		// move up
		Move( 0.0f, -13.0f );
		// draw
		Draw_Game();
		// render
		pVideo->Render();
		pFramerate->Update();
	}

	// very small delay until falling animation
	SDL_Delay( 300 );

	pFramerate->Reset();
	walk_count = 0.0f;
	
	for( i = 0.0f; m_pos_y < game_res_h + pActive_Camera->y + m_col_rect.m_h; i++ )
	{
		walk_count += pFramerate->m_speed_factor * 0.75f;

		if( walk_count > 4.0f )
		{
			walk_count = 0.0f;
		}
		
		// move down
		Move( 0.0f, 14.0f );

		if( walk_count > 2.0f )
		{
			Set_Image_Num( MARYO_IMG_DEAD );
		}
		else
		{
			Set_Image_Num( MARYO_IMG_DEAD + 1 );
		}
		
		// draw
		Draw_Game();
		// render
		pVideo->Render();
		pFramerate->Update();
	}

	// lost a live
	if( lives >= 0 )
	{
		// disable fixed camera velocity
		pLevel_Manager->camera->fixed_hor_vel = 0.0f;
		// unload level
		pActive_Level->Unload( 1 );
	}
	// game over
	else
	{
		cGL_Surface *game_over = pVideo->Get_Surface( "game/game_over.png" );
		cSprite *sprite = new cSprite( game_over, ( game_res_w * 0.5f ) - ( game_over->m_w * 0.5f ), ( game_res_h * 0.5f ) - ( game_over->m_h * 0.5f ) );
		sprite->m_pos_z = 0.8f;
		sprite->m_spawned = 1;
		sprite->Set_Ignore_Camera( 1 );
		pActive_Sprite_Manager->Add( sprite );

		cParticle_Emitter *anim = new cParticle_Emitter();
		anim->Set_Emitter_Rect( sprite->m_rect.m_x + pActive_Camera->x, sprite->m_rect.m_y + pActive_Camera->y, sprite->m_rect.m_w, sprite->m_rect.m_h );
		anim->Set_Emitter_Time_to_Live( 24.0f );
		anim->Set_Emitter_Iteration_Interval( 0.1f );
		anim->Set_Quota( 1 );
		anim->Set_Pos_Z( 0.79f );
		anim->Set_Image( pVideo->Get_Surface( "animation/particles/axis.png" ) );
		anim->Set_Time_to_Live( 10.0f );
		anim->Set_Fading_Alpha( 1 );
		anim->Set_Speed( 2.0f, 0.5f );
		anim->Set_Scale( 0.9f );
		anim->Set_Color( Color( static_cast<Uint8>(250), 140, 90 ), Color( static_cast<Uint8>(5), 100, 0, 0 ) );
		anim->Set_Const_Rotation_Z( -2.0f, 4.0f );

		anim->Init_Anim();

		for( i = 10.0f; i > 0.0f; i -= 0.011f * pFramerate->m_speed_factor )
		{
			// update pressed keys
			SDL_PumpEvents();
			Uint8 *keys = SDL_GetKeyState( NULL );
			// Escape stops
			if( keys[SDLK_ESCAPE] || keys[SDLK_RETURN] || keys[SDLK_SPACE] || keys[pPreferences->m_key_action] )
			{
				break;
			}

			// if joystick enabled and exit pressed
			if( pPreferences->m_joy_enabled && SDL_JoystickGetButton( pJoystick->joystick, pPreferences->m_joy_button_exit ) )
			{
				 break;
			}

			// update animation
			anim->Update();
			
			// draw
			Draw_Game();
			anim->Draw();

			pVideo->Render();
			pFramerate->Update();
		}

		delete anim;
		pAudio->Stop_Sounds();
	}
	
	// clear
	Set_Type( MARYO_SMALL, 0, 0 );
	Clear_Input_Events();
	pFramerate->Reset();

	// game over
	if( lives < 0 )
	{
		// reset saved data
		Reset_Save();
		Game_Action = GA_ENTER_MENU;
	}
	// custom level
	else if( Game_Mode_Type == MODE_TYPE_LEVEL_CUSTOM )
	{
		Game_Action = GA_ENTER_MENU;
		Game_Action_Data.add( "level", Trim_Filename( pActive_Level->m_level_filename, 0, 0 ) );
		// reset saved data
		Reset_Save();
	}
	// back to overworld
	else
	{
		Game_Action = GA_ENTER_WORLD;
	}
}

void cPlayer :: Move_Player( float velocity, float vel_wrongway )
{
	if( m_direction == DIR_LEFT )
	{
		velocity *= -1;
		vel_wrongway *= -1;
	}

	// get collision list
	cObjectCollisionType *col_list = Collision_Check_Relative( velocity, 0.0f, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING );
	// if collision with a blocking object
	bool is_col = 0;

	// check collisions
	for( cObjectCollision_List::iterator itr = col_list->objects.begin(), itr_end = col_list->objects.end(); itr != itr_end; ++itr )
	{
		cObjectCollision *col_obj = (*itr);

		// massive object is blocking
		if( col_obj->obj->m_massive_type == MASS_MASSIVE )
		{
			is_col = 1;
			break;
		}
	}

	delete col_list;

	// don't move if colliding
	if( is_col )
	{
		if( Is_Float_Equal( m_velx, 0.0f ) )
		{
			Set_Moving_State( STA_STAY );
		}
		// slow down
		else
		{
			m_velx -= (m_velx * 0.1f) * pFramerate->m_speed_factor;

			if( m_velx > -0.1f && m_velx < 0.1f )
			{
				m_velx = 0.0f;
			}
		}

		return;
	}

	// move right
	if( m_direction == DIR_RIGHT )
	{
		if( m_velx > 0.0f )
		{
			Add_Velocity( velocity, 0.0f );

			if( m_velx > 10 * Get_Vel_Modifier() )
			{
				m_velx = 10 * Get_Vel_Modifier();
			}
		}
		else
		{
			// small smoke clouds under foots
			if( m_velx < 0.0f )
			{
				Generate_Feet_Clouds();
			}

			// slow down
			Add_Velocity( vel_wrongway, 0.0f );
		}
	}
	// move left
	else if( m_direction == DIR_LEFT )
	{
		if( m_velx < 0.0f )
		{
			Add_Velocity( velocity, 0 );

			if( m_velx < -10.0f * Get_Vel_Modifier() )
			{
				m_velx = -10.0f * Get_Vel_Modifier();
			}
		}
		else
		{
			// small smoke clouds under foots
			if( m_velx > 0.0f )
			{
				Generate_Feet_Clouds();
			}

			// slow down
			Add_Velocity( vel_wrongway, 0.0f );
		}
	}

	// start walking
	if( m_state == STA_STAY )
	{
		Set_Moving_State( STA_WALK );
	}
}

void cPlayer :: Generate_Feet_Clouds( cParticle_Emitter *anim /* = NULL */ )
{
	// check if valid
	if( !m_ground_object || !m_ground_object->m_image || m_ground_object->m_image->m_ground_type == GROUND_NORMAL )
	{
		return;
	}

	bool create_anim = 0;

	if( !anim )
	{
		create_anim = 1;
		// create animation
		anim = new cParticle_Emitter();
	}

	anim->Set_Emitter_Rect( m_col_rect.m_x, m_col_rect.m_y + m_col_rect.m_h - 2.0f, m_col_rect.m_w );
	anim->Set_Pos_Z( m_pos_z - 0.000001f );

	float vel;

	if( m_velx > 0.0f )
	{
		vel = m_velx;
	}
	else
	{
		vel = -m_velx;
	}
	
	// ground type
	switch( m_ground_object->m_image->m_ground_type )
	{
		case GROUND_EARTH:
		{
			anim->Set_Image( pVideo->Get_Surface( "animation/particles/dirt.png" ) );
			anim->Set_Time_to_Live( 0.3f );
			anim->Set_Scale( 0.5f );
			anim->Set_Speed( 0.08f + vel * 0.1f, vel * 0.05f );
			break;
		}
		case GROUND_ICE:
		{
			anim->Set_Image( pVideo->Get_Surface( "animation/particles/ice_1.png" ) );
			anim->Set_Time_to_Live( 0.6f );
			anim->Set_Scale( 0.3f );
			anim->Set_Speed( 0.1f + vel * 0.05f, vel * 0.04f );
			break;
		}
		case GROUND_SAND:
		{
			anim->Set_Emitter_Iteration_Interval( 4.0f );
			anim->Set_Image( pVideo->Get_Surface( "animation/particles/cloud.png" ) );
			anim->Set_Time_to_Live( 0.3f );
			anim->Set_Scale( 0.2f );
			anim->Set_Color( lightorange );
			anim->Set_Speed( 0.2f + vel * 0.15f, vel * 0.15f );
			break;
		}
		case GROUND_STONE:
		{
			anim->Set_Image( pVideo->Get_Surface( "animation/particles/smoke_black.png" ) );
			anim->Set_Time_to_Live( 0.3f );
			anim->Set_Scale( 0.3f );
			anim->Set_Speed( vel * 0.08f, 0.1f + vel * 0.1f );
			break;
		}
		case GROUND_PLASTIC:
		{
			anim->Set_Image( pVideo->Get_Surface( "animation/particles/light.png" ) );
			anim->Set_Time_to_Live( 0.2f );
			anim->Set_Scale( 0.2f );
			anim->Set_Color( lightgrey );
			anim->Set_Speed( 0.05f, vel * 0.05f );
			break;
		}
		default:
		{
			anim->Set_Image( pVideo->Get_Surface( "animation/particles/smoke.png" ) );
			anim->Set_Time_to_Live( 0.3f );
			anim->Set_Speed( 0.1f + vel * 0.1f, vel * 0.1f );
			break;
		}
	}

	if( m_direction == DIR_RIGHT )
	{
		anim->Set_Direction_Range( 180.0f, 90.0f );
	}
	else
	{
		anim->Set_Direction_Range( 270.0f, 90.0f );
	}

	if( create_anim )
	{
		// add animation
		pAnimation_Manager->Add( anim );
	}
}

void cPlayer :: Update_Walking( void )
{
	if( ducked || !m_ground_object || ( m_state != STA_STAY && m_state != STA_WALK && m_state != STA_RUN ) )
	{
		return;
	}

	// validate ground object
	if( ( m_ground_object->m_type == TYPE_EATO || m_ground_object->m_type == TYPE_SPIKA || m_ground_object->m_type == TYPE_ROKKO || m_ground_object->m_type == TYPE_STATIC_ENEMY ) && invincible <= 0 && !godmode )
	{
		Reset_On_Ground();
	}

	// only if left or right is pressed
	if( pKeyboard->keys[pPreferences->m_key_left] || pKeyboard->keys[pPreferences->m_key_right] || pJoystick->left || pJoystick->right )
	{
		float ground_mod = 1.0f;

		if( m_ground_object && m_ground_object->m_image )
		{
			// ground type
			switch( m_ground_object->m_image->m_ground_type )
			{
				case GROUND_ICE:
				{
					ground_mod = 0.5f;
					break;
				}
				case GROUND_SAND:
				{
					ground_mod = 0.7f;
					break;
				}
				case GROUND_PLASTIC:
				{
					ground_mod = 0.85f;
					break;
				}
				default:
				{
					break;
				}
			}
		}

		Move_Player( ( 0.6f * ground_mod * Get_Vel_Modifier() ), ( 1.2f * ground_mod * Get_Vel_Modifier() ) );
	}

	// update walking time
	if( m_state == STA_WALK )
	{
		// update
		if( m_velx > 10.0f || m_velx < -10.0f )
		{
			walk_time += pFramerate->m_speed_factor;

			if( walk_time > speedfactor_fps )
			{
				Set_Moving_State( STA_RUN );
			}
		}
		// reset
		else if( walk_time )
		{
			walk_time = 0.0f;
		}
	}
}

void cPlayer :: Update_Running( void )
{
	if( ducked || !m_ground_object || m_state != STA_RUN )
	{
		return;
	}

	running_particle_counter += pFramerate->m_speed_factor * 0.5f;

	// create particles
	while( running_particle_counter > 1 )
	{
		float vel = ( m_velx > 0 ? m_velx : -m_velx );

		// start on high velocity
		if( vel > 17 )
		{
			// light particles
			cParticle_Emitter *anim = new cParticle_Emitter();
			anim->Set_Emitter_Rect( m_col_rect.m_x + m_col_rect.m_w * 0.25f, m_col_rect.m_y + m_col_rect.m_h * 0.1f, m_col_rect.m_w * 0.5f, m_col_rect.m_h * 0.8f );
			anim->Set_Image( pVideo->Get_Surface( "animation/particles/light.png" ) );
			anim->Set_Pos_Z( m_pos_z + 0.000001f );
			anim->Set_Time_to_Live( 0.1f + vel * 0.03f );
			anim->Set_Fading_Alpha( 1 );
			anim->Set_Fading_Size( 1 );
			anim->Set_Speed( vel * 0.9f, 0 );
			// right
			if( m_velx > 0 )
			{
				anim->Set_Direction_Range( 0, 0 );
			}
			// left
			else
			{
				anim->Set_Direction_Range( 180, 0 );
			}
			anim->Set_Scale( 0.15f );

			anim->Set_Blending( BLEND_ADD );
			pAnimation_Manager->Add( anim );
		}

		// smoke clouds under foots
		Generate_Feet_Clouds();

		running_particle_counter--;
	}
}

void cPlayer :: Update_Staying( void )
{
	// only if player is onground
	if( !m_ground_object || ducked || m_state == STA_JUMP || m_state == STA_CLIMB )
	{
		return;
	}

	// if left and right is not pressed
	if( !pKeyboard->keys[pPreferences->m_key_left] && !pKeyboard->keys[pPreferences->m_key_right] && !pJoystick->left && !pJoystick->right )
	{
		// walking
		if( m_velx )
		{
			if( m_ground_object->m_image && m_ground_object->m_image->m_ground_type == GROUND_ICE )
			{
				Auto_Slow_Down( 1.1f / 5.0f );
			}
			else
			{
				Auto_Slow_Down( 1.1f );
			}

			// stopped walking
			if( !m_velx )
			{
				Set_Moving_State( STA_STAY );
			}
		}

		// walk on spika
		if( m_ground_object->m_type == TYPE_SPIKA )
		{
			cMovingSprite *moving_ground_object = static_cast<cMovingSprite *>(m_ground_object);

			if( moving_ground_object->m_velx < 0 )
			{
				walk_count -= moving_ground_object->m_velx * pFramerate->m_speed_factor * 0.1f;
			}
			else if( moving_ground_object->m_velx > 0 )
			{
				walk_count += moving_ground_object->m_velx * pFramerate->m_speed_factor * 0.1f;
			}
		}
		// if not moving anymore
		else if( m_velx == 0 )
		{
			walk_count = 0;
		}
	}

	// if staying don't move vertical
	if( m_vely > 0 )
	{
		m_vely = 0;
	}
}

void cPlayer :: Update_Flying( void )
{
	/* only if not onground
	 * if jumping update jump is already used
	*/
	if( m_ground_object || m_state == STA_CLIMB || m_state == STA_JUMP )
	{
		return;
	}

	// flying
	if( m_state == STA_FLY )
	{
		if( m_direction == DIR_LEFT )
		{
			if( m_velx > -15 * Get_Vel_Modifier() )
			{
				Add_Velocity( -1.1f, 0 );
			}
		}
		else
		{
			if( m_velx < 15 * Get_Vel_Modifier() )
			{
				Add_Velocity( 1.1f, 0 );
			}
		}

		// move down 
		if( pKeyboard->keys[pPreferences->m_key_down] || pJoystick->down )
		{
			if( m_vely < 5 * Get_Vel_Modifier() )
			{
				Add_Velocity( 0, 0.6f );
			}
		}
		// move up
		else if( pKeyboard->keys[pPreferences->m_key_up] || pJoystick->up )
		{
			if( m_vely > -5 * Get_Vel_Modifier() )
			{
				Add_Velocity( 0, -0.6f );
			}
		}
		// slow down if not pressed
		else
		{
			Auto_Slow_Down( 0, 0.4f );
		}

		// set flying rotation
		float fly_rot = m_vely * 2.5f;

		// limit angle
		if( fly_rot > 70 )
		{
			fly_rot = 70;
		}
		else if( fly_rot < -70 )
		{
			fly_rot = -70;
		}

		Set_Rotation_Z( fly_rot );
	}
	// falling
	else
	{
		// move left
		if( ( pKeyboard->keys[pPreferences->m_key_left] || pJoystick->left ) && !ducked )
		{
			if( !parachute )
			{
				if( m_velx > -10 * Get_Vel_Modifier() )
				{
					Add_Velocity( -1.2f, 0 );
				}
			}
			// slow fall/parachute
			else
			{
				if( m_velx > -5 * Get_Vel_Modifier() )
				{
					Add_Velocity( -0.6f, 0 );
				}
			}

		}
		// move right
		else if( ( pKeyboard->keys[pPreferences->m_key_right] || pJoystick->right ) && !ducked )
		{
			if( !parachute )
			{
				if( m_velx < 10 * Get_Vel_Modifier() )
				{
					Add_Velocity( 1.2f, 0 );
				}
			}
			// slow fall/parachute
			else
			{
				if( m_velx < 5 * Get_Vel_Modifier() )
				{
					Add_Velocity( 0.6f, 0 );
				}
			}
		}
		// slow down if not pressed
		else
		{
			Auto_Slow_Down( 0.4f );
		}
	}
}

void cPlayer :: Stop_Flying( bool nparachute /* = 1 */ )
{
	if( m_state != STA_FLY )
	{
		return;
	}

	if( nparachute )
	{
		parachute = 1;
	}

	Set_Moving_State( STA_FALL );
}

void cPlayer :: Start_Falling( void )
{
	const float move_y = 1.9f;

	// check if something else is now blocking
	cObjectCollisionType *col_list = Collision_Check_Relative( 0.0f, move_y, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING );

	// check possible new ground objects
	for( cObjectCollision_List::iterator itr = col_list->objects.begin(), itr_end = col_list->objects.end(); itr != itr_end; ++itr )
	{
		cObjectCollision *col_obj = (*itr);
		
		// blocked by new ground object
		if( col_obj->obj != m_ground_object && col_obj->obj->m_can_be_ground )
		{
			Set_On_Ground( col_obj->obj );
			delete col_list;
			return;
		}
	}

	delete col_list;

	// fall through ground object
	Move( 0.0f, move_y, 1 );
	Set_Moving_State( STA_FALL );
}

void cPlayer :: Update_Falling( void )
{
	// only if not on ground
	if( m_ground_object || m_state == STA_CLIMB || m_state == STA_FLY )
	{
		return;
	}

	// Add Gravitation
	if( !parachute )
	{
		if( m_vely < 25 )
		{
			Add_Velocity( 0, 2.8f );
		}
	}
	// slow fall/parachute
	else
	{
		if( m_vely < 10 )
		{
			Add_Velocity( 0, 0.6f );
		}
	}


	if( m_state != STA_JUMP && m_state != STA_FALL )
	{
		Set_Moving_State( STA_FALL );
	}
}

void cPlayer :: Start_Ducking( void )
{ 
	// only if massive ground
	if( ( m_state != STA_STAY && m_state != STA_WALK && m_state != STA_RUN ) || m_state == STA_CLIMB || !m_ground_object || m_ground_object->m_massive_type == MASS_HALFMASSIVE || ducked )
	{
		return;
	}

	duck_direction = m_direction;
	Release_Item( 1, 1 );

	// set ducking image ( without Check_OutofLevel from cMovingSprite )
	cSprite::Move( 0, m_image->m_col_h - m_images[MARYO_IMG_DUCK].m_image->m_col_h, 1 );
	Set_Image_Num( MARYO_IMG_DUCK + m_direction );

	// fixme : should not be based on the current time
	ducked = SDL_GetTicks();
	Set_Moving_State( STA_STAY );
}

void cPlayer :: Stop_Ducking( void )
{
	if( !ducked )
	{
		return;
	}

	// get space needed to stand up
	float move_y = -( m_images[MARYO_IMG_STAND].m_image->m_col_h - m_image->m_col_h );

	cObjectCollisionType *col_list = Collision_Check_Relative( 0.0f, move_y, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING );

	// failed to stand up because something is blocking
	if( col_list->size() )
	{
		// set ducked time again to stop possible power jump while in air
		ducked = SDL_GetTicks();
		delete col_list;
		return;
	}

	delete col_list;

	// unset ducking image ( without Check_out_of_Level from cMovingSprite )
	cSprite::Move( 0, move_y, 1 );
	Set_Image_Num( MARYO_IMG_STAND + m_direction );

	ducked = 0;
	ducked_animation_counter = 0;
	Set_Moving_State( STA_STAY );
}

void cPlayer :: Update_Ducking( void )
{
	if( !ducked )
	{
		return;
	}

	// lost ground object
	if( !m_ground_object )
	{
		Stop_Ducking();
		return;
	}

	if( SDL_GetTicks() - ducked > power_jump_delta )
	{
		// particle animation
		ducked_animation_counter += pFramerate->m_speed_factor * 2;

		while( ducked_animation_counter > 1 )
		{
			// create particle
			cParticle_Emitter *anim = new cParticle_Emitter();
			anim->Set_Emitter_Rect( m_col_rect.m_x, m_col_rect.m_y + ( m_col_rect.m_h * 0.8f ), m_col_rect.m_w * 0.9f, m_col_rect.m_h * 0.1f );
			anim->Set_Image( pVideo->Get_Surface( "animation/particles/star_2.png" ) );
			anim->Set_Pos_Z( m_pos_z - 0.000001f, 0.000002f );
			anim->Set_Time_to_Live( 0.3f );
			anim->Set_Fading_Alpha( 1 );
			anim->Set_Fading_Size( 1 );
			anim->Set_Const_Rotation_Z( -5, 10 );
			anim->Set_Direction_Range( 90, 20 );
			anim->Set_Speed( 1.5f, 0.5f );
			anim->Set_Scale( 0.2f );
			anim->Set_Color( whitealpha128 );
			anim->Set_Blending( BLEND_ADD );
			pAnimation_Manager->Add( anim );

			ducked_animation_counter--;
		}
	}

	// slow down
	if( m_velx )
	{
		Auto_Slow_Down( 0.6f );
	}
}

void cPlayer :: Start_Climbing( void )
{
	// invalid state
	if( ducked || m_state == STA_CLIMB || jump_power > 5 )
	{
		return;
	}

	float move_y = -1.0f;

	// if not on climbable
	if( !Is_On_Climbable( move_y ) )
	{
		return;
	}

	// avoid that we are set on the ground again
	Col_Move( 0.0f, move_y, 1, 1 );

	Release_Item( 1, 1 );
	Set_Moving_State( STA_CLIMB );
}

void cPlayer :: Update_Climbing( void )
{
	if( m_state != STA_CLIMB )
	{
		return;
	}

	m_velx = 0.0f;
	m_vely = 0.0f;

	if( Is_On_Climbable() )
	{
		// set velocity
		if( pKeyboard->keys[pPreferences->m_key_left] || pJoystick->left )
		{
			m_velx = -2.0f * Get_Vel_Modifier();
		}
		else if( pKeyboard->keys[pPreferences->m_key_right] || pJoystick->right )
		{
			m_velx = 2.0f * Get_Vel_Modifier();
		}

		if( pKeyboard->keys[pPreferences->m_key_up] || pJoystick->up )
		{
			m_vely = -4.0f * Get_Vel_Modifier();
		}
		else if( pKeyboard->keys[pPreferences->m_key_down] || pJoystick->down )
		{
			m_vely = 4.0f * Get_Vel_Modifier();
		}

		// check if reached climbable top
		if( m_vely < 0.0f )
		{
			// do not climb further upwards if maryo will loose contact
			if( !Is_On_Climbable( -10.0f ) )
			{
				// only stop if loosing contact upwards
				m_vely = 0.0f;
			}
		}
	}
	else
	{
		// lost contact
		Set_Moving_State( STA_FALL );
	}
}

bool cPlayer :: Is_On_Climbable( float move_y /* = 0.0f */ )
{
	// create climb rect
	GL_rect climb_rect = m_col_rect;
	climb_rect.m_x += (climb_rect.m_w * 0.5f) - 2;
	climb_rect.m_y += move_y;
	climb_rect.m_w = 4.0f;
	climb_rect.m_h = 4.0f;

	// collision testing
	cObjectCollisionType *col_list = Collision_Check( &climb_rect, COLLIDE_ONLY_INTERNAL );

	// check objects
	for( cObjectCollision_List::iterator itr = col_list->objects.begin(), itr_end = col_list->objects.end(); itr != itr_end; ++itr )
	{
		cObjectCollision *col_obj = (*itr);

		// collision with climbable object
		if( col_obj->obj->m_massive_type == MASS_CLIMBABLE )
		{
			delete col_list;
			return 1;
		}
	}

	delete col_list;
	return 0;
}

void cPlayer :: Start_Jump_Keytime( void )
{
	if( godmode || m_state == STA_STAY || m_state == STA_WALK || m_state == STA_RUN || m_state == STA_FALL || m_state == STA_FLY || m_state == STA_JUMP || ( m_state == STA_CLIMB && !pKeyboard->keys[pPreferences->m_key_up] ) )
	{
		UpKeytime = speedfactor_fps / 4;
	}
}

void cPlayer :: Update_Jump_Keytime( void )
{
	// handle jumping start
	if( force_jump || ( UpKeytime && ( m_ground_object || godmode || m_state == STA_CLIMB ) ) )
	{
		Start_Jump();
	}
}

void cPlayer :: Start_Jump( float deaccel /* = 0.08f */ )
{
	// play sound
	if( next_jump_sound )
	{
		// small
		if( maryo_type == MARYO_SMALL )
		{
			if( force_jump )
			{
				pAudio->Play_Sound( "player/jump_small_power.ogg", RID_MARYO_JUMP );
			}
			else
			{
				pAudio->Play_Sound( "player/jump_small.ogg", RID_MARYO_JUMP );
			}
		}
		// ghost
		else if( maryo_type == MARYO_GHOST )
		{
			pAudio->Play_Sound( "player/jump_ghost.ogg", RID_MARYO_JUMP );
		}
		// big
		else
		{
			if( force_jump )
			{
				pAudio->Play_Sound( "player/jump_big_power.ogg", RID_MARYO_JUMP );
			}
			else
			{
				pAudio->Play_Sound( "player/jump_big.ogg", RID_MARYO_JUMP );
			}
		}
	}

	bool jump_key = 0;

	// if jump key pressed
	if( pKeyboard->keys[pPreferences->m_key_jump] || ( pPreferences->m_joy_analog_jump && pJoystick->up ) || pJoystick->Button( pPreferences->m_joy_button_jump ) )
	{
		jump_key = 1;
	}

	// todo : is this needed ?
	// avoid that we are set on the ground again
	Col_Move( 0.0f, -1.0f, 1, 1 );

	// fly
	if( maryo_type == MARYO_CAPE && !force_jump && m_state == STA_RUN && jump_key && ( ( m_direction == DIR_RIGHT && m_velx > 14 ) || ( m_direction == DIR_LEFT && m_velx < -14 ) ) )
	{
		m_vely = -next_jump_power * 0.5f;
		Set_Moving_State( STA_FLY );
	}
	// jump
	else
	{
		jump_accel_up = next_jump_accel;
		jump_vel_deaccel = deaccel;

		if( jump_key )
		{
			jump_power = next_jump_power * 0.59f;
		}
		else
		{
			jump_power = next_jump_power * 0.12f;
		}
		
		m_vely = -next_jump_power;
		Set_Moving_State( STA_JUMP );
	}

	// jump higher when running
	if( m_velx < 0.0f )
	{
		m_vely += m_velx / 9.5f;
	}
	else if( m_velx > 0.0f )
	{
		m_vely -= m_velx / 9.5f;
	}

	// slow down if running
	m_velx = m_velx * 0.9f;

	// jump with velx if ducking but only into the opposite start duck direction to get out of a hole
	if( ducked )
	{
		if( m_direction == DIR_RIGHT && duck_direction != m_direction )
		{
			if( m_velx < 5.0f )
			{
				m_velx += 2.0f;
			}
		}
		else if( m_direction == DIR_LEFT && duck_direction != m_direction )
		{
			if( m_velx > -5.0f )
			{
				m_velx -= 2.0f;
			}
		}
	}

	// reset variables
	UpKeytime = 0.0f;
	force_jump = 0;
	next_jump_sound = 1;
	next_jump_power = 17.0f;
	next_jump_accel = 4.0f;
}

void cPlayer :: Update_Jump( void )
{
	// jumping keytime
	if( UpKeytime )
	{
		UpKeytime -= pFramerate->m_speed_factor;

		if( UpKeytime < 0.0f )
		{
			UpKeytime = 0.0f;
		}
	}
	
	// only if jumping
	if( m_state != STA_JUMP )
	{
		return;
	}

	// jumping physics
	if( pKeyboard->keys[pPreferences->m_key_jump] || ( pPreferences->m_joy_analog_jump && pJoystick->up ) || pJoystick->Button( pPreferences->m_joy_button_jump ) )
	{
		Add_Velocity( 0.0f, -( jump_accel_up + ( m_vely * jump_vel_deaccel ) / Get_Vel_Modifier() ) );
		jump_power -= pFramerate->m_speed_factor;
	}
	else
	{
		Add_Velocity( 0.0f, 0.5f );
		jump_power -= 6 * pFramerate->m_speed_factor;
	}
	
	// left right physics
	if( ( pKeyboard->keys[pPreferences->m_key_left] || pJoystick->left ) && !ducked )
	{
		if( m_velx > -10.0f * Get_Vel_Modifier() )
		{
			Add_Velocity( ( -1.1f * Get_Vel_Modifier() ) - ( m_velx / 100 ), 0.0f );
		}
		
	}	
	else if( ( pKeyboard->keys[pPreferences->m_key_right] || pJoystick->right ) && !ducked && m_state != STA_STAY )
	{
		if( m_velx < 10.0f * Get_Vel_Modifier() )
		{
			Add_Velocity( ( 1.1f * Get_Vel_Modifier() ) + ( m_velx / 100 ), 0.0f );
		}
	}
	// slow down
	else 
	{
		Auto_Slow_Down( 0.2f );
	}

	// if no more jump power set to falling
	if( jump_power <= 0.0f )
	{
		Set_Moving_State( STA_FALL );
	}
}

void cPlayer :: Update_Item( void )
{
	// if active object item is available
	if( active_object )
	{
		// collision with something or activated
		if( active_object->m_type == TYPE_TURTLE && static_cast<cTurtle *>(active_object)->m_dead )
		{
			Release_Item( 0 );
			return;
		}

		// update the item position
		float item_pos_x = 0.0f;
		float item_pos_y = 0.0f;

		if( maryo_type == MARYO_SMALL )
		{
			if( m_direction == DIR_LEFT )
			{
				item_pos_x = (m_rect.m_w * 0.20f) - active_object->m_col_rect.m_w;
			}
			else
			{
				item_pos_x = m_rect.m_w * 0.64f;
			}
			
			item_pos_y = m_rect.m_h * 0.14f;
		}
		// big
		else
		{
			if( m_direction == DIR_LEFT )
			{
				item_pos_x = (m_rect.m_w * 0.21f) - active_object->m_col_rect.m_w;
			}
			else
			{
				item_pos_x = m_rect.m_w * 0.66f;
			}

			item_pos_y = m_rect.m_h * 0.18f;
		}

		active_object->Set_Pos( m_pos_x + item_pos_x, m_pos_y + item_pos_y );

		return;
	}

	// invalid state
	if( m_state == STA_FLY || ducked || m_state == STA_CLIMB )
	{
		return;
	}

	// if control is pressed search for items in front of the player
	if( pKeyboard->keys[pPreferences->m_key_action] || pJoystick->Button( pPreferences->m_joy_button_action ) )
	{
		// next position velocity with extra size
		float check_x = ( m_velx > 0.0f ) ? ( m_velx + 5.0f ) : ( m_velx - 5.0f );
		
		// if wrong direction return
		if( ( m_direction == DIR_LEFT && check_x >= 0.0f ) || ( m_direction == DIR_RIGHT && check_x <= 0.0f ) )
		{
			return;
		}

		// check the next player position for objects
		cObjectCollisionType *col_list = Collision_Check_Relative( ( m_direction == DIR_LEFT ) ? (check_x) : (m_col_rect.m_w), 0, ( m_direction == DIR_LEFT ) ? (-check_x) : (check_x) );

		// possible objects
		for( cObjectCollision_List::iterator itr = col_list->objects.begin(), itr_end = col_list->objects.end(); itr != itr_end; ++itr )
		{
			cObjectCollision *col = (*itr);

			// enemy item
			if( col->m_array == ARRAY_ENEMY )
			{
				if( col->obj->m_type == TYPE_TURTLE )
				{
					cEnemy *enemy = static_cast<cEnemy *>(col->obj);

					if( enemy->m_state == STA_STAY )
					{
						Get_Item( TYPE_TURTLE, 0, enemy );
						break;
					}
				}
			}
			// other items here...
		}

		delete col_list;
	}
}

void cPlayer :: Release_Item( bool set_position /* = 1 */, bool no_action /* = 0 */ )
{
	if( !active_object )
	{
		return;
	}

	ObjectDirection kick_direction = DIR_UNDEFINED;

	// if flying : opposite direction
	if( m_state == STA_FLY )
	{
		kick_direction = Get_Opposite_Direction( m_direction );
	}
	// if jumping
	else if( jump_power > 0.0f )
	{
		// kick it upwards
		kick_direction = DIR_UP;
	}
	// use current direction
	else
	{
		kick_direction = m_direction;
	}

	// add back to level
	if( active_object->m_type == TYPE_TURTLE )
	{
		cTurtle *turtle = static_cast<cTurtle *>(active_object);
		
		// play kick sound if not dead
		if( !turtle->m_dead )
		{
			pAudio->Play_Sound( "enemy/turtle/shell/hit.ogg" );
		}

		// if object got kicked upwards use state stay
		if( kick_direction == DIR_UP || no_action )
		{
			turtle->m_turtle_state = TURTLE_DEAD;
			turtle->Set_Turtle_Moving_State( TURTLE_SHELL_STAND );
			
			// small direction acceleration if no object action
			if( kick_direction == DIR_LEFT )
			{
				if( no_action )
				{
					turtle->m_direction = kick_direction;
					active_object->m_velx = -5.0f;
				}
			}
			else if( kick_direction == DIR_RIGHT )
			{
				if( no_action )
				{
					turtle->m_direction = kick_direction;
					active_object->m_velx = 5.0f;
				}
			}
			// upwards
			else if( kick_direction == DIR_UP )
			{
				if( !no_action )
				{
					active_object->m_vely = m_vely - 10.0f;
				}
			}
		}
		// default object horizontal kicking
		else
		{
			turtle->Set_Direction( kick_direction );
			turtle->m_turtle_state = TURTLE_DEAD;
			turtle->Set_Turtle_Moving_State( TURTLE_SHELL_RUN );
		}

		if( !turtle->m_dead )
		{
			turtle->m_massive_type = MASS_MASSIVE;

			// if shell and not frozen
			if( turtle->m_state == STA_RUN && !turtle->m_freeze_counter )
			{
				// safe time
				turtle->m_player_counter = speedfactor_fps * 0.5f;
			}
		}

		turtle->Update_Valid_Update();
	}

	// if the new object position is in the player ( e.g. add velx to move slowly out of the player position )
	bool position_in_player = 0;

	// set position
	if( set_position )
	{
		// set start position
		// center it horizontally
		float start_posx = m_pos_x + m_col_pos.m_x + ( active_object->m_col_rect.m_w / 2 ) - ( m_col_rect.m_w / 2 );
		float start_posy = m_pos_y + m_col_pos.m_y;

		// put in front if left or right
		if( kick_direction == DIR_LEFT )
		{
			start_posx -= m_col_rect.m_w + 9.0f;
		}
		else if( kick_direction == DIR_RIGHT )
		{
			start_posx += m_col_rect.m_w + 2.0f;
		}
		else if( kick_direction == DIR_UP )
		{
			// put on top
			start_posy += -( active_object->m_col_pos.m_y + active_object->m_col_rect.m_h ) + ( active_object->m_vely * pFramerate->m_speed_factor );
		}

		active_object->Set_Pos( start_posx, start_posy );

		// set step size
		float step_size = 0.0f;
		cObjectCollisionType *col_list = NULL;

		// check for a valid position to release the object
		while( step_size < 50.0f )
		{
			if( col_list )
			{
				delete col_list;
			}
			
			// check left side
			col_list = active_object->Collision_Check_Relative( step_size, 0.0f, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING );
			
			// collides with a blocking object on the left side
			if( !col_list->empty() && ( col_list->Is_Included( ARRAY_MASSIVE ) || col_list->Is_Included( ARRAY_ACTIVE ) || col_list->Is_Included( ARRAY_ENEMY ) ) )
			{
				delete col_list;
				// check right side
				col_list = active_object->Collision_Check_Relative( -step_size, 0.0f, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING );

				// also collides on the right side
				if( !col_list->empty() && ( col_list->Is_Included( ARRAY_MASSIVE ) || col_list->Is_Included( ARRAY_ACTIVE ) || col_list->Is_Included( ARRAY_ENEMY ) ) )
				{
					// continue moving
				}
				else
				{
					// move to the right
					active_object->Move( -step_size, 0.0f, 1 );
					break;
				}
			}
			else
			{
				// move to the left
				active_object->Move( step_size, 0.0f, 1 );
				break;
			}

			step_size += 2.0f;
		}

		// check if still blocking objects on the final position
		if( !col_list->empty() && ( col_list->Is_Included( ARRAY_MASSIVE ) || col_list->Is_Included( ARRAY_ACTIVE ) || col_list->Is_Included( ARRAY_ENEMY ) ) )
		{
			if( active_object->m_type == TYPE_TURTLE )
			{
				// shell
				if( active_object->m_state == STA_RUN )
				{
					// if collision with static blocking objects
					if( col_list->Is_Included( ARRAY_MASSIVE ) || col_list->Is_Included( ARRAY_ACTIVE ) )
					{
						active_object->DownGrade( 1 );
					}
				}
				// normal
				else if( active_object->m_state == STA_STAY )
				{
					// downgrade always
					active_object->DownGrade( 1 );
				}
			}
		}

		delete col_list;
	}

	active_object->Clear_Collisions();
	active_object->Reset_On_Ground();

	// unset link
	active_object = NULL;
	// load default images
	Load_Images();
}

void cPlayer :: Set_Type( SpriteType item_type, bool animation /* = 1 */, bool sound /* = 1 */, bool temp_power /* = 0 */ )
{
	if( item_type == TYPE_PLAYER )
	{
		Set_Type( MARYO_SMALL, animation, sound, temp_power );
	}
	else if( item_type == TYPE_MUSHROOM_DEFAULT )
	{
		Set_Type( MARYO_BIG, animation, sound, temp_power );
	}
	else if( item_type == TYPE_MUSHROOM_BLUE )
	{
		Set_Type( MARYO_ICE, animation, sound, temp_power );
	}
	else if( item_type == TYPE_MUSHROOM_GHOST )
	{
		Set_Type( MARYO_GHOST, animation, sound, temp_power );
	}
	else if( item_type == TYPE_FIREPLANT )
	{
		Set_Type( MARYO_FIRE, animation, sound, temp_power );
	}
}

void cPlayer :: Set_Type( Maryo_type new_type, bool animation /* = 1 */, bool sound /* = 1 */, bool temp_power /* = 0 */ )
{
	// already set
	if( maryo_type == new_type )
	{
		return;
	}

	// play sound
	if( sound )
	{
		if( new_type == MARYO_BIG )
		{
			pAudio->Play_Sound( "item/mushroom.ogg", RID_MUSHROOM );
		}
		else if( new_type == MARYO_FIRE )
		{
			pAudio->Play_Sound( "item/fireplant.ogg", RID_FIREPLANT );
		}
		else if( new_type == MARYO_ICE )
		{
			pAudio->Play_Sound( "item/mushroom_blue.wav", RID_MUSHROOM_BLUE );
		}
		else if( new_type == MARYO_CAPE )
		{
			pAudio->Play_Sound( "item/feather.ogg", RID_FEATHER );
		}
		else if( new_type == MARYO_GHOST )
		{
			pAudio->Play_Sound( "item/mushroom_ghost.ogg", RID_MUSHROOM_GHOST );
		}
	}

	if( !temp_power )
	{
		// was flying
		if( maryo_type == MARYO_CAPE )
		{
			Stop_Flying( 0 );
		}
	}

	// remember old type
	Maryo_type old_type = maryo_type;

	// draw animation and set new type
	if( animation )
	{
		Draw_Animation( new_type );

		if( temp_power )
		{
			maryo_type = old_type;
			maryo_type_temp_power = new_type;
			// Draw_Animation ends with the new type images
			Load_Images();
		}

	}
	// only set type
	else
	{
		if( temp_power )
		{
			maryo_type_temp_power = new_type;
		}
		else
		{
			maryo_type = new_type;
			Load_Images();
		}
	}

	// nothing more needed for setting the temp power
	if( temp_power )
	{
		return;
	}

	// to ghost
	if( maryo_type == MARYO_GHOST )
	{
		ghost_time = speedfactor_fps * 10;
		maryo_type_temp_power = old_type;
	}
	// was ghost
	else if( old_type == MARYO_GHOST )
	{
		ghost_time = 0;
		ghost_time_mod = 0;
		maryo_type_temp_power = MARYO_DEAD;

		// check for ghost ground
		if( m_ground_object )
		{
			cBaseBox *box = dynamic_cast<cBaseBox *>( m_ground_object );

			if( box )
			{
				// ghost box
				if( box->box_invisible == BOX_GHOST )
				{
					Reset_On_Ground();
				}
			}
		}
	}

	// check if on ground as the size could also change
	Check_on_Ground();
}

void cPlayer :: Set_Moving_State( Moving_state new_state )
{
	if( new_state == m_state )
	{
		return;
	}

	// can not change from linked state
	if( m_state == STA_OBJ_LINKED )
	{
		return;
	}

	// was falling
	if( m_state == STA_FALL )
	{
		// reset slow fall/parachute
		parachute = 0;
	}
	// was flying
	else if( m_state == STA_FLY )
	{
		Change_Size( 0, -( m_images[MARYO_IMG_FALL].m_image->m_h - m_images[MARYO_IMG_FLY].m_image->m_h ) );
		Set_Image_Num( MARYO_IMG_FALL + m_direction );

		// reset flying rotation
		m_rot_z = 0;
	}
	// was jumping
	else if( m_state == STA_JUMP )
	{
		// reset variables
		UpKeytime = 0;
		jump_power = 0;
		force_jump = 0;
		next_jump_sound = 1;
		next_jump_power = 17;
		next_jump_accel = 4;
	}
	// was running
	else if( m_state == STA_RUN )
	{
		walk_time = 0;
		running_particle_counter = 0;
	}

	// # before new state is set
	if( new_state == STA_STAY )
	{
		if( m_ground_object )
		{
			m_vely = 0;
		}
	}
	else if( new_state == STA_FALL )
	{
		Reset_On_Ground();
		walk_count = 0;
		jump_power = 0;
	}
	else if( new_state == STA_FLY )
	{
		Reset_On_Ground();
	}
	else if( new_state == STA_JUMP )
	{
		Reset_On_Ground();
	}
	else if( new_state == STA_CLIMB )
	{
		Reset_On_Ground();
		m_velx = 0;
		m_vely = 0;
	}

	m_state = new_state;

	// # after new state is set
	if( m_state == STA_FLY )
	{
		Release_Item( 1 );
	}
}

bool cPlayer :: Change_Size( float x, float y, bool only_check /* = 0 */ )
{
	bool valid_hor = 0;
	float check_pos = x;

	// no value
	if( Is_Float_Equal( x, 0.0f ) )
	{
		valid_hor = 1;
	}

	while( !valid_hor )
	{
		cObjectCollisionType *col_list = Collision_Check_Relative( check_pos, 0.0f, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING );

		if( col_list->empty() )
		{
			if( !only_check )
			{
				Col_Move( check_pos, 0.0f, 1, 1 );
			}

			valid_hor = 1;
			delete col_list;
			break;
		}

		delete col_list;

		// move to opposite direction
		if( x > 0.0f )
		{
			check_pos--;

			// nothing found
			if( check_pos < -x )
			{
				break;
			}
		}
		else
		{
			check_pos++;

			// nothing found
			if( check_pos > -x )
			{
				break;
			}
		}
	}

	bool valid_ver = 0;
	check_pos = y;

	// no value
	if( Is_Float_Equal( y, 0.0f ) )
	{
		valid_ver = 1;
	}

	while( !valid_ver )
	{
		cObjectCollisionType *col_list = Collision_Check_Relative( 0.0f, check_pos, 0.0f, 0.0f, COLLIDE_ONLY_BLOCKING );

		if( col_list->empty() )
		{
			if( !only_check )
			{
				Col_Move( 0.0f, check_pos, 1, 1 );
			}

			valid_ver = 1;
			delete col_list;
			break;
		}

		delete col_list;

		// move to opposite direction
		if( y > 0.0f )
		{
			check_pos--;

			// nothing found
			if( check_pos < -y )
			{
				break;
			}
		}
		else
		{
			check_pos++;

			// nothing found
			if( check_pos > -y )
			{
				break;
			}
		}
	}


	// if both directions valid
	if( valid_hor && valid_ver )
	{
		return 1;
	}

	return 0;
}

void cPlayer :: Reset_Save( void )
{
	// reset world
	pOverworld_Manager->Reset();

	// unload level
	pActive_Level->Unload();

	// reset player
	Set_Type( MARYO_SMALL, 0, 0 );
	Reset();
	lives = 3;
	goldpieces = 0;
	points = 0;

	pHud_Time->Reset();
	pHud_Manager->Update_Text();
	pHud_Itembox->Reset();
}

void cPlayer :: Reset( bool full /* = 1 */ )
{
	// reset level states
	Set_Active( 1 );
	Reset_Position();
	Set_Direction( m_start_direction );
	ducked = 0;
	Set_Moving_State( STA_FALL );
	Set_Image_Num( Get_Image() + m_direction );
	jump_power = 0;
	jump_accel_up = 0;
	jump_vel_deaccel = 0;
	no_velx_counter = 0;
	no_vely_counter = 0;
	UpKeytime = 0;
	m_velx = 0;
	m_vely = 0;
	Clear_Collisions();
	walk_count = 0;
	Reset_On_Ground();
	if( active_object )
	{
		active_object = NULL;
		Load_Images();
	}
	kill_multiplier = 1;
	last_kill_counter = 0;

	// reset item and camera
	if( full )
	{
		Ball_Clear();
		invincible = 0;
		invincible_mod = 0;
		invincible_star = 0;
		Set_Color_Combine( 0, 0, 0, 0 );
		pHud_Itembox->Push_back();
	}
}

void cPlayer :: Reset_Position( void )
{
	Set_Pos( m_start_pos_x, m_start_pos_y );
	
	/* change player position to the current state height
	 * move to correct position but do not change start position
	*/
	Col_Move( 0, m_start_image->m_h - m_rect.m_h, 1, 0, 0 );
}

void cPlayer :: Goto_Next_Level( void )
{
	life_bar = MAX_LIFE;

	// custom level
	if( Game_Mode_Type == MODE_TYPE_LEVEL_CUSTOM )
	{
		// Enter Menu
		Game_Action_Data.add( "level", Trim_Filename( pActive_Level->m_level_filename, 0, 0 ) );
		Game_Action = GA_ENTER_MENU;
		// delay unload level
		pActive_Level->Unload( 1 );
	}
	else
	{
		// Finish level
		pActive_Overworld->Goto_Next_Level();
		// Enter World
		Game_Action = GA_ENTER_WORLD;
	}
}

void cPlayer :: Goto_Sub_Level( std::string str_level, const std::string &str_entry, Camera_movement move_camera /* = CAMERA_MOVE_FLY */, const std::string &path_identifier /* = "" */ )
{
	// if empty use same level
	if( str_level.empty() )
	{
		str_level = Trim_Filename( pActive_Level->data_file, 0, 0 );
	}

	// same level - scroll to the position
	if( str_level.compare( Trim_Filename( pActive_Level->data_file, 0, 0 ) ) == 0 )
	{
		Release_Item( 1, 1 );
		// fixme: reset does not release active_object correctly
		Reset( 0 );
		
		// get entry
		cLevel_Entry *entry = pActive_Level->Get_Entry( str_entry );

		// use entry position
		if( entry )
		{
			Set_Pos( entry->Get_Player_Pos_X(), entry->Get_Player_Pos_Y() );
			// set invisible for warp animation
			Set_Active( 0 );
		}
		// not found
		else if( !str_entry.empty() )
		{
			printf( "Warning : Level entry %s not found\n", str_entry.c_str() );
		}

		 // move camera to new position
		if( move_camera != CAMERA_MOVE_NONE )
		{
			pFramerate->m_speed_factor = 1;

			// fly to new position or move along path to new position
			if( move_camera == CAMERA_MOVE_FLY || move_camera == CAMERA_MOVE_ALONG_PATH || move_camera == CAMERA_MOVE_ALONG_PATH_BACKWARDS )
			{
				// move along path
				if( move_camera == CAMERA_MOVE_ALONG_PATH || move_camera == CAMERA_MOVE_ALONG_PATH_BACKWARDS )
				{
					// create path state
					cPath_State path_state = cPath_State();
					// set parent path
					path_state.Set_Path_Identifier( path_identifier );

					// path found
					if( path_state.m_path )
					{
						// forward
						if( move_camera == CAMERA_MOVE_ALONG_PATH )
						{
							path_state.Move_Start_Forward();
						}
						// backward
						else
						{
							path_state.Move_Start_Backward();
						}

						// start path position
						float start_path_pos_x = path_state.m_path->m_col_rect.m_x - (game_res_w * 0.5f);
						float start_path_pos_y = path_state.m_path->m_col_rect.m_y - (game_res_h * 0.5f);
						// move gradually to start point
						pActive_Camera->Move_to_Position_Gradually( start_path_pos_x + path_state.pos_x, start_path_pos_y + path_state.pos_y, 20 );
						// reset offset
						//pActive_Camera->x_offset = 0;
						//pActive_Camera->y_offset = 0;

						while( 1 )
						{
							// move along path
							if( path_state.Path_Move( 30 * pFramerate->m_speed_factor ) == 0 )
							{
								// we can not move further along the path
								break;
							}

							// update audio
							pAudio->Update();
							// Move camera
							pActive_Camera->Set_Pos( start_path_pos_x + path_state.pos_x, start_path_pos_y + path_state.pos_y );
							// keep global effect particles on screen
							pActive_Level->m_global_effect->Update_Particles();

							// draw
							Draw_Game();

							pVideo->Render();
							pFramerate->Update();
						}
					}
					// not found
					else
					{
						printf( "Warning : Level path %s not found\n", path_identifier.c_str() );
					}
				}

				/* fly to player position
				 * also needed after the move along path or it is not smooth
				*/
				pActive_Camera->Move_to_Position_Gradually( pActive_Camera->Get_Center_Pos_X(), pActive_Camera->Get_Center_Pos_Y() );
			}
			// blink to new position
			else
			{
				Draw_Effect_Out( EFFECT_OUT_BLACK, 5 );
				pActive_Camera->Center();
				Draw_Effect_In( EFFECT_IN_BLACK, 5 );
			}
		}

		// warp animation
		if( entry )
		{
			// set back visible
			Set_Active( 1 );
			// activate entry
			entry->Activate();
		}
	}
	// another level - load the level
	else
	{
		Game_Action = GA_ENTER_LEVEL;
		Game_Action_Data.add( "level", str_level.c_str() );
		Game_Action_Data.add( "entry", str_entry.c_str() );
	}
}

void cPlayer :: Update( void )
{
	if( editor_enabled )
	{
		return;
	}

	// check if got stuck
	if( !ducked )
	{
		Update_Anti_Stuck();
	}

	// check if starting a jump is possible
	Update_Jump_Keytime();

	// update states
	Update_Jump();
	Update_Climbing();
	Update_Falling();
	Update_Walking();
	Update_Running();
	Update_Ducking();
	Update_Staying();
	Update_Flying();
	// throw animation counter
	if( throwing_counter > 0 )
	{
		throwing_counter -= pFramerate->m_speed_factor;

		if( throwing_counter < 0 )
		{
			throwing_counter = 0;
		}
	}
	// shoot counter
	if( shoot_counter > 0 )
	{
		if (num_shots < 3)
		{
			shoot_counter = 0;
		}
		else
		{
			shoot_counter -= ((pFramerate->m_speed_factor));
			//shoot_counter -= pFramerate->m_speed_factor;
		}
	
		if( shoot_counter < 0 )
		{
			shoot_counter = 0;
		}
	}	
	// ghost
	if( ghost_time > 0 )
	{
		ghost_time -= pFramerate->m_speed_factor;

		// ended
		if( ghost_time <= 0 )
		{
			pAudio->Play_Sound( "player/ghost_end.ogg", RID_MUSHROOM_GHOST );
			Set_Type( maryo_type_temp_power, 1, 0 );
		}
		// near end
		else if( ghost_time * 0.5f < speedfactor_fps * 3 )
		{
			ghost_time_mod = ( speedfactor_fps * 3 ) - ( ghost_time * 0.5f );
		}
	}
	// invincible
	if( invincible > 0 )
	{
		invincible -= pFramerate->m_speed_factor;

		if( invincible < 0 )
		{
			invincible = 0;
		}
		else
		{
			invincible_mod -= pFramerate->m_speed_factor * 20;

			if( invincible_mod < 0 )
			{
				invincible_mod = invincible * 3;

				if( invincible_mod > 180 )
				{
					invincible_mod = 180;
				}
			}
		}
	}
	// draw stars if in godmode or star invincible
	if( godmode || invincible_star > 0 )
	{
		if( editor_level_enabled )
		{
			Set_Color_Combine( 0, 0, 0, 0 );
		}
		else
		{
			// draw stars
			invincible_starcounter += pFramerate->m_speed_factor;

			if( invincible_star > 0 )
			{
				invincible_star -= pFramerate->m_speed_factor;

				if( invincible_star <= 0 )
				{
					Set_Color_Combine( 0, 0, 0, 0 );
					invincible_star = 0;
					pAudio->Fadeout_Music( 500 );
				}
			}

			while( invincible_starcounter > 1 )
			{
				// set particle color
				Color particle_color = orange;
				particle_color.green += static_cast<Uint8>( invincible_mod / 5 );
				particle_color.blue += static_cast<Uint8>( invincible_mod / 1.5f );

				// create particle
				cParticle_Emitter *anim = new cParticle_Emitter();
				anim->Set_Emitter_Rect( m_col_rect.m_x + m_col_rect.m_w * 0.1f, m_col_rect.m_y + m_col_rect.m_h * 0.1f, m_col_rect.m_w * 0.8f, m_col_rect.m_h * 0.8f );
				anim->Set_Image( pVideo->Get_Surface( "animation/particles/star.png" ) );
				anim->Set_Pos_Z( m_pos_z - 0.000001f );
				anim->Set_Time_to_Live( 0.3f );
				anim->Set_Fading_Alpha( 1 );
				anim->Set_Fading_Size( 1 );
				anim->Set_Const_Rotation_Z( -5, 10 );
				// godmode stars
				if( godmode )
				{
					anim->Set_Direction_Range( 180, 180 );
					anim->Set_Speed( 3.5f, 2.5f );
					anim->Set_Scale( 0.3f );
					anim->Set_Color( lightblue );
				}
				// default
				else
				{
					anim->Set_Time_to_Live( 2 );
					anim->Set_Speed( 1, 1.5f );
					anim->Set_Scale( 0.2f );
					anim->Set_Color( particle_color );
				}

				anim->Set_Blending( BLEND_ADD );
				pAnimation_Manager->Add( anim );

				invincible_starcounter--;
			}
		}
	}

	// update active item
	Update_Item();

	// image counter
	if( m_state == STA_WALK || ( maryo_type != MARYO_CAPE && m_state == STA_RUN ) )
	{
		// 4 frames
		if( maryo_type == MARYO_SMALL )
		{
			walk_count += pFramerate->m_speed_factor * 0.35f;
		}
		// 4 frames
		else if( maryo_type == MARYO_BIG )
		{
			walk_count += pFramerate->m_speed_factor * 0.3f;
		}
		// 4 frames
		else
		{
			walk_count += pFramerate->m_speed_factor * 0.3f;
		}

		// ground type modification
		float vel = ( m_velx > 0 ? m_velx : -m_velx );

		if( vel && m_ground_object && m_ground_object->m_image)
		{
			float ground_mod = 0;

			switch( m_ground_object->m_image->m_ground_type )
			{
				case GROUND_ICE:
				{
					ground_mod = 0.125f;
					break;
				}
				case GROUND_SAND:
				{
					ground_mod = 0.075f;
					break;
				}
				case GROUND_PLASTIC:
				{
					ground_mod = 0.03f;
					break;
				}
				default:
				{
					break;
				}
			}

			if( ground_mod )
			{
				walk_count += ground_mod * ( 5 / vel ) * pFramerate->m_speed_factor;
			}
		}
	}
	else if( m_state == STA_RUN )
	{
		// ? frames
		if( maryo_type == MARYO_SMALL )
		{
			//walk_count += pFramerate->m_speed_factor * 0.35f;
		}
		// ? frames
		else if( maryo_type == MARYO_BIG )
		{
			//walk_count += pFramerate->m_speed_factor * 0.3f;
		}
		// 2 frames
		else if( maryo_type == MARYO_CAPE )
		{
			walk_count += pFramerate->m_speed_factor * 0.35f;
		}
		// ? frames
		else
		{
			//walk_count += pFramerate->m_speed_factor * 0.3f;
		}
	}
	/*else if( state == FALL || state == JUMP )
	{
		walk_count += pFramerate->m_speed_factor * 0.3f;
	}*/
	else if( m_state == STA_CLIMB )
	{
		if( m_vely != 0 || m_velx != 0 )
		{
			walk_count += pFramerate->m_speed_factor * 0.7f;
		}
	}
	else if( m_state == STA_FLY )
	{
		walk_count += pFramerate->m_speed_factor * 0.6f;
	}

	// add x velocity
	if( m_state == STA_WALK || m_state == STA_RUN || m_state == STA_CLIMB )
	{
		if( m_velx > 0 )
		{
			walk_count += ( m_velx * 0.05f ) * pFramerate->m_speed_factor;
		}
		else if( m_velx < 0 )
		{
			walk_count += ( -m_velx * 0.05f ) * pFramerate->m_speed_factor;
		}
	}

	if( m_state == STA_WALK || m_state == STA_STAY || ( maryo_type != MARYO_CAPE && m_state == STA_RUN ) )
	{
		// 4 frames
		if( maryo_type == MARYO_SMALL )
		{
			if( walk_count >= 8 )
			{
				walk_count = 0;
			}
		}
		// 4 frames
		else if( maryo_type == MARYO_BIG )
		{
			if( walk_count >= 8 )
			{
				walk_count = 0;
			}
		}
		// 4 frames
		else
		{
			if( walk_count >= 8 )
			{
				walk_count = 0;
			}
		}
	}
	else if( m_state == STA_RUN )
	{
		// ? frames
		if( maryo_type == MARYO_SMALL )
		{
		}
		// ? frames
		else if( maryo_type == MARYO_BIG )
		{
		}
		// 2 frames
		else if( maryo_type == MARYO_CAPE )
		{
			if( walk_count >= 4 )
			{
				walk_count = 0;
			}
		}
		// ? frames
		else
		{
		}
	}
	/*else if( m_state == STA_FALL || m_state == STA_JUMP )
	{
		if( walk_count > 6 )
		{
			walk_count = 6;
		}
	}*/
	else if( m_state == STA_FLY )
	{
		// 4 frames
		if( walk_count > 8 )
		{
			walk_count = 0;
		}
	}
	else
	{
		// 4 frames
		if( walk_count >= 8 )
		{
			walk_count = 0;
		}
	}

	// Set image
	if( m_state != STA_CLIMB )
	{
		Set_Image_Num( Get_Image() + m_direction );
	}
	else
	{
		Set_Image_Num( Get_Image() );
	}

	// Special
	Update_Kill_Multiplier();
}

void cPlayer :: Draw( cSurface_Request *request /* = NULL */ )
{
	if( !m_valid_draw )
	{
		return;
	}

	// invincible
	if( invincible > 0 )
	{
		// star invincible
		if( invincible_star > 0 )
		{
			Set_Color_Combine( invincible_mod / 130, invincible_mod / 130, invincible_mod / 180, GL_ADD );
		}
		// default invincible
		else
		{
			Set_Color( 255, 255, 255, 255 - static_cast<Uint8>(invincible_mod) );
		}
	}
	// ghost
	if( ghost_time > 0 )
	{
		// ghost shadows
		m_color.alpha = 32 + static_cast<int>(ghost_time_mod);
		float old_posx = m_pos_x;
		float old_posy = m_pos_y;
		float old_posz = m_pos_z;

		for( unsigned int i = 0; i < 5; i++ )
		{
			m_color.alpha -= 5;
			m_pos_x -= m_velx * 0.2f + Get_Random_Float( 0, 1 );
			m_pos_y -= m_vely * 0.2f + Get_Random_Float( 0, 1 );
			m_pos_z -= 0.000001f;

			cMovingSprite::Draw( request );
		}

		// set original values
		m_pos_x = old_posx;
		m_pos_y = old_posy;
		m_pos_z = old_posz;
		m_color.alpha = 48 + static_cast<int>(ghost_time_mod);
	}

	cMovingSprite::Draw( request );

	if( invincible > 0 || ghost_time > 0 )
	{
		Set_Color( white );
	}
}

void cPlayer :: Draw_Animation( Maryo_type new_mtype )
{
	// already set or invalid
	if( new_mtype == maryo_type || maryo_type == MARYO_DEAD || new_mtype == MARYO_DEAD )
	{
		return;
	}

	Maryo_type maryo_type_old = maryo_type;
	bool parachute_old = parachute;

	float posx_old = m_pos_x;
	float posy_old = m_pos_y;

	// Change_Size needs new state size
	maryo_type = maryo_type_old;
	parachute = parachute_old;
	Load_Images();

	// correct position for bigger maryo
	if( maryo_type_old == MARYO_SMALL && ( new_mtype == MARYO_BIG || new_mtype == MARYO_FIRE || new_mtype == MARYO_ICE || new_mtype == MARYO_CAPE || new_mtype == MARYO_GHOST ) )
	{
		Change_Size( -5.0f, -12.0f );
	}
	// correct position for small maryo
	else if( ( maryo_type_old == MARYO_BIG || maryo_type_old == MARYO_FIRE || maryo_type_old == MARYO_ICE || new_mtype == MARYO_CAPE || maryo_type_old == MARYO_GHOST ) && new_mtype == MARYO_SMALL )
	{
		Change_Size( 5.0f, 12.0f );
	}

	float posx_new = m_pos_x;
	float posy_new = m_pos_y;

	// draw animation
	for( unsigned int i = 0; i < 7; i++ )
	{
		// set to current type
		if( i % 2 )
		{
			maryo_type = maryo_type_old;
			parachute = parachute_old;
			Load_Images();
			
			Set_Pos( posx_old, posy_old );
		}
		// set to new type
		else
		{
			maryo_type = new_mtype;
			if( new_mtype != MARYO_CAPE )
			{
				parachute = 0;
			}
			Load_Images();
			
			// always set the ghost type to draw the ghost rect until it ends
			if( i < 6 && maryo_type_old == MARYO_GHOST )
			{
				maryo_type = maryo_type_old;
			}

			Set_Pos( posx_new, posy_new );
		}

		// draw
		Draw_Game();
		pVideo->Render();

		// frame delay
		SDL_Delay( 120 );
	}

	pFramerate->Reset();
}

unsigned int cPlayer :: Get_Image( void ) const
{
	// throwing
	if( throwing_counter && ( maryo_type == MARYO_FIRE || maryo_type == MARYO_ICE ) && !ducked && ( m_state == STA_FALL || m_state == STA_STAY || m_state == STA_WALK || m_state == STA_RUN || m_state == STA_JUMP ) )
	{
		int imgnum = 0;

		if( throwing_counter < speedfactor_fps * 0.2f )
		{
			imgnum = 2;
		}

		return MARYO_IMG_THROW + imgnum;
	}

	// ducked
	if( ducked && ( m_state == STA_STAY || m_state == STA_WALK || m_state == STA_RUN || m_state == STA_JUMP || m_state == STA_FALL ) )
	{
		return MARYO_IMG_DUCK;
	}

	// parachute
	if( parachute && m_state == STA_FALL )
	{
		return MARYO_IMG_SPECIAL_1;
	}

	if( m_state == STA_STAY || m_state == STA_WALK || ( maryo_type != MARYO_CAPE && m_state == STA_RUN ) )
	{
		unsigned int imgnum = static_cast<unsigned int>(walk_count);

		for( unsigned int i = 0; i < imgnum; i++ )
		{
			if( imgnum % 2 )
			{
				imgnum--;
			}
		}

		return MARYO_IMG_WALK + imgnum;
	}
	else if( m_state == STA_RUN )
	{
		unsigned int imgnum = static_cast<unsigned int>(walk_count);

		for( unsigned int i = 0; i < imgnum; i++ )
		{
			if( imgnum % 2 )
			{
				imgnum--;
			}
		}

		return MARYO_IMG_RUN + imgnum;
	}
	else if( m_state == STA_FALL )
	{
		/*unsigned int imgnum = static_cast<unsigned int>(walk_count);

		for( unsigned int i = 0; i < imgnum; i++ )
		{
			if( imgnum % 2 )
			{
				imgnum--;
			}
		}

		return MARYO_IMG_FALL + imgnum;*/
		return MARYO_IMG_FALL;
	}
	/*else if( m_state == STAY )
	{
		return MARYO_IMG_STAND;
	}*/
	else if( m_state == STA_JUMP )
	{
		/*unsigned int imgnum = static_cast<unsigned int>(walk_count);

		for( unsigned int i = 0; i < imgnum; i++ )
		{
			if( imgnum % 2 )
			{
				imgnum--;
			}
		}

		return MARYO_IMG_JUMP + imgnum;*/
		return MARYO_IMG_JUMP;
	}
	else if( m_state == STA_FLY )
	{
		unsigned int imgnum = static_cast<unsigned int>(walk_count);

		for( unsigned int i = 0; i < imgnum; i++ )
		{
			if( imgnum % 2 )
			{
				imgnum--;
			}
		}

		return MARYO_IMG_FLY + imgnum;
	}
	else if( m_state == STA_CLIMB )
	{
		if( walk_count > 4 )
		{
			return MARYO_IMG_CLIMB;
		}
		else
		{
			return MARYO_IMG_CLIMB + 1;
		}
	}

	return MARYO_IMG_STAND;
}

void cPlayer :: Load_Images( void )
{
	// not valid
	if( maryo_type == MARYO_DEAD )
	{
		return;
	}

	Clear_Images();

	// special maryo images state
	std::string special_state;
	// if holding item
	if( active_object )
	{
		special_state = "_holding";
	}

	if( maryo_type == MARYO_SMALL )
	{
		/********************* Small **************************/
		// standing
		Add_Image( pVideo->Get_Surface( "maryo/small/stand_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/stand_right" + special_state + ".png" ) );
		// walking
		Add_Image( pVideo->Get_Surface( "maryo/small/walk_left_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/walk_right_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/walk_left_2" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/walk_right_2" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/walk_left_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/walk_right_1" + special_state + ".png" ) );
		// running
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		// falling
		Add_Image( pVideo->Get_Surface( "maryo/small/fall_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/fall_right" + special_state + ".png" ) );
		// jumping
		Add_Image( pVideo->Get_Surface( "maryo/small/jump_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/jump_right" + special_state + ".png" ) );
		// dead
		Add_Image( pVideo->Get_Surface( "maryo/small/dead_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/dead_right.png" ) );
		// ducked
		Add_Image( pVideo->Get_Surface( "maryo/small/duck_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/duck_right.png" ) );
		// climbing
		Add_Image( pVideo->Get_Surface( "maryo/small/climb_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/climb_right.png" ) );
		/****************************************************/
	}
	else if( maryo_type == MARYO_BIG )
	{
		/********************* Big ****************************/
		// standing
		Add_Image( pVideo->Get_Surface( "maryo/big/stand_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/big/stand_right" + special_state + ".png" ) );
		// walking
		Add_Image( pVideo->Get_Surface( "maryo/big/walk_left_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/big/walk_right_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/big/walk_left_2" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/big/walk_right_2" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/big/walk_left_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/big/walk_right_1" + special_state + ".png" ) );
		// running
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		// falling
		Add_Image( pVideo->Get_Surface( "maryo/big/fall_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/big/fall_right" + special_state + ".png" ) );
		// jumping
		Add_Image( pVideo->Get_Surface( "maryo/big/jump_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/big/jump_right" + special_state + ".png" ) );
		// dead
		Add_Image( pVideo->Get_Surface( "maryo/small/dead_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/dead_right.png" ) );
		// ducked
		Add_Image( pVideo->Get_Surface( "maryo/big/duck_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/big/duck_right.png" ) );
		// climbing
		Add_Image( pVideo->Get_Surface( "maryo/big/climb_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/big/climb_right.png" ) );
		// throwing
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		/****************************************************/
	}
	else if( maryo_type == MARYO_FIRE )
	{
		/********************* Fire **************************/
		// standing
		Add_Image( pVideo->Get_Surface( "maryo/fire/stand_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/fire/stand_right" + special_state + ".png" ) );
		// walking
		Add_Image( pVideo->Get_Surface( "maryo/fire/walk_left_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/fire/walk_right_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/fire/walk_left_2" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/fire/walk_right_2" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/fire/walk_left_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/fire/walk_right_1" + special_state + ".png" ) );
		// running
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		// falling
		Add_Image( pVideo->Get_Surface( "maryo/fire/fall_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/fire/fall_right" + special_state + ".png" ) );
		// jumping
		Add_Image( pVideo->Get_Surface( "maryo/fire/jump_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/fire/fall_right" + special_state + ".png" ) );
		// dead
		Add_Image( pVideo->Get_Surface( "maryo/small/dead_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/dead_right.png" ) );
		// ducked
		Add_Image( pVideo->Get_Surface( "maryo/fire/duck_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/fire/duck_right.png" ) );
		// climbing
		Add_Image( pVideo->Get_Surface( "maryo/fire/climb_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/fire/climb_right.png" ) );
		// throwing
		Add_Image( pVideo->Get_Surface( "maryo/fire/throw_left_1.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/fire/throw_right_1.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/fire/throw_left_2.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/fire/throw_right_2.png" ) );
		/****************************************************/
	}
	else if( maryo_type == MARYO_ICE )
	{
		/********************* Ice **************************/
		// standing
		Add_Image( pVideo->Get_Surface( "maryo/ice/stand_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ice/stand_right" + special_state + ".png" ) );
		// walking
		Add_Image( pVideo->Get_Surface( "maryo/ice/walk_left_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ice/walk_right_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ice/walk_left_2" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ice/walk_right_2" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ice/walk_left_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ice/walk_right_1" + special_state + ".png" ) );
		// running
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		// falling
		Add_Image( pVideo->Get_Surface( "maryo/ice/fall_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ice/fall_right" + special_state + ".png" ) );
		// jumping
		Add_Image( pVideo->Get_Surface( "maryo/ice/jump_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ice/fall_right" + special_state + ".png" ) );
		// dead
		Add_Image( pVideo->Get_Surface( "maryo/small/dead_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/dead_right.png" ) );
		// ducked
		Add_Image( pVideo->Get_Surface( "maryo/ice/duck_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ice/duck_right.png" ) );
		// climbing
		Add_Image( pVideo->Get_Surface( "maryo/ice/climb_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ice/climb_right.png" ) );
		// throwing
		Add_Image( pVideo->Get_Surface( "maryo/ice/throw_left_1.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ice/throw_right_1.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ice/throw_left_2.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ice/throw_right_2.png" ) );
		/****************************************************/
	}
	else if( maryo_type == MARYO_CAPE )
	{
		/********************* Cape **************************/
		// standing
		Add_Image( pVideo->Get_Surface( "maryo/flying/left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/right" + special_state + ".png" ) );
		// walking
		Add_Image( pVideo->Get_Surface( "maryo/flying/walk_left_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/walk_right_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/walk_left_2" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/walk_right_2" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/walk_left_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/walk_right_1" + special_state + ".png" ) );
		// running
		Add_Image( pVideo->Get_Surface( "maryo/flying/run_left_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/run_right_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/run_left_2" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/run_right_2" + special_state + ".png" ) );
		// falling
		Add_Image( pVideo->Get_Surface( "maryo/flying/fall_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/fall_right" + special_state + ".png" ) );
		// jumping
		Add_Image( pVideo->Get_Surface( "maryo/flying/jump_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/fall_right" + special_state + ".png" ) );
		// dead
		Add_Image( pVideo->Get_Surface( "maryo/small/dead_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/dead_right.png" ) );
		// ducked
		Add_Image( pVideo->Get_Surface( "maryo/flying/duck_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/duck_right.png" ) );
		// climbing
		Add_Image( pVideo->Get_Surface( "maryo/flying/climb_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/climb_right.png" ) );
		// throwing
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		// flying
		Add_Image( pVideo->Get_Surface( "maryo/flying/fly_left_1.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/fly_right_1.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/fly_left_2.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/fly_right_2.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/fly_left_3.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/fly_right_3.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/fly_left_4.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/fly_right_4.png" ) );
		// slow fall/parachute
		Add_Image( pVideo->Get_Surface( "maryo/flying/slow_fall_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/flying/slow_fall_right.png" ) );
		/****************************************************/
	}
	else if( maryo_type == MARYO_GHOST )
	{
		/********************* Ghost **************************/
		// standing
		Add_Image( pVideo->Get_Surface( "maryo/ghost/stand_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ghost/stand_right" + special_state + ".png" ) );
		// walking
		Add_Image( pVideo->Get_Surface( "maryo/ghost/walk_left_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ghost/walk_right_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ghost/walk_left_2" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ghost/walk_right_2" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ghost/walk_left_1" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ghost/walk_right_1" + special_state + ".png" ) );
		// running
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		// falling
		Add_Image( pVideo->Get_Surface( "maryo/ghost/fall_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ghost/fall_right" + special_state + ".png" ) );
		// jumping
		Add_Image( pVideo->Get_Surface( "maryo/ghost/jump_left" + special_state + ".png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ghost/fall_right" + special_state + ".png" ) );
		// dead
		Add_Image( pVideo->Get_Surface( "maryo/small/dead_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/small/dead_right.png" ) );
		// ducked
		Add_Image( pVideo->Get_Surface( "maryo/ghost/duck_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ghost/duck_right.png" ) );
		// climbing
		Add_Image( pVideo->Get_Surface( "maryo/ghost/climb_left.png" ) );
		Add_Image( pVideo->Get_Surface( "maryo/ghost/climb_right.png" ) );
		// throwing
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		Add_Image( NULL );
		/****************************************************/
	}

	// set image
	Set_Image_Num( Get_Image() + m_direction );
}

void cPlayer :: Get_Item( SpriteType item_type, bool force /* = 0 */, cMovingSprite *base /* = NULL */ )
{
	Maryo_type current_maryo_type;
	bool use_temp_power;

	// use original type
	if( maryo_type == MARYO_GHOST )
	{
		current_maryo_type = maryo_type_temp_power;
		use_temp_power = 1;
	}
	// already using original type
	else
	{
		current_maryo_type = maryo_type;
		use_temp_power = 0;
	}

	// Default Mushroom
	if( item_type == TYPE_MUSHROOM_DEFAULT ) 
	{
		pHud_LifeDisplay->IncreaseLife(1);
		life_bar++;
	
		if( ( current_maryo_type == MARYO_SMALL || force ) && Change_Size( -5, -12, 1 ) ) 
		{
			// change to big
			if( current_maryo_type == MARYO_SMALL )
			{
				// set type
				Set_Type( MARYO_BIG, 1, 1, use_temp_power );
			}
			// move item to itembox
			else if( current_maryo_type == MARYO_BIG )
			{
				// item to itembox
				pHud_Itembox->Set_Item( TYPE_MUSHROOM_DEFAULT );
			}
			// change to big
			else if( current_maryo_type == MARYO_FIRE )
			{
				// set type
				Set_Type( MARYO_BIG, 0, 1, use_temp_power );
				// old item to itembox
				pHud_Itembox->Set_Item( TYPE_FIREPLANT );
			}
		}
		// move item to itembox
		else
		{
			pHud_Itembox->Set_Item( TYPE_MUSHROOM_DEFAULT );
		}
	}
	// Fireplant
	else if( item_type == TYPE_FIREPLANT ) 
	{
		if( ( ( current_maryo_type == MARYO_SMALL || force ) && Change_Size( -5, -12, 1 ) ) || 
			( current_maryo_type == MARYO_BIG || current_maryo_type == MARYO_ICE || force ) )
		{
			// move item to itembox
			if( current_maryo_type == MARYO_FIRE ) 
			{
				// move item to itembox
				pHud_Itembox->Set_Item( TYPE_FIREPLANT );
			}
			// change to fire
			else
			{
				// set type
				Set_Type( MARYO_FIRE, 1, 1, use_temp_power );
			}
		}
		// fire explosion
		else if( current_maryo_type == MARYO_FIRE && pHud_Itembox->m_item_id == TYPE_FIREPLANT )
		{
			unsigned int ball_amount = 10;

			// if star add another ball
			if( invincible_star )
			{
				ball_amount = 20;
			}

			Ball_Add( FIREBALL_EXPLOSION, 180, ball_amount );
			pHud_Points->Add_Points( 1000, m_pos_x + ( m_col_rect.m_w / 2 ), m_pos_y + 2 );	
		}
		// move item to itembox
		else
		{
			pHud_Itembox->Set_Item( TYPE_FIREPLANT );
		}
	}
	// Blue Mushroom
	else if( item_type == TYPE_MUSHROOM_BLUE ) 
	{
		if( ( ( current_maryo_type == MARYO_SMALL || force ) && Change_Size( -5, -20, 1 ) ) || 
			( ( current_maryo_type == MARYO_BIG || current_maryo_type == MARYO_FIRE || force ) ) )
		{
			// move item to itembox
			if( current_maryo_type == MARYO_ICE ) 
			{
				// move item to itembox
				pHud_Itembox->Set_Item( TYPE_MUSHROOM_BLUE );
			}
			// change to ice
			else
			{
				// set type
				Set_Type( MARYO_ICE, 1, 1, use_temp_power );
			}
		}
		// ice explosion
		else if( current_maryo_type == MARYO_ICE && pHud_Itembox->m_item_id == TYPE_MUSHROOM_BLUE )
		{
			unsigned int ball_amount = 10;

			// if star add another ball
			if( invincible_star )
			{
				ball_amount = 20;
			}

			Ball_Add( ICEBALL_EXPLOSION, 180, ball_amount );
			pHud_Points->Add_Points( 1000, m_pos_x + ( m_col_rect.m_w / 2 ), m_pos_y + 2 );	
		}
		// move item to itembox
		else
		{
			pHud_Itembox->Set_Item( TYPE_MUSHROOM_BLUE );
		}
	}
	// Ghost Mushroom
	else if( item_type == TYPE_MUSHROOM_GHOST ) 
	{
		if( ( ( maryo_type == MARYO_SMALL || force ) && Change_Size( -5, -20, 1 ) ) || 
			maryo_type != MARYO_SMALL )
		{
			// set back ghost time
			if( maryo_type == MARYO_GHOST )
			{
				ghost_time = speedfactor_fps * 10;
				ghost_time_mod = 0;
			}
			// change to ghost
			else
			{
				// set type
				Set_Type( MARYO_GHOST, 1, 1 );
			}
		}
		// move item to itembox
		else
		{
			pHud_Itembox->Set_Item( TYPE_MUSHROOM_GHOST );
		}
	}
	// Mushroom 1-UP
	else if( item_type == TYPE_MUSHROOM_LIVE_1 ) 
	{
		pAudio->Play_Sound( "item/live_up.ogg", RID_1UP_MUSHROOM );
		pHud_Lives->Add_Lives( 1 );
	}
	// Mushroom Poison
	else if( item_type == TYPE_MUSHROOM_POISON ) 
	{
		DownGrade_Player();
	}
	// Moon
	else if( item_type == TYPE_MOON ) 
	{
		pAudio->Play_Sound( "item/moon.ogg", RID_MOON );
		pHud_Lives->Add_Lives( 3 );
	}
	// Jumping Star
	else if( item_type == TYPE_JSTAR ) 
	{
		// todo : check if music is already playing
		pAudio->Play_Music( "game/star.ogg", 0, 1, 500 );
		pAudio->Play_Music( pActive_Level->m_musicfile, -1, 0 );
		pHud_Points->Add_Points( 1000, m_pos_x + ( m_col_rect.m_w / 2 ), m_pos_y + 2 );
		invincible = speedfactor_fps * 16.0f;
		invincible_star = speedfactor_fps * 15.0f;
	}
	// Turtle Shell
	else if( item_type == TYPE_TURTLE ) 
	{
		pAudio->Play_Sound( "player/pickup_item.wav" );

		active_object = base;
		active_object->m_massive_type = MASS_PASSIVE;
		active_object->m_state = STA_OBJ_LINKED;
		active_object->Update_Valid_Update();
		active_object->Reset_On_Ground();
		active_object->m_velx = 0;
		active_object->m_vely = 0;
		
		
		cTurtle *turtle = static_cast<cTurtle *>(active_object);
		// clear the standing counter
		turtle->m_counter = 0.0f;
		// clear player counter
		turtle->m_player_counter = 0.0f;
		turtle->Set_Image_Num( 5 );

		// load holding images
		Load_Images();
	}
}

float cPlayer :: Get_Vel_Modifier( void ) const
{
	float vel_mod = 1;

	// if running key is pressed or always run
	if( pPreferences->m_always_run || pKeyboard->keys[pPreferences->m_key_action] || pJoystick->Button( pPreferences->m_joy_button_action ) )
	{
		vel_mod = 1.5f;
	}

	if( invincible_star > 0 )
	{
		vel_mod *= 1.2f;
	}

	if( m_state == STA_RUN )
	{
		vel_mod *= 1.2f;
	}

	return vel_mod;
}

void cPlayer :: Action_Jump( bool enemy_jump /* = 0 */ )
{
	if( ducked )
	{
		// power jump
		if( SDL_GetTicks() - ducked > power_jump_delta )
		{
			force_jump = 1;
			next_jump_power += 2;
			next_jump_accel += 0.2f;
		}
		// stop ducking after setting power jump
		Stop_Ducking();
	}

	// enemy jump
	if( enemy_jump )
	{
		force_jump = 1;
		next_jump_sound = 0;
		next_jump_power += 1;
		next_jump_accel += 0.1f;
	}

	// start keytime
	Start_Jump_Keytime();
	// check if starting a jump is possible
	Update_Jump_Keytime();
}

void cPlayer :: Action_Interact( input_identifier key_type )
{
	// Up
	if( key_type == INP_UP )
	{
		// Search for colliding level exit Objects
		for( cSprite_List::iterator itr = pActive_Sprite_Manager->objects.begin(), itr_end = pActive_Sprite_Manager->objects.end(); itr != itr_end; ++itr )
		{
			cSprite *obj = (*itr);

			// skip destroyed objects
			if( obj->m_auto_destroy )
			{
				continue;
			}

			if( !m_col_rect.Intersects( obj->m_col_rect ) )
			{
				continue;
			}
			
			// level exit
			if( obj->m_type == TYPE_LEVEL_EXIT )
			{
				cLevel_Exit *level_exit = static_cast<cLevel_Exit *>(obj);

				// beam
				if( level_exit->exit_type == LEVEL_EXIT_BEAM )
				{
					// needs to be on ground
					if( m_ground_object )
					{
						Game_Action = GA_ACTIVATE_LEVEL_EXIT;
						Game_Action_ptr = level_exit;
						return;
					}
				}
				// warp
				else if( level_exit->exit_type == LEVEL_EXIT_WARP )
				{
					if( level_exit->m_direction == DIR_UP )
					{
						if( m_vely <= 0 )
						{
							Game_Action = GA_ACTIVATE_LEVEL_EXIT;
							Game_Action_ptr = level_exit;
							return;
						}
					}
				}
			}
			// climbable
			else if( obj->m_massive_type == MASS_CLIMBABLE )
			{
				Start_Climbing();
			}
		}
	}
	// Down
	else if( key_type == INP_DOWN )
	{
		// Search for colliding Levelexit Objects
		for( cSprite_List::iterator itr = pActive_Sprite_Manager->objects.begin(), itr_end = pActive_Sprite_Manager->objects.end(); itr != itr_end; ++itr )
		{
			cSprite *obj = (*itr);

			// skip destroyed objects
			if( obj->m_auto_destroy )
			{
				continue;
			}

			if( !m_col_rect.Intersects( obj->m_col_rect ) )
			{
				continue;
			}

			// level exit
			if( obj->m_type == TYPE_LEVEL_EXIT )
			{
				cLevel_Exit *level_exit = static_cast<cLevel_Exit *>(obj);

				// warp
				if( level_exit->exit_type == LEVEL_EXIT_WARP )
				{
					if( level_exit->m_direction == DIR_DOWN )
					{
						// needs to be on ground
						if( m_ground_object )
						{
							Game_Action = GA_ACTIVATE_LEVEL_EXIT;
							Game_Action_ptr = level_exit;
							return;
						}
					}
				}
			}
		}

		// ducking / falling
		if( m_state != STA_FLY )
		{
			if( m_ground_object )
			{
				if( m_ground_object->m_massive_type == MASS_MASSIVE )
				{
					Start_Ducking();
				}
				else if( m_ground_object->m_massive_type == MASS_HALFMASSIVE )
				{
					Start_Falling();
				}
			}
		}
	}
	// Left
	else if( key_type == INP_LEFT )
	{
		// Search for colliding Levelexit Objects
		for( cSprite_List::iterator itr = pActive_Sprite_Manager->objects.begin(), itr_end = pActive_Sprite_Manager->objects.end(); itr != itr_end; ++itr )
		{
			cSprite *obj = (*itr);

			// skip destroyed objects
			if( obj->m_auto_destroy )
			{
				continue;
			}

			if( !m_col_rect.Intersects( obj->m_col_rect ) )
			{
				continue;
			}

			// level exit
			if( obj->m_type == TYPE_LEVEL_EXIT )
			{
				cLevel_Exit *level_exit = static_cast<cLevel_Exit *>(obj);

				// warp
				if( level_exit->exit_type == LEVEL_EXIT_WARP )
				{
					if( level_exit->m_direction == DIR_LEFT )
					{
						if( m_velx >= 0 )
						{
							Game_Action = GA_ACTIVATE_LEVEL_EXIT;
							Game_Action_ptr = level_exit;
							return;
						}
					}
				}
			}
		}

		// direction
		if( m_state != STA_FLY )
		{
			if( m_direction != DIR_LEFT )
			{
				// play stop sound if already running
				if( m_velx > 12 && m_ground_object )
				{
					pAudio->Play_Sound( "player/maryo_stop.ogg", RID_MARYO_STOP );
				}

				m_direction = DIR_LEFT;
			}
		}
	}
	// Right
	else if( key_type == INP_RIGHT )
	{
		// Search for colliding Levelexit Objects
		for( cSprite_List::iterator itr = pActive_Sprite_Manager->objects.begin(), itr_end = pActive_Sprite_Manager->objects.end(); itr != itr_end; ++itr )
		{
			cSprite *obj = (*itr);

			// skip destroyed objects
			if( obj->m_auto_destroy )
			{
				continue;
			}

			if( !m_col_rect.Intersects( obj->m_col_rect ) )
			{
				continue;
			}

			// level exit
			if( obj->m_type == TYPE_LEVEL_EXIT )
			{
				cLevel_Exit *level_exit = static_cast<cLevel_Exit *>(obj);

				// warp
				if( level_exit->exit_type == LEVEL_EXIT_WARP )
				{
					if( level_exit->m_direction == DIR_RIGHT )
					{
						if( m_velx <= 0 )
						{
							Game_Action = GA_ACTIVATE_LEVEL_EXIT;
							Game_Action_ptr = level_exit;
							return;
						}
					}
				}
			}
		}

		// direction
		if( m_state != STA_FLY )
		{
			if( m_direction != DIR_RIGHT )
			{
				// play stop sound if already running
				if( m_velx < -12 && m_ground_object )
				{
					pAudio->Play_Sound( "player/maryo_stop.ogg", RID_MARYO_STOP );
				}

				m_direction = DIR_RIGHT;
			}
		}
	}
	// Shoot
	else if( key_type == INP_SHOOT )
	{
		Action_Shoot();
	}
	// Jump
	else if( key_type == INP_JUMP )
	{
		Action_Jump();
	}
	// Request Item
	else if( key_type == INP_ITEM )
	{
		pHud_Itembox->Request_Item();
	}
	// Exit
	else if( key_type == INP_EXIT )
	{
		Game_Action = GA_ENTER_MENU;

		if( Game_Mode_Type == MODE_TYPE_LEVEL_CUSTOM )
		{
			Game_Action_Data.add( "level", Trim_Filename( pActive_Level->m_level_filename, 0, 0 ) );
		}
	}
}

void cPlayer :: Action_Shoot( void )
{
	// add fire or ice-ball
	ball_effect ball_type = FIREBALL_DEFAULT;
	// if ice maryo
	if( maryo_type == MARYO_ICE || maryo_type_temp_power == MARYO_ICE )
	{
		ball_type = ICEBALL_DEFAULT;
	}

	if (num_shots < 4)
	{
		num_shots++;
	}
	else if (num_shots > 3)
	{
		num_shots = 0;
	}
	else
	{
		num_shots = 0;
	}
	

	unsigned int ball_amount = num_shots;

	// if star add another ball
	if( invincible_star )
	{
		ball_amount = num_shots * 2;
	}

	// if added ball
	if( Ball_Add( ball_type, -1, ball_amount ) )
	{
		throwing_counter = speedfactor_fps * 0.3f;
	}
}

void cPlayer :: Action_Stop_Jump( void )
{
	Stop_Flying();
	UpKeytime = 0;
}

void cPlayer :: Action_Stop_Interact( input_identifier key_type )
{
	// Action
	if( key_type == INP_ACTION )
	{
		Release_Item();
	}
	// Down
	else if( key_type == INP_DOWN )
	{
		Stop_Ducking();
	}
	// Left
	else if( key_type == INP_LEFT )
	{
		// if key in opposite direction is still pressed only change direction
		if( pKeyboard->keys[pPreferences->m_key_right] || pJoystick->right )
		{
			m_direction = DIR_RIGHT;
		}
		else
		{
			Hold();
		}
	}
	// Right
	else if( key_type == INP_RIGHT )
	{
		// if key in opposite direction is still pressed only change direction
		if( pKeyboard->keys[pPreferences->m_key_left] || pJoystick->left )
		{
			m_direction = DIR_LEFT;
		}
		else
		{
			Hold();
		}
	}
	// Jump
	else if( key_type == INP_JUMP )
	{
		Action_Stop_Jump();
	}
	// Shoot
	else if( key_type == INP_SHOOT )
	{
		Action_Stop_Shoot();
	}
}

void cPlayer :: Action_Stop_Shoot( void )
{
	// nothing
}

bool cPlayer :: Ball_Add( ball_effect effect_type /* = FIREBALL_DEFAULT */, float ball_start_angle /* = -1 */, unsigned int amount /* = 0 */ ) const
{
	//if( ( maryo_type != MARYO_FIRE && maryo_type_temp_power != MARYO_FIRE && maryo_type != MARYO_ICE && maryo_type_temp_power != MARYO_ICE && maryo_type != MARYO_SMALL) || ducked )
	if (ducked)
	{
		return 0;
	}

	if( amount == 0 )
	{
		amount = 1;
	}

	// default fire/ice-ball
	if( effect_type == FIREBALL_DEFAULT || effect_type == ICEBALL_DEFAULT || effect_type == SMOKEBALL_DEFAULT )
	{
		// if time not passed between last shot
		if( ( effect_type == FIREBALL_DEFAULT || effect_type == ICEBALL_DEFAULT || effect_type == SMOKEBALL_DEFAULT ) && shoot_counter > 0 )
		{
			return 0;
		}

		// position
		float ball_posx = m_pos_x;
		// horizontal speed
		float ball_vel_x = 17;

		if( effect_type == ICEBALL_DEFAULT )
		{
			// iceball has slow horizontal speed
			ball_vel_x = 12;

			// sound
			pAudio->Play_Sound( "item/iceball.wav", RID_MARYO_BALL );
		}
		else if (effect_type == SMOKEBALL_DEFAULT )
		{
			// sound
			pAudio->Play_Sound( "item/fireball.ogg", RID_MARYO_BALL );
		}
		// fireball
		else
		{
			// sound
			pAudio->Play_Sound( "item/fireball.ogg", RID_MARYO_BALL );
		}

		if( m_direction == DIR_LEFT )
		{
			ball_posx += 8;
			// change velocity direction
			ball_vel_x *= -1;
		}
		else
		{
			ball_posx += 20;
		}

		for( unsigned int i = 0; i < amount; i++ )
		{
			// add ball
			cBall *ball = new cBall( ball_posx, m_rect.m_y + (m_rect.m_h * 0.7f) - 20, this, effect_type );
			pActive_Sprite_Manager->Add( ball );

			// set speed
			if( ball_start_angle >= 0 )
			{
				ball->Set_Direction( ball_start_angle, ball_vel_x );
				ball_start_angle += 20;
			}
			else
			{
				ball->Set_Velocity( ball_vel_x, -2.0f - static_cast<float>( i * 4 ) );
			}
		}
	}
	// half circle fire/ice-ball explosion
	else if( effect_type == FIREBALL_EXPLOSION || effect_type == ICEBALL_EXPLOSION )
	{
		// start angle
		float ball_angle = 180;

		if( ball_start_angle >= 0 )
		{
			ball_angle = ball_start_angle;
		}

		for( unsigned int i = 0; i < amount; i++ )
		{
			// 10 * 18 = 180 degrees
			ball_angle += 180.0f / amount;

			// add ball
			cBall *ball = new cBall( m_pos_x + m_col_rect.m_w / 2, m_pos_y + m_col_rect.m_h / 2, this, effect_type );
			ball->Set_Direction( ball_angle, 15 );
			ball->Col_Move( ball->m_velx * 2, ball->m_vely * 2, 1 );
			pActive_Sprite_Manager->Add( ball );
		}

		// explosion animation and sound
		if( effect_type == FIREBALL_EXPLOSION )
		{
			cAnimation_Fireball *anim = new cAnimation_Fireball( m_pos_x + ( m_col_rect.m_w / 2 ), m_pos_y + ( m_col_rect.m_h / 3 ), 10 );
			anim->Set_Fading_Speed( 0.3f );
			pAnimation_Manager->Add( anim );

			pAudio->Play_Sound( "item/fireball_explosion.wav", RID_MARYO_BALL );
		}
		else
		{
			// create animation
			cParticle_Emitter *anim = new cParticle_Emitter();
			anim->Set_Pos( m_pos_x + (m_col_rect.m_w * 0.5f), m_pos_y + (m_col_rect.m_h * 0.5f) );
			anim->Set_Image( pVideo->Get_Surface( "animation/particles/light.png" ) );
			anim->Set_Quota( 10 );
			anim->Set_Time_to_Live( 1.5f );
			anim->Set_Pos_Z( m_pos_z + 0.0001f );
			anim->Set_Color( Color( static_cast<Uint8>(50), 50, 250 ) );
			anim->Set_Blending( BLEND_ADD );
			anim->Set_Speed( 0.8f, 0.7f );
			anim->Set_Scale( 0.4f, 0.2f );
			// add animation
			pAnimation_Manager->Add( anim );

			pAudio->Play_Sound( "item/iceball_2.ogg", RID_MARYO_BALL );
		}
	}
	// unknown type
	else
	{
		return 0;
	}

	return 1;
}

void cPlayer :: Ball_Clear( void ) const
{
	// destroy all fireballs from the player
	for( cSprite_List::iterator itr = pActive_Sprite_Manager->objects.begin(), itr_end = pActive_Sprite_Manager->objects.end(); itr != itr_end; ++itr )
	{
		cSprite *obj = (*itr);

		if( obj->m_type == TYPE_BALL )
		{
			cBall *ball = static_cast<cBall *>(obj);

			// if from player
			if( ball->m_origin_type == TYPE_PLAYER )
			{
				obj->Destroy();
			}
		}
	}
}

void cPlayer :: Add_Kill_Multiplier( void )
{
	kill_multiplier += 0.1f;

	// give lives after 10 continuous kills
	if( kill_multiplier >= 2 )
	{
		// only every fifth kill
		if( static_cast<int>( kill_multiplier * 10 ) % 5 == 0 )
		{
			// add 1 live
			pHud_Goldpieces->Add_Gold( 100 );
		}
	}

	last_kill_counter = 0;
}

void cPlayer :: Update_Kill_Multiplier( void )
{
	last_kill_counter += pFramerate->m_speed_factor;

	if( kill_multiplier > 1 )
	{
		// if longer than a second reset the multiplier
		if( last_kill_counter > speedfactor_fps )
		{
			kill_multiplier = 1;
		}
	}
}

Col_Valid_Type cPlayer :: Validate_Collision( cSprite *obj )
{
	// don't collide with anything if dead
	if( maryo_type == MARYO_DEAD )
	{
		return COL_VTYPE_NOT_VALID;
	}

	if( obj->m_massive_type == MASS_MASSIVE )
	{
		switch( obj->m_type )
		{
			case TYPE_FLYON:
			{
				if( invincible > 0.0f )
				{
					return COL_VTYPE_NOT_VALID;
				}
				
				// internal as the player could be invincible_star or flyon is frozen
				return COL_VTYPE_INTERNAL;
			}
			case TYPE_GEE:
			{
				if( invincible_star > 0.0f )
				{
					return COL_VTYPE_INTERNAL;
				}

				// block if on top
				if( m_vely >= 0.0f && Is_On_Top( obj ) )
				{
					return COL_VTYPE_BLOCKING;
				}

				if( invincible > 0.0f )
				{
					return COL_VTYPE_NOT_VALID;
				}

				// internal as it could be frozen
				return COL_VTYPE_INTERNAL;
			}
			// walk on spika and rokko
			case TYPE_SPIKA:
			case TYPE_ROKKO:
			{
				// block if on top
				if( m_vely >= 0.0f && Is_On_Top( obj ) )
				{
					return COL_VTYPE_BLOCKING;
				}

				if( invincible_star > 0 )
				{
					return COL_VTYPE_INTERNAL;
				}

				return COL_VTYPE_NOT_VALID;
			}
			case TYPE_EATO:
			{
				// block if on top
				if( m_vely >= 0.0f && Is_On_Top( obj ) )
				{
					return COL_VTYPE_BLOCKING;
				}

				if( invincible_star > 0.0f )
				{
					return COL_VTYPE_INTERNAL;
				}

				return COL_VTYPE_BLOCKING;
			}
			case TYPE_TURTLE:
			{
				// block if on top
				if( m_vely >= 0.0f && Is_On_Top( obj ) )
				{
					return COL_VTYPE_BLOCKING;
				}
				// if not on top
				else
				{
					cTurtle *turtle = static_cast<cTurtle *>(obj);

					// if player counter active
					if( turtle->m_player_counter > 0.0f )
					{
						// let shell slide through us
						return COL_VTYPE_NOT_VALID;
					}
				}

				if( invincible_star > 0.0f )
				{
					return COL_VTYPE_INTERNAL;
				}

				return COL_VTYPE_BLOCKING;
			}
			case TYPE_BALL:
			{
				cBall *ball = static_cast<cBall *>(obj);

				if( ball->m_origin_type != TYPE_PLAYER )
				{
					return COL_VTYPE_INTERNAL;
				}

				return COL_VTYPE_NOT_VALID;
			}
			case TYPE_BONUSBOX:
			case TYPE_SPINBOX:
			{
				cBaseBox *box = static_cast<cBaseBox *>(obj);

				// ghost
				if( box->box_invisible == BOX_GHOST )
				{
					// maryo is not ghost
					if( maryo_type != MARYO_GHOST )
					{
						return COL_VTYPE_NOT_VALID;
					}
				}

				return COL_VTYPE_BLOCKING;
			}
			default:
			{
				break;
			}
		}

		// temporarily disabled
		// don't collide with ground
		/*if( m_ground_object == obj )
		{
			return COL_VTYPE_NOT_VALID;
		}*/

		return COL_VTYPE_BLOCKING;
	}
	else if( obj->m_massive_type == MASS_HALFMASSIVE )
	{
		// fall through
		if( pKeyboard->keys[pPreferences->m_key_down] )
		{
			return COL_VTYPE_NOT_VALID;
		}

		// if moving downwards and the object is on bottom
		if( m_vely >= 0.0f && Is_On_Top( obj ) )
		{
			// don't collide with ground
			if( m_ground_object == obj )
			{
				return COL_VTYPE_NOT_VALID;
			}

			return COL_VTYPE_BLOCKING;
		}

		return COL_VTYPE_NOT_VALID;
	}
	// allow climbing
	else if( obj->m_massive_type == MASS_CLIMBABLE )
	{
		// if flying
		if( m_state == STA_FLY )
		{
			return COL_VTYPE_NOT_VALID;
		}
		
		return COL_VTYPE_INTERNAL;
	}
	else if( obj->m_massive_type == MASS_PASSIVE )
	{
		switch( obj->m_type )
		{
			case TYPE_BONUSBOX:
			case TYPE_SPINBOX:
			case TYPE_TEXT_BOX:
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

				return COL_VTYPE_NOT_VALID;
			}
			case TYPE_LEVEL_EXIT:
			{
				cLevel_Exit *levelexit = static_cast<cLevel_Exit *>(obj);

				// warp levelexit key check
				if( levelexit->exit_type == LEVEL_EXIT_WARP )
				{
					// joystick events are sent as keyboard keys
					if( pKeyboard->keys[pPreferences->m_key_up] )
					{
						if( levelexit->m_start_direction == DIR_UP )
						{
							Action_Interact( INP_UP );
						}
					}
					else if( pKeyboard->keys[pPreferences->m_key_down] )
					{
						if( levelexit->m_start_direction == DIR_DOWN )
						{
							Action_Interact( INP_DOWN );
						}
					}
					else if( pKeyboard->keys[pPreferences->m_key_right] )
					{
						if( levelexit->m_start_direction == DIR_RIGHT )
						{
							Action_Interact( INP_RIGHT );
						}
					}
					else if( pKeyboard->keys[pPreferences->m_key_left] )
					{
						if( levelexit->m_start_direction == DIR_LEFT )
						{
							Action_Interact( INP_LEFT );
						}
					}
				}

				return COL_VTYPE_NOT_VALID;
			}
			case TYPE_FIREPLANT:
			case TYPE_GOLDPIECE:
			case TYPE_MOON:
			{
				if( !obj->m_active )
				{
					return COL_VTYPE_NOT_VALID;
				}

				return COL_VTYPE_INTERNAL;
			}
			default:
			{
				break;
			}
		}
	}

	return COL_VTYPE_NOT_VALID;
}

void cPlayer :: Handle_Collision_Enemy( cObjectCollision *collision )
{
	// if invalid
	if( collision->direction == DIR_UNDEFINED || maryo_type == MARYO_DEAD || !m_active )
	{
		return;
	}

	cEnemy *enemy = static_cast<cEnemy *>(pActive_Sprite_Manager->Get_Pointer( collision->number ));

	// if enemy already dead
	if( enemy->m_dead )
	{
		return;
	}

	// if invincible
	if( invincible_star > 0.0f )
	{
		bool hit_enemy = 1;

		// spika and eato ignore top collisions
		if( ( enemy->m_type == TYPE_SPIKA || enemy->m_type == TYPE_EATO || enemy->m_type == TYPE_THROMP ) && collision->direction == DIR_DOWN )
		{
			hit_enemy = 0;
		}
		// rokko
		else if( enemy->m_type == TYPE_ROKKO )
		{
			hit_enemy = 0;
		}
		// turtle boss
		else if( enemy->m_type == TYPE_TURTLE_BOSS )
		{
			hit_enemy = 0;
		}
		// furball boss
		else if( enemy->m_type == TYPE_FURBALL_BOSS )
		{
			hit_enemy = 0;
		}

		// hit
		if( hit_enemy )
		{
			pAudio->Play_Sound( "item/star_kill.ogg" );
			pHud_Points->Add_Points( static_cast<unsigned int>( enemy->m_kill_points * 1.2f ), m_pos_x, m_pos_y, "", yellow, 1 );
			// force complete downgrade
			enemy->DownGrade( 1 );
			Add_Kill_Multiplier();
			return;
		}
		// no hit
		else
		{
			// eato blocks
			if( enemy->m_type == TYPE_EATO )
			{
				if( collision->direction == DIR_LEFT )
				{
					m_velx = 0;
				}
				else if( collision->direction == DIR_RIGHT )
				{
					m_velx = 0;
				}
			}
		}
	}

	// enemy is frozen
	if( enemy->m_freeze_counter )
	{
		// enemy rect particle ice animation
		for( unsigned int w = 0; w < enemy->m_col_rect.m_w; w += 10 )
		{
			for( unsigned int h = 0; h < enemy->m_col_rect.m_h; h += 10 )
			{
				// animation
				cParticle_Emitter *anim = new cParticle_Emitter();
				anim->Set_Pos( enemy->m_pos_x + w, enemy->m_pos_y + h );
				anim->Set_Image( pVideo->Get_Surface( "animation/particles/light.png" ) );
				anim->Set_Time_to_Live( 0.6f, 0.4f );
				anim->Set_Color( Color( static_cast<Uint8>(160), 160, 240 ), Color( static_cast<Uint8>( rand() % 80 ), rand() % 80, rand() % 10, 0 ) );
				anim->Set_Fading_Alpha( 1 );
				anim->Set_Fading_Size( 1 );
				anim->Set_Speed( 0.5f, 0.2f );
				anim->Set_Blending( BLEND_DRIVE );
				// add animation
				pAnimation_Manager->Add( anim );
			}
		}

		// ice sound
		pAudio->Play_Sound( "item/ice_kill.wav" );

		// get points
		pHud_Points->Add_Points( enemy->m_kill_points, enemy->m_pos_x, enemy->m_pos_y, "", static_cast<Uint8>(255), 1 );
		
		// kill enemy
		enemy->DownGrade( 1 );
		Add_Kill_Multiplier();

		// slow down
		m_velx *= 0.7f;
		m_vely *= 0.7f;

		// if enemy was ground object
		if( enemy == m_ground_object )
		{
			Reset_On_Ground();
		}

		return;
	}

	if( collision->direction == DIR_TOP )
	{
		m_vely = 0;

		if( m_state != STA_FLY )
		{
			if( m_state != STA_CLIMB )
			{
				Set_Moving_State( STA_FALL );
				UpKeytime = 0;
				jump_power = 0;
			}
		}
	}

	// send collision
	Send_Collision( collision );
}

void cPlayer :: Handle_Collision_Massive( cObjectCollision *collision )
{
	// if invalid
	if( collision->direction == DIR_UNDEFINED )
	{
		return;
	}

	/* fixme: the collision direction is sometimes wrong as left/right if landing on a moving platform which moves upwards
	* this seems to cause maryo to hang or to get stuck in it
	* easily reproducible with a low framerate and fast moving platform
	*/
	//printf( "direction is %s\n", Get_Direction_Name( collision->direction ).c_str() );

	cSprite *col_obj = pActive_Sprite_Manager->Get_Pointer( collision->number );

	// climbable
	if( col_obj->m_massive_type == MASS_CLIMBABLE && m_state != STA_CLIMB && m_state != STA_FLY )
	{
		// if not climbing and player wants to climb
		if( pKeyboard->keys[pPreferences->m_key_up] || pJoystick->up || ( ( pKeyboard->keys[pPreferences->m_key_down] || pJoystick->down ) && !m_ground_object ) )
		{
			// start climbing
			Start_Climbing();
		}

		return;
	}

	// ignore ball
	if( col_obj->m_type == TYPE_BALL )
	{
		return;
	}

	// send back if item
	if( col_obj->m_type == TYPE_FIREPLANT || col_obj->m_type == TYPE_GOLDPIECE || col_obj->m_type == TYPE_MOON )
	{
		Send_Collision( collision );
		return;
	}

	if( collision->direction == DIR_UP )
	{
		m_vely = 0;

		if( m_state != STA_FLY )
		{
			if( m_state != STA_CLIMB )
			{
				Set_Moving_State( STA_FALL );

				cMovingSprite *moving_object = dynamic_cast<cMovingSprite *>(col_obj);

				// if valid moving sprite
				if( moving_object )
				{
					// add its velocity
					if( moving_object->m_vely > 0 )
					{
						m_vely += moving_object->m_vely;
					}
				}

				if( collision->m_array == ARRAY_MASSIVE )
				{
					pAudio->Play_Sound( "wall_hit.wav", RID_MARYO_WALL_HIT );

					// create animation
					cParticle_Emitter *anim = new cParticle_Emitter();

					anim->Set_Emitter_Rect( m_col_rect.m_x, m_col_rect.m_y + 6, m_col_rect.m_w );
					anim->Set_Image( pVideo->Get_Surface( "animation/particles/light.png" ) );
					anim->Set_Quota( 4 );
					anim->Set_Pos_Z( m_pos_z - 0.000001f );
					anim->Set_Time_to_Live( 0.3f );
					anim->Set_Color( Color( static_cast<Uint8>(150), 150, 150, 200 ), Color( static_cast<Uint8>( rand() % 55 ), rand() % 55, rand() % 55, 0 ) );
					anim->Set_Speed( 2, 0.6f );
					anim->Set_Scale( 0.6f );
					anim->Set_Direction_Range( 0, 180 );
					anim->Set_Fading_Alpha( 1 );
					anim->Set_Fading_Size( 1 );
					
					// add animation
					pAnimation_Manager->Add( anim );
				}
			}
		}
		// flying
		else
		{
			m_velx -= m_velx * 0.05f * pFramerate->m_speed_factor;

			// too slow
			if( m_velx > -5 && m_velx < 5 )
			{
				Stop_Flying();
			}
		}
	}
	else if( collision->direction == DIR_DOWN )
	{
		// flying
		if( m_state == STA_FLY )
		{
			m_vely = 0;
			m_velx -= m_velx * 0.05f * pFramerate->m_speed_factor;

			// too slow
			if( m_velx > -5 && m_velx < 5 )
			{
				Stop_Flying();
			}
		}
		// not flying
		else
		{
			// jumping
			if( m_state == STA_JUMP )
			{
				/* hack : only pick up after some time of jumping because moving objects collide move before the player
				 * also see cMovingSprite :: Validate_Collision_Object_On_Top
				*/
				if( jump_power < 6.0f )
				{
					m_vely = 0;
				}
			}
			// all other states
			else
			{
				m_vely = 0;
			}
		}
	}
	else if( collision->direction == DIR_LEFT )
	{
		m_velx = 0;
		m_vely -= m_vely * 0.01f * pFramerate->m_speed_factor;

		if( m_state == STA_WALK || m_state == STA_RUN )
		{
			walk_count = 0;
		}
		// flying
		else if( m_state == STA_FLY )
		{
			// box collision
			if( col_obj->m_type == TYPE_BONUSBOX || col_obj->m_type == TYPE_SPINBOX )
			{
				cBaseBox *box = static_cast<cBaseBox *>(col_obj);
				box->Activate_Collision( collision->direction );
			}

			Stop_Flying();
		}
	}
	else if( collision->direction == DIR_RIGHT )
	{
		m_velx = 0;
		m_vely -= m_vely * 0.01f * pFramerate->m_speed_factor;

		if( m_state == STA_WALK || m_state == STA_RUN )
		{
			walk_count = 0;
		}
		// flying
		else if( m_state == STA_FLY )
		{
			// box collision
			if( col_obj->m_type == TYPE_BONUSBOX || col_obj->m_type == TYPE_SPINBOX )
			{
				cBaseBox *box = static_cast<cBaseBox *>(col_obj);
				box->Activate_Collision( collision->direction );
			}

			Stop_Flying();
		}
	}

	if( collision->m_array == ARRAY_ACTIVE )
	{
		// send collision
		Send_Collision( collision );
	}
}

void cPlayer :: Handle_Collision_Passive( cObjectCollision *collision )
{
	// if invalid
	if( collision->direction == DIR_UNDEFINED )
	{
		return;
	}

	//cSprite *col_obj = pActive_Sprite_Manager->Get_Pointer( collision->number );
	//printf( "passive %s\n", col_obj->name.c_str() );

	// send it
	Send_Collision( collision );
}

void cPlayer :: Handle_out_of_Level( ObjectDirection dir )
{
	if( dir == DIR_LEFT )
	{
		Set_Pos_X( pActive_Camera->limit_rect.m_x - m_col_pos.m_x );
		m_velx = 0.0f;
		Stop_Flying();
	}
	else if( dir == DIR_RIGHT )
	{
		Set_Pos_X( pActive_Camera->limit_rect.m_x + pActive_Camera->limit_rect.m_w - m_col_pos.m_x - m_col_rect.m_w - 0.01f );
		m_velx = 0.0f;
		Stop_Flying();
	}
	else if( dir == DIR_TOP )
	{
		/*if( pActive_Level->limit_top_blocks )
		{
			Set_PosY( pActive_Camera->limit_rect.y + pActive_Camera->limit_rect.h + 0.01f );
			m_vely = 0.0f;
		}*/
	}
	else if( dir == DIR_BOTTOM )
	{
		if( godmode )
		{
			m_vely = -35.0f;
		}
		else
		{
			/*if( pActive_Level->limit_bottom_blocks )
			{
				Set_PosY( pActive_Camera->limit_rect.y + game_res_h - col_pos.x - col_rect.w + 0.01f );
				m_vely = 0.0f;
			}
			// falling below ground
			else
			{*/
				invincible = 0.0f;
				DownGrade_Player( 1, 1 );
			//}
		}
	}
}

void cPlayer :: Editor_Activate( void )
{
	CEGUI::WindowManager &wmgr = CEGUI::WindowManager::getSingleton();

	// direction
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox *>(wmgr.createWindow( "TaharezLook/Combobox", "editor_player_direction" ));
	Editor_Add( UTF8_("Direction"), UTF8_("Initial direction"), combobox, 100, 75 );

	combobox->addItem( new CEGUI::ListboxTextItem( "right" ) );
	combobox->addItem( new CEGUI::ListboxTextItem( "left" ) );
	combobox->setText( Get_Direction_Name( m_start_direction ) );

	combobox->subscribeEvent( CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber( &cPlayer::Editor_Direction_Select, this ) );

	// init
	Editor_Init();
}

bool cPlayer :: Editor_Direction_Select( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	CEGUI::ListboxItem *item = static_cast<CEGUI::Combobox *>( windowEventArgs.window )->getSelectedItem();

	Set_Direction( Get_Direction_Id( item->getText().c_str() ), 1 );

	return 1;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cPlayer *pPlayer = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
