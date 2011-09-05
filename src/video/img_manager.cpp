/***************************************************************************
 * img_manager.cpp  -  Image Handler/Manager
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

#include "../video/img_manager.h"
#include "../video/renderer.h"
#include "../core/i18n.h"

namespace SMC
{

/* *** *** *** *** *** cSaved_Texture *** *** *** *** *** *** *** *** *** *** *** *** */

cSaved_Texture :: cSaved_Texture( void )
{
	base = NULL;
	pixels = NULL;

	width = 0;
	height = 0;
	format = 0;

	min_filter = 0;
	mag_filter = 0;
	wrap_s = 0;
	wrap_t = 0;
}

cSaved_Texture :: ~cSaved_Texture( void )
{
	if( pixels )
	{
		delete[] pixels;
	}
}

/* *** *** *** *** *** *** cImage_Manager *** *** *** *** *** *** *** *** *** *** *** */

cImage_Manager :: cImage_Manager( void )
: cObject_Manager<cGL_Surface>()
{
	high_texture_id = 0;
}

cImage_Manager :: ~cImage_Manager( void )
{
	cImage_Manager::Delete_All();
}

void cImage_Manager :: Add( cGL_Surface *obj )
{
	if( !obj )
	{
		return;
	}

	// it is now managed
	obj->m_managed = 1;

	// Add
	cObject_Manager<cGL_Surface>::Add( obj );
}

cGL_Surface *cImage_Manager :: Get_Pointer( const std::string &path ) const
{
	for( GL_Surface_List::const_iterator itr = objects.begin(), itr_end = objects.end(); itr != itr_end; ++itr )
	{
		cGL_Surface *obj = (*itr);

		// return first match
		if( obj->m_filename.compare( path ) == 0 )
		{
			return obj;
		}
	}

	// not found
	return NULL;
}

cGL_Surface *cImage_Manager :: Copy( const std::string &path )
{
	for( GL_Surface_List::iterator itr = objects.begin(), itr_end = objects.end(); itr != itr_end; ++itr )
	{
		// get object
		cGL_Surface *obj = (*itr);

		// first match
		if( obj->m_filename.compare( path ) == 0 )
		{
			return obj->Copy();
		}
	}

	// not found
	return NULL;
}

void cImage_Manager :: Grab_Textures( bool from_file /* = 0 */, bool draw_gui /* = 0 */ )
{
	// progress bar
	CEGUI::ProgressBar *progress_bar = NULL;

	if( draw_gui )
	{
		// get progress bar
		progress_bar = static_cast<CEGUI::ProgressBar *>(CEGUI::WindowManager::getSingleton().getWindow( "progress_bar" ));
		progress_bar->setProgress( 0 );
		// set loading screen text
		Loading_Screen_Draw_Text( _("Saving Textures") );
	}

	unsigned int loaded_files = 0;
	unsigned int file_count = objects.size();

	// save all textures
	for( GL_Surface_List::iterator itr = objects.begin(), itr_end = objects.end(); itr != itr_end; ++itr )
	{
		// get surface
		cGL_Surface *obj = (*itr);

		// skip surfaces with an already deleted texture
		if( !glIsTexture( obj->m_image ) )
		{
			continue;
		}

		// get software texture and save it to software memory
		saved_textures.push_back( obj->Get_Software_Texture( from_file ) );
		// delete hardware texture
		if( glIsTexture( obj->m_image ) )
		{
			glDeleteTextures( 1, &obj->m_image );
		}

		// count files
		loaded_files++;

		// draw
		if( draw_gui )
		{
			// update progress
			progress_bar->setProgress( static_cast<float>(loaded_files) / static_cast<float>(file_count) );

			Loading_Screen_Draw();
		}
	}
}

void cImage_Manager :: Restore_Textures( bool draw_gui /* = 0 */ )
{
	// progress bar
	CEGUI::ProgressBar *progress_bar = NULL;

	if( draw_gui )
	{
		// get progress bar
		progress_bar = static_cast<CEGUI::ProgressBar *>(CEGUI::WindowManager::getSingleton().getWindow( "progress_bar" ));
		progress_bar->setProgress( 0 );
		// set loading screen text
		Loading_Screen_Draw_Text( _("Restoring Textures") );
	}

	unsigned int loaded_files = 0;
	unsigned int file_count = saved_textures.size();

	// load back into hardware textures
	for( Saved_Texture_List::iterator itr = saved_textures.begin(), itr_end = saved_textures.end(); itr != itr_end; ++itr )
	{
		// get saved texture
		cSaved_Texture *soft_tex = (*itr);

		// load it
		soft_tex->base->Load_Software_Texture( soft_tex );
		// delete
		delete soft_tex;

		// count files
		loaded_files++;

		// draw
		if( draw_gui )
		{
			// update progress
			progress_bar->setProgress( static_cast<float>(loaded_files) / static_cast<float>(file_count) );

			Loading_Screen_Draw();
		}
	}

	saved_textures.clear();
}

void cImage_Manager :: Delete_Image_Textures( void )
{
	for( GL_Surface_List::iterator itr = objects.begin(), itr_end = objects.end(); itr != itr_end; ++itr ) 
	{
		// get object
		cGL_Surface *obj = (*itr);

		if( obj->m_auto_del_img && glIsTexture( obj->m_image ) )
		{
			glDeleteTextures( 1, &obj->m_image );
		}
	}
}

void cImage_Manager :: Delete_Hardware_Textures( void )
{
	// delete all hardware surfaces
	for( GLuint i = 0; i < high_texture_id; i++ )
	{
		if( glIsTexture( i ) )
		{
			printf( "ImageManager : deleting texture %d\n", i );
			glDeleteTextures( 1, &i );
		}
	}

	high_texture_id = 0;
}

void cImage_Manager :: Delete_All( void )
{
	// stops cGL_Surface destructor from checking if GL texture id still in use
	Delete_Image_Textures();
	cObject_Manager<cGL_Surface>::Delete_All();
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

cImage_Manager *pImage_Manager = NULL;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC
