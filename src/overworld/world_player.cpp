/***************************************************************************
 * ow_player.cpp  - Overworld Player class
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
 
#include "../overworld/world_player.h"
#include "../core/game_core.h"
#include "../core/framerate.h"
#include "../overworld/overworld.h"
#include "../level/level.h"
#include "../video/font.h"
#include "../audio/audio.h"
#include "../gui/menu.h"
#include "../video/renderer.h"

namespace SMC
{

/* *** *** *** *** *** *** *** *** cOverworld_Player *** *** *** *** *** *** *** *** *** */

cOverworld_Player :: cOverworld_Player( void )
: cAnimated_Sprite()
{
	m_sprite_array = ARRAY_PLAYER;
	m_type = TYPE_PLAYER;
	m_pos_z = 0.0999f;
	m_massive_type = MASS_MASSIVE;
	m_player_range = 0;
	m_name = "Maryo";

	current_waypoint = -2; // no waypoint
	line_waypoint = 0;
	current_line = -2;

	fixed_walking = 0;

	line_hor = cLine_collision();
	line_ver = cLine_collision();

	debug_current_line_last = -100;
	debug_lines_last = -100;
	debug_current_waypoint_last = -100;

	debug_lines = new cHudSprite( NULL, 150, game_res_h * 0.97f );
	debug_current_line = new cHudSprite( NULL, 300, game_res_h * 0.97f );
	debug_current_waypoint = new cHudSprite( NULL, 500, game_res_h * 0.97f );

	Set_Type( MARYO_SMALL );
	m_direction = DIR_DOWN;
	Set_Direction( DIR_UNDEFINED );
}

cOverworld_Player :: ~cOverworld_Player( void )
{
	delete debug_lines;
	delete debug_current_line;
	delete debug_current_waypoint;

	Unload_Images();
}

void cOverworld_Player :: Load_Images( void )
{
	Unload_Images();

	if( maryo_state == MARYO_SMALL )
	{
		Add_Image( pVideo->Get_Surface( "world/maryo/small/down.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/down_1.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/down.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/down_2.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/up_1.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/up.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/up_1.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/up_2.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/left.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/left_1.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/left.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/left_2.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/right.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/right_1.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/right.png" ) );
		Add_Image( pVideo->Get_Surface( "world/maryo/small/right_2.png" ) );

		Set_Animation( 1 );
	}
	else
	{
		printf( "Unsupported Maryo state : %d\n", static_cast<unsigned int>(maryo_state) );
		return;
	}
	
	Set_Image_Num( 0, 1 );	
}

void cOverworld_Player :: Unload_Images( void )
{
	Clear_Images();
	Reset_Animation();
}

void cOverworld_Player :: Set_Direction( const ObjectDirection dir, bool new_start_direction /* = 0 */ )
{
	if( dir != m_direction )
	{
		Reset_Animation();

		// undefined is set if not moving
		if( dir == DIR_UNDEFINED )
		{
			Set_Animation_Image_Range( 0, 3 );
			Set_Time_All( 300, 1 );
		}
		else if( dir == DIR_DOWN )
		{
			Set_Animation_Image_Range( 0, 3 );
			Set_Time_All( 200, 1 );
		}
		else if( dir == DIR_UP )
		{
			Set_Animation_Image_Range( 4, 7 );
			Set_Time_All( 200, 1 );
		}
		else if( dir == DIR_LEFT )
		{
			Set_Animation_Image_Range( 8, 11 );
			Set_Time_All( 200, 1 );
		}
		else if( dir == DIR_RIGHT )
		{
			Set_Animation_Image_Range( 12, 15 );
			Set_Time_All( 200, 1 );
		}

		Set_Image_Num( m_anim_img_start );
	}

	cAnimated_Sprite::Set_Direction( dir, new_start_direction );

	Update_Vel();
}

void cOverworld_Player :: Set_Type( Maryo_type new_type )
{
	// already set
	if( maryo_state == new_type )
	{
		return;
	}

	maryo_state = new_type;
	Load_Images();
}

void cOverworld_Player :: Update( void )
{
	cAnimated_Sprite::Update();

	Update_Animation();

	if( m_direction == DIR_UNDEFINED )
	{
		return;
	}

	// default walking
	if( !fixed_walking )
	{
		Update_Walk();
	} 
	// fixed walking
	else
	{
		Update_Waypoint_Walk();
	}
}

void cOverworld_Player :: Draw( cSurface_Request *request /* = NULL */ )
{
	bool create_request = 0;

	if( !request )
	{
		create_request = 1;
		// create request
		request = new cSurface_Request();
	}

	// Draw player
	cAnimated_Sprite::Draw( request );
	// alpha in debug mode
	if( pOverworld_Manager->debugmode )
	{
		request->color.alpha = 64;
	}

	if( create_request )
	{
		// add request
		pRenderer->Add( request );
	}

	// Draw debug text
	Draw_Debug_Text();
}

void cOverworld_Player :: Draw_Debug_Text( void )
{
	if( !pOverworld_Manager->debugmode )
	{
		return;
	}

	// Update Lines Texts
	if( static_cast<int>(pActive_Overworld->m_layer->size()) != debug_lines_last )
	{
		debug_lines->Set_Image( pFont->Render_Text( pFont->m_font_small, "Lines : " + int_to_string( pActive_Overworld->m_layer->size() ), blue ), 1, 1 );
	}
	// Update Current Line Texts
	if( current_line != debug_current_line_last )
	{
		debug_current_line->Set_Image( pFont->Render_Text( pFont->m_font_small, "Curr Line : " + int_to_string( current_line ), green ), 1, 1 );
	}
	// Update Current Waypoint Texts
	if( current_waypoint != debug_current_waypoint_last )
	{
		debug_current_waypoint->Set_Image( pFont->Render_Text( pFont->m_font_small, "Curr Waypoint : " + int_to_string( current_waypoint ), green ), 1, 1 );
	}

	// Draw Line count
	cSurface_Request *request = new cSurface_Request();
	debug_lines->Draw( request );
	request->shadow_pos = 1.0f;
	request->shadow_color = black;
	// add request
	pRenderer->Add( request );

	// Draw Current Line
	request = new cSurface_Request();
	debug_current_line->Draw( request );
	request->shadow_pos = 1.0f;
	request->shadow_color = black;
	// add request
	pRenderer->Add( request );
	
	// Draw Current Waypoint
	request = new cSurface_Request();
	debug_current_waypoint->Draw( request );
	request->shadow_pos = 1.0f;
	request->shadow_color = black;
	// add request
	pRenderer->Add( request );
}

void cOverworld_Player :: Reset( void )
{
	current_waypoint = -2;
	line_waypoint = 0;
	current_line = -2;

	fixed_walking = 0;
	Set_Direction( DIR_UNDEFINED );
}

void cOverworld_Player :: Action_Interact( input_identifier key_type )
{
	// Left
	if( key_type == INP_LEFT )
	{
		pOverworld_Player->Start_Walk( DIR_LEFT );
	}
	// Right
	else if( key_type == INP_RIGHT )
	{
		pOverworld_Player->Start_Walk( DIR_RIGHT );
	}
	// Up
	else if( key_type == INP_UP )
	{
		pOverworld_Player->Start_Walk( DIR_UP );
	}
	// Down
	else if( key_type == INP_DOWN )
	{
		pOverworld_Player->Start_Walk( DIR_DOWN );
	}
	// Action
	else if( key_type == INP_ACTION )
	{
		// enter
		pOverworld_Player->Activate_Waypoint();
		Clear_Input_Events();
	}
	// Exit
	else if( key_type == INP_EXIT )
	{
		Game_Action = GA_ENTER_MENU;
	}
}

void cOverworld_Player :: Action_Stop_Interact( input_identifier key_type )
{
	// nothing yet
}

void cOverworld_Player :: Activate_Waypoint( void )
{
	// if no waypoint or already walking
	if( current_waypoint < 0 || m_direction != DIR_UNDEFINED )
	{
		return;
	}

	// get waypoint
	cWaypoint *waypoint = Get_Waypoint();

	// normal level waypoint
	if( waypoint->waypoint_type == WAYPOINT_NORMAL )
	{
		// Enter Level
		Game_Action = GA_ENTER_LEVEL;
		Game_Action_Data.add( "level", waypoint->Get_Destination() );
	}
	// world link waypoint
	else if( waypoint->waypoint_type == WAYPOINT_WORLD_LINK )
	{
		// remember Overworld origin
		cOverworld *overworld_origin = pActive_Overworld;

		// world string
		std::string str_world = waypoint->Get_Destination();

		// Enter Credits Menu ( World End )
		if( str_world.compare( "credits" ) == 0 )
		{
			Draw_Effect_Out( EFFECT_OUT_HORIZONTAL_VERTICAL );
			Game_Action = GA_ENTER_MENU_CREDITS;
		}
		// world link
		else
		{
			cOverworld *new_world = pOverworld_Manager->Get( str_world );

			if( new_world )
			{
				Game_Action = GA_ENTER_WORLD;
				Game_Action_Data.add( "world", str_world.c_str() );
				Game_Action_Data.add( "player_waypoint", overworld_origin->m_description->path.c_str() );
			}
			else
			{
				printf( "Warning : Overworld not found %s\n", str_world.c_str() );
			}
		}
	}
}

void cOverworld_Player :: Update_Vel( void )
{
	if( m_direction == DIR_UP )
	{
		m_velx = 0.0f;
		m_vely = -3.0f;
	}
	else if( m_direction == DIR_DOWN )
	{
		m_velx = 0.0f;
		m_vely = 3.0f;
	}
	else if( m_direction == DIR_LEFT )
	{
		m_velx = -3.0f;
		m_vely = 0.0f;
	}
	else if( m_direction == DIR_RIGHT )
	{
		m_velx = 3.0f;
		m_vely = 0.0f;
	}
}

bool cOverworld_Player :: Start_Walk( ObjectDirection new_direction )
{
	// already walking into the given direction
	if( new_direction == m_direction )
	{
		return 0;
	}

	// invalid direction
	if( !( new_direction == DIR_UP || new_direction == DIR_DOWN || new_direction == DIR_LEFT || new_direction == DIR_RIGHT ) )
	{
		printf( "Warning : New direction is invalid : %d\n", new_direction );
		return 0;
	}

	// print debug info
	if( pOverworld_Manager->debugmode )
	{
		printf( "Current Maryo Direction : %d\n", m_direction );

		if( current_waypoint > 0 )
		{
			printf( "Waypoint Direction Forward : %d Backward : %d\n", Get_Waypoint()->direction_forward, Get_Waypoint()->direction_backward );
		}
	}

	// a start from waypoint
	if( current_waypoint >= 0 && m_direction == DIR_UNDEFINED )
	{
		// Get Layer Line in front
		cLayer_Line_Point_Start *front_line = Get_Front_Line( new_direction );

		if( !front_line )
		{
			if( pOverworld_Manager->debugmode )
			{
				printf( "Waypoint Front line not detected\n" );
			}

			return 0;
		}

		if( pOverworld_Manager->debugmode )
		{
			printf( "Waypoint Front line id %d, origin id %d\n", pActive_Overworld->m_layer->Get_Array_Num( front_line ), front_line->origin );
		}

		// forward
		if( Get_Waypoint()->direction_forward == new_direction )
		{
			cWaypoint *next_waypoint = front_line->Get_End_Waypoint();

			if( !next_waypoint )
			{
				printf( "Next waypoint not detected\n" );
				return 0;
			}

			// next waypoint is not accessible
			if( !next_waypoint->access )
			{
				if( pOverworld_Manager->debugmode )
				{
					printf( "No access to next waypoint\n" );
				}

				return 0;
			}

			if( pOverworld_Manager->debugmode )
			{
				printf( "Next waypoint id : %d\n", current_waypoint );
			}

			// set line waypoint
			line_waypoint = front_line->origin;
		}
		// backward
		else if( Get_Waypoint()->direction_backward == new_direction )
		{
			// set line waypoint
			line_waypoint = front_line->origin;
		}
		// invalid direction
		else
		{
			return 0;
		}
	}

	// remember current direction
	ObjectDirection direction_old = m_direction;

	// if already walking
	if( m_direction != DIR_UNDEFINED )
	{
		if( m_direction == Get_Opposite_Direction( new_direction ) && !fixed_walking && pActive_Overworld->Get_Waypoint_Collision( m_col_rect ) < 0 )
		{
			Set_Direction( new_direction );
		}
	}
	// start walking
	else
	{
		Set_Direction( new_direction );
		// force a small move into the new direction
		Move( m_velx, m_vely, 1 );
	}

	// if maryo is not on his current waypoint collide with every waypoint
	if( current_waypoint >= 0 && !m_col_rect.Intersects( Get_Waypoint()->m_rect ) )
	{
		current_waypoint = -1;
	}

	// if maryo is walking into a new direction
	if( direction_old != m_direction )
	{
		return 1;
	}

	// direction unchanged
	return 0;
}

void cOverworld_Player :: Update_Walk( void )
{
	Move( m_velx, m_vely );
	Update_Path_Diff();

	/* Check if Maryo is too far away from the line in the current direction
	 * or if no line is anymore connected
	 * if true change direction
	*/
	if( ( ( m_direction == DIR_UP || m_direction == DIR_DOWN ) && line_hor.line_number >= 0 && ( line_hor.difference > 20 || line_hor.difference < -20 ) ) || 
		( ( m_direction == DIR_LEFT || m_direction == DIR_RIGHT ) && line_ver.line_number >= 0 && ( line_ver.difference > 20 || line_ver.difference < -20 ) ) ||
		 ( line_hor.line_number < 0 && line_ver.line_number < 0 ) )
	{
		if( pOverworld_Manager->debugmode )
		{
			if( m_direction == DIR_UP || m_direction == DIR_DOWN )
			{
				printf( "horizontal line connection broken : num %d diff %f\n", line_hor.line_number, line_hor.difference );
			}
			else if( m_direction == DIR_LEFT || m_direction == DIR_RIGHT )
			{
				printf( "vertical line connection broken : num %d diff %f\n", line_ver.line_number, line_ver.difference );
			}
		}

		current_waypoint = -1;

		Change_Direction();
	}

	Auto_Pos_Correction();

	// check if a new waypoint is near maryo
	for( unsigned int i = 0; i < pActive_Overworld->m_waypoints.size(); i++ )
	{
		// skip the start waypoint
		if( static_cast<int>(i) == current_waypoint )
		{
			continue;
		}

		if( pActive_Overworld->m_waypoints[i]->m_rect.Intersects( m_col_rect ) )
		{
			Start_Waypoint_Walk( i );
			break;
		}
	}
}

void cOverworld_Player :: Start_Waypoint_Walk( int new_waypoint )
{
	fixed_walking = 1;
	current_waypoint = new_waypoint;
	current_line = -2;

	pActive_Overworld->Update_Waypoint_text();
}

void cOverworld_Player :: Update_Waypoint_Walk( void )
{
	// invalid current waypoint
	if( current_waypoint < 0 )
	{
		return;
	}

	unsigned int reached = 0;
	cWaypoint *waypoint = Get_Waypoint();

	// horizontal position to waypoint
	const float x = m_col_rect.m_x + ( m_col_rect.m_w * 0.5f );

	if( waypoint->m_rect.m_x + ( waypoint->m_rect.m_w * 0.4f ) > x )
	{
		Move( 3.0f, 0.0f );
	}
	else if( waypoint->m_rect.m_x + ( waypoint->m_rect.m_w * 0.7f ) < x )
	{
		Move( -3.0f, 0.0f );
	}
	else
	{
		reached++;
	}

	// vertical position to waypoint
	const float y = m_col_rect.m_y + ( m_col_rect.m_h * 0.5f );

	if( waypoint->m_rect.m_y + ( waypoint->m_rect.m_h * 0.4f ) > y )
	{
		Move( 0.0f, 3.0f );
	}
	else if( waypoint->m_rect.m_y + ( waypoint->m_rect.m_h * 0.7f ) < y )
	{
		Move( 0.0f, -3.0f );
	}
	else
	{
		reached++;
	}

	if( reached == 2 )
	{
		fixed_walking = 0;

		Set_Direction( DIR_UNDEFINED );
		Set_Waypoint( current_waypoint );

		pAudio->Play_Sound( "waypoint_reached.ogg" );
	}
}

bool cOverworld_Player :: Set_Waypoint( int waypoint, bool new_startpos /* = 0 */ )
{
	if( waypoint < 0 || waypoint >= static_cast<int>(pActive_Overworld->m_waypoints.size()) ) 
	{
		return 0;
	}

	cWaypoint *wp = pActive_Overworld->m_waypoints[waypoint];

	Set_Pos( wp->m_rect.m_x - m_col_pos.m_x + ( ( wp->m_rect.m_w - m_col_rect.m_w ) * 0.5f ), wp->m_rect.m_y - m_col_pos.m_y + ( ( wp->m_rect.m_h - m_col_rect.m_h ) * 0.5f ), new_startpos );
	current_waypoint = waypoint;

	// Update Camera
	pActive_Camera->Update();
	// Update Waypoint text
	pActive_Overworld->Update_Waypoint_text();

	return 1;
}

cWaypoint *cOverworld_Player :: Get_Waypoint( void )
{
	if( current_waypoint < 0 )
	{
		return NULL;
	}

	return pActive_Overworld->Get_Waypoint( current_waypoint );
}

cLayer_Line_Point_Start *cOverworld_Player :: Get_Front_Line( ObjectDirection dir ) const
{
	return pActive_Overworld->m_layer->Get_Line_Collision_Direction( m_col_rect.m_x + ( m_col_rect.m_w * 0.5f ), m_col_rect.m_y + ( m_col_rect.m_h * 0.5f ), dir ).line;
}

void cOverworld_Player :: Update_Path_Diff( unsigned int csize /* = 25 */ )
{
	float x = m_col_rect.m_x + ( m_col_rect.m_w * 0.5f ) + m_velx;
	float y = m_col_rect.m_y + ( m_col_rect.m_h * 0.5f ) + m_vely;

	// bigger direction check size
	unsigned int hor_advance = 0;
	unsigned int ver_advance = 0;
	if( m_direction == DIR_LEFT || m_direction == DIR_RIGHT )
	{
		ver_advance += 10;
	}
	else if( m_direction == DIR_UP || m_direction == DIR_DOWN )
	{
		hor_advance += 10;
	}

	// get nearest lines
	line_hor = pActive_Overworld->m_layer->Get_Nearest( x, y, DIR_HORIZONTAL, csize + hor_advance, line_waypoint );
 	line_ver = pActive_Overworld->m_layer->Get_Nearest( x, y, DIR_VERTICAL, csize + ver_advance, line_waypoint );
}

void cOverworld_Player :: Change_Direction( void )
{
	line_hor = cLine_collision();
	line_ver = cLine_collision();

	float x = m_col_rect.m_x + ( m_col_rect.m_w * 0.5f );
	float y = m_col_rect.m_y + ( m_col_rect.m_h * 0.5f );

	/* check with a big collision line the difference to the next layer line behind maryo
	 * if no line is found turn around
	 * if a line is found walk into it's direction
	*/
	if( m_direction == DIR_RIGHT || m_direction == DIR_LEFT )
	{
		// search for a line with increasing distance
		for( unsigned int i = 0; i < 30; i++ )
		{
			line_ver = pActive_Overworld->m_layer->Get_Nearest( x - i, y, DIR_VERTICAL, 80, line_waypoint );

			// found
			if( line_ver.line_number >= 0 )
			{
				break;
			}

			line_ver = pActive_Overworld->m_layer->Get_Nearest( x + i, y, DIR_VERTICAL, 80, line_waypoint );

			// found
			if( line_ver.line_number >= 0 )
			{
				break;
			}
		}

		// if line is found
		if( line_ver.line_number >= 0 )
		{
			if( line_ver.difference < 0 )
			{
				Set_Direction( DIR_UP );
			}
			else if( line_ver.difference > 0 )
			{
				Set_Direction( DIR_DOWN );
			}
		}
		// if no line found walk in to the opposite direction
		else
		{
			Set_Direction( Get_Opposite_Direction( m_direction ) );
		}
	}
	else if( m_direction == DIR_UP || m_direction == DIR_DOWN )
	{
		// search for a line with increasing distance
		for( unsigned int i = 0; i < 30; i++ )
		{
			line_hor = pActive_Overworld->m_layer->Get_Nearest( x, y + i, DIR_HORIZONTAL, 80, line_waypoint );

			// found
			if( line_hor.line_number >= 0 )
			{
				break;
			}

			line_hor = pActive_Overworld->m_layer->Get_Nearest( x, y - i, DIR_HORIZONTAL, 80, line_waypoint );

			// found
			if( line_hor.line_number >= 0 )
			{
				break;
			}
		}

		// if line is found
		if( line_hor.line_number >= 0 )
		{
			if( line_hor.difference < 0 )
			{
				Set_Direction( DIR_LEFT );
			}
			else if( line_hor.difference > 0 )
			{
				Set_Direction( DIR_RIGHT );
			}
		}
		// if no line found walk in to the opposite direction
		else
		{
			Set_Direction( Get_Opposite_Direction( m_direction ) );
		}
	}
	else
	{
		printf( "Warning : Unknown Maryo direction %d\n", static_cast<int>(m_direction) );
		Set_Direction( DIR_DOWN );
	}

	if( pOverworld_Manager->debugmode )
	{
		printf( "Changed direction to %s\n", Get_Direction_Name( m_direction ).c_str() );
	}
}

void cOverworld_Player :: Auto_Pos_Correction( float size /* = 1.7f */, float min_distance /* = 5.0f */ )
{
	// if colliding with a waypoint
	if( pActive_Overworld->Get_Waypoint_Collision( m_col_rect ) != -1 )
	{
		return;
	}

	if( m_direction == DIR_LEFT || m_direction == DIR_RIGHT )
	{
		// up and down correction
		if( line_ver.difference > min_distance )
		{
			Move( 0.0f, size );
		}
		else if( line_ver.difference < -min_distance )
		{
			Move( 0.0f, -size );
		}

		// Set current line number
		if( current_line != line_ver.line_number )
		{
			current_line = line_ver.line_number;
		}
	}
	else if( m_direction == DIR_UP || m_direction == DIR_DOWN )
	{ 
		// left correction
		if( line_hor.difference < -min_distance )
		{
			Move( -size, 0.0f );
		}
		// right correction
		else if( line_hor.difference > min_distance )
		{
			Move( size, 0.0f );
		}

		// Set current line number
		if( current_line != line_hor.line_number )
		{
			current_line = line_hor.line_number;
		}
	}
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cOverworld_Player *pOverworld_Player = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
