#ifndef __GUNDO_UI_H__
#define __GUNDO_UI_H__
/*
    Gtk Undo: Multilevel undo/redo for Gtk
    Copyright (C) 1999  Nat Pryce

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifdef __cplusplus
extern "C" {
#endif

/** Makes a widget sensitive to the current undo state of a GundoSequence.  
    A widget that is undo-sensitive will only be sensitive 
    when it is possible to call \Ref{gundo_sequence_undo} on its associated 
    GundoSequence (that is, when \Ref{gundo_sequence_can_undo} returns TRUE).
    
    @param widget
        The widget to make undo-sensitive.
    @param seq
        The undo sequence that the widget should be sensitive to.
 */
void gundo_make_undo_sensitive( GtkWidget *widget, GundoSequence *seq );

/** Makes a widget sensitive to the current redo state of a GundoSequence.  
    A widget that is redo-sensitive will only be sensitive when it is 
    possible to call \Ref{gundo_sequence_redo} on its associated GundoSequence 
    (that is, when \Ref{gundo_sequence_can_redo} returns TRUE).
    
    @param widget
        The widget to make redo-sensitive.
    @param seq
        The undo sequence that the widget should be sensitive to.
 */
void gundo_make_redo_sensitive( GtkWidget *widget, GundoSequence *seq );


#ifdef __cplusplus
}
#endif
#endif
