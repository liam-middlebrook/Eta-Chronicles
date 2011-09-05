/***************************************************************************
 * worlds.cpp  -  class for handling worlds data
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
 
#include "../overworld/world_manager.h"
#include "../core/game_core.h"
#include "../overworld/overworld.h"
#include "../core/filesystem/filesystem.h"
#include "../core/filesystem/resource_manager.h"
// boost filesystem
#include "boost/filesystem/convenience.hpp"
namespace fs = boost::filesystem;
// CEGUI
#include "CEGUIXMLParser.h"

namespace SMC
{

/* *** *** *** *** *** *** *** *** cOverworld_Manager *** *** *** *** *** *** *** *** *** */

cOverworld_Manager :: cOverworld_Manager( void )
: cObject_Manager<cOverworld>()
{
	worlds_filename = DATA_DIR "/" GAME_OVERWORLD_DIR "/worlds.xml";

	debugmode = 0;
	draw_layer = 0;
	cameramode = 0;

	camera = new cCamera();

	Load();
}

cOverworld_Manager :: ~cOverworld_Manager( void )
{
	Delete_All();

	delete camera;
}

bool cOverworld_Manager :: New( const std::string &name )
{
	// no name given
	if( name.empty() )
	{
		return 0;
	}

	// already available
	if( Dir_Exists( pResource_Manager->user_data_dir + USER_WORLD_DIR + "/" + name ) )
	{
		return 0;
	}

	cOverworld *overworld = new cOverworld();

	// set path
	overworld->m_description->path = name;
	// default name is the path
	overworld->m_description->name = name;

	objects.push_back( overworld );

	pActive_Overworld = overworld;

	pOverworld_Player->Reset();

	return 1;
}

void cOverworld_Manager :: Load( void )
{
	// if already loaded
	if( !objects.empty() )
	{
		Delete_All();
	}

	// Load Worlds
	Load_Dir( pResource_Manager->user_data_dir + USER_WORLD_DIR, 1 );
	Load_Dir( DATA_DIR "/" GAME_OVERWORLD_DIR );

	// Get Overworld User Comments
	if( File_Exists( worlds_filename ) )
	{
		// Parse
		CEGUI::System::getSingleton().getXMLParser()->parseXMLFile( *this, worlds_filename.c_str(), DATA_DIR "/" GAME_SCHEMA_DIR "/Worlds_User_Data.xsd", "" );
	}
	else
	{
		// filename not valid
		printf( "Warning : Couldn't open Worlds description file : %s\n", worlds_filename.c_str() );
	}

	// set default overworld
	Set_Active( "World 1" );
}

void cOverworld_Manager :: Load_Dir( const std::string &dir, bool user_dir /* = 0 */ ) 
{
	// set world directory
	fs::path full_path( dir, fs::native );
	fs::directory_iterator end_iter;

	for( fs::directory_iterator dir_itr( full_path ); dir_itr != end_iter; ++dir_itr )
	{
		try
		{
			std::string current_dir = dir_itr->path().leaf();

			// only directories with an existing description
			if( fs::is_directory( *dir_itr ) && File_Exists( dir + "/" + current_dir + "/description.xml" ) )
			{
				// already available
				if( Get_from_Path( current_dir ) )
				{
					continue;
				}

				cOverworld *overworld = new cOverworld();

				// set path
				overworld->m_description->path = current_dir;
				// default name is the path
				overworld->m_description->name = current_dir;
				// set user
				overworld->m_description->user = user_dir;

				objects.push_back( overworld );

				overworld->Load();
			}
		}
		catch( const std::exception &ex )
		{
			printf( "%s %s\n", dir_itr->path().leaf().c_str(), ex.what() );
		}
	}
}

bool cOverworld_Manager :: Set_Active( const std::string &str ) 
{
	return Set_Active( Get( str ) );
}

bool cOverworld_Manager :: Set_Active( cOverworld *world )
{
	if( !world )
	{
		return 0;
	}

	pActive_Overworld = world;

	// set player start waypoint
	pOverworld_Player->Reset();
	pOverworld_Player->Set_Waypoint( pActive_Overworld->m_player_start_waypoint, 1 );

	return 1;
}

void cOverworld_Manager :: Reset( void )
{
	// default Overworld
	Set_Active( "World 1" );

	// Set Player to first Waypoint
	pOverworld_Player->Set_Waypoint( pActive_Overworld->m_player_start_waypoint );

	// Reset all Waypoints
	for( vector<cOverworld *>::iterator itr = objects.begin(), itr_end = objects.end(); itr != itr_end; ++itr )
	{
		(*itr)->Reset_Waypoints();
	}
}

cOverworld *cOverworld_Manager :: Get( const std::string &str )
{
	cOverworld *world = Get_from_Name( str );

	if( world )
	{
		return world;
	}

	return Get_from_Path( str );
}

cOverworld *cOverworld_Manager :: Get_from_Path( const std::string &path )
{
	for( vector<cOverworld *>::iterator itr = objects.begin(), itr_end = objects.end(); itr != itr_end; ++itr )
	{
		cOverworld *obj = (*itr);

		if( obj->m_description->path.compare( path ) == 0 )
		{
			return obj;
		}
	}

	return NULL;
}

cOverworld *cOverworld_Manager :: Get_from_Name( const std::string &name )
{
	for( vector<cOverworld *>::iterator itr = objects.begin(), itr_end = objects.end(); itr != itr_end; ++itr )
	{
		cOverworld *obj = (*itr);

		if( obj->m_description->name.compare( name ) == 0 )
		{
			return obj;
		}
	}

	return NULL;
}

int cOverworld_Manager :: Get_Array_Num( const std::string &path ) const
{
	for( unsigned int i = 0; i < objects.size(); i++ )
	{
		if( objects[i]->m_description->path.compare( path ) == 0 )
		{
			return i;
		}
	}

	return -1;
}

// XML element start
void cOverworld_Manager :: elementStart( const CEGUI::String &element, const CEGUI::XMLAttributes &attributes )
{
	// Property of an Element
	if( element == "Property" )
	{
		xml_attributes.add( attributes.getValueAsString( "Name" ), attributes.getValueAsString( "Value" ) );
	}
}

// XML element end
void cOverworld_Manager :: elementEnd( const CEGUI::String &element )
{
	if( element != "Property" )
	{
		if( element == "World" )
		{
			handle_world( xml_attributes );
		}
		else if( element == "Worlds" )
		{
			// ignore
		}
		else if( element.length() )
		{
			printf( "Warning : Overworld Description Unknown element : %s\n", element.c_str() );
		}

		// clear
		xml_attributes = CEGUI::XMLAttributes();
	}
}

void cOverworld_Manager :: handle_world( const CEGUI::XMLAttributes &attributes )
{
	std::string ow_name = attributes.getValueAsString( "Name" ).c_str();
	std::string ow_comment = attributes.getValueAsString( "Comment" ).c_str();

	// if available
	cOverworld *overworld = Get_from_Name( ow_name );

	// set comment
	if( overworld )
	{
		overworld->m_description->comment = ow_comment;
	}
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// Overworld information handler
cOverworld_Manager *pOverworld_Manager = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
