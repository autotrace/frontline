#ifndef __UNDO_H__
#define __UNDO_H__
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

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif


/** <pre>
    GtkObject
     +----GundoSequence
    </pre>
    <p>
    A GundoSequence is a Gtk object that manages a sequence of actions that
    can be undone and redone.  The structure is opaque; its members should 
    be accessed only by the functions provided by the Gundo library.
    <p>
    GundoSequence objects provide the following signals:
    <dd>
      <dt>"can_undo"</dt>
      <dd>Fired when the undo sequence transitions from a state where it
          cannot undo actions to one where it can.</dd>
      <dt>"can_redo"</dt>
      <dd>Fired when the undo sequence transitions from a state where it
          cannot redo actions to one where it can.</dd>
    </dd>
    
    @memo A Gtk object that manages a sequence of actions that can be 
    undone and redone.
 */
typedef struct _GundoSequence GundoSequence;
struct _GundoSequence
{
    GtkObject base;
    GArray *actions;
    int next_redo;
    GundoSequence *group;
};


/** The Gtk object class of GundoSequence objects.  Used by the type
    checking and casting macros.
 */
typedef struct _GundoSequenceClass GundoSequenceClass;
struct _GundoSequenceClass
{
    GtkObjectClass base;
    
    void (*can_undo)( GundoSequence*, gboolean );
    void (*can_redo)( GundoSequence*, gboolean );
};


/** The type of function called to undo or redo an action or free data
    associated with an action.
    
    @param action_data
        Data about the action.  The action_data pointer must be
        cast to a concrete type by GundoActionCallback implementations.
    @see GundoActionType
    @see _GundoActionType
 */
typedef void (*GundoActionCallback)( gpointer action_data );

/** An GundoActionType defines the operations that can be applied to an undo
    action that has been added to an GundoSequence.  All operations are of
    type \Ref{GundoActionCallback}.  That is, they take a pointer to some data
    about the action and do not return a value.
    <p>
    Operations:
    <dl>
      <dt>undo</dt> 
      <dd>Undoes an action of this type.</dd>
      <dt>redo</dt>
      <dd>Redoes an action of this type.</dd>
      <dt>free</dt> 
      <dd>Frees the data about an action of this type.  
          Can be <em>NULL</em>, in which case the data is not freed.</dd>
    </dl>
    
    @see _GundoActionType
    @see gundo_sequence_add_action
 */
typedef struct _GundoActionType GundoActionType;

/** Information about the type of an undoable action.  This structure should be
    referred to by the \Ref{GundoActionType} typedef.
    
    @see GundoActionType
    @see gundo_sequence_add_action
 */
struct _GundoActionType {
    /** Function called to undo the action.
     */
    GundoActionCallback undo;
    
    /** Function called to redo the action. 
     */
    GundoActionCallback redo;
    
    /** Function called to free the action_data.  Can be NULL, in which case
        the action_data is not free'd.
     */
    GundoActionCallback free;
};


/** Support for the Gtk type system */
#define GUNDO_TYPE_SEQUENCE \
    (gundo_sequence_get_type())
/** Support for the Gtk type system */
#define GUNDO_SEQUENCE(object) \
    (GTK_CHECK_CAST((object), GUNDO_TYPE_SEQUENCE, GundoSequence) )
/** Support for the Gtk type system */
#define GUNDO_SEQUENCE_CLASS(klass) \
    (GTK_CHECK_CLASS_CAST( (klass), GUNDO_TYPE_SEQUENCE, GtkWidgetClass ))
/** Support for the Gtk type system */
#define GUNDO_IS_SEQUENCE(object) \
    (GTK_CHECK_TYPE( (object), GUNDO_TYPE_SEQUENCE ))
/** Support for the Gtk type system */
#define GUNDO_IS_SEQUENCE_CLASS(klass) \
    (GTK_CHECK_CLASS_TYPE( (klass), GUNDO_TYPE_SEQUENCE ))

/** Returns the Gtk type for a \Ref{GundoSequence}.
 */
guint gundo_sequence_get_type();


/** Creates a new, empty GundoSequence.  
    <p>
    The GundoSequence is destroyed by a call to gtk_object_destroy.  All
    of the actions in a sequence are destroyed (by calling the free callback
    of their GundoActionType) when the sequence is destroyed.
   
    @return 
        A new GundoSequence object.
 */
GundoSequence *gundo_sequence_new();

/** Adds an action to the end of the GundoSequence.  Any actions that
    have been undone but not redone are destroyed.  If any groups have
    been started, the action is appended to the end of the most recently
    started group.
    
    @param seq
        The undo sequence to which to add an action.
    @param type
        The type of the action.
    @param data
        Data about the action.  The data is destroyed by calling the
        "free" callback of the action type.
    
    @see GundoActionType
 */
void gundo_sequence_add_action( GundoSequence *seq, 
                                const GundoActionType *type, 
                                gpointer data );

/** Queries whether the undo sequence contains any actions that can be undone.
    
    @param seq
        The undo sequence.
    @return 
        TRUE if there are actions that can be undone, FALSE otherwise.
 */
gboolean gundo_sequence_can_undo( GundoSequence *seq );

/** Queries whether the undo sequence contains any actions that can be redone.
    
    @param seq
        The undo sequence.
    @return 
        TRUE if there are actions that have already been undone, and
        can therefore be redone, FALSE otherwise.
 */
gboolean gundo_sequence_can_redo( GundoSequence *seq );

/** Undoes the action at the end of the action sequence.
    <p>
    <em>Pre</em>: no group is being constructed && undo_sequence_can_undo(seq).
    
    @param seq
        The undo sequence.
 */
void gundo_sequence_undo( GundoSequence *seq );

/** Redoes the last action that was undone.
    <p>
    <em>Pre</em>: no group is being constructed && undo_sequence_can_redo(seq).
    
    @param seq
        The undo sequence.
 */
void gundo_sequence_redo( GundoSequence *seq );

/** Starts a new group of actions that are treated as a single action.
    E.g. a single call to undo and redo undoes or redoes all actions of
    the group.  This is useful if a single action by the user causes
    the program to issue multiple internal actions.  Groups can be
    nested.
    
    @param seq
        The undo sequence.
 */
void gundo_sequence_start_group( GundoSequence *seq );

/** Ends a group, adding it to the end of the undo sequence, or its parent
    group.
 
    @param seq
        The undo sequence.
 */
void gundo_sequence_end_group( GundoSequence *seq );

/** Aborts the construction of a group, freeing all of its actions.
    
    @param seq
        The undo sequence.
 */
void gundo_sequence_abort_group( GundoSequence *seq );

/** Frees all actions in the undo sequence.
    <p>
    <em>Pre</em>:  no group is being constructed.
    
    @param seq
        The undo sequence.
 */
void gundo_sequence_clear( GundoSequence *seq );


#ifdef __cplusplus
}
#endif
#endif




