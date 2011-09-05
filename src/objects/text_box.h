/***************************************************************************
 * text_box.h  -  header for the corresponding cpp file
 *
 * Copyright (C) 2007 - 2009 Florian Richter
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SMC_TEXT_BOX_H
#define SMC_TEXT_BOX_H

#include "../core/globals.h"
#include "../objects/box.h"

namespace SMC
{

/* *** *** *** *** *** *** *** *** cText_Box *** *** *** *** *** *** *** *** *** */

class cText_Box : public cBaseBox
{
public:
	// constructor
	cText_Box( float x, float y );
	// create from stream
	cText_Box( CEGUI::XMLAttributes &attributes );
	// destructor
	virtual ~cText_Box( void );

	// init defaults
	void Init( void );
	// copy
	virtual cText_Box *Copy( void );

	// create from stream
	virtual void Create_From_Stream( CEGUI::XMLAttributes &attributes );
	// save to stream
	virtual void Save_To_Stream( ofstream &file );

	// Activate
	virtual void Activate( void );

	// update
	virtual void Update( void );

	// Set Text
	void Set_Text( const std::string &str_text );

	// editor activation
	virtual void Editor_Activate( void );
	// editor text text changed event
	bool Editor_Text_Text_Changed( const CEGUI::EventArgs &event );

	// the text
	std::string text;
};

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace SMC

#endif
