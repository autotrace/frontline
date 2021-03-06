/* trace.c --- autotrace/frontline plugin for the GIMP
 
  Copyright (C) 2002 Masatake YAMATO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */ 

#include "config.h"
#include <gnome.h>
#include "frontline/frontline.h"

#include <autotrace/autotrace.h>
#include <errno.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#define PLUG_IN_NAME "plug_in_trace"
#define PLUG_IN_VERSION "0.0.0"
#define PLUG_IN_PIVALS_GENERATION "0"

#define GIMP_DRAWABLE_TO_AT_BITMAP_DEBUG 1

static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GimpParam  *param,
		   gint    *nreturn_vals,
		   GimpParam **return_vals);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

typedef struct _ReloadData {
  FrontlineDialog * dialog;
  GimpDrawable * drawable;
} ReloadData;

static at_fitting_opts_type opts_pivals;
static at_color_type        background_color_pivals;

static void             frontline                  (GimpDrawable *drawable, at_fitting_opts_type * opts, const gchar * file_name);

static void             get_data                   (at_fitting_opts_type * opts, at_color_type * background_color);
static void             set_data                   (at_fitting_opts_type * opts);
static GtkWidget*       header_new                 (const gchar * file_name, const gchar * drawable_name);
static void             preview_splines            (FrontlineDialog * fl_dialog, gpointer user_data);
static void             quit_callback              (GtkWidget * widget, gpointer no_use);
static at_bitmap_type * gimp_drawable_to_at_bitmap (GimpDrawable * drawable, at_fitting_opts_type * opts);

static void             open_filesel                (FrontlinePreview * fl_preview,
						     gpointer user_data);
static void             save_splines                (GtkButton * button,
						     gpointer user_data);
static void             msg_write                    (at_string msg, 
						      at_msg_type msg_type, 
						      at_address client_data);
static void             reload_bitmap                (GtkButton * button,
						      gpointer user_data);
static void             reset_bitmap                 (FrontlineDialog * dialog,
						      gpointer user_data);

static void
query (void)
{
  static GimpParamDef args[] =
  {
    /* 0 */{ GIMP_PDB_INT32,    "run_mode", "Interactive, non-interactive" },
    /* 1 */{ GIMP_PDB_IMAGE,    "image",    "Input image (unused)" },
    /* 2 */{ GIMP_PDB_DRAWABLE, "drawable", "Input drawable" }
  };
  static gint nargs = sizeof(args) / sizeof (args[0]);

  const gchar *blurb 
    = "Trace the outline (or centerline) of image.";
  const gchar *help = "";
  const gchar *author = "Masatake YAMATO<jet@gyve.org>";
  const gchar *copyrights = "Masatake YAMATO";
  const gchar *copyright_date = "2002";
  
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);

  frontline_init();  

  gimp_install_procedure (PLUG_IN_NAME,
			  (gchar *) blurb,
			  (gchar *) help,
			  (gchar *) author,
			  (gchar *) copyrights,
			  (gchar *) copyright_date,
			  _("<Image>/Filters/Trace/Trace..."),
			  "RGB*, GRAY*",
			  GIMP_PLUGIN,
			  nargs, 0,
			  args, NULL);
}

static void
run (gchar   *name, 
     gint     nparams, 
     GimpParam  *param, 
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  char * dummy_argv[] = {"trace"};
  int dummy_argc      = 1;
  
  guint32       image_ID;
  gchar        *image_name ;
  guint32       drawable_ID;
  GimpDrawable *drawable;
  GimpRunModeType run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  static GimpParam values[1];

  at_fitting_opts_type * opts;
  
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);

  frontline_init();
  
  run_mode    = param[0].data.d_int32;
  image_ID    = param[1].data.d_image;
  drawable_ID = param[2].data.d_drawable;
  drawable = gimp_drawable_get(drawable_ID);

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  *nreturn_vals = 1;
  *return_vals = values;

  opts 	      		       = at_fitting_opts_new();
  opts_pivals 		       = *opts;
  opts_pivals.background_color = opts->background_color;
  
  if (gimp_drawable_is_rgb (drawable->id) ||
      gimp_drawable_is_gray (drawable->id))
    {
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  gimp_ui_init ("trace", FALSE);
	  gnome_init("trace", PLUG_IN_VERSION, dummy_argc, dummy_argv);
	  
	  get_data(&opts_pivals, &background_color_pivals);
	  image_name = gimp_image_get_filename(image_ID);
	  frontline(drawable, 
		    &opts_pivals,
		    image_name);
	  g_free(image_name);
	  set_data(&opts_pivals);

	  break;

	default:
	  status = GIMP_PDB_EXECUTION_ERROR;
	  break;
        }
    }
  else
    status = GIMP_PDB_EXECUTION_ERROR;
  
  values[0].data.d_status = status;
  gimp_drawable_detach (drawable);
}

static void
frontline (GimpDrawable *drawable,
	   at_fitting_opts_type * opts,
	   const gchar * file_name)
{
  GtkWidget * dialog;
  GtkWidget * preview;
  GtkWidget * header_area;
  GtkWidget * header_button;
  GtkWidget * header_sep;  
  at_bitmap_type * bitmap;
  ReloadData reload_data;
  GtkTooltips * tooltips;
  
  dialog = frontline_dialog_new_with_opts(opts);
  gtk_window_set_title(GTK_WINDOW(dialog), _("Trace"));
  gtk_signal_connect(GTK_OBJECT(dialog),
		     "delete_event",
		     GTK_SIGNAL_FUNC(quit_callback),
		     NULL);
  gtk_signal_connect(GTK_OBJECT(FRONTLINE_DIALOG(dialog)->close_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(quit_callback),
		     NULL);

  header_area 	= header_new(file_name, gimp_drawable_name(drawable->id));
  header_button = gtk_button_new();
  gtk_container_add(GTK_CONTAINER(header_button), header_area);
  gtk_box_pack_start_defaults(GTK_BOX(FRONTLINE_DIALOG(dialog)->header_area),
			      header_button);
  gtk_widget_show(header_button);
  gtk_widget_show(header_area);
  tooltips = gtk_tooltips_new();
  gtk_tooltips_set_tip (tooltips, 
			header_button, 
			_("Click here to reload the image from the Gimp"),
			"Click here to reload the image from the Gimp");
  
  header_sep = gtk_hseparator_new();
  gtk_box_pack_start_defaults(GTK_BOX(FRONTLINE_DIALOG(dialog)->header_area), 
			      header_sep);
  gtk_widget_show(header_sep);
  gtk_widget_show(dialog);

  preview = frontline_preview_new ();
  gtk_quit_add_destroy(1, GTK_OBJECT(preview));
  gtk_window_set_title(GTK_WINDOW(preview), _("Trace Preview"));
  gtk_window_set_transient_for(GTK_WINDOW(preview),
			       GTK_WINDOW(dialog));

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
  
  
  bitmap = gimp_drawable_to_at_bitmap(drawable, opts);
  frontline_dialog_set_bitmap(FRONTLINE_DIALOG(dialog), bitmap);
  frontline_preview_set_image_by_bitmap (FRONTLINE_PREVIEW(preview), 
					 bitmap);
  frontline_preview_show_splines(FRONTLINE_PREVIEW(preview),
				 FL_PREVIEW_SHOW_IN_STATIC_COLOR);
  gtk_widget_show(preview);

  /* Reload */
  reload_data.dialog   = FRONTLINE_DIALOG(dialog);
  reload_data.drawable = drawable;
  gtk_signal_connect(GTK_OBJECT(header_button),
		     "clicked", 
		     reload_bitmap, &reload_data);
  gtk_signal_connect(GTK_OBJECT(dialog), 
		     "set_bitmap", 
		     reset_bitmap, preview);
  gtk_main();
}

static void
reload_bitmap (GtkButton * button, gpointer user_data)
{
  ReloadData * reload_data = user_data;
  at_bitmap_type * bitmap;
  FrontlineDialog * dialog     = reload_data->dialog;
  at_fitting_opts_type *  opts = frontline_option_get_value(FRONTLINE_OPTION(dialog->option));
  
  reload_data->drawable = gimp_drawable_get(reload_data->drawable->id);
  bitmap = gimp_drawable_to_at_bitmap(reload_data->drawable, opts);
  frontline_dialog_set_bitmap(dialog, bitmap);
}

static void
reset_bitmap (FrontlineDialog * dialog, gpointer user_data)
{
  GtkWidget * preview = GTK_WIDGET(user_data);
  
  frontline_preview_set_image_by_bitmap (FRONTLINE_PREVIEW(preview), 
					 dialog->bitmap);
  frontline_preview_show_image(FRONTLINE_PREVIEW(preview), 
			       TRUE);
}

static void
quit_callback(GtkWidget * widget, gpointer no_use)
{
  gtk_main_quit();
}

static void
get_data(at_fitting_opts_type * opts, at_color_type * background_color)
{
  gimp_get_data (PLUG_IN_NAME "_opts_" PLUG_IN_PIVALS_GENERATION, 
		 opts);
  if (opts->background_color != NULL)
    {
      gimp_get_data (PLUG_IN_NAME "_background_color_" PLUG_IN_PIVALS_GENERATION,
		     background_color);
      opts->background_color = background_color;
    }
}

static void
set_data(at_fitting_opts_type * opts)
{
  gimp_set_data (PLUG_IN_NAME "_opts_" PLUG_IN_PIVALS_GENERATION, 
		 opts, sizeof (at_fitting_opts_type));
  if (opts->background_color != NULL)
    {
      background_color_pivals = *(opts->background_color);
      gimp_set_data (PLUG_IN_NAME "_background_color_" PLUG_IN_PIVALS_GENERATION, 
		     &background_color_pivals, sizeof (at_color_type));
    }
}

static GtkWidget *
header_new(const gchar * file_name, const gchar * drawable_name)
{
  GtkWidget * vbox;
  GtkWidget * hbox;
  GtkWidget * label;
  GtkWidget * entry;

  if (file_name == NULL)
    file_name = _("Not given");
  if (drawable_name == NULL)
    drawable_name = _("Not given");
  
  vbox = gtk_vbox_new(TRUE, 4);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);

  hbox = gtk_hbox_new(FALSE, 2);
  gtk_box_pack_start_defaults(GTK_BOX(vbox), hbox);
  gtk_widget_show(hbox);

  label = gtk_label_new(_("File name: "));
  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), file_name);
  gtk_entry_set_editable(GTK_ENTRY(entry), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 4);
  gtk_widget_show(label);
  gtk_widget_show(entry);
  gtk_widget_set_sensitive(entry, FALSE);
  
  hbox = gtk_hbox_new(FALSE, 2);
  gtk_box_pack_start_defaults(GTK_BOX(vbox), hbox);
  gtk_widget_show(hbox);

  label = gtk_label_new(_("Drawable name: "));
  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), drawable_name);
  gtk_entry_set_editable(GTK_ENTRY(entry), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 4);
  gtk_widget_show(label);
  gtk_widget_show(entry);
  gtk_widget_set_sensitive(entry, FALSE);
  
  return vbox;
}

static void
preview_splines          (FrontlineDialog * fl_dialog,
			  gpointer user_data)
{
  g_return_if_fail (fl_dialog);
  g_return_if_fail (FRONTLINE_IS_DIALOG(fl_dialog));
  g_return_if_fail (user_data);
  g_return_if_fail (FRONTLINE_IS_PREVIEW(user_data));

  if (!fl_dialog->splines)
    return;

  if (frontline_preview_set_splines(FRONTLINE_PREVIEW(user_data),
				    fl_dialog->splines))
    frontline_preview_show_splines(FRONTLINE_PREVIEW(user_data),
				   FL_PREVIEW_SHOW_AUTO);
  gtk_widget_show(GTK_WIDGET(user_data));
}

static at_bitmap_type *
gimp_drawable_to_at_bitmap (GimpDrawable * drawable, at_fitting_opts_type * opts)
{
  gint width, height;
  gint bytes;
  gint has_alpha;
  GimpPixelRgn pixel_rgn;
  guchar * data;
  gint x, y, i;
  at_bitmap_type * bitmap;
  at_color_type * bg;
  
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha(drawable->id);
  
  gimp_pixel_rgn_init (&pixel_rgn, drawable,
		       0, 0, 
		       width, height, 
		       FALSE, FALSE);

  if (!((bytes == 3 && has_alpha == FALSE)
	|| (bytes == 4 && has_alpha == TRUE)
	|| (bytes == 1 && has_alpha == FALSE)
	|| (bytes == 2 && has_alpha == TRUE)))
    g_error (_("bpp: %d has_alpha: %d"), bytes, has_alpha);
  else if (GIMP_DRAWABLE_TO_AT_BITMAP_DEBUG)
    g_message (_("bpp: %d has_alpha: %d"), bytes, has_alpha);

  bitmap = at_bitmap_new (width, height, bytes - has_alpha);
  data = g_new(guchar, bytes * width);  
  for (y = 0; y < height; y++)
    {
      gimp_pixel_rgn_get_row(&pixel_rgn, data, 0, y, width);
      for (x = 0; x < width; x++)
	{
	  gint alpha_value = 0;

	  if (has_alpha == TRUE)
	    alpha_value = data[x * bytes + (bytes - 1)];

	  if (has_alpha == FALSE || alpha_value != 0)
	    for (i = 0; i < (bytes - has_alpha); i++)
	      bitmap->bitmap[(bytes - has_alpha) * (y * width + x) + i] = data[x * bytes + i];
	  else
	    {
	      if (opts->background_color)
		{
		  gint tmp_gray;

		  bg = opts->background_color;
		  if (bytes - has_alpha == 1)
		    {
		      tmp_gray = (bg->r + bg->g + bg->g)/3;
		      bitmap->bitmap[(bytes - has_alpha) * (y * width + x)] = tmp_gray;
		    }
		  else
		    {
		      bitmap->bitmap[(bytes - has_alpha) * (y * width + x) + 0] = bg->r;
		      bitmap->bitmap[(bytes - has_alpha) * (y * width + x) + 1] = bg->g;
		      bitmap->bitmap[(bytes - has_alpha) * (y * width + x) + 2] = bg->b;
		    }
		      
		}
	      else
		for (i = 0; i < (bytes - has_alpha); i++)
		  bitmap->bitmap[(bytes - has_alpha) * (y * width + x) + i] = 255;
	    }
	}
    }
  g_free(data);
  return bitmap;
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
      gchar * msg = g_strconcat(_("Cannot open file to write splines: "), 
				filename, 
				"\n",
				g_strerror(errno));
      gnome_error_dialog_parented  (msg, GTK_WINDOW(user_data));
      g_free(msg);
      return ;
    }

  at_splines_write(writer, fp, filename, NULL, splines, msg_write, user_data);
  fclose(fp);
}

static void
msg_write                (at_string msg, 
			  at_msg_type msg_type, 
			  at_address client_data)
{
  
  if (msg_type == AT_MSG_FATAL)
    gnome_error_dialog_parented (msg, GTK_WINDOW(client_data));
  else
    gnome_warning_dialog_parented (msg, GTK_WINDOW(client_data));
}
