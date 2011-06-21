/***************************************************************************
 * layer.cpp  -  Overworld Layer class
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
 
#include "../overworld/world_layer.h"
#include "../video/renderer.h"
#include "../core/game_core.h"
#include "../overworld/overworld.h"
#include "../core/i18n.h"
// CEGUI
#include "CEGUIXMLParser.h"

namespace SMC
{

/* *** *** *** *** *** *** *** *** cLayer_Line_Point *** *** *** *** *** *** *** *** *** */

cLayer_Line_Point :: cLayer_Line_Point( SpriteType ntype )
: cSprite()
{
	m_sprite_array = ARRAY_PASSIVE;
	m_type = ntype;
	m_massive_type = MASS_PASSIVE;
	m_pos_z = 0.087f;

	linked_point = NULL;

	if( m_type == TYPE_OW_LINE_START )
	{
		m_pos_z += 0.001f;
		m_color = orange;
		m_name = _("Line Start Point");
	}
	else
	{
		m_color = red;
		m_name = _("Line End Point");
	}

	m_rect.m_w = 4;
	m_rect.m_h = 4;
	m_col_rect.m_w = m_rect.m_w;
	m_col_rect.m_h = m_rect.m_h;
	m_start_rect.m_w = m_rect.m_w;
	m_start_rect.m_h = m_rect.m_h;

	Update_Position_Rect();

	m_player_range = 0;
}

cLayer_Line_Point :: ~cLayer_Line_Point( void )
{
	Destroy();

	// remove linked point
	if( linked_point )
	{
		linked_point->linked_point = NULL;
	}
}

void cLayer_Line_Point :: Draw( cSurface_Request *request /* = NULL */ )
{
	if( m_auto_destroy || !pOverworld_Manager->draw_layer )
	{
		return;
	}

	// point rect
	pVideo->Draw_Rect( m_col_rect.m_x - pActive_Camera->x, m_col_rect.m_y - pActive_Camera->y, m_col_rect.m_w, m_col_rect.m_h, m_pos_z, &m_color );
}

void cLayer_Line_Point :: Destroy( void )
{
	if( m_auto_destroy )
	{
		return;
	}

	if( m_type == TYPE_OW_LINE_START )
	{
		pActive_Overworld->m_layer->Delete( static_cast<cLayer_Line_Point_Start *>(this), 0 );
	}

	cSprite::Destroy();

	// destroy linked point
	if( linked_point && !linked_point->m_auto_destroy )
	{
		linked_point->Destroy();
	}
}

float cLayer_Line_Point :: Get_Line_Pos_X( void ) const
{
	return m_pos_x + ( m_col_rect.m_w * 0.5f );
}

float cLayer_Line_Point :: Get_Line_Pos_Y( void ) const
{
	return m_pos_y + ( m_col_rect.m_h * 0.5f );
}


/* *** *** *** *** *** *** *** *** cLayer_Line_Point_Start *** *** *** *** *** *** *** *** *** */

cLayer_Line_Point_Start :: cLayer_Line_Point_Start( void )
: cLayer_Line_Point( TYPE_OW_LINE_START )
{
	cLayer_Line_Point_Start::Init();
}

cLayer_Line_Point_Start :: cLayer_Line_Point_Start( CEGUI::XMLAttributes &attributes )
: cLayer_Line_Point( TYPE_OW_LINE_START )
{
	cLayer_Line_Point_Start::Init();
	cLayer_Line_Point_Start::Create_From_Stream( attributes );
}

cLayer_Line_Point_Start :: ~cLayer_Line_Point_Start( void )
{
	Destroy();

	// remove linked point
	if( linked_point )
	{
		linked_point->linked_point = NULL;
	}
}

void cLayer_Line_Point_Start :: Init( void )
{
	anim_type = 0;
	origin = 0;

	// create end point
	linked_point = new cLayer_Line_Point( TYPE_OW_LINE_END );
	linked_point->linked_point = this;
}

cLayer_Line_Point_Start *cLayer_Line_Point_Start :: Copy( void )
{
	// create layer line
	cLayer_Line_Point_Start *layer_line = new cLayer_Line_Point_Start();

	// start position
	layer_line->Set_Pos( m_pos_x, m_pos_y, 1 );
	// end position
	layer_line->linked_point->Set_Pos( m_pos_x + 20, m_pos_y, 1 );

	// origin
	layer_line->origin = origin;

	return layer_line;
}

void cLayer_Line_Point_Start :: Create_From_Stream( CEGUI::XMLAttributes &attributes )
{
	// Start
	Set_Pos( static_cast<float>(attributes.getValueAsInteger( "X1" )) - 2, static_cast<float>(attributes.getValueAsInteger( "Y1" )) - 2, 1 );
	// End
	linked_point->Set_Pos( static_cast<float>(attributes.getValueAsInteger( "X2" )) - 2, static_cast<float>(attributes.getValueAsInteger( "Y2" )) - 2, 1 );
	// origin
	origin = attributes.getValueAsInteger( "origin" );
}

void cLayer_Line_Point_Start :: Draw( cSurface_Request *request /* = NULL */ )
{
	// not a valid draw
	if( m_auto_destroy || linked_point->m_auto_destroy || !pOverworld_Manager->draw_layer )
	{
		return;
	}

	// create request
	cLine_Request *line_request = new cLine_Request();

	// drawing color
	Color color = darkgreen;

	// if active
	if( pOverworld_Player->current_line >= 0 && pActive_Overworld->m_layer->objects[pOverworld_Player->current_line] == this )
	{
		color = lightblue;
	}

	pVideo->Draw_Line( Get_Line_Pos_X() - pActive_Camera->x, Get_Line_Pos_Y() - pActive_Camera->y, linked_point->Get_Line_Pos_X() - pActive_Camera->x, linked_point->Get_Line_Pos_Y() - pActive_Camera->y, 0.085f, &color, line_request );
	line_request->line_width = 6;

	// add request
	pRenderer->Add( line_request );

	// draw point rect
	cLayer_Line_Point::Draw( request );
}

GL_line cLayer_Line_Point_Start :: Get_Line( void ) const
{
	return GL_line( m_pos_x + ( m_col_rect.m_w * 0.5f ), m_pos_y + ( m_col_rect.m_h * 0.5f ), linked_point->m_pos_x + ( linked_point->m_col_rect.m_w * 0.5f ), linked_point->m_pos_y + ( linked_point->m_col_rect.m_h * 0.5f ) );
}

cWaypoint *cLayer_Line_Point_Start :: Get_End_Waypoint( void ) const
{
	// get waypoint number
	int wp_num = pActive_Overworld->Get_Waypoint_Collision( linked_point->m_col_rect );

	// no waypoint collision
	if( wp_num < 0 )
	{
		cLayer_Line_Point_Start *line_col = pActive_Overworld->m_layer->Get_Line_Collision_Start( linked_point->m_col_rect );

		// line collision
		if( line_col )
		{
			// follow line
			return line_col->Get_End_Waypoint();
		}
	}

	// return Waypoint
	return pActive_Overworld->Get_Waypoint( wp_num );
}

void cLayer_Line_Point_Start :: Editor_Activate( void )
{
	// get window manager
	CEGUI::WindowManager &wmgr = CEGUI::WindowManager::getSingleton();

	// origin
	CEGUI::Editbox *editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "layer_line_origin" ));
	Editor_Add( UTF8_("Waypoint origin"), UTF8_("Waypoint origin"), editbox, 100 );

	editbox->setValidationString( "^[+]?\\d*$" );
	editbox->setText( int_to_string( origin ) );
	editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cLayer_Line_Point_Start::Editor_Origin_Text_Changed, this ) );

	// init
	Editor_Init();
}

bool cLayer_Line_Point_Start :: Editor_Origin_Text_Changed( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	std::string str_text = static_cast<CEGUI::Editbox *>( windowEventArgs.window )->getText().c_str();

	origin = string_to_int( str_text );

	return 1;
}

/* *** *** *** *** *** *** *** *** Line Collision *** *** *** *** *** *** *** *** *** */

cLine_collision :: cLine_collision( void )
{
	line = NULL;
	line_number = -2;
	difference = 0;
}

/* *** *** *** *** *** *** *** *** Near Line Collision *** *** *** *** *** *** *** *** *** */

cNearLine_collision :: cNearLine_collision( void )
{
	line_number = -2;
	start = 0;
}

/* *** *** *** *** *** *** *** *** Layer *** *** *** *** *** *** *** *** *** */

cLayer :: cLayer( cOverworld *origin )
{
	n_origin = origin;
}

cLayer :: ~cLayer( void )
{
	Delete_All();
}

void cLayer :: Add( cLayer_Line_Point_Start *line_point )
{
	for( LayerLineList::iterator itr = objects.begin(), itr_end = objects.end(); itr != itr_end; ++itr )
	{
		// get pointer
		cLayer_Line_Point_Start *layer_line = (*itr);

		// if already in layer
		if( layer_line == line_point )
		{
			return;
		}
	}

	cObject_Manager<cLayer_Line_Point_Start>::Add( line_point );

	// check if in sprite manager
	if( n_origin->m_sprite_manager->Get_Array_Num( line_point ) == -1 )
	{
		// add start point
		n_origin->m_sprite_manager->Add( line_point );
	}
	// check if in sprite manager
	if( n_origin->m_sprite_manager->Get_Array_Num( line_point->linked_point ) == -1 )
	{
		// add end point
		n_origin->m_sprite_manager->Add( line_point->linked_point );
	}
}

void cLayer :: Load( const std::string &filename )
{
	Delete_All();

	try
	{
		// parse layer
		CEGUI::System::getSingleton().getXMLParser()->parseXMLFile( *this, filename.c_str(), DATA_DIR "/" GAME_SCHEMA_DIR "/World/Lines.xsd", "" );
	}
	// catch CEGUI Exceptions
	catch( CEGUI::Exception &ex )
	{
		printf( "Loading Line Layer %s CEGUI Exception %s\n", filename.c_str(), ex.getMessage().c_str() );
		pHud_Debug->Set_Text( _("Line Layer Loading failed : ") + (const std::string)ex.getMessage().c_str() );
	}
}

bool cLayer :: Save( const std::string &filename )
{
	ofstream file( filename.c_str(), ios::out | ios::trunc );

	if( !file )
	{
		pHud_Debug->Set_Text( _("Couldn't save world layer ") + filename );
		return 0;
	}

	// xml info
	file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
	// begin layer
	file << "<layer>" << std::endl;

	// lines
	for( LayerLineList::iterator itr = objects.begin(), itr_end = objects.end(); itr != itr_end; ++itr )
	{
		cLayer_Line_Point_Start *line = (*itr);

		// begin line
		file << "\t<line>" << std::endl;
			// start
			file << "\t\t<Property name=\"X1\" value=\"" << static_cast<int>(line->Get_Line_Pos_X()) << "\" />" << std::endl;
			file << "\t\t<Property name=\"Y1\" value=\"" << static_cast<int>(line->Get_Line_Pos_Y()) << "\" />" << std::endl;
			// end
			file << "\t\t<Property name=\"X2\" value=\"" << static_cast<int>(line->linked_point->Get_Line_Pos_X()) << "\" />" << std::endl;
			file << "\t\t<Property name=\"Y2\" value=\"" << static_cast<int>(line->linked_point->Get_Line_Pos_Y()) << "\" />" << std::endl;
			// origin
			file << "\t\t<Property name=\"origin\" value=\"" << line->origin << "\" />" << std::endl;
		// end line
		file << "\t</line>" << std::endl;
	}

	// end layer
	file << "</layer>" << std::endl;
	file.close();

	return 1;
}

void cLayer :: Delete_All( void )
{
	// only clear array
	objects.clear();
}

cLayer_Line_Point_Start *cLayer :: Get_Line_Collision_Start( const GL_rect &line_rect )
{
	for( LayerLineList::iterator itr = objects.begin(), itr_end = objects.end(); itr != itr_end; ++itr )
	{
		// get pointer
		cLayer_Line_Point_Start *layer_line = (*itr);

		// check line 1
		if( line_rect.Intersects( layer_line->m_col_rect ) )
		{
			return layer_line;
		}
	}

	return NULL;
}

cLine_collision cLayer :: Get_Line_Collision_Direction( float x, float y, ObjectDirection dir, float dir_size /* = 10 */, unsigned int check_size /* = 10 */ ) const
{
	if( dir == DIR_UP )
	{
		y -= dir_size;
	}
	else if( dir == DIR_DOWN )
	{
		y += dir_size;
	}
	else if( dir == DIR_RIGHT )
	{
		x += dir_size;
	}
	else if( dir == DIR_LEFT )
	{
		x -= dir_size;
	}

	if( dir == DIR_LEFT || dir == DIR_RIGHT )
	{
		return n_origin->m_layer->Get_Nearest( x, y, DIR_VERTICAL, check_size );
	}
	else if( dir == DIR_UP || dir == DIR_DOWN )
	{
		return n_origin->m_layer->Get_Nearest( x, y, DIR_HORIZONTAL, check_size );
	}

	// invalid direction
	return cLine_collision();
}

cLine_collision cLayer :: Get_Nearest( float x, float y, ObjectDirection dir /* = DIR_HORIZONTAL */, unsigned int check_size /* = 15 */, int only_origin_id /* = -1 */ ) const
{
	for( LayerLineList::const_iterator itr = objects.begin(), itr_end = objects.end(); itr != itr_end; ++itr )
	{
		// get pointer
		cLayer_Line_Point_Start *layer_line = (*itr);

		// line is not from waypoint
		if( only_origin_id >= 0 && only_origin_id != layer_line->origin )
		{
			continue;
		}

		cLine_collision col = Get_Nearest_Line( layer_line, x, y, dir, check_size );

		// found
		if( col.line )
		{
			return col;
		}
	}

	// none found
	return cLine_collision();
}

cLine_collision cLayer :: Get_Nearest_Line( cLayer_Line_Point_Start *map_layer_line, float x, float y, ObjectDirection dir /* = DIR_HORIZONTAL */, unsigned int check_size /* = 15  */ ) const
{
	GL_line line_1, line_2;

	// create map line
	GL_line map_line = map_layer_line->Get_Line();

	// check into both directions from inside
	for( float csize = 0; csize < check_size; csize++ )
	{
		line_1.m_x1 = x;
		line_1.m_y1 = y;
		line_1.m_x2 = x;
		line_1.m_y2 = y;
		line_2 = line_1;

		// set line size
		if( dir == DIR_HORIZONTAL )
		{
			line_1.m_x1 += csize;
			line_2.m_x2 -= csize;
		}
		else // vertical
		{
			line_1.m_y1 += csize;
			line_2.m_y2 -= csize;
		}

		// debug drawing
		if( pOverworld_Manager->debugmode && pOverworld_Manager->draw_layer )
		{
			// create request
			cLine_Request *line_request = new cLine_Request();
			pVideo->Draw_Line( line_1.m_x1 - pActive_Camera->x, line_1.m_y1 - pActive_Camera->y, line_1.m_x2 - pActive_Camera->x, line_1.m_y2 - pActive_Camera->y, map_layer_line->m_pos_z + 0.001f, &white, line_request );
			line_request->line_width = 2;
			line_request->render_count = 50;
			// add request
			pRenderer->Add( line_request );

			// create request
			line_request = new cLine_Request();
			pVideo->Draw_Line( line_2.m_x1 - pActive_Camera->x, line_2.m_y1 - pActive_Camera->y, line_2.m_x2 - pActive_Camera->x, line_2.m_y2 - pActive_Camera->y, map_layer_line->m_pos_z + 0.001f, &black, line_request );
			line_request->line_width = 2;
			line_request->render_count = 50;
			// add request
			pRenderer->Add( line_request );
		}

		// check direction line 1
		if( line_1.Intersects( &map_line ) )
		{
			cLine_collision col = cLine_collision();

			col.line = map_layer_line;
			col.line_number = Get_Array_Num( map_layer_line );
			col.difference = csize;

			// found
			return col;
		}

		// check direction line 2
		if( line_2.Intersects( &map_line ) )
		{
			cLine_collision col = cLine_collision();

			col.line = map_layer_line;
			col.line_number = Get_Array_Num( map_layer_line );
			col.difference = -csize;

			// found
			return col;
		}
	}

	// not found
	return cLine_collision();
}

// XML element start
void cLayer :: elementStart( const CEGUI::String &element, const CEGUI::XMLAttributes &attributes )
{
	// Property of an Element
	if( element == "Property" )
	{
		xml_attributes.add( attributes.getValueAsString( "name" ), attributes.getValueAsString( "value" ) );
	}
}

// XML element end
void cLayer :: elementEnd( const CEGUI::String &element )
{
	if( element != "Property" )
	{
		if( element == "line" )
		{
			// add layer line
			Add( new cLayer_Line_Point_Start( xml_attributes ) );
		}
		else if( element == "layer" )
		{
			// ignore
		}
		else if( element.length() )
		{
			printf( "Warning : Overworld Layer Unknown element : %s\n", element.c_str() );
		}

		// clear
		xml_attributes = CEGUI::XMLAttributes();
	}
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
