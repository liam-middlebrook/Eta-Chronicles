/***************************************************************************
 * level_editor.cpp  -  Level Editor class
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

#include "../core/globals.h"
#include "../level/level_editor.h"
#include "../level/level.h"
#include "../core/game_core.h"
#include "../core/sprite_manager.h"
#include "../user/preferences.h"
#include "../input/mouse.h"
#include "../input/keyboard.h"
#include "../audio/audio.h"
#include "../core/i18n.h"
#include "../player/player.h"
#include "../core/filesystem/filesystem.h"

namespace SMC
{

/* *** *** *** *** *** *** *** cEditor_Level *** *** *** *** *** *** *** *** *** *** */

cEditor_Level :: cEditor_Level( void )
: cEditor()
{
	menu_filename = DATA_DIR "/" GAME_EDITOR_DIR "/level_menu.xml";
	items_filename = DATA_DIR "/" GAME_EDITOR_DIR "/level_items.xml";

	editor_item_tag = "level";

	pSettings = new cLevel_Settings();
}

cEditor_Level :: ~cEditor_Level( void )
{
	delete pSettings;
}

void cEditor_Level :: Init( void )
{
	// already loaded
	if( editor_window )
	{
		return;
	}

	// nothing

	cEditor::Init();
}

void cEditor_Level :: Enable( void )
{
	// already enabled
	if( enabled )
	{
		return;
	}

	editor_level_enabled = 1;

	if( Game_Mode == MODE_LEVEL )
	{
		editor_enabled = 1;
	}

	// reset ground object
	// player
	pPlayer->Reset_On_Ground();
	// sprite manager
	for( cSprite_List::iterator itr = pActive_Sprite_Manager->objects.begin(), itr_end = pActive_Sprite_Manager->objects.end(); itr != itr_end; ++itr )
	{
		// get object pointer
		cSprite *obj = (*itr);

		// skip destroyed objects
		if( obj->m_auto_destroy )
		{
			continue;
		}

		// enemies
		if( obj->m_sprite_array == ARRAY_ENEMY )
		{
			cMovingSprite *moving_sprite = static_cast<cMovingSprite *>(obj);

			moving_sprite->Reset_On_Ground();
		}
	}

	cEditor::Enable();
}

void cEditor_Level :: Disable( bool native_mode /* = 0 */ )
{
	// already disabled
	if( !enabled )
	{
		return;
	}

	pHud_Debug->Set_Text( _("Level Editor disabled") );

	editor_level_enabled = 0;

	if( Game_Mode == MODE_LEVEL )
	{
		native_mode = 1;
		editor_enabled = 0;
	}
	
	cEditor::Disable( native_mode );
}

bool cEditor_Level :: Key_Down( SDLKey key )
{
	if( !enabled )
	{
		return 0;
	}


	// check basic editor events
	if( cEditor::Key_Down( key ) )
	{
		return 1;
	}
	// save level
	else if( key == SDLK_s && input_event.key.keysym.mod & KMOD_CTRL )
	{
		pActive_Level->Save();
	}
	// focus last levelexit
	else if( key == SDLK_END )
	{
		float new_cameraposx = 0;
		float new_cameraposy = 0;

		for( cSprite_List::iterator itr = pActive_Sprite_Manager->objects.begin(), itr_end = pActive_Sprite_Manager->objects.end(); itr != itr_end; ++itr )
		{
			cSprite *obj = (*itr);

			if( obj->m_sprite_array != ARRAY_ACTIVE )
			{
				continue;
			}

			if( obj->m_type == TYPE_LEVEL_EXIT && new_cameraposx < obj->m_pos_x )
			{
				new_cameraposx = obj->m_pos_x;
				new_cameraposy = obj->m_pos_y;
			}
		}

		if( new_cameraposx != 0 || new_cameraposy != 0 )
		{
			pActive_Camera->Set_Pos( new_cameraposx - ( game_res_w * 0.5f ), new_cameraposy - ( game_res_h * 0.5f ) );
		}
	}
	// modify selected objects state
	else if( key == SDLK_m )
	{
		if( !pMouseCursor->m_selected_objects.empty() )
		{
			cSprite *mouse_obj = pMouseCursor->m_selected_objects[0]->obj;

			// change state of the base object
			if( Switch_Object_State( mouse_obj ) )
			{
				// change all object states to the base object state
				for( SelectedObjectList::iterator itr = pMouseCursor->m_selected_objects.begin(), itr_end = pMouseCursor->m_selected_objects.end(); itr != itr_end; ++itr )
				{
					cSprite *obj = (*itr)->obj;

					// skip base object
					if( obj == mouse_obj )
					{
						continue;
					}

					// sprites need additional data
					if( obj->m_type == TYPE_PASSIVE || obj->m_type == TYPE_FRONT_PASSIVE || obj->m_type == TYPE_MASSIVE || obj->m_type == TYPE_CLIMBABLE || obj->m_type == TYPE_HALFMASSIVE )
					{
						obj->m_type = mouse_obj->m_type;
						obj->m_sprite_array = mouse_obj->m_sprite_array;
						obj->m_can_be_ground = mouse_obj->m_can_be_ground;
					}
					// special objects
					else if( obj->m_type == TYPE_MOVING_PLATFORM )
					{
						// fall through
					}
					else
					{
						// massivetype change is not valid
						continue;
					}
					
					// set state
					obj->Set_Massive_Type( mouse_obj->m_massive_type );
				}
			}
		}
	}
	// modify mouse object state
	else if( key == SDLK_m && pMouseCursor->m_hovering_object->obj )
	{
		Switch_Object_State( pMouseCursor->m_hovering_object->obj );
		pMouseCursor->Clear_Mouse_Object();
	}
	else
	{
		// not processed
		return 0;
	}

	// key got processed
	return 1;
}

void cEditor_Level :: Activate_Menu_Item( cEditor_Menu_Object *entry )
{
	// If Function
	if( entry->bfunction )
	{
		if( entry->tags.compare( "new" ) == 0 )
		{
			Function_New();
		}
		else if( entry->tags.compare( "load" ) == 0 )
		{
			Function_Load();
		}
		else if( entry->tags.compare( "save" ) == 0 )
		{
			Function_Save();
		}
		else if( entry->tags.compare( "save_as" ) == 0 )
		{
			Function_Save_as();
		}
		else if( entry->tags.compare( "delete" ) == 0 )
		{
			Function_Delete();
		}
		else if( entry->tags.compare( "reload" ) == 0 )
		{
			Function_Reload();
		}
		else if( entry->tags.compare( "clear" ) == 0 )
		{
			Function_Clear();
		}
		else if( entry->tags.compare( "settings" ) == 0 )
		{
			Function_Settings();
		}
		// unknown level function
		else
		{
			cEditor::Activate_Menu_Item( entry );
		}
	}
	// unknown level function
	else
	{
		cEditor::Activate_Menu_Item( entry );
	}
}

bool cEditor_Level :: Switch_Object_State( cSprite *obj ) const
{
	// empty object
	if( !obj )
	{
		return 0;
	}

	// from Passive to Front Passive
	if( obj->m_type == TYPE_PASSIVE )
	{
		obj->Set_Sprite_Type( TYPE_FRONT_PASSIVE );
	}
	// from Front Passive to Massive
	else if( obj->m_type == TYPE_FRONT_PASSIVE )
	{
		obj->Set_Sprite_Type( TYPE_MASSIVE );
	}
	// from Massive to Halfmassive
	else if( obj->m_type == TYPE_MASSIVE )
	{
		obj->Set_Sprite_Type( TYPE_HALFMASSIVE );
	}
	// from Halfmassive to Climbable
	else if( obj->m_type == TYPE_HALFMASSIVE )
	{
		obj->Set_Sprite_Type( TYPE_CLIMBABLE );
	}
	// from Climbable to Passive
	else if( obj->m_type == TYPE_CLIMBABLE )
	{
		obj->Set_Sprite_Type( TYPE_PASSIVE );
	}
	// moving platform
	else if( obj->m_type == TYPE_MOVING_PLATFORM )
	{
		if( obj->m_massive_type == MASS_PASSIVE )
		{
			obj->Set_Massive_Type( MASS_MASSIVE );
		}
		else if( obj->m_massive_type == MASS_MASSIVE )
		{
			obj->Set_Massive_Type( MASS_HALFMASSIVE );
		}
		else if( obj->m_massive_type == MASS_HALFMASSIVE )
		{
			obj->Set_Massive_Type( MASS_CLIMBABLE );
		}
		else if( obj->m_massive_type == MASS_CLIMBABLE )
		{
			obj->Set_Massive_Type( MASS_PASSIVE );
		}
	}
	// invalid object type
	else
	{
		return 0;
	}

	return 1;
}

bool cEditor_Level :: Function_New( void )
{
	std::string level_name = Box_Text_Input( _("Create a new Level"), _("Name") );

	// aborted/invalid
	if( level_name.empty() )
	{
		return 0;
	}

	if( pActive_Level->New( level_name ) )
	{
		pHud_Debug->Set_Text( _("Created ") + level_name );
		return 1;
	}
	else
	{
		pHud_Debug->Set_Text( _("Level ") + level_name + _(" already exists") );
	}

	return 0;
}

void cEditor_Level :: Function_Load( void )
{
	std::string level_name = _("Name");

	// valid level
	while( level_name.length() )
	{
		level_name = Box_Text_Input( level_name, _("Load a Level"), level_name.compare( _("Name") ) == 0 ? 1 : 0 );

		// break if empty
		if( level_name.empty() )
		{
			break;
		}

		// if available
		if( pActive_Level->Get_Path( level_name ) )
		{
			Game_Action = GA_ENTER_LEVEL;
			Game_Mode_Type = MODE_TYPE_LEVEL_CUSTOM;
			Game_Action_Data.add( "level", level_name.c_str() );

			pHud_Debug->Set_Text( _("Loaded ") + Trim_Filename( level_name, 0, 0 ) );

			break;
		}
		// not found
		else
		{
			pAudio->Play_Sound( "error.ogg" );
		}
	}
}

void cEditor_Level :: Function_Save( bool with_dialog /* = 0 */ )
{
	// not loaded
	if( !pActive_Level->Is_Loaded() )
	{
		return;
	}

	// if denied
	if( with_dialog && !Box_Question( _("Save ") + Trim_Filename( pActive_Level->m_level_filename, 0, 0 ) + " ?" ) )
	{
		return;
	}

	pActive_Level->Save();
}

void cEditor_Level :: Function_Save_as( void )
{
	std::string levelname = Box_Text_Input( _("Save Level as"), _("New name"), 1 );

	// aborted/invalid
	if( levelname.empty() )
	{
		return;
	}

	pActive_Level->Set_Levelfile( levelname, 0 );
	pActive_Level->Save();
}

void cEditor_Level :: Function_Delete( void )
{
	std::string filename = pActive_Level->m_level_filename;
	if( !pActive_Level->Get_Path( filename, 1 ) )
	{
		return;
	}

	// if denied
	if( !Box_Question( _("Delete and Unload ") + Trim_Filename( filename, 0, 0 ) + " ?" ) )
	{
		return;
	}

	pActive_Level->Delete();
	Disable();

	Game_Action = GA_ENTER_MENU;
}

void cEditor_Level :: Function_Reload( void )
{
	// if denied
	if( !Box_Question( _("Reload Level ?") ) )
	{
		return;
	}

	pActive_Level->Save();
	if( pActive_Level->Load( Trim_Filename( pActive_Level->data_file, 0 ) ) )
	{
		pActive_Level->Enter();
	}
}

void cEditor_Level :: Function_Clear( void )
{
	// if denied
	if( !Box_Question( _("Clear Level ?") ) )
	{
		return;
	}

	pActive_Sprite_Manager->Delete_All();
}

void cEditor_Level :: Function_Settings( void )
{
	Game_Action = GA_ENTER_LEVEL_SETTINGS;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cEditor_Level *pLevel_Editor = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
