/***************************************************************************
 * sprite.cpp  -  basic sprite class
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

#include "../objects/sprite.h"
#include "../objects/movingsprite.h"
#include "../core/game_core.h"
#include "../level/level.h"
#include "../core/framerate.h"
#include "../player/player.h"
#include "../gui/hud.h"
#include "../video/gl_surface.h"
#include "../video/renderer.h"
#include "../core/sprite_manager.h"
#include "../core/editor.h"

namespace SMC
{

/* *** *** *** *** *** *** *** *** cCollidingSprite *** *** *** *** *** *** *** *** *** */

cCollidingSprite :: cCollidingSprite( void )
{

}

cCollidingSprite :: ~cCollidingSprite( void )
{
	Clear_Collisions();
}

void cCollidingSprite :: Handle_Collisions( void )
{
	// get collision list
	cObjectCollision_List col_list;
	collisions.swap( col_list );

	// parse the given collisions
	for( cObjectCollision_List::iterator itr = col_list.begin(), itr_end = col_list.end(); itr != itr_end; ++itr )
	{
		// handle it
		Handle_Collision( (*itr) );
	}

	// clear
	for( cObjectCollision_List::iterator itr = col_list.begin(), itr_end = col_list.end(); itr != itr_end; ++itr )
	{
		delete *itr;
	}

	col_list.clear();
}

cObjectCollision *cCollidingSprite :: Create_Collision_Object( const cSprite *base, cSprite *col, Col_Valid_Type valid_type ) const
{
	// if invalid
	if( !base || valid_type == COL_VTYPE_NOT_VALID )
	{
		return NULL;
	}

	// create
	cObjectCollision *collision = new cObjectCollision();

	// if col object is available
	if( col )
	{
		// object
		collision->obj = col;
		// identifier
		if( col->m_sprite_array != ARRAY_PLAYER )
		{
			collision->number = pActive_Sprite_Manager->Get_Array_Num( col );
		}
		// type
		collision->m_array = col->m_sprite_array;
		// direction 
		collision->Set_Direction( base, col );
	}

	// valid type
	collision->valid_type = valid_type;

	return collision;
}

bool cCollidingSprite :: Add_Collision( cObjectCollision *collision, bool add_if_new /* = 0 */ )
{
	// invalid collision data
	if( !collision )
	{
		return 0;
	}

	// check if collision data is new
	if( add_if_new )
	{
		// already in list
		if( Is_Collision_Included( collision->obj ) )
		{
			delete collision;
			return 0;
		}
	}

	collisions.push_back( collision );

	return 1;
}

void cCollidingSprite :: Add_Collisions( cObjectCollisionType *col_list, bool add_if_new /* = 0 */ )
{
	// insert all objects
	for( cObjectCollision_List::iterator itr = col_list->objects.begin(), itr_end = col_list->objects.end(); itr != itr_end; ++itr )
	{
		cObjectCollision *col = (*itr);

		Add_Collision( col, add_if_new );
	}

	col_list->objects.clear();
}

void cCollidingSprite :: Delete_Collision( cObjectCollision *collision )
{
	if( !collision )
	{
		return;
	}

	// get iterator
	cObjectCollision_List::iterator itr = std::find( collisions.begin(), collisions.end(), collision );

	// not available
	if( itr == collisions.end() )
	{
		return;
	}

	// erase from list
	collisions.erase( itr );
	// delete
	delete collision;
}

void cCollidingSprite :: Delete_Last_Collision( void )
{
	if( collisions.empty() )
	{
		return;
	}
	
	cObjectCollision_List::iterator end_itr = collisions.end() - 1;

	delete *end_itr;
	collisions.erase( end_itr );
}

int cCollidingSprite :: Is_Collision_In_Direction( const ObjectDirection dir ) const
{
	int pos = 0;

	for( cObjectCollision_List::const_iterator itr = collisions.begin(), itr_end = collisions.end(); itr != itr_end; ++itr )
	{
		cObjectCollision *col = (*itr);

		if( col->direction == dir )
		{
			return pos;
		}

		pos++;
	}

	return -1;
}

cObjectCollision *cCollidingSprite :: Get_First_Collision( void ) const
{
	// no collision available
	if( collisions.empty() )
	{
		return NULL;	
	}

	return *collisions.begin();
}

cObjectCollision *cCollidingSprite :: Get_Last_Collision( bool only_blocking /* = 0 */ ) const
{
	// no collisions available
	if( collisions.empty() )
	{
		return NULL;
	}

	if( only_blocking )
	{
		for( cObjectCollision_List::const_reverse_iterator itr = collisions.rbegin(), itr_end = collisions.rend(); itr != itr_end; ++itr )
		{
			// get object pointer
			cObjectCollision *col = (*itr);

			cSprite *col_obj = pActive_Sprite_Manager->Get_Pointer( col->number );

			// ignore passive
			if( col_obj->m_massive_type == MASS_PASSIVE )
			{
				continue;
			}
			// if active check if not climbable
			if( col->m_array == ARRAY_ACTIVE )
			{
				// not a valid object
				if( col_obj->m_massive_type == MASS_CLIMBABLE )
				{
					continue;
				}
			}

			// is blocking
			return col;
		}

		// not found
		return NULL;
	}

	return *(collisions.end() - 1);
}

bool cCollidingSprite :: Is_Collision_Included( const cSprite *obj ) const
{
	// check if in collisions list
	for( cObjectCollision_List::const_iterator itr = collisions.begin(), itr_end = collisions.end(); itr != itr_end; ++itr )
	{
		// get object pointer
		cObjectCollision *col = (*itr);

		// is in list
		if( col->obj == obj )
		{
			return 1;
		}
	}

	// not found
	return 0;
}

void cCollidingSprite :: Clear_Collisions( void )
{
	for( cObjectCollision_List::iterator itr = collisions.begin(), itr_end = collisions.end(); itr != itr_end; ++itr )
	{
		delete *itr;
	}

	collisions.clear();
}

void cCollidingSprite :: Handle_Collision( cObjectCollision *collision )
{
	// player
	if( collision->m_array == ARRAY_PLAYER )
	{
		Handle_Collision_Player( collision );
	}
	// enemy
	else if( collision->m_array == ARRAY_ENEMY )
	{
		Handle_Collision_Enemy( collision );
	}
	// massive
	else if( collision->m_array == ARRAY_MASSIVE || collision->m_array == ARRAY_ACTIVE )
	{
		Handle_Collision_Massive( collision );
	}
	// passive
	else if( collision->m_array == ARRAY_PASSIVE )
	{
		Handle_Collision_Passive( collision );
	}
}

void cCollidingSprite :: Handle_Collision_Player( cObjectCollision *collision )
{
	// virtual
}

void cCollidingSprite :: Handle_Collision_Enemy( cObjectCollision *collision )
{
	// virtual
}

void cCollidingSprite :: Handle_Collision_Massive( cObjectCollision *collision )
{
	// virtual
}

void cCollidingSprite :: Handle_Collision_Passive( cObjectCollision *collision )
{
	// virtual
}

void cCollidingSprite :: Handle_Collision_Box( ObjectDirection direction, GL_rect *r2 )
{
	// virtual
}

/* *** *** *** *** *** *** *** cSprite *** *** *** *** *** *** *** *** *** *** */

cSprite :: cSprite( cGL_Surface *new_image /* = NULL */, float x /* = 0 */, float y /* = 0 */, bool del_img /* = 0 */ )
{
	cSprite::Init();

	if( new_image )
	{
		cSprite::Set_Image( new_image, 0, del_img );
	}

	Set_Pos( x, y );
}

cSprite :: cSprite( CEGUI::XMLAttributes &attributes )
{
	cSprite::Init();
	cSprite::Create_From_Stream( attributes );
}

cSprite :: ~cSprite( void )
{
	if( m_delete_image && m_image )
	{
		delete m_image;
		m_image = NULL;
	}
}

void cSprite :: Init( void )
{
	// undefined
	m_type = TYPE_UNDEFINED;
	m_sprite_array = ARRAY_UNDEFINED;

	// collision data
	m_col_pos.m_x = 0;
	m_col_pos.m_y = 0;
	m_col_rect.m_x = 0;
	m_col_rect.m_y = 0;
	m_col_rect.m_w = 0;
	m_col_rect.m_h = 0;
	// image data
	m_rect.m_x = 0;
	m_rect.m_y = 0;
	m_rect.m_w = 0;
	m_rect.m_h = 0;
	m_start_rect.clear();

	m_start_pos_x = 0;
	m_start_pos_y = 0;

	m_start_image = NULL;
	m_image = NULL;
	m_auto_destroy = 0;
	m_delete_image = 0;
	m_shadow_pos = 0;
	m_shadow_color = black;
	m_no_camera = 0;

	m_color = static_cast<Uint8>(255);

	m_combine_type = 0;
	m_combine_color[0] = 0;
	m_combine_color[1] = 0;
	m_combine_color[2] = 0;

	m_pos_x = 0;
	m_pos_y = 0;
	m_pos_z = 0;
	m_editor_pos_z = 0;

	m_massive_type = MASS_PASSIVE;
	m_active = 1;
	m_spawned = 0;
	m_player_range = 1000;
	m_can_be_ground = 0;

	// rotation
	m_rotation_affects_rect = 0;
	m_start_rot_x = 0;
	m_start_rot_y = 0;
	m_start_rot_z = 0;
	m_rot_x = 0;
	m_rot_y = 0;
	m_rot_z = 0;
	// scale
	m_scale_affects_rect = 0;
	m_scale_up = 0;
	m_scale_down = 1;
	m_scale_left = 0;
	m_scale_right = 1;
	m_start_scale_x = 1;
	m_start_scale_y = 1;
	m_scale_x = 1;
	m_scale_y = 1;

	m_valid_draw = 1;
	m_valid_update = 1;

	m_editor_window_name_width = 0;
}

void cSprite :: Init_Links( void )
{
	// virtual
}

cSprite *cSprite :: Copy( void )
{
	cSprite *basic_sprite = new cSprite( m_start_image, m_start_pos_x, m_start_pos_y );

	basic_sprite->m_type = m_type;
	basic_sprite->m_sprite_array = m_sprite_array;
	basic_sprite->Set_Massive_Type( m_massive_type );
	basic_sprite->m_can_be_ground = m_can_be_ground;
	basic_sprite->Set_Rotation_Affects_Rect( m_rotation_affects_rect );
	basic_sprite->Set_Scale_Affects_Rect( m_scale_affects_rect );
	basic_sprite->Set_Scale_Directions( m_scale_up, m_scale_down, m_scale_left, m_scale_right );
	basic_sprite->Set_Ignore_Camera( m_no_camera );
	basic_sprite->Set_Shadow_Pos( m_shadow_pos );
	basic_sprite->Set_Shadow_Color( m_shadow_color );
	basic_sprite->m_spawned = m_spawned;

	return basic_sprite;
}

void cSprite :: Create_From_Stream( CEGUI::XMLAttributes &attributes )
{
	// position
	Set_Pos( static_cast<float>(attributes.getValueAsInteger( "posx" )), static_cast<float>(attributes.getValueAsInteger( "posy" )), 1 );
	// image
	Set_Image( pVideo->Get_Surface( attributes.getValueAsString( "image" ).c_str() ), 1 ) ;
	// type
	Set_Sprite_Type( Get_Sprite_Type_Id( attributes.getValueAsString( "type" ).c_str() ) );
}

void cSprite :: Save_To_Stream( ofstream &file )
{
	// begin sprite
	file << "\t<sprite>" << std::endl;

	// position
	file << "\t\t<Property name=\"posx\" value=\"" << static_cast<int>(m_start_pos_x) << "\" />" << std::endl;
	file << "\t\t<Property name=\"posy\" value=\"" << static_cast<int>(m_start_pos_y) << "\" />" << std::endl;
	// image
	std::string img_filename;

	if( m_start_image )
	{
		img_filename = m_start_image->m_filename;
	}
	else if( m_image )
	{
		img_filename = m_image->m_filename;
	}
	else
	{
		printf( "Warning: cSprite::Save_To_Stream no image from type %d\n", m_type );
	}

	// remove pixmaps directory from string
	if( img_filename.find( DATA_DIR "/" GAME_PIXMAPS_DIR "/" ) == 0 )
	{
		img_filename.erase( 0, strlen( DATA_DIR "/" GAME_PIXMAPS_DIR "/" ) );
	}
	file << "\t\t<Property name=\"image\" value=\"" << img_filename << "\" />" << std::endl;
	// type
	file << "\t\t<Property name=\"type\" value=\"" << Get_Sprite_Type_String() << "\" />" << std::endl;

	// end sprite
	file << "\t</sprite>" << std::endl;
}

void cSprite :: Load_From_Savegame( cSave_Level_Object *save_object )
{
	// virtual
}

cSave_Level_Object *cSprite :: Save_To_Savegame( void )
{
	return NULL;
}

void cSprite :: Set_Image( cGL_Surface *new_image, bool new_start_image /* = 0 */, bool del_img /* = 0 */ )
{
	if( m_delete_image )
	{
		if( m_image )
		{
			// if same image reset start_image
			if( m_start_image == m_image )
			{
				m_start_image = NULL;
			}

			delete m_image;
			m_image = NULL;
		}

		m_delete_image = 0;
	}

	m_image = new_image;

	if( m_image )
	{
		// collision data
		m_col_pos = m_image->m_col_pos;
		// scale affects the rect
		if( m_scale_affects_rect )
		{
			m_col_rect.m_w = m_image->m_col_w * m_scale_x;
			m_col_rect.m_h = m_image->m_col_h * m_scale_y;
			// image data
			m_rect.m_w = m_image->m_w * m_scale_x;
			m_rect.m_h = m_image->m_h * m_scale_y;
		}
		// scale does not affect the rect
		else
		{
			m_col_rect.m_w = m_image->m_col_w;
			m_col_rect.m_h = m_image->m_col_h;
			// image data
			m_rect.m_w = m_image->m_w;
			m_rect.m_h = m_image->m_h;
		}
		// rotation affects the rect
		if( m_rotation_affects_rect )
		{
			Update_Rect_Rotation();
		}

		m_delete_image = del_img;

		// if no name is set use the first image name
		if( !m_name.length() )
		{
			m_name = m_image->m_name;
		}
		// if no editor tags are set use the first image editor tags
		if( !m_editor_tags.length() )
		{
			m_editor_tags = m_image->m_editor_tags;
		}
	}
	else
	{
		// clear image data
		m_col_pos.m_x = 0;
		m_col_pos.m_y = 0;
		m_col_rect.m_w = 0;
		m_col_rect.m_h = 0;
		m_rect.m_w = 0;
		m_rect.m_h = 0;
	}

	if( !m_start_image || new_start_image )
	{
		m_start_image = new_image;

		if( new_image )
		{
			m_start_rect.m_w = new_image->m_w;
			m_start_rect.m_h = new_image->m_h;
		}
		else
		{
			m_start_rect.m_w = 0;
			m_start_rect.m_h = 0;
		}
	}
	
	// because col_pos could have changed
	Update_Position_Rect();
}

void cSprite :: Set_Sprite_Type( SpriteType ntype )
{
	// set first because of massivetype z calculation
	m_type = ntype;

	if( ntype == TYPE_MASSIVE )
	{
		m_sprite_array = ARRAY_MASSIVE;
		Set_Massive_Type( MASS_MASSIVE );
		m_can_be_ground = 1;
	}
	else if( ntype == TYPE_PASSIVE )
	{
		m_sprite_array = ARRAY_PASSIVE;
		Set_Massive_Type( MASS_PASSIVE );
		m_can_be_ground = 0;
	}
	else if( ntype == TYPE_FRONT_PASSIVE )
	{
		m_sprite_array = ARRAY_PASSIVE;
		Set_Massive_Type( MASS_PASSIVE );
		m_can_be_ground = 0;
	}
	else if( ntype == TYPE_HALFMASSIVE )
	{
		m_sprite_array = ARRAY_ACTIVE;
		Set_Massive_Type( MASS_HALFMASSIVE );
		m_can_be_ground = 1;
	}
	else if( ntype == TYPE_CLIMBABLE )
	{
		m_sprite_array = ARRAY_ACTIVE;
		Set_Massive_Type( MASS_CLIMBABLE );
		m_can_be_ground = 0;
	}
}

std::string cSprite :: Get_Sprite_Type_String( void ) const
{
	if( m_sprite_array == ARRAY_UNDEFINED )
	{
		return "undefined";
	}
	else if( m_sprite_array == ARRAY_PASSIVE )
	{
		if( m_type == TYPE_FRONT_PASSIVE )
		{
			return "front_passive";
		}

		return "passive";
	}
	else if( m_sprite_array == ARRAY_ACTIVE )
	{
		if( m_type == TYPE_HALFMASSIVE )
		{
			return "halfmassive";
		}
		else if( m_type == TYPE_CLIMBABLE )
		{
			return "climbable";
		}
		else
		{
			printf( "Warning : Sprite array set as active but unknown type %d\n", m_type );
			return "active";
		}
	}
	else if( m_sprite_array == ARRAY_MASSIVE )
	{
		return "massive";
	}
	else if( m_sprite_array == ARRAY_HUD )
	{
		return "hud";
	}
	else if( m_sprite_array == ARRAY_ANIM )
	{
		return "animation";
	}
	else
	{
		printf( "Warning : Sprite unknown array %d\n", m_sprite_array );
	}

	return "";
}

void cSprite :: Set_Ignore_Camera( bool enable /* = 0 */ )
{
	// already set
	if( m_no_camera == enable )
	{
		return;
	}

	m_no_camera = enable;

	Update_Valid_Draw();
}

void cSprite :: Set_Pos( float x, float y, bool new_startpos /* = 0 */ )
{
	m_pos_x = x;
	m_pos_y = y;

	if( new_startpos || ( m_start_pos_x == 0 && m_start_pos_y == 0 ) )
	{
		m_start_pos_x = x;
		m_start_pos_y = y;
	}

	Update_Position_Rect();
}

void cSprite :: Set_Pos_X( float x, bool new_startpos /* = 0 */ )
{
	m_pos_x = x;

	if( new_startpos )
	{
		m_start_pos_x = x;
	}

	Update_Position_Rect();
}

void cSprite :: Set_Pos_Y( float y, bool new_startpos /* = 0 */ )
{
	m_pos_y = y;

	if( new_startpos )
	{
		m_start_pos_y = y;
	}

	Update_Position_Rect();
}

void cSprite :: Set_Active( bool enabled )
{
	// already set
	if( m_active == enabled )
	{
		return;
	}

	m_active = enabled;

	Update_Valid_Draw();
	Update_Valid_Update();
}

void cSprite :: Set_Shadow( const Color &shadow, float pos )
{
	Set_Shadow_Pos( pos );
	Set_Shadow_Color( shadow );
}

void cSprite :: Set_Shadow_Pos( float pos )
{
	m_shadow_pos = pos;
}

void cSprite :: Set_Shadow_Color( const Color &shadow )
{
	m_shadow_color = shadow;
}

void cSprite :: Set_Color( Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha /* = 255 */ )
{
	m_color.red = red;
	m_color.green = green;
	m_color.blue = blue;
	m_color.alpha = alpha;
}

void cSprite :: Set_Color( const Color &col )
{
	m_color = col;
}

void cSprite :: Set_Color_Combine( float red, float green, float blue, GLint com_type )
{
	m_combine_type = com_type;
	m_combine_color[0] = red;
	m_combine_color[1] = green;
	m_combine_color[2] = blue;

	// red limits
	if( m_combine_color[0] > 1.0f )
	{
		m_combine_color[0] = 1.0f;
	}
	else if( m_combine_color[0] <= 0.0f )
	{
		m_combine_color[0] = 0.000001f;
	}
	// green limits
	if( m_combine_color[1] > 1.0f )
	{
		m_combine_color[1] = 1.0f;
	}
	else if( m_combine_color[1] <= 0.0f )
	{
		m_combine_color[1] = 0.000001f;
	}
	// blue limits
	if( m_combine_color[2] > 1.0f )
	{
		m_combine_color[2] = 1.0f;
	}
	else if( m_combine_color[2] <= 0.0f )
	{
		m_combine_color[2] = 0.000001f;
	}
}

void cSprite :: Set_Rotation_Affects_Rect( bool enable /* = 0 */ )
{
	m_rotation_affects_rect = enable;
}

void cSprite :: Update_Rect_Rotation( void )
{
	// Z must be first for correct rotation ( see Eato )
	Update_Rect_Rotation_Z();
	Update_Rect_Rotation_X();
	Update_Rect_Rotation_Y();
}

void cSprite :: Update_Rect_Rotation_X( void )
{
	// mirror
	if( m_rot_x >= 180.0f )
	{
		m_col_pos.m_y = m_rect.m_h - ( m_col_rect.m_h + m_col_pos.m_y );
	}
}

void cSprite :: Update_Rect_Rotation_Y( void )
{
	// mirror
	if( m_rot_y >= 180.0f )
	{
		m_col_pos.m_x = m_rect.m_w - ( m_col_rect.m_w + m_col_pos.m_x );
	}
}

void cSprite :: Update_Rect_Rotation_Z( void )
{
	// rotate 270�
	if( m_rot_z >= 270.0f )
	{
		// rotate collision position
		float orig_x = m_col_pos.m_x;
		m_col_pos.m_x = m_col_pos.m_y;
		m_col_pos.m_y = orig_x;

		// switch width and height
		float orig_w = m_rect.m_w;
		m_rect.m_w = m_rect.m_h;
		m_rect.m_h = orig_w;
		// switch collision width and height
		float orig_col_w = m_col_rect.m_w;
		m_col_rect.m_w = m_col_rect.m_h;
		m_col_rect.m_h = orig_col_w;
	}
	// mirror
	else if( m_rot_z >= 180.0f )
	{
		m_col_pos.m_x = m_rect.m_w - ( m_col_rect.m_w + m_col_pos.m_x );
		m_col_pos.m_y = m_rect.m_h - ( m_col_rect.m_h + m_col_pos.m_y );
	}
	// rotate 90�
	else if( m_rot_z >= 0.00001f )
	{
		// rotate collision position
		float orig_x = m_col_pos.m_x;
		m_col_pos.m_x = m_rect.m_h - ( m_col_rect.m_h + m_col_pos.m_y );
		m_col_pos.m_y = orig_x;

		// switch width and height
		float orig_w = m_rect.m_w;
		m_rect.m_w = m_rect.m_h;
		m_rect.m_h = orig_w;
		// switch collision width and height
		float orig_col_w = m_col_rect.m_w;
		m_col_rect.m_w = m_col_rect.m_h;
		m_col_rect.m_h = orig_col_w;
	}
}

void cSprite :: Set_Rotation_X( float rot, bool new_start_rot /* = 0 */ )
{
	m_rot_x = fmod( rot, 360.0f );

	if( new_start_rot )
	{
		m_start_rot_x = m_rot_x;
	}

	if( m_rotation_affects_rect )
	{
		Update_Rect_Rotation_X();
	}
}

void cSprite :: Set_Rotation_Y( float rot, bool new_start_rot /* = 0 */ )
{
	m_rot_y = fmod( rot, 360.0f );

	if( new_start_rot )
	{
		m_start_rot_y = m_rot_y;
	}

	if( m_rotation_affects_rect )
	{
		Update_Rect_Rotation_Y();
	}
}

void cSprite :: Set_Rotation_Z( float rot, bool new_start_rot /* = 0 */ )
{
	m_rot_z = fmod( rot, 360.0f );

	if( new_start_rot )
	{
		m_start_rot_z = m_rot_z;
	}

	if( m_rotation_affects_rect )
	{
		Update_Rect_Rotation_Z();
	}
}

void cSprite :: Set_Rotation( float x, float y, float z, bool new_start_rot /* = 0 */ )
{
	Set_Rotation_X( x, new_start_rot );
	Set_Rotation_Y( y, new_start_rot );
	Set_Rotation_Z( z, new_start_rot );
}

void cSprite :: Add_Rotation_X( float rot )
{
	Set_Rotation_X( m_rot_x + rot );
}

void cSprite :: Add_Rotation_Y( float rot )
{
	Set_Rotation_Y( m_rot_y + rot );
}

void cSprite :: Add_Rotation_Z( float rot )
{
	Set_Rotation_Z( m_rot_z + rot );
}

void cSprite :: Add_Rotation( float x, float y, float z )
{
	Set_Rotation_X( m_rot_x + x );
	Set_Rotation_Y( m_rot_y + y );
	Set_Rotation_Z( m_rot_z + z );
}

void cSprite :: Set_Scale_Affects_Rect( bool enable /* = 0 */ )
{
	m_scale_affects_rect = enable;
}

void cSprite :: Set_Scale_Directions( bool up /* = 1 */, bool down /* = 0 */, bool left /* = 1 */, bool right /* = 0 */ )
{
	m_scale_up = up;
	m_scale_down = down;
	m_scale_left = left;
	m_scale_right = right;
}

void cSprite :: Set_Scale_X( const float scale, const bool new_startscale /* = 0 */ )
{
	// invalid value
	if( scale == 0.0f )
	{
		return;
	}

	// undo previous scale from rect
	if( m_scale_affects_rect && m_scale_x != 1.0f )
	{
		m_col_rect.m_w /= m_scale_x;
		m_rect.m_w /= m_scale_x;
	}

	m_scale_x = scale;

	// set new scale to rect
	if( m_scale_affects_rect && m_scale_x != 1.0f )
	{
		m_col_rect.m_w *= m_scale_x;
		m_rect.m_w *= m_scale_x;
	}

	if( new_startscale )
	{
		m_start_scale_x = m_scale_x;
	}
}

void cSprite :: Set_Scale_Y( const float scale, const bool new_startscale /* = 0 */ )
{
	// invalid value
	if( scale == 0.0f )
	{
		return;
	}

	// undo previous scale from rect
	if( m_scale_affects_rect && m_scale_y != 1.0f )
	{
		m_col_rect.m_h /= m_scale_y;
		m_rect.m_h /= m_scale_y;
	}

	m_scale_y = scale;

	// set new scale to rect
	if( m_scale_affects_rect && m_scale_y != 1.0f )
	{
		m_col_rect.m_h *= m_scale_y;
		m_rect.m_h *= m_scale_y;
	}

	if( new_startscale )
	{
		m_start_scale_y = m_scale_y;
	}
}

void cSprite :: Set_Scale( const float scale, const bool new_startscale /* = 0 */ )
{
	Set_Scale_X( scale, new_startscale );
	Set_Scale_Y( scale, new_startscale );
}

void cSprite :: Add_Scale_X( const float val )
{
	Set_Scale_X( m_scale_x + val );
}

void cSprite :: Add_Scale_Y( const float val )
{
	Set_Scale_Y( m_scale_y + val );
}

void cSprite :: Add_Scale( const float val )
{
	Set_Scale_X( m_scale_x + val );
	Set_Scale_Y( m_scale_y + val );
}

void cSprite :: Set_On_Top( const cSprite *sprite, bool optimize_hor_pos /* = 1 */ )
{
	// set ground position 0.1f over it
	m_pos_y = sprite->m_col_rect.m_y - m_col_pos.m_y - m_col_rect.m_h - 0.1f;

	// optimize the horizontal position if given
	if( optimize_hor_pos && ( m_pos_x < sprite->m_pos_x || m_pos_x > sprite->m_pos_x + sprite->m_col_rect.m_w ) )
	{
		m_pos_x = sprite->m_pos_x + sprite->m_col_rect.m_w / 3;
	}

	Update_Position_Rect();
}

void cSprite :: Move( float move_x, float move_y, const bool real /* = 0 */ )
{
	if( Is_Float_Equal( move_x, 0.0f ) && Is_Float_Equal( move_y, 0.0f ) )
	{
		return;
	}

	if( !real )
	{
		move_x *= pFramerate->m_speed_factor;
		move_y *= pFramerate->m_speed_factor;
	}

	m_pos_x += move_x;
	m_pos_y += move_y;

	Update_Position_Rect();
}

void cSprite :: Collide_Move( void )
{
	// virtual
}

void cSprite :: Update_Position_Rect( void )
{
	// if not editor mode
	if( !editor_enabled )
	{
		m_rect.m_x = m_pos_x;
		m_rect.m_y = m_pos_y;
		// editor rect
		m_start_rect.m_x = m_pos_x;
		m_start_rect.m_y = m_pos_y;
		// collision rect
		m_col_rect.m_x = m_pos_x + m_col_pos.m_x;
		m_col_rect.m_y = m_pos_y + m_col_pos.m_y;
	}
	// editor mode
	else
	{
		m_rect.m_x = m_start_pos_x;
		m_rect.m_y = m_start_pos_y;
		m_start_rect.m_x = m_start_pos_x;
		m_start_rect.m_y = m_start_pos_y;
		// Do not use m_start_pos_x/m_start_pos_y because col_rect is not the editor/start rect
		m_col_rect.m_x = m_pos_x + m_col_pos.m_x; // todo : startcol_pos ?
		m_col_rect.m_y = m_pos_y + m_col_pos.m_y;
	}

	Update_Valid_Draw();
}

void cSprite :: Update( void )
{
	// virtual
}

void cSprite :: Update_Late( void )
{
	// virtual
}

void cSprite :: Update_Valid_Draw( void )
{
	m_valid_draw = Is_Draw_Valid();
}

void cSprite :: Update_Valid_Update( void )
{
	m_valid_update = Is_Update_Valid();
}

void cSprite :: Draw( cSurface_Request *request /* = NULL */ )
{
	if( !m_valid_draw )
	{
		return;
	}

	Draw_Image( request );

	// draw debugging collision rects
	if( game_debug )
	{
		// - image rect
		// create request
		cRect_Request *rect_request = new cRect_Request();
		// draw
		pVideo->Draw_Rect( &m_rect, m_pos_z + 0.000008f, &lightgrey, rect_request );
		rect_request->no_camera = m_no_camera;
		rect_request->filled = 0;
		rect_request->blend_sfactor = GL_SRC_COLOR;
		rect_request->blend_dfactor = GL_DST_ALPHA;
		// scale
		if( !m_scale_affects_rect )
		{
			// scale position y
			if( m_scale_up )
			{
				rect_request->rect.m_y += ( m_image->m_int_y * m_scale_y ) - ( ( m_image->m_h * 0.5f ) * ( m_scale_y - 1.0f ) );
			}

			// scale height
			if( m_scale_down )
			{
				rect_request->rect.m_h += m_image->m_h * ( m_scale_y - 1.0f );
			}

			// scale position x
			if( m_scale_left )
			{
				rect_request->rect.m_x += ( m_image->m_int_x * m_scale_x ) - ( ( m_image->m_w * 0.5f ) * ( m_scale_x - 1.0f ) );
			}

			// scale width
			if( m_scale_right )
			{
				rect_request->rect.m_w += m_image->m_w * ( m_scale_x - 1.0f );
			}
		}
		// add request
		pRenderer->Add( rect_request );

		// - collision rect
		// create request
		rect_request = new cRect_Request();
		// draw
		Color sprite_color = Get_Sprite_Color( this );
		pVideo->Draw_Rect( &m_col_rect, m_pos_z + 0.000007f, &sprite_color, rect_request );
		rect_request->no_camera = m_no_camera;
		// blending
		rect_request->blend_sfactor = GL_SRC_COLOR;
		rect_request->blend_dfactor = GL_DST_ALPHA;

		// add request
		pRenderer->Add( rect_request );
	}

	// no editor drawing
	if( !editor_enabled )
	{
		// draw debug rect
		if( game_debug )
		{
			if( m_type == TYPE_PLAYER )
			{
				// Get Moving Sprite
				cMovingSprite *moving_sprite = static_cast<cMovingSprite *>(this);

				// if on ground
				if( moving_sprite->m_ground_object )
				{
					// create request
					cRect_Request *rect_request = new cRect_Request();
					// draw
					pVideo->Draw_Rect( &moving_sprite->m_ground_object->m_col_rect, m_pos_z + 0.000009f, &grey, rect_request );
					rect_request->no_camera = 0;
					// blending
					rect_request->blend_sfactor = GL_SRC_COLOR;
					rect_request->blend_dfactor = GL_DST_ALPHA;

					// add request
					pRenderer->Add( rect_request );
				}
			}
		}
	}
	// show obsolete images in editor
	else if( m_image && m_image->m_obsolete )
	{
		// create request
		cRect_Request *rect_request = new cRect_Request();
		// draw
		pVideo->Draw_Rect( &m_col_rect, m_pos_z + 0.000005f, &red, rect_request );
		rect_request->no_camera = 0;

		// blending
		rect_request->blend_sfactor = GL_SRC_COLOR;
		rect_request->blend_dfactor = GL_DST_ALPHA;

		// add request
		pRenderer->Add( rect_request );
	}
}

void cSprite :: Draw_Image( cSurface_Request *request /* = NULL */ ) const
{
	if( !m_valid_draw )
	{
		return;
	}

	bool create_request = 0;

	if( !request )
	{
		create_request = 1;
		// create request
		request = new cSurface_Request();
	}

	// editor
	if( editor_enabled )
	{
		// texture id
		request->texture_id = m_start_image->m_image;

		// size
		request->w = m_start_image->m_start_w;
		request->h = m_start_image->m_start_h;

		// rotation
		request->rotx += m_start_rot_x + m_start_image->m_base_rot_x;
		request->roty += m_start_rot_y + m_start_image->m_base_rot_y;
		request->rotz += m_start_rot_z + m_start_image->m_base_rot_z;

		// position x and
		// scalex
		if( m_start_scale_x != 1.0f )
		{
			// scale to the right and left
			if( m_scale_right && m_scale_left )
			{
				request->scale_x = m_start_scale_x;
				request->pos_x = m_start_pos_x + ( m_start_image->m_int_x * m_start_scale_x ) - ( ( m_start_image->m_w * 0.5f ) * ( m_start_scale_x - 1.0f ) );
			}
			// scale to the right only
			else if( m_scale_right )
			{
				request->scale_x = m_start_scale_x;
				request->pos_x = m_start_pos_x + ( m_start_image->m_int_x * m_start_scale_x );
			}
			// scale to the left only
			else if( m_scale_left )
			{
				request->scale_x = m_start_scale_x;
				request->pos_x = m_start_pos_x + ( m_start_image->m_int_x * m_start_scale_x ) - ( ( m_start_image->m_w ) * ( m_start_scale_x - 1.0f ) );
			}
			// no scaling
			else
			{
				request->pos_x = m_start_pos_x + m_start_image->m_int_x;
			}
		}
		// no scalex
		else
		{
			request->pos_x = m_start_pos_x + m_start_image->m_int_x;
		}
		// position y and
		// scaley
		if( m_start_scale_y != 1.0f )
		{
			// scale down and up
			if( m_scale_down && m_scale_up )
			{
				request->scale_y = m_start_scale_y;
				request->pos_y = m_start_pos_y + ( m_start_image->m_int_y * m_start_scale_y ) - ( ( m_start_image->m_h * 0.5f ) * ( m_start_scale_y - 1.0f ) );
			}
			// scale down only
			else if( m_scale_down )
			{
				request->scale_y = m_start_scale_y;
				request->pos_y = m_start_pos_y + ( m_start_image->m_int_y * m_start_scale_y );
			}
			// scale up only
			else if( m_scale_up )
			{
				request->scale_y = m_start_scale_y;
				request->pos_y = m_start_pos_y + ( m_start_image->m_int_y * m_start_scale_y ) - ( ( m_start_image->m_h ) * ( m_start_scale_y - 1.0f ) );
			}
			// no scaling
			else
			{
				request->pos_y = m_start_pos_y + m_start_image->m_int_y;
			}
		}
		// no scaley
		else
		{
			request->pos_y = m_start_pos_y + m_start_image->m_int_y;
		}
	
		// if editor z position is given
		if( m_editor_pos_z > 0.0f )
		{
			request->pos_z = m_editor_pos_z;
		}
		// normal position z
		else
		{
			request->pos_z = m_pos_z;
		}
	}
	// no editor
	else
	{
		// texture id
		request->texture_id = m_image->m_image;

		// size
		request->w = m_image->m_start_w;
		request->h = m_image->m_start_h;

		// rotation
		request->rotx += m_rot_x + m_image->m_base_rot_x;
		request->roty += m_rot_y + m_image->m_base_rot_y;
		request->rotz += m_rot_z + m_image->m_base_rot_z;

		// position x and
		// scalex
		if( m_scale_x != 1.0f )
		{
			// scale to the right and left
			if( m_scale_right && m_scale_left )
			{
				request->scale_x = m_scale_x;
				request->pos_x = m_pos_x + ( m_image->m_int_x * m_scale_x ) - ( ( m_image->m_w * 0.5f ) * ( m_scale_x - 1.0f ) );
			}
			// scale to the right only
			else if( m_scale_right )
			{
				request->scale_x = m_scale_x;
				request->pos_x = m_pos_x + ( m_image->m_int_x * m_scale_x );
			}
			// scale to the left only
			else if( m_scale_left )
			{
				request->scale_x = m_scale_x;
				request->pos_x = m_pos_x + ( m_image->m_int_x * m_scale_x ) - ( ( m_image->m_w ) * ( m_scale_x - 1.0f ) );
			}
			// no scaling
			else
			{
				request->pos_x = m_pos_x + m_image->m_int_x;
			}
		}
		// no scalex
		else
		{
			request->pos_x = m_pos_x + m_image->m_int_x;
		}
		// position y and
		// scaley
		if( m_scale_y != 1.0f )
		{
			// scale down and up
			if( m_scale_down && m_scale_up )
			{
				request->scale_y = m_scale_y;
				request->pos_y = m_pos_y + ( m_image->m_int_y * m_scale_y ) - ( ( m_image->m_h * 0.5f ) * ( m_scale_y - 1.0f ) );
			}
			// scale down only
			else if( m_scale_down )
			{
				request->scale_y = m_scale_y;
				request->pos_y = m_pos_y + ( m_image->m_int_y * m_scale_y );
			}
			// scale up only
			else if( m_scale_up )
			{
				request->scale_y = m_scale_y;
				request->pos_y = m_pos_y + ( m_image->m_int_y * m_scale_y ) - ( ( m_image->m_h ) * ( m_scale_y - 1.0f ) );
			}
			// no scaling
			else
			{
				request->pos_y = m_pos_y + m_image->m_int_y;
			}
		}
		// no scaley
		else
		{
			request->pos_y = m_pos_y + m_image->m_int_y;
		}
	
		// position z
		request->pos_z = m_pos_z;
	}

	// no camera setting
	request->no_camera = m_no_camera;

	// color
	request->color = m_color;
	// combine color
	if( m_combine_type )
	{
		request->combine_type = m_combine_type;
		request->combine_col[0] = m_combine_color[0];
		request->combine_col[1] = m_combine_color[1];
		request->combine_col[2] = m_combine_color[2];
	}

	// shadow
	if( m_shadow_pos )
	{
		request->shadow_pos = m_shadow_pos;
		request->shadow_color = m_shadow_color;
	}

	if( create_request )
	{
		// add request
		pRenderer->Add( request );
	}
}

void cSprite :: Set_Massive_Type( MassiveType mtype )
{
	// set massivetype z position
	if( mtype == MASS_MASSIVE )
	{
		m_pos_z = 0.08f;
	}
	else if( mtype == MASS_PASSIVE )
	{
		if( m_type == TYPE_FRONT_PASSIVE )
		{
			m_pos_z = 0.1f;
		}
		else
		{
			m_pos_z = 0.01f;
		}
	}
	else if( mtype == MASS_CLIMBABLE || mtype == MASS_HALFMASSIVE )
	{
		m_pos_z = 0.04f;
	}

	m_massive_type = mtype;

	/* set correct Z position
	 * fixme : this could use the wrong sprite manager
	*/
	pActive_Sprite_Manager->Set_Pos_Z( this );
}

bool cSprite :: Is_On_Top( const cSprite *obj ) const
{
	// invalid
	if( !obj )
	{
		return 0;
	}

	// always collide upwards because of the image size collision checking
	if( m_col_rect.m_x + m_col_rect.m_w > obj->m_col_rect.m_x && m_col_rect.m_x < obj->m_col_rect.m_x + obj->m_col_rect.m_w &&
		m_col_rect.m_y + m_col_rect.m_h < obj->m_col_rect.m_y )
	{
		return 1;
	}

	return 0;
}

bool cSprite :: Is_Visible_On_Screen( void ) const
{
	// camera position
	float cam_x = 0, cam_y = 0;

	if( !m_no_camera )
	{
		cam_x = pActive_Camera->x;
		cam_y = pActive_Camera->y;
	}

	// not visible left
	if( m_rect.m_x + m_rect.m_w < cam_x )
	{
		return 0;
	}
	// not visible right
	else if( m_rect.m_x > cam_x + game_res_w )
	{
		return 0;
	}
	// not visible down
	else if( m_rect.m_y + m_rect.m_h < cam_y )
	{
		return 0;
	}
	// not visible up
	else if( m_rect.m_y > cam_y + game_res_h )
	{
		return 0;
	}

	return 1;
}

bool cSprite :: Is_In_Player_Range( void ) const
{
	// no player range set
	if( m_player_range < 300 )
	{
		return Is_Visible_On_Screen();
	}
	// check if not in range
	else if( m_pos_x < pPlayer->m_pos_x - m_player_range || m_pos_y < pPlayer->m_pos_y - m_player_range ||
		m_pos_x > pPlayer->m_pos_x + m_player_range || m_pos_y > pPlayer->m_pos_y + m_player_range )
	{
		return 0;
	}

	// is in range
	return 1;
}

bool cSprite :: Is_Update_Valid( void )
{
	// if destroyed
	if( m_auto_destroy )
	{
		return 0;
	}

	return 1;
}

bool cSprite :: Is_Draw_Valid( void )
{
	// if editor not enabled
	if( !editor_enabled )
	{
		// if not active or no image is set
		if( !m_active || !m_image )
		{
			return 0;
		}
	}
	// editor enabled
	else
	{
		// if destroyed
		if( m_auto_destroy )
		{
			return 0;
		}

		// no image
		if( !m_start_image )
		{
			return 0;
		}
	}

	// not visible on the screen
	if( !Is_Visible_On_Screen() )
	{
		return 0;
	}

	return 1;
}

void cSprite :: Destroy( void )
{
	// already destroyed
	if( m_auto_destroy )
	{
		return;
	}

	m_auto_destroy = 1;
	m_active = 0;
	m_valid_draw = 0;
	m_valid_update = 0;
	Set_Image( NULL, 1 );

	Clear_Collisions();
}

void cSprite :: Editor_Add( const CEGUI::String &name, const CEGUI::String &tooltip, CEGUI::Window *window_setting, float obj_width, float obj_height /* = 28 */, bool advance_row /* = 1 */ )
{
	if( obj_height < 28 )
	{
		obj_height = 28;
	}

	// get gui sheet
	CEGUI::Window *guisheet = pGuiSystem->getGUISheet();
	// get window manager
	CEGUI::WindowManager &wmgr = CEGUI::WindowManager::getSingleton();

	// create name window
	CEGUI::Window *window_name = wmgr.createWindow( "TaharezLook/StaticText", "text_" + window_setting->getName() );
	window_name->setText( name );
	window_name->setTooltipText( tooltip );
	// get text width
	CEGUI::Font *font = CEGUI::FontManager::getSingleton().getFont( "bluebold_medium" );
	float text_width = 12 + font->getTextExtent( name ) * global_downscalex;
	// all names should have the same width
	if( text_width > m_editor_window_name_width )
	{
		m_editor_window_name_width = text_width;
	}
	// set size
	window_name->setWidth( CEGUI::UDim( 0, text_width * global_upscalex ) );
	window_name->setHeight( CEGUI::UDim( 0, 28 * global_upscaley ) );

	// create settings window
	cEditor_Object_Settings_Item *settings_item = new cEditor_Object_Settings_Item();

	// set size
	window_setting->setWidth( CEGUI::UDim( 0, obj_width * global_upscalex ) );
	window_setting->setHeight( CEGUI::UDim( 0, obj_height * global_upscaley ) );

	settings_item->window_name = window_name;
	settings_item->window_setting = window_setting;
	settings_item->advance_row = advance_row;

	m_editor_windows.push_back( settings_item );

	// add to main window
	guisheet->addChildWindow( window_name );
	guisheet->addChildWindow( window_setting );
}

void cSprite :: Editor_Activate( void )
{
	// virtual
}

void cSprite :: Editor_Deactivate( void )
{
	// remove editor controls
	for( Editor_Object_Settings_List::iterator itr = m_editor_windows.begin(), itr_end = m_editor_windows.end(); itr != itr_end; ++itr )
	{
		cEditor_Object_Settings_Item *obj = (*itr);

		delete obj;
	}

	m_editor_windows.clear();
}

void cSprite :: Editor_Position_Update( void )
{
	float obj_posx = 0;
	float obj_posy = 0;
	float row_height = 0;

	// set all positions
	for( Editor_Object_Settings_List::iterator itr = m_editor_windows.begin(), itr_end = m_editor_windows.end(); itr != itr_end; ++itr )
	{
		cEditor_Object_Settings_Item *obj = (*itr);
		CEGUI::Window *window_name = obj->window_name;
		CEGUI::Window *window_setting = obj->window_setting;

		// start a new row
		if( obj->advance_row )
		{
			obj_posx = 0;
			obj_posy += row_height;
			row_height = 0;
		}

		// get window text width
		float window_name_width = window_name->getWidth().asAbsolute( static_cast<float>(game_res_w) ) * global_downscalex;
		float window_name_height = window_name->getHeight().asAbsolute( static_cast<float>(game_res_h) ) * global_downscaley;

		// get window setting width
		float window_setting_width;
		float window_setting_height;

		// if combobox get the editbox/droplist dimension
		if( window_setting->getType() == "TaharezLook/Combobox" )
		{
			window_setting_width = static_cast<CEGUI::Combobox *>(window_setting)->getDropList()->getWidth().asAbsolute( static_cast<float>(game_res_w) ) * global_downscalex;
			window_setting_height = static_cast<CEGUI::Combobox *>(window_setting)->getEditbox()->getHeight().asAbsolute( static_cast<float>(game_res_h) ) * global_downscaley;
		}
		// get default dimension
		else
		{
			window_setting_width = window_setting->getWidth().asAbsolute( static_cast<float>(game_res_w) ) * global_downscalex;
			window_setting_height = window_setting->getHeight().asAbsolute( static_cast<float>(game_res_h) ) * global_downscaley;
		}

		// update row height
		if( window_setting_height > row_height )
		{
			row_height = window_setting_height;
		}
		if( window_name_height > row_height )
		{
			row_height = window_name_height;
		}

		// current position
		float object_final_pos_x = m_start_pos_x + m_rect.m_w + 5 + obj_posx - pActive_Camera->x;
		float object_final_pos_y = m_start_pos_y + 5 + obj_posy - pActive_Camera->y;

		// set name position
		window_name->setXPosition( CEGUI::UDim( 0, object_final_pos_x * global_upscalex ) );
		window_name->setYPosition( CEGUI::UDim( 0, object_final_pos_y * global_upscaley ) );
		// set setting position
		window_setting->setXPosition( CEGUI::UDim( 0, ( window_name_width + object_final_pos_x ) * global_upscalex ) );
		window_setting->setYPosition( CEGUI::UDim( 0, object_final_pos_y * global_upscaley ) );
		
		// set new position x
		obj_posx += window_name_width + window_setting_width + 0.05f;
	}
}

void cSprite :: Editor_Init( void )
{
	// set state
	Editor_State_Update();

	// init
	for( Editor_Object_Settings_List::iterator itr = m_editor_windows.begin(), itr_end = m_editor_windows.end(); itr != itr_end; ++itr )
	{
		cEditor_Object_Settings_Item *obj = (*itr);
		CEGUI::Window *window_name = obj->window_name;

		// set first row width
		if( obj->advance_row )
		{
			window_name->setWidth( CEGUI::UDim( 0, m_editor_window_name_width * global_upscalex ) );
		}
	}

	// set position
	Editor_Position_Update();
}

void cSprite :: Editor_State_Update( void )
{
	// virtual
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
