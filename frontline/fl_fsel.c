/* fl_fsel.h --- 
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

/* TODO: Use gnome_icon_entry.h, error and warning handler */

#include "config.h"
#include "private.h"
#include "frontline.h"
#include <gtk/gtksignal.h>
#include <gnome.h>

static void frontline_file_selection_class_init  (FrontlineFileSelectionClass * klass);
static void frontline_file_selection_init        (FrontlineFileSelection      * fsel);
static void frontline_file_selection_finalize    (GtkObject * object);

static void frontline_file_selection_load (GtkEditable * entry, gpointer uesr_data);
static void frontline_file_selection_drag_data_received (GtkWidget * widget,
							 GdkDragContext * drag_context,
							 gint x, gint y,
							 GtkSelectionData * data,
							 guint info,
							 guint event_time,
							 gpointer user_data);
enum {
  LOADED,
  LAST_SIGNAL
};

static GtkVBoxClass *  parent_class;
static guint fl_file_selection_signals[LAST_SIGNAL] = { 0 };

GtkType
frontline_file_selection_get_type (void)
{
  static GtkType fl_file_selection_type = 0;
  
  if (!fl_file_selection_type)
    {
      static const GtkTypeInfo fl_file_selection_info =
      {
        "FrontlineFileSelection",
        sizeof (FrontlineFileSelection),
        sizeof (FrontlineFileSelectionClass),
        (GtkClassInitFunc) frontline_file_selection_class_init,
        (GtkObjectInitFunc) frontline_file_selection_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      
      fl_file_selection_type = gtk_type_unique (GTK_TYPE_VBOX, &fl_file_selection_info);
    }
  return fl_file_selection_type;
}

static void
frontline_file_selection_class_init  (FrontlineFileSelectionClass * klass)
{
  GtkObjectClass * object_class;
  parent_class = gtk_type_class (gtk_vbox_get_type ());
  
  object_class = (GtkObjectClass*)klass;
  object_class->finalize = frontline_file_selection_finalize;
  
  fl_file_selection_signals[LOADED] =
    gtk_signal_new ("loaded",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (FrontlineFileSelectionClass, 
				       loaded),
		    gtk_marshal_NONE__POINTER_POINTER,
		    GTK_TYPE_NONE,
		    2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER);
  klass->loaded = NULL;
  gtk_object_class_add_signals (object_class, fl_file_selection_signals, LAST_SIGNAL);
 }


static void
fl_load_msg_handler (at_string msg, at_msg_type msg_type, at_address client_data)
{
  g_warning("%s", msg);
  if (msg_type == AT_MSG_FATAL)
    *(int *)client_data = 1;
}

static void
frontline_file_selection_load(GtkEditable * entry,
			      gpointer uesr_data)
{
  FrontlineFileSelection * fl_fsel;
  gchar * filename; 
  at_input_read_func reader;
  at_bitmap_type * bitmap;
  int errorp = 0;
  gchar * msg;
  
  fl_fsel  = FRONTLINE_FILE_SELECTION(uesr_data);
  filename    = gtk_entry_get_text(GTK_ENTRY(entry));
  reader      = at_input_get_handler(filename);
  if (reader)
    {
      bitmap = at_bitmap_read(reader, filename, fl_fsel->opts, fl_load_msg_handler, &errorp);
      if (errorp == 0)
	{
	  gtk_signal_emit(GTK_OBJECT(fl_fsel),
			  fl_file_selection_signals[LOADED],
			  filename,
			  bitmap);
	}
      else
	{
	  msg = g_strdup_printf(_("Fail to load image: %s"), filename);
	  gnome_error_dialog(msg);
	  g_free(msg);
	}
      if (bitmap)
	at_bitmap_free(bitmap);
    }
  else
    {
      msg = g_strdup_printf(_("Cannot find load handler for: %s"), 
			    filename);
      gnome_error_dialog(msg);
      g_free(msg);
    }
}

static void
frontline_file_selection_drag_data_received (GtkWidget * widget,
					     GdkDragContext * drag_context,
					     gint x, gint y,
					     GtkSelectionData * data,
					     guint info,
					     guint event_time,
					     gpointer user_data)
{
  frontline_file_selection_load(GTK_EDITABLE(widget), user_data);
}

static void
frontline_file_selection_init (FrontlineFileSelection * fl_fsel)
{

  GtkWidget * hbox;
  GtkWidget * label;
  GtkWidget * fentry;
  GtkTooltips * tooltips;

  fl_fsel->opts = NULL;
  
  gtk_box_set_homogeneous(GTK_BOX(fl_fsel), FALSE);
  gtk_box_set_spacing(GTK_BOX(fl_fsel), 4);
  gtk_container_set_border_width(GTK_CONTAINER(fl_fsel), 10);
  
  hbox 	= gtk_hbox_new(FALSE, 4);
  label = gtk_label_new(_("Image"));
  gtk_box_pack_start_defaults(GTK_BOX(hbox), label);

  fl_fsel->ifentry = gnome_file_entry_new("frontline::fsel::input", _("Select"));
  gtk_box_pack_start_defaults(GTK_BOX(hbox), fl_fsel->ifentry);
  gtk_widget_show_all(hbox);
  gtk_box_pack_start_defaults(GTK_BOX(fl_fsel), hbox);

  fentry    = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(fl_fsel->ifentry));
  gtk_signal_connect_after(GTK_OBJECT(fentry),                           
			   "activate",
			   GTK_SIGNAL_FUNC(frontline_file_selection_load), 
			   fl_fsel);
  gtk_signal_connect_after(GTK_OBJECT(fentry),
			   "drag_data_received",
			   GTK_SIGNAL_FUNC(frontline_file_selection_drag_data_received), 
			   fl_fsel);
#if 0
  hbox 	= gtk_hbox_new(FALSE, 4);
  label = gtk_label_new(_("Result Figure"));
  gtk_box_pack_start_defaults(GTK_BOX(hbox), label);

  fentry = gnome_file_entry_new(NULL, _("Write Result Figure"));
  gtk_box_pack_start_defaults(GTK_BOX(hbox), fentry);
  gtk_widget_show_all(hbox);
  gtk_box_pack_start_defaults(GTK_BOX(fl_fsel), hbox);
#endif /* 0 */
  tooltips = gtk_tooltips_new();
  gtk_tooltips_set_tip (tooltips, 
			GTK_WIDGET(fentry), 
			_("Drop an image file here to load it"),
			_("Drop an image file here to load it"));
}

GtkWidget*
frontline_file_selection_new ()
{
  GtkWidget *fl_fsel;
  fl_fsel = gtk_type_new (FRONTLINE_TYPE_FILE_SELECTION);
  return fl_fsel;
}

gboolean
frontline_file_selection_load_file (FrontlineFileSelection * fsel,
				    const gchar * filename)
{
  GtkWidget * fentry;
  fentry = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(fsel->ifentry));
  gtk_entry_set_text(GTK_ENTRY(fentry), filename);
  frontline_file_selection_load(GTK_EDITABLE(fentry), fsel);
  return TRUE;
}

static void
frontline_file_selection_finalize    (GtkObject * object)
{
  FrontlineFileSelection * fl_sel;
  fl_sel = FRONTLINE_FILE_SELECTION(object);

  if (fl_sel->opts)
    at_input_opts_free(fl_sel->opts);
  GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

void
frontline_file_selection_set_input_option(FrontlineFileSelection * fsel,
					  at_input_opts_type * opts)
{
  g_return_if_fail (fsel);
  if (fsel->opts)
    at_input_opts_free(fsel->opts);
  fsel->opts = at_input_opts_copy(opts);
  /* TODO: When option is updated, the image must be reloaded again. */
}
