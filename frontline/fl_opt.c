/* fl_opt.c --- Frontline option class
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

/* TODO: warning/error reports mechanism  */

#include "config.h"
#include "private.h"
#include "frontline.h"
#include <gundo/gundo.h>
#include <gundo/gundo_ui.h>

#include <gtk/gtksignal.h>
#include <libgnomeui/gnome-dialog-util.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnome/gnome-mime.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

static void frontline_option_class_init  (FrontlineOptionClass * klass);
static void frontline_option_init        (FrontlineOption      * option);
static void frontline_option_finalize    (GtkObject * object);
static void frontline_real_option_value_changed (FrontlineOption * fl_opt);
static void frontline_option_undo (GtkButton * button, gpointer user_data);
static void frontline_option_redo (GtkButton * button, gpointer user_data);
static void frontline_option_save (GtkButton * button, gpointer user_data);
static void frontline_option_load (GtkButton * button, gpointer user_data);

/* Drag and Drop */
typedef enum {
  URI_LIST
} fl_opt_drop_target_info;
static GtkTargetEntry fl_opt_drop_target_entries [] = {
  {"text/uri-list", 0, URI_LIST},
};
#define ENTRIES_SIZE(n) sizeof(n)/sizeof(n[0]) 
static guint nfl_opt_drop_target_entries = ENTRIES_SIZE(fl_opt_drop_target_entries);
static void fl_opt_drag_data_received (GtkWidget * widget,
				       GdkDragContext * drag_context,
				       gint x, gint y,
				       GtkSelectionData * data,
				       guint info,
				       guint event_time,
				       gpointer user_data);
static at_fitting_opts_type * at_fitting_opts_new_from_uri(gchar * uri);


enum {
  VALUE_CHANGED,
  LAST_SIGNAL
};
static GtkNotebookClass *  parent_class;
static guint fl_option_signals[LAST_SIGNAL] = { 0 };

GtkType
frontline_option_get_type (void)
{
  static GtkType fl_option_type = 0;
  
  if (!fl_option_type)
    {
      static const GtkTypeInfo fl_option_info =
      {
        "FrontlineOption",
        sizeof (FrontlineOption),
        sizeof (FrontlineOptionClass),
        (GtkClassInitFunc) frontline_option_class_init,
        (GtkObjectInitFunc) frontline_option_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      
      fl_option_type = gtk_type_unique (GTK_TYPE_NOTEBOOK, &fl_option_info);
    }
  
  return fl_option_type;
}

static void
frontline_option_class_init  (FrontlineOptionClass * klass)
{
  GtkObjectClass * object_class;
  parent_class = gtk_type_class (gtk_notebook_get_type ());
  
  object_class 			   = (GtkObjectClass*)klass;
  object_class->finalize = frontline_option_finalize;  

  fl_option_signals[VALUE_CHANGED] =
    gtk_signal_new ("value_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (FrontlineOptionClass, value_changed),
		    gtk_marshal_NONE__NONE,
		    GTK_TYPE_NONE,
		    0);
  klass->value_changed = frontline_real_option_value_changed;

  gtk_object_class_add_signals (object_class, fl_option_signals, LAST_SIGNAL);
}

static void
frontline_option_init (FrontlineOption * fl_opt)
{
  GtkWidget * vbox;
  GtkWidget * hbox;
  GundoSequence * tmp;

  GtkWidget * pixmap;
  GtkWidget * label;

  vbox = gtk_vbox_new(TRUE, 0);
  gtk_notebook_append_page(GTK_NOTEBOOK(fl_opt), vbox, gtk_label_new(_("Options")));
  
  gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
  gtk_box_set_spacing(GTK_BOX(vbox), 8);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  fl_opt->block_count 	= 0;
  fl_opt->value_changed = FALSE;
  fl_opt->filesel       = gtk_file_selection_new("");
  gtk_object_ref(GTK_OBJECT(fl_opt->filesel));
  gtk_object_sink(GTK_OBJECT(fl_opt->filesel));

  /* Dirty */
  fl_opt->priv  = fl_opt_priv_new (fl_opt, GTK_BOX(vbox));
  fl_opt->opts  = fl_opt_priv_get_value (fl_opt->priv);
  fl_opt->opts  = at_fitting_opts_copy(fl_opt->opts);

  hbox 	      = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(vbox), hbox);
  

  fl_opt->undo_button 	 = gnome_stock_button(GNOME_STOCK_PIXMAP_UNDO);
  fl_opt->redo_button 	 = gnome_stock_button(GNOME_STOCK_PIXMAP_REDO);

  gtk_box_pack_start_defaults(GTK_BOX(hbox), fl_opt->undo_button);
  gtk_box_pack_start_defaults(GTK_BOX(hbox), fl_opt->redo_button);

  tmp = gundo_sequence_new();
  fl_opt->undo_seq = GTK_OBJECT(tmp);
  gtk_object_ref(fl_opt->undo_seq);
  gtk_object_sink(fl_opt->undo_seq);
  fl_opt->in_undo_action = FALSE;

  gtk_signal_connect(GTK_OBJECT(fl_opt->undo_button), 
		     "clicked", GTK_SIGNAL_FUNC(frontline_option_undo), fl_opt);
  gundo_make_undo_sensitive(fl_opt->undo_button, GUNDO_SEQUENCE(fl_opt->undo_seq));
  gtk_signal_connect(GTK_OBJECT(fl_opt->redo_button), 
		     "clicked", GTK_SIGNAL_FUNC(frontline_option_redo), fl_opt);
  gundo_make_redo_sensitive(fl_opt->redo_button, GUNDO_SEQUENCE(fl_opt->undo_seq));

  gtk_drag_dest_set(GTK_WIDGET(fl_opt), 
		    GTK_DEST_DEFAULT_ALL,
		    fl_opt_drop_target_entries,
		    nfl_opt_drop_target_entries,
		    GDK_ACTION_COPY);
  gtk_signal_connect(GTK_OBJECT(fl_opt),
		     "drag_data_received",
		     GTK_SIGNAL_FUNC(fl_opt_drag_data_received),
		     NULL);

  fl_opt->load_button = gnome_stock_button(GNOME_STOCK_PIXMAP_OPEN);
  fl_opt->save_button = gnome_stock_button(GNOME_STOCK_PIXMAP_SAVE);
  gtk_box_pack_start_defaults(GTK_BOX(hbox), fl_opt->load_button);
  gtk_box_pack_start_defaults(GTK_BOX(hbox), fl_opt->save_button);
  gtk_signal_connect(GTK_OBJECT(fl_opt->load_button), 
		     "clicked", GTK_SIGNAL_FUNC(frontline_option_load), fl_opt);
  gtk_signal_connect(GTK_OBJECT(fl_opt->save_button), 
		     "clicked", GTK_SIGNAL_FUNC(frontline_option_save), fl_opt);

  gtk_widget_show_all(vbox);

  /* Predefined */
#if 0
  vbox = gtk_vbox_new(TRUE, 0);
  gtk_notebook_append_page(GTK_NOTEBOOK(fl_opt), vbox, gtk_label_new(_("Predefined")));

  hbox 	      = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(vbox), hbox);

  load_button = gnome_stock_button(GNOME_STOCK_PIXMAP_OPEN);
  gtk_box_pack_start_defaults(GTK_BOX(hbox), load_button);
  gtk_signal_connect(GTK_OBJECT(load_button), 
		     "clicked", GTK_SIGNAL_FUNC(frontline_option_load), fl_opt);
  gtk_widget_show_all(vbox);
#endif /* 0 */

  /* About */
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_notebook_append_page(GTK_NOTEBOOK(fl_opt), vbox, gtk_label_new(_("About")));

  pixmap = gnome_pixmap_new_from_file (GNOME_ICONDIR "/fl-splash.png");
  gtk_container_add(GTK_CONTAINER(vbox), pixmap);  

  {
    gchar * msg;
    msg = g_strdup_printf(_("Frontline Vesion: %s"), VERSION);
    label = gtk_label_new(msg);
    g_free(msg);
    gtk_container_add(GTK_CONTAINER(vbox), label);
  }
  label = gtk_label_new(_("Frontline Author: Masatake YAMATO<jet@gyve.org>"));
  gtk_container_add(GTK_CONTAINER(vbox), label);  

  {
    gchar * msg;
    msg = g_strdup_printf(_("Autotrace Vesion: %s"), at_version(false));
    label = gtk_label_new(msg);
    g_free(msg);
    gtk_container_add(GTK_CONTAINER(vbox), label);  
  }
  label = gtk_label_new(_("Autotrace Author: Martin Weber<martweb@gmx.net>"));
  gtk_container_add(GTK_CONTAINER(vbox), label);  

  {
    gchar * msg;
    msg = g_strdup_printf(_("Project web page: %s"), at_home_site());
    label = gtk_label_new(msg);
    g_free(msg);
    gtk_container_add(GTK_CONTAINER(vbox), label);  
  }
  label = gtk_label_new(_("Copyright (C) 2002 Masatake YAMATO"));
  gtk_container_add(GTK_CONTAINER(vbox), label);
  label = gtk_label_new(_("This program is free software; you can redistribute\n"
			"it and/or modify it under the terms of the GNU\n"
			"General Public License as published by the Free\n"
			"Software Foundation; either version 2 of the License, \n"
			"or (at your option) any later version.\n"));
  gtk_container_add(GTK_CONTAINER(vbox), label);
  
  gtk_widget_show_all(vbox);
}

GtkWidget*
frontline_option_new (void)
{
  return frontline_option_new_with_value(NULL);
}

GtkWidget*
frontline_option_new_with_value (at_fitting_opts_type * value)
{
  GtkWidget *fl_opt;
  fl_opt = gtk_type_new (FRONTLINE_TYPE_OPTION);
  if (value)
    {
      frontline_option_set_value(FRONTLINE_OPTION(fl_opt), value);
      frontline_option_clear_undo_sequence (FRONTLINE_OPTION(fl_opt));
    }
  return fl_opt;
}

void
frontline_option_clear_undo_sequence (FrontlineOption * fl_opt)
{
  g_return_if_fail (fl_opt);
  g_return_if_fail (FRONTLINE_OPTION(fl_opt));

  gundo_sequence_clear (GUNDO_SEQUENCE(fl_opt->undo_seq));
}

at_fitting_opts_type *
frontline_option_get_value (FrontlineOption * fl_opt)
{
  g_return_val_if_fail (fl_opt, NULL);
  g_return_val_if_fail (FRONTLINE_IS_OPTION(fl_opt), NULL);
  g_return_val_if_fail (fl_opt->priv, NULL);
  return fl_opt_priv_get_value(fl_opt->priv);
}


void
frontline_option_set_value (FrontlineOption * fl_opt, at_fitting_opts_type * value)
{
  frontline_option_value_changed_block(fl_opt);
  fl_opt_priv_set_value(fl_opt->priv, value);
  frontline_option_value_changed_unblock(fl_opt);
}

void
frontline_option_value_changed (FrontlineOption * fl_opt)
{
  g_return_if_fail (fl_opt);

  fl_opt->value_changed = TRUE;
  if (fl_opt->block_count == 0)
    gtk_signal_emit (GTK_OBJECT (fl_opt), fl_option_signals[VALUE_CHANGED]);
}

void
frontline_option_value_changed_block (FrontlineOption * fl_opt)
{
  fl_opt->block_count++;
}

void
frontline_option_value_changed_unblock (FrontlineOption * fl_opt)
{
  fl_opt->block_count--;

  if (fl_opt->block_count == 0 && fl_opt->value_changed)
    frontline_option_value_changed(fl_opt);
}

static void
frontline_real_option_value_changed (FrontlineOption * fl_opt)
{
  FrontlineOptionUndoData * data;
  if (!fl_opt->in_undo_action)
    {
      data = fl_opt_undo_data_new(fl_opt->opts, fl_opt);
      gundo_sequence_add_action(GUNDO_SEQUENCE(fl_opt->undo_seq), 
				&fl_opt_undo_action,
				data);
      at_fitting_opts_free(fl_opt->opts);
      fl_opt->opts = fl_opt_priv_get_value(fl_opt->priv);
      fl_opt->opts = at_fitting_opts_copy(fl_opt->opts);
    }
  fl_opt->value_changed = FALSE;
}

static void
frontline_option_finalize    (GtkObject * object)
{
  FrontlineOption * fl_opt;
  g_return_if_fail (object != NULL);
  g_return_if_fail (FRONTLINE_IS_OPTION (object));
  
  fl_opt = FRONTLINE_OPTION(object);
  fl_opt_priv_free(fl_opt->priv);
  fl_opt->priv = NULL;
  at_fitting_opts_free(fl_opt->opts);
  gtk_object_ref(GTK_OBJECT(fl_opt->filesel));
  gtk_object_unref(fl_opt->undo_seq);
  GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

/*
 * Undo & Redo
 */
static void
frontline_option_undo (GtkButton * button, gpointer user_data)
{
  FrontlineOption * fl_opt = FRONTLINE_OPTION(user_data);
  fl_opt->in_undo_action = TRUE;
  gundo_sequence_undo(GUNDO_SEQUENCE(fl_opt->undo_seq));
  fl_opt->in_undo_action = FALSE;
}

static void
frontline_option_redo (GtkButton * button, gpointer user_data)
{
  FrontlineOption * fl_opt = FRONTLINE_OPTION(user_data);
  fl_opt->in_undo_action = TRUE;
  gundo_sequence_redo(GUNDO_SEQUENCE(fl_opt->undo_seq));
  fl_opt->in_undo_action = FALSE;
}

/*
 * Save & Load
 */
static void frontline_option_save_ok (GtkButton * button, gpointer user_data);
static void frontline_option_load_ok (GtkButton * button, gpointer user_data);
static void frontline_option_filesel_cancel (GtkObject * filesel);


static void
frontline_option_save (GtkButton * button, gpointer user_data)
{
  FrontlineOption * fl_opt = FRONTLINE_OPTION(user_data);
  GtkWidget * filesel 	= fl_opt->filesel;
  GtkWidget * ok_button = GTK_FILE_SELECTION(filesel)->ok_button; 
  GtkWidget * cancel_button = GTK_FILE_SELECTION(filesel)->cancel_button;
  
  gtk_window_set_title(GTK_WINDOW(filesel), _("Select Option File"));
  gtk_signal_connect(GTK_OBJECT(ok_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(frontline_option_save_ok),
		     fl_opt);
  gtk_signal_connect_object(GTK_OBJECT(cancel_button),
			    "clicked",
			    GTK_SIGNAL_FUNC(frontline_option_filesel_cancel),
			    GTK_OBJECT(fl_opt->filesel));
  gtk_signal_connect_object(GTK_OBJECT(fl_opt->filesel),
			    "delete_event",
			    GTK_SIGNAL_FUNC(frontline_option_filesel_cancel),
			    GTK_OBJECT(fl_opt->filesel));
  gtk_window_set_modal (GTK_WINDOW(filesel), TRUE);
  
  gtk_widget_show(filesel);
}

static void
frontline_option_save_ok (GtkButton * button, gpointer user_data)
{
  FrontlineOption * fl_opt = FRONTLINE_OPTION(user_data);
  GtkWidget * filesel 	= fl_opt->filesel;
  gchar * file_name;
  FILE * fp;
  int    fd;
  mode_t mode;
  int    chmod_result;
  gtk_window_set_modal (GTK_WINDOW(filesel), FALSE);
  gtk_widget_hide(GTK_WIDGET(filesel));
  file_name = gtk_file_selection_get_filename (GTK_FILE_SELECTION(filesel));
  if (!file_name)
    return;

  fp = fopen(file_name, "w");
  if (!fp)
    {
      GtkWidget * dialog;
      dialog = gnome_ok_dialog(_("Cannot open file to write"));
      /* TODO: destroy the dialog? */
      return;
    }

  at_fitting_opts_save(fl_opt->opts, fp);  
  fd = fileno(fp);
  g_assert(fd != -1);
  mode = S_IXUSR| S_IWUSR| S_IRUSR | S_IXOTH| S_IROTH | S_IXGRP| S_IRGRP;
  chmod_result = fchmod(fd, mode);
  fclose(fp);
  if (chmod_result < -1)
    {
      GtkWidget * dialog;
      gchar * msg;
      msg = g_strdup_printf("%s: %s", file_name, g_strerror(errno));
      dialog = gnome_ok_dialog(msg);
      g_free(msg);
      return;
    }
}


static void
frontline_option_filesel_cancel (GtkObject * filesel)
{
  gtk_window_set_modal (GTK_WINDOW(filesel), FALSE);
  gtk_widget_hide(GTK_WIDGET(filesel));
}

static void
frontline_option_load (GtkButton * button, gpointer user_data)
{
  FrontlineOption * fl_opt = FRONTLINE_OPTION(user_data);
  GtkWidget * filesel 	= fl_opt->filesel;
  GtkWidget * ok_button = GTK_FILE_SELECTION(filesel)->ok_button; 
  GtkWidget * cancel_button = GTK_FILE_SELECTION(filesel)->cancel_button;
  
  gtk_window_set_title(GTK_WINDOW(filesel), _("Select Option File"));
  gtk_signal_connect(GTK_OBJECT(ok_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(frontline_option_load_ok),
		     fl_opt);
  gtk_signal_connect_object(GTK_OBJECT(cancel_button),
			    "clicked",
			    GTK_SIGNAL_FUNC(frontline_option_filesel_cancel),
			    GTK_OBJECT(fl_opt->filesel));
  gtk_signal_connect_object(GTK_OBJECT(fl_opt->filesel),
			    "delete_event",
			    GTK_SIGNAL_FUNC(frontline_option_filesel_cancel),
			    GTK_OBJECT(fl_opt->filesel));
  gtk_window_set_modal (GTK_WINDOW(filesel), TRUE);
  
  gtk_widget_show(filesel);
}

static void
frontline_option_load_ok (GtkButton * button, gpointer user_data)
{
  FrontlineOption * fl_opt = FRONTLINE_OPTION(user_data);
  GtkWidget * filesel 	= fl_opt->filesel;
  gchar * file_name;
  FILE * fp;
  at_fitting_opts_type * loaded_opts;
  
  gtk_window_set_modal (GTK_WINDOW(filesel), FALSE);
  gtk_widget_hide(GTK_WIDGET(filesel));
  file_name = gtk_file_selection_get_filename (GTK_FILE_SELECTION(filesel));
  if (!file_name)
    return;

  fp = fopen(file_name, "r");
  if (!fp)
    {
      GtkWidget * dialog;
      dialog = gnome_ok_dialog(_("Cannot open file to load"));
      /* TODO: destroy the dialog? */
      return;
    }
  loaded_opts = at_fitting_opts_new_from_file(fp);
  if (loaded_opts)
    {
      frontline_option_set_value(fl_opt, loaded_opts);
      at_fitting_opts_free(loaded_opts);
    }
  
  fclose(fp);
}

/*
 * DnD
 */
void 
fl_opt_drag_data_received (GtkWidget * widget,
			   GdkDragContext * drag_context,
			   gint x, gint y,
			   GtkSelectionData * data,
			   guint info,
			   guint event_time,
			   gpointer user_data)
{
  gchar * uri;
  at_fitting_opts_type * opts;

  switch(info) {
  case URI_LIST:
    uri  = (gchar *)data->data;
    opts = at_fitting_opts_new_from_uri(uri);
    if (opts)
      {
	frontline_option_set_value(FRONTLINE_OPTION(widget),
				   opts);
	at_fitting_opts_free(opts);
	
      }
    else
      {
	gchar * msg = g_strdup_printf(_("Cannot load autotrace option file: %s"),
				      uri);
	gnome_error_dialog(msg);
	g_free(msg);
      }
    break;
  }
}

static at_fitting_opts_type *
at_fitting_opts_new_from_uri(gchar * buffer)
{
  at_fitting_opts_type * opts = NULL;
  gchar * filename;
  GList * list = gnome_uri_list_extract_filenames(buffer);
  FILE * fp;
  
  if (!list)
    return NULL;

  filename = (gchar *)list->data;
  
  fp 	   = fopen(filename, "r");
  if (fp)
    {
      opts = at_fitting_opts_new_from_file(fp);
      fclose(fp);
      gnome_uri_list_free_strings(list);
    }
  return opts;
}
