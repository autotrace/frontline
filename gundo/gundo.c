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

#include <stdio.h>
#include <glib.h>
#include "gundo.h"


typedef struct _UndoAction UndoAction;
struct _UndoAction {
    const GundoActionType *type;
    gpointer data;
};

enum UndoSignalType {
    UNDO_SEQUENCE_SIGNAL_CAN_UNDO,
    UNDO_SEQUENCE_SIGNAL_CAN_REDO,
    UNDO_SEQUENCE_SIGNAL_LAST = UNDO_SEQUENCE_SIGNAL_CAN_REDO
};


static void gundo_sequence_class_init( GundoSequenceClass* );
static void gundo_sequence_init( GundoSequence* );
static void gundo_sequence_destroy( GtkObject *object );
static void group_undo( GundoSequence *seq );
static void group_redo( GundoSequence *seq );
static void free_actions( int actc, UndoAction *actv );


static gint gundo_sequence_signals[UNDO_SEQUENCE_SIGNAL_LAST+1];
static GtkObjectClass *base_class = 0;
static GundoActionType gundo_action_group = {
    (GundoActionCallback)group_undo,
    (GundoActionCallback)group_redo,
    (GundoActionCallback)gtk_object_destroy
};



guint gundo_sequence_get_type () {
    static guint gundo_sequence_type = 0;
    
    if( !gundo_sequence_type ) {
        GtkTypeInfo type_info = {
            "GundoSequence",
            sizeof(GundoSequence),
            sizeof(GundoSequenceClass),
            (GtkClassInitFunc)gundo_sequence_class_init,
            (GtkObjectInitFunc)gundo_sequence_init,
            (GtkArgSetFunc)NULL,
            (GtkArgGetFunc)NULL
        };
        
        gundo_sequence_type = 
            gtk_type_unique( gtk_object_get_type(), &type_info );
    }
    
    return gundo_sequence_type;
}


static void gundo_sequence_class_init( GundoSequenceClass *klass ) {
    GtkObjectClass *base = (GtkObjectClass*)klass;
    base->destroy = gundo_sequence_destroy;
    
    gundo_sequence_signals[UNDO_SEQUENCE_SIGNAL_CAN_UNDO] = 
        gtk_signal_new( "can_undo",
                        GTK_RUN_FIRST,
                        gundo_sequence_get_type(),
                        GTK_SIGNAL_OFFSET( GundoSequenceClass, can_undo ),
                        gtk_marshal_NONE__BOOL,
                        GTK_TYPE_NONE, 1, GTK_TYPE_BOOL );
    
    gundo_sequence_signals[UNDO_SEQUENCE_SIGNAL_CAN_REDO] = 
        gtk_signal_new( "can_redo",
                        GTK_RUN_FIRST,
                        gundo_sequence_get_type(),
                        GTK_SIGNAL_OFFSET( GundoSequenceClass, can_redo ),
                        gtk_marshal_NONE__BOOL,
                        GTK_TYPE_NONE, 1, GTK_TYPE_BOOL );
    
    gtk_object_class_add_signals( base, gundo_sequence_signals,
                                  UNDO_SEQUENCE_SIGNAL_LAST+1 );
    
    klass->can_undo = NULL;
    klass->can_redo = NULL;
    
    base_class = gtk_type_class( gtk_object_get_type() );
}

static void gundo_sequence_init( GundoSequence *seq ) {
    seq->actions = g_array_new( FALSE, FALSE, sizeof(UndoAction) );
    seq->next_redo = 0;
    seq->group = NULL;
}

static void gundo_sequence_destroy( GtkObject *object ) {
    GundoSequence *seq;
    
    g_return_if_fail( object != NULL );
    
    seq = GUNDO_SEQUENCE(object);
    free_actions( seq->actions->len, (UndoAction*)seq->actions->data );
    if( seq->group ) gtk_object_destroy( GTK_OBJECT(seq->group) );
    g_array_free( seq->actions, TRUE );
    
    if (GTK_OBJECT_CLASS(base_class)->destroy) {
        (GTK_OBJECT_CLASS(base_class)->destroy)( object );
    }
}


GundoSequence *gundo_sequence_new() {
    return GUNDO_SEQUENCE( gtk_type_new( gundo_sequence_get_type() ) );
}


void gundo_sequence_clear( GundoSequence *seq ) {
    gboolean could_undo, could_redo;
    
    g_assert( seq->group == NULL );
    
    could_undo = gundo_sequence_can_undo(seq);
    could_redo = gundo_sequence_can_redo(seq);
    
    free_actions( seq->actions->len, (UndoAction*)seq->actions->data );
    g_array_set_size( seq->actions, 0 );
    seq->next_redo = 0;
    if( seq->group ) {
        gtk_object_destroy( GTK_OBJECT(seq->group) );
        seq->group = 0;
    }
    
    if( could_undo ) {
        gtk_signal_emit( GTK_OBJECT(seq),
                         gundo_sequence_signals[UNDO_SEQUENCE_SIGNAL_CAN_UNDO],
                         FALSE );
    }
    if( could_redo ) {
        gtk_signal_emit( GTK_OBJECT(seq),
                         gundo_sequence_signals[UNDO_SEQUENCE_SIGNAL_CAN_REDO],
                         FALSE );
    }
}


void gundo_sequence_add_action( GundoSequence *seq, 
                               const GundoActionType *type, 
                               gpointer data ) 
{
    if( seq->group ) {
        gundo_sequence_add_action( seq->group, type, data );
    } else {
        UndoAction action;
        gboolean could_undo = gundo_sequence_can_undo(seq);
        gboolean could_redo = gundo_sequence_can_redo(seq);
        
        if( seq->next_redo < seq->actions->len ) {
            free_actions( seq->actions->len - seq->next_redo,
                          (UndoAction*)seq->actions->data + seq->next_redo );
            
            g_array_set_size( seq->actions, seq->next_redo );
        }
        
        action.type = type;
        action.data = data;
        
        g_array_append_val( seq->actions, action );
        seq->next_redo++;
        
        if( !could_undo ) {
            gtk_signal_emit( 
                GTK_OBJECT(seq),
                gundo_sequence_signals[UNDO_SEQUENCE_SIGNAL_CAN_UNDO],
                TRUE );
        }
        if( could_redo ) {
            gtk_signal_emit( 
                GTK_OBJECT(seq),
                gundo_sequence_signals[UNDO_SEQUENCE_SIGNAL_CAN_REDO],
                FALSE );
        }
    }
}

void gundo_sequence_start_group( GundoSequence *seq ) {
    if( seq->group ) {
        gundo_sequence_start_group( seq->group );
    } else {
        seq->group = gundo_sequence_new();
    }
}

void gundo_sequence_end_group( GundoSequence *seq ) {
    g_assert( seq->group != NULL );
    
    if( seq->group->group ) {
        gundo_sequence_end_group( seq->group );
    } else {
        GundoSequence *group = seq->group;
        seq->group = NULL;
        
        if( group->actions->len > 0 ) {
            gundo_sequence_add_action( seq, &gundo_action_group, group );
        } else {
            gtk_object_destroy( GTK_OBJECT(group) );
        }
    }
}

void gundo_sequence_abort_group( GundoSequence *seq ) {
    g_assert( seq->group != NULL );
    
    if( seq->group->group ) {
        gundo_sequence_abort_group( seq->group );
    } else {
        GundoSequence *group = seq->group;
        seq->group = 0;
        gtk_object_destroy( GTK_OBJECT(group) );
    }
}

gboolean gundo_sequence_can_undo( GundoSequence *seq ) {
    return seq->next_redo > 0;
}

void gundo_sequence_undo( GundoSequence *seq ) {
    UndoAction *action;
    gboolean could_redo;
    
    g_assert( seq->group == NULL );
    g_assert( gundo_sequence_can_undo(seq) );
    
    could_redo = gundo_sequence_can_redo(seq);
    
    seq->next_redo--;
    action = &g_array_index( seq->actions, UndoAction, seq->next_redo );
    (action->type->undo)( action->data );
    
    if( !could_redo ) {
        gtk_signal_emit( GTK_OBJECT(seq),
                         gundo_sequence_signals[UNDO_SEQUENCE_SIGNAL_CAN_REDO],
                         TRUE );
    }
    if( !gundo_sequence_can_undo(seq) ) {
        gtk_signal_emit( GTK_OBJECT(seq),
                         gundo_sequence_signals[UNDO_SEQUENCE_SIGNAL_CAN_UNDO],
                         FALSE );
    }
}


gboolean gundo_sequence_can_redo( GundoSequence *seq ) {
    return seq->next_redo < seq->actions->len;
}

void gundo_sequence_redo( GundoSequence *seq ) {
    UndoAction *action;
    gboolean could_undo;
    
    g_assert( seq->group == NULL );
    g_assert( gundo_sequence_can_redo(seq) );
    
    could_undo = gundo_sequence_can_undo(seq);
    
    action = &g_array_index( seq->actions, UndoAction, seq->next_redo );
    seq->next_redo++;
    (action->type->redo)( action->data );
    
    if( !could_undo ) {
        gtk_signal_emit( GTK_OBJECT(seq),
                         gundo_sequence_signals[UNDO_SEQUENCE_SIGNAL_CAN_UNDO],
                         TRUE );
    }
    if( !gundo_sequence_can_redo(seq) ) {
        gtk_signal_emit( GTK_OBJECT(seq),
                         gundo_sequence_signals[UNDO_SEQUENCE_SIGNAL_CAN_REDO],
                         FALSE );
    }
}


static void group_undo( GundoSequence *seq ) {
    while( gundo_sequence_can_undo(seq) ) gundo_sequence_undo(seq);
}


static void group_redo( GundoSequence *seq ) {
    while( gundo_sequence_can_redo(seq) ) gundo_sequence_redo(seq);
}


static void free_actions( int actc, UndoAction *actv ) {
    int i;
    
    for( i = 0; i < actc; i++ ) {
        if( (actv[i].type->free) ) (actv[i].type->free)( actv[i].data );
    }
}
