/***************************************************************************
 * menu_data.h  -  header for the corresponding cpp file
 *
 * Copyright (C) 2004 - 2009 Florian Richter
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SMC_MENU_DATA_H
#define SMC_MENU_DATA_H

#include "../core/globals.h"
#include "../gui/menu.h"
#include "../gui/hud.h"

namespace SMC
{

/* *** *** *** *** *** *** *** cMenu_Base *** *** *** *** *** *** *** *** *** *** */

class cMenu_Base
{
public:
	cMenu_Base( void );
	virtual ~cMenu_Base( void );

	virtual void Init( void );
	virtual void Init_GUI( void );
	virtual void Update( void );
	virtual void Draw( void );
	void Draw_End( void );

	// Set the game mode to return on exit
	void Set_Exit_To_Game_Mode( GameMode gamemode );
	// Exit
	virtual void Exit( void );

	// gui layout filename
	std::string layout_file;
	// CEGUI window
	CEGUI::Window *guiwindow;

	// if button/key action
	bool action;

	// menu position
	float menu_posy;
	// default text color
	Color text_color;
	// value text color
	Color text_color_value;
	// return to this game mode on exit
	GameMode exit_to_gamemode;

	// current menu sprites
	typedef vector<cHudSprite *> HudSpriteList;
	HudSpriteList drawlist;
};

/* *** *** *** *** *** *** *** cMenu_Main *** *** *** *** *** *** *** *** *** *** */

class cMenu_Main : public cMenu_Base
{
public:
	cMenu_Main( void );
	virtual ~cMenu_Main( void );

	virtual void Init( void );
	virtual void Init_GUI( void );
	virtual void Update( void );
	virtual void Draw( void );

	// Exit
	virtual void Exit( void );
};

/* *** *** *** *** *** *** *** cMenu_Start *** *** *** *** *** *** *** *** *** *** */

class cMenu_Start : public cMenu_Base
{
public:
	cMenu_Start( void );
	virtual ~cMenu_Start( void );

	virtual void Init( void );
	virtual void Init_GUI( void );
	virtual void Update( void );
	virtual void Draw( void );

	// Exit
	virtual void Exit( void );

	// Get all weapons from the given directory
	void Get_Weapons( std::string dir, CEGUI::colour color );

	// Get all levels from the given directory
	void Get_Levels( std::string dir, CEGUI::colour color );

	/* Load the Selected Listbox item
	 * and exit if successful
	*/
	void Load_Selected( void );

	/* Load the Selected World
	 * and exit if successful
	*/
	void Load_World( std::string level_name );
	/* Load the Selected Level
	 * and exit if successful
	*/
	bool Load_Level( std::string name );

	bool Load_Weapon( std::string name);

	// tabcontrol selection changed event
	bool TabControl_Selection_Changed( const CEGUI::EventArgs &event );
	// key down event
	bool TabControl_Keydown( const CEGUI::EventArgs &event );
	// listbox level/world key down event
	bool Listbox_Keydown( const CEGUI::EventArgs &event );
	// listbox level/world character key event
	bool Listbox_Character_Key( const CEGUI::EventArgs &event );

	// world selected event
	bool World_Select( const CEGUI::EventArgs &event );
	// world selected for entering event
	bool World_Select_final_list( const CEGUI::EventArgs &event );

	// level selected event
	bool Level_Select( const CEGUI::EventArgs &event );
	// level selected for entering event
	bool Level_Select_Final_List( const CEGUI::EventArgs &event );

	bool Weapon_Select( const CEGUI::EventArgs &event);
	bool Weapon_Select_Final_List( const CEGUI::EventArgs &event );

	// level new button event
	bool Button_Level_New_Clicked( const CEGUI::EventArgs &event );
	// level edit button event
	bool Button_Level_Edit_Clicked( const CEGUI::EventArgs &event );
	// level delete button event
	bool Button_Level_Delete_Clicked( const CEGUI::EventArgs &event );
	// enter button event
	bool Button_Enter_Clicked( const CEGUI::EventArgs &event );
	// back button event
	bool Button_Back_Clicked( const CEGUI::EventArgs &event );

	// buffer if user types characters in the listbox
	CEGUI::String listbox_search_buffer;
	// counter until buffer is cleared
	float listbox_search_buffer_counter;
};

/* *** *** *** *** *** *** *** cMenu_Options *** *** *** *** *** *** *** *** *** *** */

class cMenu_Options : public cMenu_Base
{
public:
	cMenu_Options( void );
	virtual ~cMenu_Options( void );

	virtual void Init( void );
	virtual void Init_GUI( void );
	virtual void Update( void );
	virtual void Draw( void );

	// Exit
	virtual void Exit( void );
};

/* *** *** *** *** *** *** *** cMenu_Options_Game *** *** *** *** *** *** *** *** *** *** */

class cMenu_Options_Game : public cMenu_Options
{
public:
	cMenu_Options_Game( void );
	virtual ~cMenu_Options_Game( void );

	virtual void Init( void );
	virtual void Init_GUI( void );
	virtual void Update( void );
	virtual void Draw( void );

	// always run option selected event
	bool Always_Run_Select( const CEGUI::EventArgs &event );
	// camera horizontal value changed event
	bool Camera_Hor_Select( const CEGUI::EventArgs &event );
	// camera vertical value changed event
	bool Camera_Ver_Select( const CEGUI::EventArgs &event );
	// language option selected event
	bool Language_Select( const CEGUI::EventArgs &event );
	// menu level option selected event
	bool Menu_Level_Select( const CEGUI::EventArgs &event );
	// menu level text changed event
	bool Menu_Level_Text_Changed( const CEGUI::EventArgs &event );
	// editor show item images option selected event
	bool Editor_Show_Item_Images_Select( const CEGUI::EventArgs &event );
	// editor item image size value changed event
	bool Editor_Item_Image_Size_Select( const CEGUI::EventArgs &event );
	// editor auto hide mouse option selected event
	bool Editor_Auto_Hide_Mouse_Select( const CEGUI::EventArgs &event );
	// Button reset game clicked event
	bool Button_Reset_Game_Clicked( const CEGUI::EventArgs &event );
	// Button reset editor clicked event
	bool Button_Reset_Editor_Clicked( const CEGUI::EventArgs &event );

	// game
	CEGUI::Combobox *combo_always_run;
	CEGUI::Spinner *spinner_camera_hor_speed;
	CEGUI::Spinner *spinner_camera_ver_speed;
	CEGUI::Combobox *combo_language;
	CEGUI::Combobox *combo_menu_level;
	// editor
	CEGUI::Combobox *combo_editor_show_item_images;
	CEGUI::Spinner *spinner_editor_item_image_size;
	CEGUI::Combobox *combo_editor_mouse_auto_hide;
};

/* *** *** *** *** *** *** *** cMenu_Options_Video *** *** *** *** *** *** *** *** *** *** */

class cMenu_Options_Video : public cMenu_Options
{
public:
	cMenu_Options_Video( void );
	virtual ~cMenu_Options_Video( void );

	virtual void Init( void );
	virtual void Init_GUI( void );
	virtual void Update( void );
	virtual void Draw( void );

	// Resolution option selected event
	bool Res_Select( const CEGUI::EventArgs &event );
	// Bpp option selected event
	bool Bpp_Select( const CEGUI::EventArgs &event );
	// Fullscreen option selected event
	bool Fullscreen_Select( const CEGUI::EventArgs &event );
	// Vsync option selected event
	bool Vsync_Select( const CEGUI::EventArgs &event );
	// Geometry quality value changed event
	bool Slider_Geometry_Quality_Changed( const CEGUI::EventArgs &event );
	// Texture quality value changed event
	bool Slider_Texture_Quality_Changed( const CEGUI::EventArgs &event );
	// Button reset clicked event
	bool Button_Reset_Clicked( const CEGUI::EventArgs &event );
	// Button apply clicked event
	bool Button_Apply_Clicked( const CEGUI::EventArgs &event );
	// Button recreate cache clicked event
	bool Button_Recreate_Cache_Clicked( const CEGUI::EventArgs &event );

	CEGUI::Combobox *combo_resolution;
	CEGUI::Combobox *combo_bpp;
	CEGUI::Combobox *combo_fullscreen;
	CEGUI::Combobox *combo_vsync;
	CEGUI::Slider *slider_geometry_quality;
	CEGUI::Slider *slider_texture_quality;

	// video settings
	unsigned int vid_w;
	unsigned int vid_h;
	unsigned int vid_bpp;
	bool vid_fullscreen;
	bool vid_vsync;
	float vid_geometry_detail;
	float vid_texture_detail;
};

/* *** *** *** *** *** *** *** cMenu_Options_Audio *** *** *** *** *** *** *** *** *** *** */

class cMenu_Options_Audio : public cMenu_Options
{
public:
	cMenu_Options_Audio( void );
	virtual ~cMenu_Options_Audio( void );

	virtual void Init( void );
	virtual void Init_GUI( void );
	virtual void Update( void );
	virtual void Draw( void );

	// Audio Hz option selected event
	bool Audio_Hz_Select( const CEGUI::EventArgs &event );
	// music option selected event
	bool Music_Select( const CEGUI::EventArgs &event );
	// music volume changed event
	bool Music_Vol_Changed( const CEGUI::EventArgs &event );
	// sound option selected event
	bool Sound_Select( const CEGUI::EventArgs &event );
	// sound volume changed event
	bool Sound_Vol_Changed( const CEGUI::EventArgs &event );
	// Button reset clicked event
	bool Button_Reset_Clicked( const CEGUI::EventArgs &event );

	CEGUI::Combobox *combo_audio_hz;
	CEGUI::Combobox *combo_music;
	CEGUI::Slider *slider_music;
	CEGUI::Combobox *combo_sounds;
	CEGUI::Slider *slider_sound;
};

/* *** *** *** *** *** *** *** cMenu_Options_Controls *** *** *** *** *** *** *** *** *** *** */

class cMenu_Options_Controls : public cMenu_Options
{
public:
	cMenu_Options_Controls( void );
	virtual ~cMenu_Options_Controls( void );

	virtual void Init( void );
	virtual void Init_GUI( void );
	virtual void Update( void );
	virtual void Draw( void );

	/* Build the shortcut list
	 * joystick : if true build it for joystick
	*/
	void Build_Shortcut_List( bool joystick = 0 );
	/* Set the given Shortcut
	 * and exit if successful
	 * joystick : if true set it for joystick
	*/
	void Set_Shortcut( std::string shortcut_name, input_identifier shortcut_id, bool joystick = 0 );

	// Select given Joystick
	void Joy_Default( unsigned int index );
	// Disable Joystick
	void Joy_Disable( void );

	// keyboard listbox item double clicked event
	bool Keyboard_List_Double_Click( const CEGUI::EventArgs &event );
	// keyboard scroll speed value changed event
	bool Keyboard_Slider_Scroll_Speed_Changed( const CEGUI::EventArgs &event );
	// joystick name click event
	bool Joystick_Name_Click( const CEGUI::EventArgs &event );
	// joystick analog jump click event
	bool Joystick_Analog_Jump_Click( const CEGUI::EventArgs &event );
	// joystick name option selected event
	bool Joystick_Name_Select( const CEGUI::EventArgs &event );
	// joystick analog jump option selected event
	bool Joystick_Analog_Jump_Select( const CEGUI::EventArgs &event );
	// joystick axis horizontal changed event
	bool Joystick_Spinner_Axis_Hor_Changed( const CEGUI::EventArgs &event );
	// joystick axis vertical changed event
	bool Joystick_Spinner_Axis_Ver_Changed( const CEGUI::EventArgs &event );
	// joystick listbox item double clicked event
	bool Joystick_List_Double_Click( const CEGUI::EventArgs &event );
	// Button reset keyboard clicked event
	bool Button_Reset_Keyboard_Clicked( const CEGUI::EventArgs &event );
	// Button reset joystick clicked event
	bool Button_Reset_Joystick_Clicked( const CEGUI::EventArgs &event );

	// Shortcut item
	class cShortcut_item
	{
	public:
		cShortcut_item( const CEGUI::String &name, const input_identifier id )
		{
			m_name = name;
			m_id = id;
		}

		CEGUI::String m_name;
		input_identifier m_id;
	};
};

/* *** *** *** *** *** *** *** cMenu_Savegames *** *** *** *** *** *** *** *** *** *** */

class cMenu_Savegames : public cMenu_Base
{
public:
	cMenu_Savegames( bool ntype_save );
	virtual ~cMenu_Savegames( void );

	virtual void Init( void );
	virtual void Init_GUI( void );
	virtual void Update( void );
	virtual void Draw( void );

	// Exit
	virtual void Exit( void );

	void Update_Load( void );
	void Update_Save( void );

	// Set Savegame Description
	std::string Set_Save_Description( unsigned int save_slot );
	// Update Savegame Descriptions
	void Update_Saved_Games_Text( void );

	// Savegame images
	HudSpriteList savegame_temp;

	// if save menu
	bool type_save;
};

/* *** *** *** *** *** *** *** cMenu_Credits *** *** *** *** *** *** *** *** *** *** */

class cMenu_Credits : public cMenu_Base
{
public:
	cMenu_Credits( void );
	virtual ~cMenu_Credits( void );

	virtual void Init( void );
	virtual void Init_GUI( void );
	virtual void Update( void );
	virtual void Draw( void );

	// Exit
	virtual void Exit( void );

	// fade the credits menu in or out
	void Fade( bool fadein = 1 );
};

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC

#endif
