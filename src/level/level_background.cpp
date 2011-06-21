/***************************************************************************
 * level_background.cpp  -  level background image and color handling class
 *
 * Copyright (C) 2005 - 2009 Florian Richter
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../level/level_background.h"
#include "../user/preferences.h"
#include "../core/game_core.h"
#include "../video/gl_surface.h"
#include "../core/framerate.h"

namespace SMC
{

/* *** *** *** *** *** *** *** cBackground *** *** *** *** *** *** *** *** *** *** */

cBackground :: cBackground( void )
{
	Init();
}

cBackground :: cBackground( CEGUI::XMLAttributes &attributes )
{
	Init();
	Create_From_Stream( attributes );
}

cBackground :: ~cBackground( void )
{
	//
}

void cBackground :: Init( void )
{
	m_type = BG_NONE;

	m_pos_x = 0.0f;
	m_pos_y = 0.0f;
	m_start_pos_x = 0.0f;
	m_start_pos_y = 0.0f;
	m_pos_z = 0.00011f;

	m_color_1 = static_cast<Uint8>(0);
	m_color_2 = static_cast<Uint8>(0);

	m_image_1_filename.reserve( 120 );
	m_image_1 = NULL;

	m_speed_x = 0.5f;
	m_speed_y = 0.5f;
	m_const_vel_x = 0.0f;
	m_const_vel_y = 0.0f;
}

void cBackground :: Create_From_Stream( CEGUI::XMLAttributes &attributes )
{
	Set_Type( static_cast<BackgroundType>(attributes.getValueAsInteger( "type" )) );

	if( m_type == BG_GR_HOR || m_type == BG_GR_VER )
	{
		Set_Color_1( Color( static_cast<Uint8>(attributes.getValueAsInteger( "bg_color_1_red" )), attributes.getValueAsInteger( "bg_color_1_green" ), attributes.getValueAsInteger( "bg_color_1_blue" ) ) );
		Set_Color_2( Color( static_cast<Uint8>(attributes.getValueAsInteger( "bg_color_2_red" )), attributes.getValueAsInteger( "bg_color_2_green" ), attributes.getValueAsInteger( "bg_color_2_blue" ) ) );
	}
	else if( m_type == BG_IMG_BOTTOM || m_type == BG_IMG_TOP || m_type == BG_IMG_ALL )
	{
		Set_Start_Pos( attributes.getValueAsFloat( "posx" ), attributes.getValueAsFloat( "posy" ) );
		Set_Pos_Z( attributes.getValueAsFloat( "posz" ) );

		Set_Image( attributes.getValueAsString( "image" ).c_str() );
		Set_Scroll_Speed( attributes.getValueAsFloat( "speedx" ), attributes.getValueAsFloat( "speedy" ) );
		Set_Const_Velocity_X( attributes.getValueAsFloat( "const_velx" ) );
		Set_Const_Velocity_Y( attributes.getValueAsFloat( "const_vely" ) );
	}
}

void cBackground :: Save_To_Stream( ofstream &file )
{
	if( m_type == BG_NONE && m_image_1_filename.length() <= 3 )
	{
		return;
	}

	// begin background
	file << "\t<background>" << std::endl;

	// type
	file << "\t\t<Property name=\"type\" value=\"" << m_type << "\" />" << std::endl;

	// gradient
	if( m_type == BG_GR_HOR || m_type == BG_GR_VER )
	{
		// background color 1
		file << "\t\t<Property name=\"bg_color_1_red\" value=\"" << static_cast<int>(m_color_1.red) << "\" />" << std::endl;
		file << "\t\t<Property name=\"bg_color_1_green\" value=\"" << static_cast<int>(m_color_1.green) << "\" />" << std::endl;
		file << "\t\t<Property name=\"bg_color_1_blue\" value=\"" << static_cast<int>(m_color_1.blue) << "\" />" << std::endl;
		// background color 2
		file << "\t\t<Property name=\"bg_color_2_red\" value=\"" << static_cast<int>(m_color_2.red) << "\" />" << std::endl;
		file << "\t\t<Property name=\"bg_color_2_green\" value=\"" << static_cast<int>(m_color_2.green) << "\" />" << std::endl;
		file << "\t\t<Property name=\"bg_color_2_blue\" value=\"" << static_cast<int>(m_color_2.blue) << "\" />" << std::endl;
	}
	// image
	else if( m_type == BG_IMG_BOTTOM || m_type == BG_IMG_TOP || m_type == BG_IMG_ALL )
	{
		// position
		file << "\t\t<Property name=\"posx\" value=\"" << m_start_pos_x << "\" />" << std::endl;
		file << "\t\t<Property name=\"posy\" value=\"" << m_start_pos_y << "\" />" << std::endl;
		file << "\t\t<Property name=\"posz\" value=\"" << m_pos_z << "\" />" << std::endl;

		// image filename
		file << "\t\t<Property name=\"image\" value=\"" << m_image_1_filename << "\" />" << std::endl;
		// speed
		file << "\t\t<Property name=\"speedx\" value=\"" << m_speed_x << "\" />" << std::endl;
		file << "\t\t<Property name=\"speedy\" value=\"" << m_speed_y << "\" />" << std::endl;
		// constant velocity
		file << "\t\t<Property name=\"const_velx\" value=\"" << m_const_vel_x << "\" />" << std::endl;
		file << "\t\t<Property name=\"const_vely\" value=\"" << m_const_vel_y << "\" />" << std::endl;
	}
	else
	{
		printf( "Error : Unknown Background Type %d\n", m_type );
	}

	// end background
	file << "\t</background>" << std::endl;
}

void cBackground :: Set_Type( const BackgroundType ntype )
{
	m_type = ntype;
}

void cBackground :: Set_Type( const std::string &ntype ) 
{
	if( ntype.compare( "Disabled" ) == 0 )
	{
		m_type = BG_NONE;
	}
	else if( ntype.compare( "Bottom" ) == 0 )
	{
		m_type = BG_IMG_BOTTOM;
	}
	else if( ntype.compare( "All" ) == 0 )
	{
		m_type = BG_IMG_ALL;
	}
	else
	{
		printf( "Warning : Unknown Background type %s\n", ntype.c_str() );
	}
}

void cBackground :: Set_Color_1( const Color &color )
{
	m_color_1 = color; 
}

void cBackground :: Set_Color_2( const Color &color )
{
	m_color_2 = color; 
}

void cBackground :: Set_Image( const std::string &nimg_file_1 )
{
	m_image_1_filename = nimg_file_1;

	// empty
	if( m_image_1_filename.empty() )
	{
		m_image_1 = NULL;
		return;
	}

	if( m_image_1_filename.find( DATA_DIR "/" GAME_PIXMAPS_DIR "/" ) != std::string::npos )
	{
		m_image_1_filename.erase( 0, strlen( DATA_DIR "/" GAME_PIXMAPS_DIR "/" ) );
	}

	m_image_1 = pVideo->Get_Surface( m_image_1_filename );
}

void cBackground :: Set_Scroll_Speed( const float x /* = 1.0f */, const float y /* = 1.0f */ )
{
	m_speed_x = x;
	m_speed_y = y;
}

void cBackground :: Set_Start_Pos( const float x, const float y )
{
	m_start_pos_x = x;
	m_start_pos_y = y;
	// reset current position
	m_pos_x = m_start_pos_x;
	m_pos_y = m_start_pos_y;
}

void cBackground :: Set_Pos_Z( const float val )
{
	m_pos_z = val;
}

void cBackground :: Set_Const_Velocity_X( const float vel )
{
	m_const_vel_x = vel;
	// reset current position
	m_pos_x = m_start_pos_x;
}

void cBackground :: Set_Const_Velocity_Y( const float vel )
{
	m_const_vel_y = vel;
	// reset current position
	m_pos_y = m_start_pos_y;
}

void cBackground :: Update( void )
{
	if( !Is_Float_Equal( m_const_vel_x, 0.0f ) )
	{
		m_pos_x += (m_const_vel_x * 2) * pFramerate->m_speed_factor;
	}

	if( !Is_Float_Equal( m_const_vel_y, 0.0f ) )
	{
		m_pos_y += (m_const_vel_y * 2) * pFramerate->m_speed_factor;
	}
}

void cBackground :: Draw( void )
{
	// gradient
	if( m_type == BG_GR_VER || m_type == BG_GR_HOR )
	{
		Draw_Gradient();
	}
	// image
	else if( m_type == BG_IMG_BOTTOM || m_type == BG_IMG_ALL ) 
	{
		// if background images are disabled or no image
		if( !pPreferences->m_level_background_images || !m_image_1 )
		{
			return;
		}

		// get position
		float posx_final = m_pos_x - ( ( pActive_Camera->x * 0.2f ) * m_speed_x );
		float posy_final = m_pos_y - ( ( pActive_Camera->y * 0.3f ) * m_speed_y );

		if( m_type == BG_IMG_BOTTOM || m_type == BG_IMG_ALL )
		{
			posy_final += game_res_h - m_image_1->m_h;
		}

		// align start position x
		// to left
		while( posx_final > 0.0f )
		{
			posx_final -= m_image_1->m_w;
		}
		// to right
		while( posx_final < -m_image_1->m_w )
		{
			posx_final += m_image_1->m_w;
		}
		// align start position y
		if( m_type == BG_IMG_ALL )
		{
			// to top
			while( posy_final > 0 )
			{
				posy_final -= m_image_1->m_h;
			}
			// to bottom
			while( posy_final < -m_image_1->m_h )
			{
				posy_final += m_image_1->m_h;
			}
		}

		// draw until width is filled
		while( posx_final < game_res_w )
		{
			// draw horizontal
			m_image_1->Blit( posx_final, posy_final, m_pos_z );

			// draw vertical
			if( m_type == BG_IMG_ALL )
			{
				float posy_temp = posy_final;

				// draw until height is filled
				while( posy_temp < game_res_h - m_image_1->m_h )
				{
					// change position first as this position y is already drawn
					posy_temp += m_image_1->m_h;

					m_image_1->Blit( posx_final, posy_temp, m_pos_z );
				}
			}

			posx_final += m_image_1->m_w;
		}
	}
}

void cBackground :: Draw_Gradient( void )
{
	pVideo->Clear_Screen();

	// no need to draw a gradient if both colors are the same
	if( m_color_1 == m_color_2 )
	{
		pVideo->Draw_Rect( NULL, m_pos_z, &m_color_1 );
	}
	else
	{
		Color color, color_start;

		// set color start
		color_start = m_color_1;

		if( pActive_Camera->y < -1.0f )
		{
			float power = ( -pActive_Camera->y / 10000 );

			if( power > 1.0f )
			{
				power = 1.0f;
			}

			color_start.red += static_cast<Uint8>( static_cast<float>( m_color_2.red - m_color_1.red ) * power );
			color_start.green += static_cast<Uint8>( static_cast<float>( m_color_2.green - m_color_1.green ) * power );
			color_start.blue += static_cast<Uint8>( static_cast<float>( m_color_2.blue - m_color_1.blue ) * power );
		}

		if( m_type == BG_GR_VER )
		{
			pVideo->Draw_Gradient( NULL, m_pos_z, &color_start, &m_color_2, DIR_VERTICAL );
		}
		else if( m_type == BG_GR_HOR )
		{
			pVideo->Draw_Gradient( NULL, m_pos_z, &color_start, &m_color_2, DIR_HORIZONTAL );
		}
	}
}

std::string cBackground :: Get_Type_Name( void ) const
{
	if( m_type == BG_NONE )
	{
		return "Disabled";
	}
	else if( m_type == BG_IMG_BOTTOM )
	{
		return "Bottom";
	}
	else if( m_type == BG_IMG_ALL )
	{
		return "All";
	}

	return "Unknown";
}

/* *** *** *** *** *** *** cBackground_Manager *** *** *** *** *** *** *** *** *** *** *** */

cBackground_Manager :: cBackground_Manager( void )
: cObject_Manager<cBackground>()
{
	//
}

cBackground_Manager :: ~cBackground_Manager( void )
{
	cBackground_Manager::Delete_All();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
