/* frontline.c --- GUI frontend for autotrace
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

/* Old version of popt is built in lingnome.
   The old version cannot handle POPT_ARG_FLOAT that
   is needed to set fields of `at_fitting_opts_popt'.
   In other hand the newest popt(1.6.3) can handle
   POPT_ARG_FLOAT. Till POPT_ARG_FLOAT handler is
   available in gnome, frontline uses libpopt directly.
   It means frontline cannot use gnome_init_with_popt_table. */
#define INCLUDE_AT_POPT_TABLE 0

#if !INCLUDE_AT_POPT_TABLE
#include <popt.h>
#ifndef POPT_TABLEEND
#define POPT_TABLEEND { NULL, '\0', 0, 0, 0, NULL, NULL }
#endif /* Not def: POPT_TABLEEND */
#endif /* NOT_INCLUDE_AT_POPT_TABLE */

#include <gnome.h>
#include "frontline.h"
#include <errno.h>

static void quit_callback            (GtkWidget * widget, gpointer no_use);
static void set_bitmap               (FrontlineFileSelection * fsel,
				      gchar * filename,
				      at_bitmap_type * bitmap,
				      gpointer user_data);
static void preview_image            (FrontlineFileSelection * fsel,
				      gchar * filename,
				      at_bitmap_type * bitmap,
				      gpointer user_data);
static void preview_splines          (FrontlineDialog * fl_dialog,
				      gpointer user_data);
static void load_options_file        (poptContext con,
				      enum poptCallbackReason foo,
				      struct poptOption * key,
				      char * arg, void * data);
static void open_filesel             (FrontlinePreview * fl_preview,
				      gpointer user_data);
static void save_splines             (GtkButton * button,
				      gpointer user_data);
static void msg_write                (at_string msg, 
				      at_msg_type msg_type, 
				      at_address client_data);
static void splash                   (const gchar * file);

static void frontline_popt_table_init(struct poptOption * fl_popt_table,
				      struct poptOption * at_popt_table,
				      at_fitting_opts_type * at_opts);


static struct poptOption frontline_popt_table [] = {
  { NULL, '\0', POPT_ARG_CALLBACK, (void *)load_options_file, '\0', NULL, NULL },
#if INCLUDE_AT_POPT_TABLE
  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, NULL, 0, "Autotrace options:", NULL },
#endif
  { "options-file", '\0', POPT_ARG_STRING|POPT_ARGFLAG_STRIP, NULL, 0, "Autotrace options file name", "filename"},
  POPT_TABLEEND
};

int
main(int argc, char ** argv)
{
  GtkWidget * dialog;
  GtkWidget * fsel;
  GtkWidget * sep;
  GtkWidget * preview;
  gchar * filename = NULL;
  
  struct poptOption at_popt_table[at_fitting_opts_popt_table_length];
  at_fitting_opts_type * opts = NULL;
  poptContext popt_ctx = NULL;

  /*
   * Parsing args
   */
  opts = at_fitting_opts_new();
  frontline_popt_table_init(frontline_popt_table,
			    at_popt_table,
			    opts);
#if INCLUDE_AT_POPT_TABLE
  gnome_init_with_popt_table(PACKAGE,
			     VERSION,
			     argc, argv,
			     frontline_popt_table,
			     0,
			     &popt_ctx);
  poptFreeContext (popt_ctx);
#else
  popt_ctx = poptGetContext(PACKAGE, argc, argv, frontline_popt_table, 0);
  while (poptGetNextOpt(popt_ctx) != -1) ;
  argc = poptStrippedArgv(popt_ctx, argc, argv);
  poptFreeContext (popt_ctx);
  gnome_init(PACKAGE, VERSION, argc, argv);
  if (argc > 1)
    filename = argv[1];
#endif /* INCLUDE_AT_POPT_TABLE */

  
  splash(GNOME_ICONDIR "/fl-splash.png");
  
  /*
   * Set up dialog
   */
  dialog = frontline_dialog_new_with_opts(opts);
  at_fitting_opts_free(opts);

  gtk_container_set_border_width(GTK_CONTAINER(dialog), 4);
  gtk_signal_connect(GTK_OBJECT(dialog),
		     "delete_event",
		     GTK_SIGNAL_FUNC(quit_callback),
		     NULL);
  gtk_signal_connect(GTK_OBJECT(FRONTLINE_DIALOG(dialog)->close_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(quit_callback),
		     NULL);

  fsel = frontline_file_selection_new();
  gtk_widget_show(fsel);
  gtk_box_pack_start_defaults(GTK_BOX(FRONTLINE_DIALOG(dialog)->header_area),
			      fsel);
  sep = gtk_hseparator_new();
  gtk_widget_show(sep);
  gtk_box_pack_start_defaults(GTK_BOX(FRONTLINE_DIALOG(dialog)->header_area),
			      sep);

  gtk_signal_connect(GTK_OBJECT(fsel),
		     "loaded",
		     set_bitmap,
		     dialog);
  gtk_widget_show(dialog);

  /*
   * Set up preview
   */

  preview = frontline_preview_new ();
  gtk_quit_add_destroy(1, GTK_OBJECT(preview));
  gtk_window_set_title(GTK_WINDOW(preview), "fl preview");
  gtk_signal_connect(GTK_OBJECT(fsel),
		     "loaded",
		     GTK_SIGNAL_FUNC(preview_image),
		     preview);
  gtk_signal_connect(GTK_OBJECT(dialog),
		     "trace_done",
		     GTK_SIGNAL_FUNC(preview_splines),
		     preview);
  gtk_signal_connect_object(GTK_OBJECT(preview),
			    "delete_event",
			    GTK_SIGNAL_FUNC(gtk_widget_hide),
			    GTK_OBJECT(preview));
  gtk_signal_connect (GTK_OBJECT(preview),
		      "request_to_save",
		      GTK_SIGNAL_FUNC(open_filesel),
		      dialog);
  gtk_window_set_transient_for(GTK_WINDOW(preview),
			       GTK_WINDOW(dialog));

  if (filename)
    if (!frontline_file_selection_load_file(FRONTLINE_FILE_SELECTION(fsel), 
					    filename))
      {
	g_warning("Cannot load file: %s", filename);
      }

  gtk_main();

  return EXIT_SUCCESS;
}

static void
quit_callback(GtkWidget * widget, gpointer no_use)
{
  gtk_main_quit();
}

static void
set_bitmap(FrontlineFileSelection * fsel,
	   gchar * filename,
	   at_bitmap_type * bitmap,
	   gpointer user_data)
{
  FrontlineDialog * dialog = FRONTLINE_DIALOG(user_data);

  bitmap = at_bitmap_copy(bitmap);
  frontline_dialog_set_bitmap(dialog, bitmap);
}

static void
frontline_popt_table_init(struct poptOption * fl_popt_table,
			  struct poptOption * at_popt_table,
			  at_fitting_opts_type * at_opts)
{
  /* TODO: Should search "options-file" */
  fl_popt_table[0].descrip = (void *)at_opts;
#if INCLUDE_AT_POPT_TABLE
  at_fitting_opts_popt_init(at_popt_table, at_opts);
  popt_table[1].arg = at_popt_table;
#endif
}

static void
load_options_file(poptContext con,
		  enum poptCallbackReason foo,
		  struct poptOption * key,
		  char * arg, void * data)
{
  FILE * opt_fp;
  char * options_file_name;
  at_fitting_opts_type * loaded_opts;
  at_fitting_opts_type * target_opts = data;
  
  if (!strcmp(key->longName, "options-file"))
    {
      options_file_name = arg;
      opt_fp = fopen(options_file_name, "r");
      if (opt_fp)
	{
	  loaded_opts = at_fitting_opts_new_from_file(opt_fp);
	  fclose(opt_fp);
	  
	  if (target_opts->background_color)
	    at_color_free(target_opts->background_color);
	  *target_opts = *loaded_opts;
	  if (loaded_opts->background_color)
	    loaded_opts->background_color = NULL;
	  
	  at_fitting_opts_free (loaded_opts);
	}
      else
	{
	  gchar * msg = g_strdup_printf("%s: %s", 
					options_file_name, 
					g_strerror(errno));
	  gnome_error_dialog(msg);
	  g_free(msg);
	}
    }
}

static void
preview_image(FrontlineFileSelection * fsel,
	      gchar * filename,
	      at_bitmap_type * bitmap,
	      gpointer user_data)
{
  frontline_preview_set_image_by_file(FRONTLINE_PREVIEW(user_data),
				      filename);
  frontline_preview_show_image(FRONTLINE_PREVIEW(user_data),
			       TRUE);
  gtk_widget_show(GTK_WIDGET(user_data));
}

static void
preview_splines          (FrontlineDialog * fl_dialog,
			  gpointer user_data)
{
  g_return_if_fail (fl_dialog);
  g_return_if_fail (FRONTLINE_IS_DIALOG(fl_dialog));
  g_return_if_fail (user_data);
  g_return_if_fail (FRONTLINE_IS_PREVIEW(user_data));

  if (frontline_preview_set_splines(FRONTLINE_PREVIEW(user_data),
				    fl_dialog->splines))
    frontline_preview_show_splines(FRONTLINE_PREVIEW(user_data),
				   FL_PREVIEW_SHOW_AUTO);
  gtk_widget_show(GTK_WIDGET(user_data));
}

static void
open_filesel             (FrontlinePreview * fl_preview,
			  gpointer user_data)
{
  GtkWidget *file_selector;
  GtkWidget * ok_button;
  file_selector = fl_save_file_selection_new();
  ok_button	= GTK_FILE_SELECTION(file_selector)->ok_button;

  gtk_object_set_data(GTK_OBJECT(ok_button),
		      "splines",
		      FRONTLINE_DIALOG(user_data)->splines);
  gtk_signal_connect (GTK_OBJECT(ok_button),
		      "clicked",
		      GTK_SIGNAL_FUNC (save_splines),
		      file_selector);
  gtk_signal_connect_object (GTK_OBJECT(ok_button),
			     "clicked",
			     GTK_SIGNAL_FUNC (gtk_grab_remove),
			     (gpointer) file_selector);
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button),
			     "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     (gpointer) file_selector);
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->cancel_button),
			     "clicked", GTK_SIGNAL_FUNC (gtk_grab_remove),
			     (gpointer) file_selector);
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->cancel_button),
			     "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     (gpointer) file_selector);
  gtk_widget_show (file_selector);
  gtk_grab_add(file_selector);
}

static void
save_splines             (GtkButton * button, gpointer user_data)
{
  at_splines_type * splines;
  gchar * filename, * ext;
  at_output_write_func writer;
  FILE * fp;
  
  splines  = gtk_object_get_data(GTK_OBJECT(button), "splines");
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION(user_data));
  ext 	   = fl_save_file_selection_get_extension(GTK_FILE_SELECTION(user_data));
  
  g_return_if_fail (splines);
  g_return_if_fail (filename);

  if (ext)
    writer = at_output_get_handler_by_suffix(ext);
  else
    writer = at_output_get_handler(filename);
  g_return_if_fail (writer);
  
  fp = fopen(filename, "w");
  if (!fp)
    {
      gchar * msg = g_strconcat("Cannot open: ", 
				filename, 
				"\n",
				g_strerror(errno));
      gnome_error_dialog_parented  (msg, GTK_WINDOW(user_data));
      g_free(msg);
      return ;
    }

  at_splines_write(splines, fp, filename, AT_DEFAULT_DPI, writer,
		   msg_write, user_data);
  fclose(fp);
}

static void
msg_write                (at_string msg, 
			  at_msg_type msg_type, 
			  at_address client_data)
{
  GtkWindow * window = GTK_WINDOW(client_data);
  if (msg_type == AT_MSG_FATAL)
    gnome_error_dialog_parented  (msg, window);
  else
    gnome_warning_dialog_parented(msg, window);
}

static void
splash                    (const gchar * file)
{
  GtkWidget * window;
  GtkWidget * pixmap;
  GtkWidget * vbox;
  GtkWidget * label;
  window = gtk_window_new(GTK_WINDOW_POPUP);
  gtk_window_set_position(GTK_WINDOW(window), 
			  GTK_WIN_POS_CENTER_ALWAYS);
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);  

  pixmap = gnome_pixmap_new_from_file (file);
  gtk_container_add(GTK_CONTAINER(vbox), pixmap);  
  
  // label = gtk_label_new("by Masatake YAMATO<jet@gyve.org>");
  // gtk_container_add(GTK_CONTAINER(vbox), label);  
  // label = gtk_label_new("Use under term of GNU GPL");
  // gtk_container_add(GTK_CONTAINER(vbox), label);  
  
  gtk_widget_show_all(GTK_WIDGET(window));
  gtk_main_iteration_do(FALSE);

  gtk_timeout_add(2000, gtk_widget_destroy, window);

  return;
}
