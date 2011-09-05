/***************************************************************************
 * path.h  -  movement along a defined path
 *
 * Copyright (C) 2008 - 2009 Florian Richter
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../objects/path.h"
#include "../core/game_core.h"
#include "../core/i18n.h"
#include "../video/renderer.h"
#include "../input/mouse.h"
#include "../user/savegame.h"
#include "../level/level.h"
#include "../core/sprite_manager.h"
#include "../enemies/static.h"
#include "../objects/moving_platform.h"

namespace SMC
{

/* *** *** *** *** *** *** *** Path state class *** *** *** *** *** *** *** *** *** *** */

cPath_State :: cPath_State( void )
{
	m_path = NULL;
	forward = 1;
	pos_x = 0;
	pos_y = 0;

	current_segment_pos = 0;
	current_segment = 0;
}

cPath_State :: ~cPath_State( void )
{
	// remove link
	if( m_path )
	{
		m_path->Remove_Link( this );
	}
}

void cPath_State :: Load_From_Savegame( cSave_Level_Object *save_object )
{
	// path position
	if( save_object->exists( "new_pos_x" ) )
	{
		pos_x = string_to_float( save_object->Get_Value( "new_pos_x" ) );
	}
	if( save_object->exists( "new_pos_y" ) )
	{
		pos_y = string_to_float( save_object->Get_Value( "new_pos_y" ) );
	}
	// current segment
	if( save_object->exists( "current_segment" ) )
	{
		current_segment = string_to_int( save_object->Get_Value( "current_segment" ) );
	}
	if( save_object->exists( "current_segment_pos" ) )
	{
		current_segment_pos = string_to_float( save_object->Get_Value( "current_segment_pos" ) );
	}
	if( save_object->exists( "forward" ) )
	{
		forward = string_to_int( save_object->Get_Value( "forward" ) ) > 0;
	}
}

void cPath_State :: Save_To_Savegame( cSave_Level_Object *save_object )
{
	// path position
	save_object->m_properties.push_back( cSave_Level_Object_Property( "new_pos_x", float_to_string( pos_x ) ) );
	save_object->m_properties.push_back( cSave_Level_Object_Property( "new_pos_y", float_to_string( pos_y ) ) );

	// current segment
	save_object->m_properties.push_back( cSave_Level_Object_Property( "current_segment", int_to_string( current_segment ) ) );

	// current segment position
	save_object->m_properties.push_back( cSave_Level_Object_Property( "current_segment_pos", float_to_string( current_segment_pos ) ) );

	// forward
	save_object->m_properties.push_back( cSave_Level_Object_Property( "forward", int_to_string( static_cast<int>(forward) ) ) );
}

void cPath_State :: Draw( void )
{
	if( !m_path )
	{
		return;
	}
	
	pVideo->Draw_Rect( m_path->m_col_rect.m_x + pos_x - 4 - pActive_Camera->x, m_path->m_col_rect.m_y + pos_y - 4 - pActive_Camera->y, 8, 8, m_path->m_editor_pos_z + 0.00002f, &orange );
}

void cPath_State :: Set_Path_Identifier( const std::string &path )
{
	// remove old link
	if( m_path )
	{
		m_path->Remove_Link( this );
	}

	// set path
	m_path_identifier = path;
	m_path = pActive_Level->Get_Path_Object( m_path_identifier );

	// not found
	if( !m_path )
	{
		return;
	}
	
	// create link
	m_path->Create_Link( this );

	// set position to start
	Move_Reset();
}

void cPath_State :: Path_Destroyed_Event( void )
{
	m_path = NULL;
}

void cPath_State :: Move_Toggle( void )
{
	if( forward )
	{
		Move_Start_Backward();
	}
	else
	{
		Move_Start_Forward();
	}
}

void cPath_State :: Move_Reset( void )
{
	if( forward )
	{
		Move_Start_Forward();
	}
	else
	{
		Move_Start_Backward();
	}
}

void cPath_State :: Move_Reverse( void )
{
	forward = !forward;
}

void cPath_State :: Move_Start_Forward( void )
{
	if( !m_path || m_path->segments.empty() )
	{
		return;
	}

	forward = 1;
	Move_From_Segment( 0 );
}

void cPath_State :: Move_Start_Backward( void )
{
	if( !m_path || m_path->segments.empty() )
	{
		return;
	}

	forward = 0;
	Move_From_Segment( m_path->segments.size() - 1 );
}

void cPath_State :: Move_From_Segment( unsigned int segment )
{
	if( !m_path )
	{
		return;
	}

	// invalid segment
	if( segment >= m_path->segments.size() )
	{
		return;
	}

	current_segment = segment;
	
	if( forward )
	{
		current_segment_pos = 0;
		pos_x = m_path->segments[current_segment].m_x1;
		pos_y = m_path->segments[current_segment].m_y1;
	}
	// backward
	else
	{
		current_segment_pos = m_path->segments[current_segment].m_distance;
		pos_x = m_path->segments[current_segment].m_x2;
		pos_y = m_path->segments[current_segment].m_y2;
	}
}

bool cPath_State :: Path_Move( float distance )
{
	if( !m_path )
	{
		return 0;
	}
	
	// invalid slot
	if( current_segment >= m_path->segments.size() )
	{
		return 0;
	}

	// get current segment object
	cPath_Segment obj = m_path->segments[current_segment];

	// walk forward
	if( forward )
	{
		while( 1 )
		{
			// how much is left
			float remaining = obj.m_distance - current_segment_pos;

			if( distance > remaining )
			{
				pos_x = obj.m_x2;
				pos_y = obj.m_y2;

				// finished
				if( current_segment + 1 >= m_path->segments.size() )
				{
					// rewind
					if( m_path->m_rewind )
					{
						current_segment = 0;
						current_segment_pos = 0;
						return 0;
					}
					// mirror
					else
					{
						return 0;
					}
				}

				// set current segment object
				current_segment++;
				obj = m_path->segments[current_segment];

				current_segment_pos = 0;
				distance -= remaining;
			}
			else
			{
				current_segment_pos += distance;
				pos_x = obj.m_x1 + obj.m_ux * current_segment_pos;
				pos_y = obj.m_y1 + obj.m_uy * current_segment_pos;

				return 1;
			}
		}
	}
	// walk backward
	else
	{
		while( 1 )
		{
			// how much is left
			float remaining = current_segment_pos;

			if( distance > remaining )
			{
				pos_x = obj.m_x1;
				pos_y = obj.m_y1;

				// finished
				if( current_segment == 0 )
				{
					// rewind
					if( m_path->m_rewind )
					{
						current_segment = m_path->segments.size() - 1;
						obj = m_path->segments[current_segment];
						current_segment_pos = obj.m_distance;
						return 0;
					}
					// mirror
					else
					{
						return 0;
					}
				}

				// set current segment object
				current_segment--;
				obj = m_path->segments[current_segment];

				current_segment_pos = obj.m_distance;
				distance -= remaining;
			}
			else
			{
				current_segment_pos -= distance;
				pos_x = obj.m_x1 + obj.m_ux * current_segment_pos;
				pos_y = obj.m_y1 + obj.m_uy * current_segment_pos;

				return 1;
			}
		}
	}

	return 0;
}

/* *** *** *** *** *** *** cPath_Segment *** *** *** *** *** *** *** *** *** *** *** */

cPath_Segment :: cPath_Segment( void )
{
	m_x1 = 0;
	m_y1 = 0;
	m_x2 = 0;
	m_y2 = 0;

	m_ux = 0;
	m_uy = 0;
	m_distance = 0;
}

cPath_Segment :: ~cPath_Segment( void )
{

}

void cPath_Segment :: Set_Pos( float x1, float y1, float x2, float y2 )
{
	m_x1 = x1;
	m_y1 = y1;
	m_x2 = x2;
	m_y2 = y2;

	Update();
}

void cPath_Segment :: Set_Pos_Start( float x1, float y1 )
{
	m_x1 = x1;
	m_y1 = y1;

	Update();
}

void cPath_Segment :: Set_Pos_End( float x2, float y2 )
{
	m_x2 = x2;
	m_y2 = y2;

	Update();
}

void cPath_Segment :: Set_Pos_Start_X( float x1 )
{
	m_x1 = x1;

	Update();
}

void cPath_Segment :: Set_Pos_Start_Y( float y1 )
{
	m_y1 = y1;

	Update();
}

void cPath_Segment :: Set_Pos_End_X( float x2 )
{
	m_x2 = x2;

	Update();
}

void cPath_Segment :: Set_Pos_End_Y( float y2 )
{
	m_y2 = y2;

	Update();
}

void cPath_Segment :: Update( void )
{
	float dx = m_x2 - m_x1;
	float dy = m_y2 - m_y1;
	// distance
	m_distance = sqrt( dx * dx + dy * dy );
	// unit vector
	if( m_distance != 0 )
	{
		m_ux = dx / m_distance;
		m_uy = dy / m_distance;
	}
}

/* *** *** *** *** *** *** *** Path class *** *** *** *** *** *** *** *** *** *** */

cPath :: cPath( void )
: cSprite()
{
	// Set defaults
	cPath::Init();
}

cPath :: cPath( CEGUI::XMLAttributes &attributes )
: cSprite()
{
	cPath::Init();
	cPath::Create_From_Stream( attributes );
}

cPath :: ~cPath( void )
{
	Remove_Links();
}

void cPath :: Init( void )
{
	m_sprite_array = ARRAY_ACTIVE;
	m_type = TYPE_PATH;
	m_massive_type = MASS_PASSIVE;
	m_editor_pos_z = 0.11f;

	m_name = _("Path");

	// size
	m_rect.m_w = 10;
	m_rect.m_h = 10;
	m_col_rect.m_w = m_rect.m_w;
	m_col_rect.m_h = m_rect.m_h;
	m_start_rect.m_w = m_rect.m_w;
	m_start_rect.m_h = m_rect.m_h;

	m_rewind = 0;
	editor_color = Color( static_cast<Uint8>(100), 150, 200, 128 );
	editor_selected_segment = 0;
}

cPath *cPath :: Copy( void )
{
	cPath *path = new cPath();
	path->Set_Pos( m_start_pos_x, m_start_pos_y, 1 );
	path->segments = segments;
	path->Set_Identifier( m_identifier );
	path->Set_Rewind( m_rewind );

	return path;
}

void cPath :: Create_From_Stream( CEGUI::XMLAttributes &attributes )
{
	segments.clear();

	// position
	Set_Pos( static_cast<float>(attributes.getValueAsInteger( "posx" )), static_cast<float>(attributes.getValueAsInteger( "posy" )), 1 );
	// identifier
	Set_Identifier( attributes.getValueAsString( "identifier" ).c_str() );
	// rewind
	Set_Rewind( attributes.getValueAsBool( "rewind", m_rewind ) );

	unsigned int count = 0;
	// load segments
	while( 1 )
	{
		std::string str_pos = int_to_string( count );

		// next line not available
		if( !attributes.exists( "segment_" + str_pos + "_x1" ) )
		{
			break;
		}

		cPath_Segment obj;
		
		obj.Set_Pos( attributes.getValueAsFloat( "segment_" + str_pos + "_x1" ), attributes.getValueAsFloat( "segment_" + str_pos + "_y1" ), attributes.getValueAsFloat( "segment_" + str_pos + "_x2" ), attributes.getValueAsFloat( "segment_" + str_pos + "_y2" ) );

		segments.push_back( obj );

		count++;
	}
}

void cPath :: Save_To_Stream( ofstream &file )
{
	// begin path
	file << "\t<path>" << std::endl;

	// position
	file << "\t\t<Property name=\"posx\" value=\"" << static_cast<int>(m_start_pos_x) << "\" />" << std::endl;
	file << "\t\t<Property name=\"posy\" value=\"" << static_cast<int>(m_start_pos_y) << "\" />" << std::endl;
	// identifier
	file << "\t\t<Property name=\"identifier\" value=\"" << string_to_xml_string( m_identifier ) << "\" />" << std::endl;
	// rewind
	file << "\t\t<Property name=\"rewind\" value=\"" << m_rewind << "\" />" << std::endl;

	// segments
	unsigned int count = segments.size();

	for( unsigned int pos = 0; pos < count; pos++ )
	{
		std::string str_pos = int_to_string( pos );

		file << "\t\t<Property name=\"" << "segment_" + str_pos + "_x1" << "\" value=\"" << segments[pos].m_x1 << "\" />" << std::endl;
		file << "\t\t<Property name=\"" << "segment_" + str_pos + "_y1" << "\" value=\"" << segments[pos].m_y1 << "\" />" << std::endl;
		file << "\t\t<Property name=\"" << "segment_" + str_pos + "_x2" << "\" value=\"" << segments[pos].m_x2 << "\" />" << std::endl;
		file << "\t\t<Property name=\"" << "segment_" + str_pos + "_y2" << "\" value=\"" << segments[pos].m_y2 << "\" />" << std::endl;
	}

	// end path
	file << "\t</path>" << std::endl;
}

void cPath :: Load_From_Savegame( cSave_Level_Object *save_object )
{

}

cSave_Level_Object *cPath :: Save_To_Savegame( void )
{
	cSave_Level_Object *save_object = new cSave_Level_Object();

	// default values
	save_object->m_type = m_type;
	save_object->m_properties.push_back( cSave_Level_Object_Property( "posx", int_to_string( static_cast<int>(m_start_pos_x) ) ) );
	save_object->m_properties.push_back( cSave_Level_Object_Property( "posy", int_to_string( static_cast<int>(m_start_pos_y) ) ) );

	return save_object;
}

void cPath :: Set_Identifier( const std::string &identifier )
{
	m_identifier = identifier;

	// remove linked objects
	Remove_Links();
	
	if( m_identifier.empty() )
	{
		return;
	}

	/* search for linked objects
	 * needed to update the links
	*/
	for( cSprite_List::iterator itr = pActive_Level->m_sprite_manager->objects.begin(), itr_end = pActive_Level->m_sprite_manager->objects.end(); itr != itr_end; ++itr )
	{
		cSprite *obj = (*itr);

		if( obj->m_auto_destroy )
		{
			continue;
		}

		if( obj->m_type == TYPE_STATIC_ENEMY )
		{
			cStaticEnemy *static_enemy = static_cast<cStaticEnemy *>(obj);

			// found
			if( static_enemy->m_path_state.m_path_identifier.compare( m_identifier ) == 0 )
			{
				// link to me
				static_enemy->Init_Links();
				//static_enemy->m_path_state.Set_Path_Identifier( m_identifier );
			}
		}
		else if( obj->m_type == TYPE_MOVING_PLATFORM )
		{
			cMoving_Platform *moving_platform = static_cast<cMoving_Platform *>(obj);

			// found
			if( moving_platform->m_path_state.m_path_identifier.compare( m_identifier ) == 0 )
			{
				// link to me
				moving_platform->Init_Links();
				//moving_platform->m_path_state.Set_Path_Identifier( m_identifier );
			}
		}
	}
}

void cPath :: Set_Rewind( bool rewind )
{
	// already set
	if( m_rewind == rewind )
	{
		return;
	}

	m_rewind = rewind;

	for( PathStateList::iterator itr = linked_path_states.begin(), itr_end = linked_path_states.end(); itr != itr_end; ++itr )
	{
		cPath_State *obj = (*itr);

		// reset backwards moving
		if( !obj->forward )
		{
			obj->Move_From_Segment( obj->current_segment );
		}
	}
}

void cPath :: Create_Link( cPath_State *path_state )
{
	if( !path_state )
	{
		return;
	}
	
	linked_path_states.push_back( path_state );
}

void cPath :: Remove_Link( cPath_State *path_state )
{
	if( !path_state )
	{
		return;
	}
	
	// get iterator
	PathStateList::iterator itr = std::find( linked_path_states.begin(), linked_path_states.end(), path_state );

	// if available
	if( itr != linked_path_states.end() )
	{
		// erase
		linked_path_states.erase( itr );
	}
}

void cPath :: Remove_Links( void )
{
	for( PathStateList::iterator itr = linked_path_states.begin(), itr_end = linked_path_states.end(); itr != itr_end; ++itr )
	{
		cPath_State *obj = (*itr);

		obj->Path_Destroyed_Event();
	}
}

void cPath :: Update( void )
{
	if( !m_valid_update )
	{
		return;
	}
}

void cPath :: Draw( cSurface_Request *request /* = NULL */ )
{
	if( !m_valid_draw )
	{
		return;
	}

	// draw color rect
	pVideo->Draw_Rect( m_col_rect.m_x - pActive_Camera->x, m_col_rect.m_y - pActive_Camera->y, m_col_rect.m_w, m_col_rect.m_h, m_editor_pos_z, &editor_color );

	// draw segments
	int count = 0;
	for( PathList::iterator itr = segments.begin(), itr_end = segments.end(); itr != itr_end; ++itr )
	{
		cPath_Segment obj = (*itr);

		Color line_color = editor_color;

		if( count % 2 != 1 )
		{
			line_color.blue = 255;
		}

		// create request
		cLine_Request *line_request = new cLine_Request();
		pVideo->Draw_Line( m_col_rect.m_x + obj.m_x1 - pActive_Camera->x, m_col_rect.m_y + obj.m_y1 - pActive_Camera->y, m_col_rect.m_x + obj.m_x2 - pActive_Camera->x, m_col_rect.m_y + obj.m_y2 - pActive_Camera->y, m_editor_pos_z + 0.00001f, &line_color, line_request );
		line_request->line_width = 2;
		// add request
		pRenderer->Add( line_request );

		count++;
	}
}

bool cPath :: Is_Draw_Valid( void )
{
	// if editor not enabled
	if( !editor_enabled && !game_debug )
	{
		return 0;
	}

	// if not active on the screen or not mouse object
	if( !m_active || ( !Is_Visible_On_Screen() && pMouseCursor->m_active_object != this ) )
	{
		return 0;
	}

	return 1;
}

void cPath :: Editor_Activate( void )
{
	CEGUI::WindowManager &wmgr = CEGUI::WindowManager::getSingleton();

	// identifier
	CEGUI::Editbox *editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "path_identifier" ));
	Editor_Add( UTF8_("Identifier"), UTF8_("Identifier name of the Path. This is needed for objects that can link to this."), editbox, 150 );

	editbox->setText( m_identifier.c_str() );
	editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cPath::Editor_Identifier_Text_Changed, this ) );

	// move type
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox *>(wmgr.createWindow( "TaharezLook/Combobox", "path_move_type" ));
	Editor_Add( UTF8_("Move Type"), UTF8_("Movement type. Mirror moves forth and back and rewind starts from the beginning again."), combobox, 100, 105 );

	combobox->addItem( new CEGUI::ListboxTextItem( "mirror" ) );
	combobox->addItem( new CEGUI::ListboxTextItem( "rewind" ) );

	if( m_rewind )
	{
		combobox->setText( "rewind" );
	}
	else
	{
		combobox->setText( "mirror" );
	}

	combobox->subscribeEvent( CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber( &cPath::Editor_Move_Type_Select, this ) );


	// selected segment
	combobox = static_cast<CEGUI::Combobox *>(wmgr.createWindow( "TaharezLook/Combobox", "path_selected_segment" ));
	Editor_Add( UTF8_("Segment"), UTF8_("Select the Segment to edit."), combobox, 100, 105 );

	for( unsigned int count = 0; count < segments.size(); count++ )
	{
		combobox->addItem( new CEGUI::ListboxTextItem( int_to_string( count ) ) );
	}

	combobox->setText( int_to_string( editor_selected_segment ) );

	combobox->subscribeEvent( CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber( &cPath::Editor_Selected_Segment_Select, this ) );

	// button add
	CEGUI::PushButton *pushbutton = static_cast<CEGUI::PushButton *>(wmgr.createWindow( "TaharezLook/Button", "path_segment_add" ));
	Editor_Add( "-", UTF8_("Add a Segment after the selected one."), pushbutton, 60, 28, 0 );

	pushbutton->setText( UTF8_("Add") );
	pushbutton->subscribeEvent( CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber( &cPath::Editor_Button_Add_Segment_Clicked, this ) );

	// button delete
	pushbutton = static_cast<CEGUI::PushButton *>(wmgr.createWindow( "TaharezLook/Button", "path_segment_delete" ));
	Editor_Add( "-", UTF8_("Delete the selected Segment."), pushbutton, 70, 28, 0 );

	pushbutton->setText( UTF8_("Delete") );
	pushbutton->subscribeEvent( CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber( &cPath::Editor_Button_Delete_Segment_Clicked, this ) );

	// selected segment x1
	editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "path_segment_x1" ));
	Editor_Add( UTF8_("Pos X1"), UTF8_("Line position X1"), editbox, 150 );

	editbox->setValidationString( "[-+]?[0-9]*\\.?[0-9]*" );
	editbox->setText( int_to_string( static_cast<int>(segments[editor_selected_segment].m_x1) ) );
	editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cPath::Editor_Pos_X1_Text_Changed, this ) );

	// selected segment y1
	editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "path_segment_y1" ));
	Editor_Add( UTF8_("Y1"), UTF8_("Line position Y1"), editbox, 150, 28, 0 );

	editbox->setValidationString( "[-+]?[0-9]*\\.?[0-9]*" );
	editbox->setText( int_to_string( static_cast<int>(segments[editor_selected_segment].m_y1) ) );
	editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cPath::Editor_Pos_Y1_Text_Changed, this ) );

	// selected segment x2
	editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "path_segment_x2" ));
	Editor_Add( UTF8_("Pos X2"), UTF8_("Line position X2"), editbox, 150 );

	editbox->setValidationString( "[-+]?[0-9]*\\.?[0-9]*" );
	editbox->setText( int_to_string( static_cast<int>(segments[editor_selected_segment].m_x2) ) );
	editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cPath::Editor_Pos_X2_Text_Changed, this ) );

	// selected segment y2
	editbox = static_cast<CEGUI::Editbox *>(wmgr.createWindow( "TaharezLook/Editbox", "path_segment_y2" ));
	Editor_Add( UTF8_("Y2"), UTF8_("Line position Y2"), editbox, 150, 28, 0 );

	editbox->setValidationString( "[-+]?[0-9]*\\.?[0-9]*" );
	editbox->setText( int_to_string( static_cast<int>(segments[editor_selected_segment].m_y2) ) );
	editbox->subscribeEvent( CEGUI::Editbox::EventTextChanged, CEGUI::Event::Subscriber( &cPath::Editor_Pos_Y2_Text_Changed, this ) );

	// init
	Editor_Init();
}

void cPath :: Editor_State_Update( void )
{
	CEGUI::WindowManager &wmgr = CEGUI::WindowManager::getSingleton();

	// selected segment
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox *>(wmgr.getWindow( "path_selected_segment" ));
	combobox->resetList();

	for( unsigned int count = 0; count < segments.size(); count++ )
	{
		combobox->addItem( new CEGUI::ListboxTextItem( int_to_string( count ) ) );
	}

	combobox->setText( int_to_string( editor_selected_segment ) );

	// Set selected segment values
	// x1
	CEGUI::Editbox *editbox_x1 = static_cast<CEGUI::Editbox *>(wmgr.getWindow( "path_segment_x1" ));
	editbox_x1->setText( float_to_string( segments[editor_selected_segment].m_x1 ) );
	// y1
	CEGUI::Editbox *editbox_y1 = static_cast<CEGUI::Editbox *>(wmgr.getWindow( "path_segment_y1" ));
	editbox_y1->setText( float_to_string( segments[editor_selected_segment].m_y1 ) );
	// x2
	CEGUI::Editbox *editbox_x2 = static_cast<CEGUI::Editbox *>(wmgr.getWindow( "path_segment_x2" ));
	editbox_x2->setText( float_to_string( segments[editor_selected_segment].m_x2 ) );
	// y2
	CEGUI::Editbox *editbox_y2 = static_cast<CEGUI::Editbox *>(wmgr.getWindow( "path_segment_y2" ));
	editbox_y2->setText( float_to_string( segments[editor_selected_segment].m_y2 ) );

	// do not allow to change the start point position
	if( editor_selected_segment == 0 )
	{
		editbox_x1->setEnabled( 0 );
		editbox_y1->setEnabled( 0 );
	}
	else
	{
		editbox_x1->setEnabled( 1 );
		editbox_y1->setEnabled( 1 );
	}
}

bool cPath :: Editor_Identifier_Text_Changed( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	std::string str_text = static_cast<CEGUI::Editbox *>( windowEventArgs.window )->getText().c_str();

	Set_Identifier( str_text );

	return 1;
}

bool cPath :: Editor_Move_Type_Select( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	CEGUI::ListboxItem *item = static_cast<CEGUI::Combobox *>( windowEventArgs.window )->getSelectedItem();
	std::string str_text = item->getText().c_str();

	if( str_text.compare( "mirror" ) == 0 )
	{
		Set_Rewind( 0 );
	}
	else
	{
		Set_Rewind( 1 );
	}

	return 1;
}

bool cPath :: Editor_Selected_Segment_Select( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	CEGUI::ListboxItem *item = static_cast<CEGUI::Combobox *>( windowEventArgs.window )->getSelectedItem();
	editor_selected_segment = string_to_int( item->getText().c_str() );

	// invalid selected segment
	if( editor_selected_segment >= segments.size() )
	{
		editor_selected_segment = segments.size() - 1;
	}

	Editor_State_Update();

	return 1;
}

bool cPath :: Editor_Button_Add_Segment_Clicked( const CEGUI::EventArgs &event )
{
	cPath_Segment new_segment = segments[editor_selected_segment];
	new_segment.Set_Pos( new_segment.m_x2, new_segment.m_y2, new_segment.m_x2 + 20, new_segment.m_y2 - 20 );
	segments.insert( segments.begin() + editor_selected_segment + 1, new_segment );

	editor_selected_segment++;
	Editor_State_Update();

	return 1;
}

bool cPath :: Editor_Button_Delete_Segment_Clicked( const CEGUI::EventArgs &event )
{
	// always keep one
	if( segments.size() == 1 )
	{
		return 1;
	}

	segments.erase( segments.begin() + editor_selected_segment );

	for( PathStateList::iterator itr = linked_path_states.begin(), itr_end = linked_path_states.end(); itr != itr_end; ++itr )
	{
		cPath_State *obj = (*itr);

		if( editor_selected_segment == obj->current_segment )
		{
			if( obj->current_segment > 0 )
			{
				obj->current_segment--;
			}

			obj->Move_From_Segment( obj->current_segment );
		}
	}

	editor_selected_segment--;
	Editor_State_Update();

	return 1;
}

bool cPath :: Editor_Pos_X1_Text_Changed( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	std::string str_text = static_cast<CEGUI::Editbox *>( windowEventArgs.window )->getText().c_str();

	segments[editor_selected_segment].Set_Pos_Start_X( string_to_float( str_text ) );

	Editor_Segment_Pos_Changed();

	return 1;
}

bool cPath :: Editor_Pos_Y1_Text_Changed( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	std::string str_text = static_cast<CEGUI::Editbox *>( windowEventArgs.window )->getText().c_str();

	segments[editor_selected_segment].Set_Pos_Start_Y( string_to_float( str_text ) );

	Editor_Segment_Pos_Changed();

	return 1;
}

bool cPath :: Editor_Pos_X2_Text_Changed( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	std::string str_text = static_cast<CEGUI::Editbox *>( windowEventArgs.window )->getText().c_str();

	segments[editor_selected_segment].Set_Pos_End_X( string_to_float( str_text ) );

	Editor_Segment_Pos_Changed();

	return 1;
}

bool cPath :: Editor_Pos_Y2_Text_Changed( const CEGUI::EventArgs &event )
{
	const CEGUI::WindowEventArgs &windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>( event );
	std::string str_text = static_cast<CEGUI::Editbox *>( windowEventArgs.window )->getText().c_str();

	segments[editor_selected_segment].Set_Pos_End_Y( string_to_float( str_text ) );

	Editor_Segment_Pos_Changed();

	return 1;
}

void cPath :: Editor_Segment_Pos_Changed( void )
{
	for( PathStateList::iterator itr = linked_path_states.begin(), itr_end = linked_path_states.end(); itr != itr_end; ++itr )
	{
		cPath_State *obj = (*itr);

		if( editor_selected_segment == obj->current_segment )
		{
			obj->Move_From_Segment( editor_selected_segment );
		}
	}
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
