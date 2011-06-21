/***************************************************************************
 * star.cpp  -  jumping star class
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

#include "../objects/star.h"
#include "../player/player.h"
#include "../core/framerate.h"
#include "../video/animation.h"
#include "../core/i18n.h"
// CEGUI
#include "CEGUIXMLAttributes.h"

namespace SMC
{

/* *** *** *** *** *** *** cjStar *** *** *** *** *** *** *** *** *** *** *** */

cjStar :: cjStar( float x, float y )
: cPowerUp( x, y )
{
	cjStar::Init();
}

cjStar :: cjStar( CEGUI::XMLAttributes &attributes )
: cPowerUp()
{
	cjStar::Init();
	cjStar::Create_From_Stream( attributes );
}

cjStar :: ~cjStar( void )
{
	//
}

void cjStar :: Init( void )
{
	m_type = TYPE_JSTAR;

	m_direction = DIR_RIGHT;
	anim_counter = 0;
	glim_mod = 1;
	glim_counter = 0;

	m_velx = 5;

	Add_Image( pVideo->Get_Surface( "game/items/star.png" ) );
	Set_Image_Num( 0, 1, 0 );

	m_name = _("Star");
}

cjStar *cjStar :: Copy( void )
{
	cjStar *star = new cjStar( m_start_pos_x, m_start_pos_y );

	return star;
}

void cjStar :: Create_From_Stream( CEGUI::XMLAttributes &attributes )
{
	// position
	Set_Pos( static_cast<float>(attributes.getValueAsInteger( "posx" )), static_cast<float>(attributes.getValueAsInteger( "posy" )), 1 );
}

void cjStar :: Save_To_Stream( ofstream &file )
{
	// begin item
	file << "\t<item>" << std::endl;

	// type
	file << "\t\t<Property name=\"type\" value=\"jstar\" />" << std::endl;
	// position
	file << "\t\t<Property name=\"posx\" value=\"" << static_cast<int>(m_start_pos_x) << "\" />" << std::endl;
	file << "\t\t<Property name=\"posy\" value=\"" << static_cast<int>(m_start_pos_y) << "\" />" << std::endl;

	// end item
	file << "\t</item>" << std::endl;
}

void cjStar :: Activate( void )
{
	if( !m_active )
	{
		return;
	}

	// animation
	Generate_Particles( m_pos_x + m_col_rect.m_w * 0.5f, m_pos_y + m_col_rect.m_h * 0.5f, 1, 20 );

	// activate star
	pPlayer->Get_Item( TYPE_JSTAR );

	// if spawned destroy
	if( m_spawned )
	{
		Destroy();
	}
	// disable
	else
	{
		Set_Active( 0 );
	}
}

void cjStar :: Update( void )
{
	if( !m_valid_update || !Is_In_Player_Range() )
	{
		return;
	}

	// Add Gravitation
	if( m_vely < 25 )
	{
		Add_Velocity( 0, 1.8f );
	}
	
	// rotate
	if( m_vely < 0.0f )
	{
		Add_Rotation_Z( ( 5 - ( m_vely / 2.5f ) ) * pFramerate->m_speed_factor );
	}
	// rotate back to 0 if falling
	else
	{
		if( m_rot_z > 5.0f && m_rot_z <= 175.0f )
		{
			Add_Rotation_Z( ( 5 - ( m_vely / 1.2f ) ) * pFramerate->m_speed_factor );
		}
		else if( m_rot_z < 355 && m_rot_z > 185 )
		{
			Add_Rotation_Z( ( -5 + ( m_vely / 1.2f ) ) * pFramerate->m_speed_factor );
		}
	}

	// generate small stars
	anim_counter += 1.1f * pFramerate->m_speed_factor;

	if( anim_counter > 1.0f )
	{
		Generate_Particles( m_pos_x, m_pos_y, 1, static_cast<int>(anim_counter) );
		anim_counter -= static_cast<int>(anim_counter);
	}

	// glim animation
	if( glim_mod )
	{
		glim_counter += pFramerate->m_speed_factor * 0.2f;

		if( glim_counter > 1.0f ) 
		{
			glim_counter = 1.0f;
			glim_mod = 0;
		}
	}
	else
	{
		glim_counter -= pFramerate->m_speed_factor * 0.2f;

		if( glim_counter < 0.0f ) 
		{
			glim_counter = 0.0f;
			glim_mod = 1;
		}
	}
}

void cjStar :: Draw( cSurface_Request *request /* = NULL */ )
{
	if( !m_valid_draw )
	{
		return;
	}

	Set_Color_Combine( glim_counter / 0.8f, glim_counter / 0.9f, glim_counter, GL_ADD );

	cPowerUp::Draw();
}

void cjStar :: Generate_Particles( float x /* = 0 */, float y /* = 0 */, bool random /* = 1 */, unsigned int quota /* = 2 */ ) const
{
	if( x == 0 )
	{
		x = m_pos_x;
	}

	if( y == 0 )
	{
		y = m_pos_y;
	}

	// set particle color
	Color particle_color = orange;
	particle_color.green += static_cast<Uint8>( glim_counter / 5 );
	particle_color.blue += static_cast<Uint8>( glim_counter / 1.5f );

	// create emitter
	cParticle_Emitter *anim = new cParticle_Emitter();
	if( random )
	{
		anim->Set_Emitter_Rect( x, y, m_col_rect.m_w * 0.9f, m_col_rect.m_h * 0.9f );
	}
	// create without random
	else
	{
		anim->Set_Emitter_Rect( x, y );
		// emit upwards
		anim->Set_Direction_Range( 180, 180 );
	}
	anim->Set_Quota( quota );
	anim->Set_Image( pVideo->Get_Surface( "animation/particles/star.png" ) );
	anim->Set_Time_to_Live( 0.4f );
	anim->Set_Fading_Alpha( 1 );
	anim->Set_Fading_Size( 1 );
	anim->Set_Speed( 1.5f, 0.5f );
	anim->Set_Scale( 0.15f );
	anim->Set_Color( particle_color );
	anim->Set_Blending( BLEND_ADD );
	anim->Set_Const_Rotation_Z( -5, 10 );
	pAnimation_Manager->Add( anim );
}

void cjStar :: Handle_Collision_Massive( cObjectCollision *collision )
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
		if( m_ground_object )
		{
			// clamp x to the ground object position
			float x = m_rect.m_x + ( m_rect.m_w / 2 );

			// left
			if( x < m_ground_object->m_col_rect.m_x )
			{
				x = m_ground_object->m_col_rect.m_x;
			}
			// right
			else if( x > m_ground_object->m_col_rect.m_x + m_ground_object->m_col_rect.m_w )
			{
				x = m_ground_object->m_col_rect.m_x +  + m_ground_object->m_col_rect.m_w;
			}

			Generate_Particles( x, m_ground_object->m_col_rect.m_y, 0, 5 );
		}

		m_vely = -25;
	}
}

void cjStar :: Handle_Collision_Player( cObjectCollision *collision )
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
