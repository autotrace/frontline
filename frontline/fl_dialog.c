/* fl_dialog.c --- Frontline dialog class
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

#define FL_COUNT_TIME 0

#include "frontline.h"
#include <gtk/gtksignal.h>
#include <glib.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/gnome-window-icon.h>

static void frontline_dialog_class_init  (FrontlineDialogClass * klass);
static void frontline_dialog_init        (FrontlineDialog      * fl_dialog);
static void frontline_dialog_finalize    (GtkObject * object);
static void frontline_real_dialog_set_bitmap (FrontlineDialog      * fl_dialog);
static void frontline_dialog_trace (GtkButton * button, gpointer * user_data);

static void frontline_dialog_trace_started(FrontlineDialog      * fl_dialog);
static void frontline_dialog_trace_done(FrontlineDialog      * fl_dialog);

static void frontline_dialog_show_error(at_string msg,
					at_msg_type msg_type,
					at_address client_data);
static void frontline_dialog_close_error(GtkButton * button, 
					 gpointer user_data);
static GtkWidget * frontline_dialog_error_action_new(at_string msg, 
						     at_msg_type msg_type);

static void frontline_dialog_notify_progress(at_real percentage,
					     at_address user_data);

static at_bool frontline_dialog_test_cancel(at_address user_data);
static void frontline_dialog_set_stopped(FrontlineProgress * fl_prog,
					 gpointer user_data);
					 
enum {
  SET_BITMAP,
  TRACE_STARTED,
  TRACE_DONE,
  LAST_SIGNAL
};
static GtkDialogClass *  parent_class;
static guint fl_dialog_signals[LAST_SIGNAL] = { 0 };

GtkType
frontline_dialog_get_type (void)
{
  static GtkType fl_dialog_type = 0;
  
  if (!fl_dialog_type)
    {
      static const GtkTypeInfo fl_dialog_info =
      {
        "FrontlineDialog",
        sizeof (FrontlineDialog),
        sizeof (FrontlineDialogClass),
        (GtkClassInitFunc) frontline_dialog_class_init,
        (GtkObjectInitFunc) frontline_dialog_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      
      fl_dialog_type = gtk_type_unique (GTK_TYPE_DIALOG, &fl_dialog_info);
    }
  return fl_dialog_type;
}

static void
frontline_dialog_class_init  (FrontlineDialogClass * klass)
{
  GtkObjectClass * object_class;
  parent_class = gtk_type_class (gtk_dialog_get_type ());
  
  object_class = (GtkObjectClass*)klass;
  object_class->finalize = frontline_dialog_finalize;  

  fl_dialog_signals[SET_BITMAP] =
    gtk_signal_new ("set_bitmap",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (FrontlineDialogClass, set_bitmap),
		    gtk_marshal_NONE__NONE,
		    GTK_TYPE_NONE,
		    0);
  klass->set_bitmap = frontline_real_dialog_set_bitmap;

  fl_dialog_signals[TRACE_STARTED] =
    gtk_signal_new ("trace_started",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (FrontlineDialogClass, trace_started),
		    gtk_marshal_NONE__NONE,
		    GTK_TYPE_NONE,
		    0);
  klass->trace_started = frontline_dialog_trace_started;

  fl_dialog_signals[TRACE_DONE] =
    gtk_signal_new ("trace_done",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (FrontlineDialogClass, trace_done),
		    gtk_marshal_NONE__NONE,
		    GTK_TYPE_NONE,
		    0);
  klass->trace_done = frontline_dialog_trace_done;

  gtk_object_class_add_signals (object_class, fl_dialog_signals, LAST_SIGNAL);  
}

static void
frontline_dialog_init (FrontlineDialog * fl_dialog)
{
  GtkWidget  * dialog = GTK_WIDGET(fl_dialog);

  gtk_window_set_policy(GTK_WINDOW(fl_dialog),
			FALSE,
			FALSE,
			TRUE);

  fl_dialog->bitmap 	  = NULL;
  fl_dialog->splines 	  = NULL;
  fl_dialog->error_action = NULL;

  fl_dialog->header_area = gtk_vbox_new (FALSE, 0);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), fl_dialog->header_area);
  gtk_widget_show(fl_dialog->header_area);
  
  fl_dialog->option  = frontline_option_new();
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), fl_dialog->option);
  gtk_widget_show(fl_dialog->option);
  
  /* Wait action area */
  fl_dialog->wait_actions = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), fl_dialog->wait_actions, TRUE, TRUE, 0);
  gtk_widget_show(fl_dialog->wait_actions);

  
  fl_dialog->trace_button = gtk_button_new_with_label("Trace");
  gtk_box_pack_start(GTK_BOX(fl_dialog->wait_actions), fl_dialog->trace_button, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (fl_dialog->trace_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (fl_dialog->trace_button);
  gtk_widget_set_sensitive(fl_dialog->trace_button, FALSE);
  gtk_widget_show(fl_dialog->trace_button);
  gtk_signal_connect(GTK_OBJECT(fl_dialog->trace_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(frontline_dialog_trace),
		     fl_dialog);
  
  fl_dialog->close_button = gtk_button_new_with_label("Close");
  GTK_WIDGET_SET_FLAGS (fl_dialog->close_button, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(fl_dialog->wait_actions), fl_dialog->close_button, TRUE, TRUE, 0);
  gtk_widget_show(fl_dialog->close_button);

  gtk_widget_ref(fl_dialog->wait_actions);
    
  /* Wait action area */
  fl_dialog->run_actions = frontline_progress_new();
  gtk_widget_show(fl_dialog->run_actions);
  gtk_widget_ref(fl_dialog->run_actions);
  
  if (g_file_exists (GNOME_ICONDIR "/frontline.png") )
    gnome_window_icon_set_from_file(GTK_WINDOW(fl_dialog), GNOME_ICONDIR "/frontline.png");
  else
    g_warning ("Colud not find %s", GNOME_ICONDIR "/frontline.png");
}

static void
frontline_dialog_finalize    (GtkObject * object)
{
  FrontlineDialog * fl_dialog;
  g_return_if_fail (object != NULL);
  g_return_if_fail (FRONTLINE_IS_DIALOG (object));

  fl_dialog = FRONTLINE_DIALOG(object);

  if (fl_dialog->bitmap)
    at_bitmap_free(fl_dialog->bitmap);
  if (fl_dialog->splines)
    at_splines_free(fl_dialog->splines);

  gtk_widget_unref(fl_dialog->run_actions);
  gtk_widget_unref(fl_dialog->wait_actions);

  GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget*
frontline_dialog_new ()
{
  return frontline_dialog_new_with_opts(NULL);
}

GtkWidget*
frontline_dialog_new_with_opts (at_fitting_opts_type * value)
{
  GtkWidget *fl_dialog;
  GtkWidget *fl_opt;
  fl_dialog = gtk_type_new (FRONTLINE_TYPE_DIALOG);
  fl_opt    = FRONTLINE_DIALOG(fl_dialog)->option;
  if (value)
    {
      frontline_option_set_value(FRONTLINE_OPTION(fl_opt), value);
      frontline_option_clear_undo_sequence(FRONTLINE_OPTION(fl_opt));
    }
  return fl_dialog;
}

void
frontline_dialog_set_bitmap(FrontlineDialog  *fl_dialog,
			    at_bitmap_type * bitmap)
{
  g_return_if_fail (fl_dialog);
  g_return_if_fail (FRONTLINE_IS_DIALOG(fl_dialog));
  g_return_if_fail (bitmap);
  
  if (fl_dialog->bitmap)
    at_bitmap_free(fl_dialog->bitmap);
  fl_dialog->bitmap = bitmap;
  gtk_signal_emit(GTK_OBJECT (fl_dialog),
		  fl_dialog_signals[SET_BITMAP]);
}
  
static void
frontline_real_dialog_set_bitmap (FrontlineDialog * fl_dialog)
{
  gtk_widget_set_sensitive(fl_dialog->trace_button, TRUE);  
}

static void
frontline_dialog_trace (GtkButton * button, gpointer * user_data)
{
  FrontlineDialog * fl_dialog;
  at_bitmap_type * bitmap;
  at_fitting_opts_type * opts;
  at_splines_type * splines;
  gboolean stopped = FALSE;
  guint connection_id;
  GTimer* profile;
  gulong prof_result;
  
  g_return_if_fail (button);
  g_return_if_fail (GTK_IS_BUTTON(button));
  g_return_if_fail (user_data);
  g_return_if_fail (FRONTLINE_IS_DIALOG(user_data));

  fl_dialog = FRONTLINE_DIALOG(user_data);

  bitmap    = fl_dialog->bitmap;
  g_return_if_fail (bitmap);
  opts      = frontline_option_get_value(FRONTLINE_OPTION(fl_dialog->option));
  g_return_if_fail (opts);

  connection_id = gtk_signal_connect(GTK_OBJECT(fl_dialog->run_actions),
				     "stopped",
				     GTK_SIGNAL_FUNC(frontline_dialog_set_stopped),
				     &stopped);
  
  gtk_signal_emit(GTK_OBJECT (fl_dialog),
		  fl_dialog_signals[TRACE_STARTED]);

  bitmap = at_bitmap_copy(bitmap);

  profile = g_timer_new ();
  at_fitting_opts_save(opts, stdout);
  g_timer_start(profile);
  splines   = at_splines_new_full(bitmap, opts,
				  frontline_dialog_show_error, fl_dialog,
				  frontline_dialog_notify_progress, fl_dialog,
				  frontline_dialog_test_cancel, &stopped);
  g_timer_stop(profile);
  if (FL_COUNT_TIME) 
    g_message("->%lf\n", (gdouble)g_timer_elapsed(profile, &prof_result));
  g_timer_destroy(profile);
  
  at_bitmap_free(bitmap);

  if (fl_dialog->splines)
    at_splines_free(fl_dialog->splines);
  fl_dialog->splines = splines;
  gtk_signal_emit(GTK_OBJECT (fl_dialog),
		  fl_dialog_signals[TRACE_DONE]);  
  
  gtk_signal_disconnect(GTK_OBJECT(fl_dialog->run_actions),
			connection_id);
}

static void
frontline_dialog_trace_started(FrontlineDialog      * fl_dialog)
{
  gtk_container_remove(GTK_CONTAINER(GTK_DIALOG(fl_dialog)->action_area), fl_dialog->wait_actions);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(fl_dialog)->action_area), fl_dialog->run_actions);
  gtk_widget_show_all(fl_dialog->run_actions);
  gtk_main_iteration_do(FALSE);
}

static void
frontline_dialog_trace_done(FrontlineDialog      * fl_dialog)
{
  if (fl_dialog->error_action)
    {
      gtk_container_remove(GTK_CONTAINER(GTK_DIALOG(fl_dialog)->action_area), fl_dialog->error_action);
      fl_dialog->error_action = NULL;
    }
  else
    gtk_container_remove(GTK_CONTAINER(GTK_DIALOG(fl_dialog)->action_area), fl_dialog->run_actions);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(fl_dialog)->action_area), fl_dialog->wait_actions);
  gtk_widget_show_all(fl_dialog->wait_actions);
  gtk_main_iteration_do(FALSE);
}

static void
frontline_dialog_notify_progress(at_real percentage,
				 at_address user_data)
{
  int n;
  FrontlineDialog * fl_dialog = FRONTLINE_DIALOG(user_data);
  FrontlineProgress * fl_prog = FRONTLINE_PROGRESS(fl_dialog->run_actions);

  n = percentage * 100000;
  if (n % 1000 == 0)
    {
      gtk_adjustment_set_value(GTK_ADJUSTMENT(fl_prog->adj), percentage);
      gtk_main_iteration_do(FALSE);
    }
}

static at_bool
frontline_dialog_test_cancel(at_address user_data)
{
  return (at_bool)*(gboolean *)user_data;
}

static void
frontline_dialog_set_stopped(FrontlineProgress * fl_prog,
			     gpointer user_data)
{
  *((gboolean *)user_data) = TRUE;
}

static void
frontline_dialog_show_error(at_string msg,
			    at_msg_type msg_type,
			    at_address client_data)
{
  FrontlineDialog * fl_dialog = FRONTLINE_DIALOG (client_data) ;
  
  if (msg_type == AT_MSG_WARNING)
    {
      g_warning(msg);
      return ;
    }

  gtk_container_remove(GTK_CONTAINER(GTK_DIALOG(fl_dialog)->action_area), 
		       fl_dialog->run_actions);
  fl_dialog->error_action = frontline_dialog_error_action_new(msg, msg_type);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(fl_dialog)->action_area), 
		    fl_dialog->error_action);
  gtk_widget_show_all(fl_dialog->error_action);

  gtk_main();
}

static void
frontline_dialog_close_error(GtkButton * button, gpointer user_data)
{
  gtk_grab_remove(GTK_WIDGET(user_data));
  gtk_main_quit();
}

static GtkWidget *
frontline_dialog_error_action_new(at_string msg,
				  at_msg_type msg_type)
{
  GtkWidget * hbox, *label, * button;
  gchar * msg_long;
  
  hbox = gtk_hbox_new(FALSE, 4);
  
  msg_long = g_strdup_printf("Error in trace: %s", msg);
  label = gtk_label_new(msg_long);
  g_free(msg_long);
  
  gtk_widget_show(label);
  gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
  button = gtk_button_new_with_label("Ok");
  gtk_widget_show(button);
  gtk_box_pack_start_defaults(GTK_BOX(hbox), button);
  
  gtk_signal_connect(GTK_OBJECT(button),
		     "clicked",
		     frontline_dialog_close_error,
		     hbox);
  gtk_grab_add(hbox);
  return hbox;
}
