/* fl_prog.c --- 
 *
 * Copyright (C) 2002 Masatake YAMATO
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */ 

/* TODO: Warning display */

#include "config.h"
#include "private.h"
#include "frontline.h"
#include <gtk/gtksignal.h>

static void frontline_progress_class_init  (FrontlineProgressClass * klass);
static void frontline_progress_init        (FrontlineProgress      * prog);
static void frontline_progress_finalize    (GtkObject * object);

enum {
  STOPPED,
  LAST_SIGNAL
};

static GtkHBoxClass *  parent_class;
static guint fl_progress_signals[LAST_SIGNAL] = { 0 };

GtkType
frontline_progress_get_type (void)
{
  static GtkType fl_progress_type = 0;
  
  if (!fl_progress_type)
    {
      static const GtkTypeInfo fl_progress_info =
      {
        "FrontlineProgress",
        sizeof (FrontlineProgress),
        sizeof (FrontlineProgressClass),
        (GtkClassInitFunc) frontline_progress_class_init,
        (GtkObjectInitFunc) frontline_progress_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      
      fl_progress_type = gtk_type_unique (GTK_TYPE_HBOX, &fl_progress_info);
    }
  return fl_progress_type;
}

static void
frontline_progress_class_init  (FrontlineProgressClass * klass)
{
  GtkObjectClass * object_class;
  parent_class = gtk_type_class (gtk_hbox_get_type ());
  
  object_class = (GtkObjectClass*)klass;
  object_class->finalize = frontline_progress_finalize;  

  fl_progress_signals[STOPPED] =
    gtk_signal_new ("stopped",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (FrontlineProgressClass, stopped),
		    gtk_marshal_NONE__NONE,
		    GTK_TYPE_NONE,
		    0);
  klass->stopped = NULL;

  gtk_object_class_add_signals (object_class, fl_progress_signals, LAST_SIGNAL);
}

static void
frontline_progress_init (FrontlineProgress * fl_prog)
{
  gtk_box_set_homogeneous(GTK_BOX(fl_prog), FALSE);
  gtk_box_set_spacing(GTK_BOX(fl_prog), 8);
  gtk_container_set_border_width(GTK_CONTAINER(fl_prog), 10);

  fl_prog->adj = gtk_adjustment_new(0.0, 0.0, 1.0, 0.01, 0.1, 1.0);
  fl_prog->progress_bar = gtk_progress_bar_new_with_adjustment(GTK_ADJUSTMENT(fl_prog->adj));
  gtk_widget_show(fl_prog ->progress_bar);
  gtk_box_pack_start(GTK_BOX(fl_prog), fl_prog ->progress_bar, TRUE, TRUE, 4);

  fl_prog->stop_button = gtk_button_new_with_label("Stop");
  gtk_widget_show(fl_prog ->stop_button);
  gtk_box_pack_start(GTK_BOX(fl_prog), fl_prog ->stop_button, TRUE, TRUE, 4);
  gtk_signal_connect_object(GTK_OBJECT(fl_prog->stop_button),
			    "clicked",
			    GTK_SIGNAL_FUNC(frontline_progress_stopped),
			    GTK_OBJECT(fl_prog));
}

GtkWidget*
frontline_progress_new ()
{
  GtkWidget *fl_prog;
  fl_prog = gtk_type_new (FRONTLINE_TYPE_PROGRESS);
  return fl_prog;
}

static void
frontline_progress_finalize    (GtkObject * object)
{
  GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

void
frontline_progress_stopped  (FrontlineProgress * fl_prog)
{
  gtk_signal_emit(GTK_OBJECT(fl_prog),
		  fl_progress_signals[STOPPED]);
}
