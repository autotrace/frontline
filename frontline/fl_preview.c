/* fl_preview.c --- Frontline preview window
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

/* TODO: open and closed path, error message, SEGV, 
 request_to_save external interface, picking bg color */

#include "config.h"
#include "private.h"
#include "frontline.h"

#include <libgnomeui/gnome-window-icon.h>
#include <gnome.h>
#include <gtk/gtk.h>
#include <gdk_imlib.h>
#include <unistd.h>

enum {
  REQUEST_TO_SAVE,
  SET_IMAGE,
  SET_SPLINES,
  LAST_SIGNAL
};
static GtkWindowClass *  parent_class;
static guint fl_preview_signals[LAST_SIGNAL] = { 0 };

static void frontline_preview_class_init  (FrontlinePreviewClass * klass);
static void frontline_preview_init        (FrontlinePreview * fl_preview);
static void frontline_preview_finalize    (GtkObject * object);

static void frontline_preview_real_set_image (FrontlinePreview * fl_preview,
					      gboolean set);
static void frontline_preview_real_set_splines (FrontlinePreview * fl_preview,
						gboolean set);

static void frontline_preview_free_tmp_svg(FrontlinePreview * fl_preview);

/* 
 * Drag and Drop 
 */
#define TMPDIR "/tmp/"
typedef enum {
  URI_LIST
} fl_preview_drop_target_info;
static GtkTargetEntry fl_preview_drop_target_entries [] = {
  {"text/uri-list", 0, URI_LIST},
};
#define ENTRIES_SIZE(n) sizeof(n)/sizeof(n[0]) 
static guint nfl_preview_drop_target_entries = ENTRIES_SIZE(fl_preview_drop_target_entries);
static void save_button_drag_begin_cb(GtkWidget * widget,
				      GdkDragContext * drag_context,
				      gpointer user_data);
static void  save_button_drag_data_get_cb (GtkWidget * widget,
					   GdkDragContext * drag_context,
					   GtkSelectionData * data,
					   guint info,
					   guint time,
					   gpointer user_data);
static void save_button_drag_end_cb (GtkWidget * widget,
				     GdkDragContext * drag_context,
				     gpointer user_data);
					 
/*
 * Save button
 */
static void
save_button_clicked_cb (GtkButton * save_button, gpointer user_data);

/* external API(frontline_preview)
   -> GUI operation(gtk) 
   -> callbacks(cb) 
   -> internal func */

/* 
 * Image 
 */
static void image_toggled_cb (GtkToggleButton * toggle, gpointer user_data);
static void image_show(FrontlinePreview * fl_preview, gboolean show);

/*
 * Splines
 */
static void splines_activated_cb(GtkMenuItem *menu_item, gpointer user_data);
static void splines_show(FrontlinePreview * fl_preview, FrontlinePreviewSplinesStatus status);

/*
 * Opacity
 */
static void opacity_value_changed_cb(GtkAdjustment * opacity, gpointer user_data);
static void opacity_set(FrontlinePreview * fl_preview, guint8 opacity);
static guint8 opacity_get(FrontlinePreview * fl_preview);

/*
 * Static color
 */
static void color_set_cb (GnomeColorPicker *cp, 
			  guint r, guint g, guint b, guint a,
			  gpointer user_data);
static void color_set    (FrontlinePreview * fl_preview, guint32 color);
static guint32 color_get (FrontlinePreview * fl_preview);

/*
 * Zoom
 */
static void zoom_factor_value_changed_cb(GtkAdjustment * zoom_factor, gpointer user_data);

/*
 * Line width
 */
static void line_width_value_changed_cb(GtkAdjustment * line_width, gpointer user_data);

GtkType
frontline_preview_get_type (void)
{
  static GtkType fl_preview_type = 0;
  
  if (!fl_preview_type)
    {
      static const GtkTypeInfo fl_preview_info =
	{
	  "FrontlinePreview",
	  sizeof (FrontlinePreview),
	  sizeof (FrontlinePreviewClass),
	  (GtkClassInitFunc) frontline_preview_class_init,
	  (GtkObjectInitFunc) frontline_preview_init,
	  /* reserved_1 */ NULL,
	  /* reserved_2 */ NULL,
	  (GtkClassInitFunc) NULL,
	};
      
      fl_preview_type = gtk_type_unique (GTK_TYPE_WINDOW, &fl_preview_info);
    }
  return fl_preview_type;
}

static void
frontline_preview_class_init  (FrontlinePreviewClass * klass)
{
  GtkObjectClass * object_class;
  parent_class = gtk_type_class (gtk_window_get_type ());
  
  object_class = (GtkObjectClass*)klass;
  object_class->finalize = frontline_preview_finalize;
  
  fl_preview_signals[REQUEST_TO_SAVE] =
    gtk_signal_new ("request_to_save",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (FrontlinePreviewClass, request_to_save),
		    gtk_marshal_NONE__NONE,
		    GTK_TYPE_NONE,
		    0);
  klass->request_to_save = NULL;
  
  fl_preview_signals[SET_IMAGE] =
    gtk_signal_new ("set_image",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (FrontlinePreviewClass, set_image),
		    gtk_marshal_NONE__BOOL,
		    GTK_TYPE_NONE,
		    1,
		    GTK_TYPE_BOOL);
  klass->set_image = frontline_preview_real_set_image;

  fl_preview_signals[SET_SPLINES] =
    gtk_signal_new ("set_splines",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (FrontlinePreviewClass, set_splines),
		    gtk_marshal_NONE__BOOL,
		    GTK_TYPE_NONE,
		    1,
		    GTK_TYPE_BOOL);
  klass->set_splines = frontline_preview_real_set_splines;
  
  gtk_object_class_add_signals (object_class, fl_preview_signals, LAST_SIGNAL);
}

static void
frontline_preview_init (FrontlinePreview * fl_preview)
{
  GtkWidget * vbox;
  GtkWidget * hbox, * subhbox;
  GtkWidget * label;
  GtkWidget * menu, * menu_item;
  GSList * group;
  GtkTooltips * tooltips;
  
  fl_preview->image 	   = NULL;
  fl_preview->splines 	   = NULL;
  fl_preview->tmp_svg_uri = NULL;
  gnome_window_icon_set_from_file(GTK_WINDOW(fl_preview), 
				  GNOME_ICONDIR "/frontline.png");  
  
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(fl_preview), vbox);
  gtk_container_set_border_width(GTK_CONTAINER(fl_preview), 4);
  /* ----------------------------------------------------------------
   * vbox[hbox[zoomscale|save_button[scrolled window[canvas]]]]
   * ---------------------------------------------------------------- */
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 4);
  
  fl_preview->zoom_factor 	= gtk_adjustment_new(1.0, 0.05, 10.0, 0.1, 1, 1);
  fl_preview->zoom_factor_scale = gtk_vscale_new(GTK_ADJUSTMENT(fl_preview->zoom_factor));
  gtk_scale_set_draw_value(GTK_SCALE(fl_preview->zoom_factor_scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(fl_preview->zoom_factor_scale),
			      GTK_UPDATE_DELAYED);
  gtk_box_pack_start(GTK_BOX(hbox), fl_preview->zoom_factor_scale, FALSE, FALSE, 0);


  fl_preview->save_button = gtk_button_new();
  gtk_box_pack_start(GTK_BOX(hbox), fl_preview->save_button, TRUE, TRUE, 0);
  tooltips = gtk_tooltips_new();
  gtk_tooltips_set_tip (tooltips, 
			fl_preview->save_button, 
			_("Click here to save the tracing result. "
			"Drag out from here to export SVG file."),
			_("Click here to save the tracing result. "
			"Drag out from here to export SVG file."));
  gtk_tooltips_enable (tooltips);
  
  fl_preview->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(fl_preview->save_button),
		    fl_preview->scrolled_window);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(fl_preview->scrolled_window),
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);
  gtk_widget_push_visual(gdk_imlib_get_visual());
  gtk_widget_push_colormap(gdk_imlib_get_colormap());
  
  /* TODO: aa is too slow. Remove aa in gnome2. */
  fl_preview->canvas = gnome_canvas_new_aa();
  
  gtk_widget_pop_colormap();
  gtk_widget_pop_visual();
  gtk_container_add(GTK_CONTAINER(fl_preview->scrolled_window), 
		    fl_preview->canvas);

  gtk_signal_connect(GTK_OBJECT(fl_preview->save_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(save_button_clicked_cb),
		     fl_preview);
  gtk_signal_connect(GTK_OBJECT(fl_preview->save_button),
		     "drag_begin",
		     GTK_SIGNAL_FUNC(save_button_drag_begin_cb),
		     fl_preview);
  gtk_signal_connect(GTK_OBJECT(fl_preview->save_button),
		     "drag_data_get",
		     GTK_SIGNAL_FUNC(save_button_drag_data_get_cb),
		     fl_preview);
  gtk_signal_connect(GTK_OBJECT(fl_preview->save_button),
		     "drag_end",
		     GTK_SIGNAL_FUNC(save_button_drag_end_cb),
		     fl_preview);
  gtk_signal_connect(GTK_OBJECT(fl_preview->zoom_factor),
		     "value_changed",
		     GTK_SIGNAL_FUNC(zoom_factor_value_changed_cb),
		     fl_preview);
  gtk_widget_set_sensitive (fl_preview->save_button, FALSE);

  /* ----------------------------------------------------------------
   * vbox[hbox[subhbox[[label: toggle]] subhbox[[label: opts menu]]]]
   * ---------------------------------------------------------------- */
  hbox = gtk_hbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
  
  /* 
   * subhbox[[label: toggle]] 
   */
  subhbox                  = gtk_hbox_new(FALSE, 0);
  label 		   = gtk_label_new (_("Image: "));
  fl_preview->image_toggle = gtk_toggle_button_new_with_label(_("Show"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fl_preview->image_toggle), TRUE);
  gtk_box_pack_start(GTK_BOX(subhbox), label, 
		     FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(subhbox), fl_preview->image_toggle, 
		     FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(fl_preview->image_toggle), 
		     "toggled",
		     GTK_SIGNAL_FUNC(image_toggled_cb),
		     fl_preview);
  gtk_widget_set_sensitive (fl_preview->image_toggle, FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), subhbox, FALSE, FALSE, 4);
  
  /*
   * subhbox[[label: opts menu]] 
   */
  subhbox                  = gtk_hbox_new(FALSE, 0);
  label 		   = gtk_label_new (_("Splines: "));
  fl_preview->splines_menu = gtk_option_menu_new ();
  menu 			   = gtk_menu_new();
  menu_item = gtk_radio_menu_item_new_with_label(NULL, _("Multiple Colors"));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menu_item));
  gtk_menu_append(GTK_MENU(menu), menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),
		     "activate",
		     GTK_SIGNAL_FUNC(splines_activated_cb),
		     fl_preview);
  gtk_object_set_data(GTK_OBJECT(menu_item),
		      "menu_item_id",
		      GINT_TO_POINTER(FL_PREVIEW_SHOW_IN_MULTIPLE_COLORS));
  fl_preview->splines_muletiple_colors_menu_item = menu_item;
  
  menu_item = gtk_radio_menu_item_new_with_label(group, _("Static Color"));
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menu_item));
  gtk_menu_append(GTK_MENU(menu), menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),
			    "activate",
			    GTK_SIGNAL_FUNC(splines_activated_cb),
			    fl_preview);
  gtk_object_set_data(GTK_OBJECT(menu_item),
		      "menu_item_id",
		      GINT_TO_POINTER(FL_PREVIEW_SHOW_IN_STATIC_COLOR));
  fl_preview->splines_static_color_menu_item = menu_item;
  
  menu_item = gtk_radio_menu_item_new_with_label(group, _("Hide"));
  gtk_menu_append(GTK_MENU(menu), menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),
		     "activate",
		     GTK_SIGNAL_FUNC(splines_activated_cb),
		     fl_preview);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(fl_preview->splines_menu), menu);
  gtk_object_set_data(GTK_OBJECT(menu_item), 
		      "menu_item_id",
		      GINT_TO_POINTER(FL_PREVIEW_HIDE));
  fl_preview->splines_hide_menu_item = menu_item;
  gtk_option_menu_set_history (GTK_OPTION_MENU(fl_preview->splines_menu), 0);

  gtk_box_pack_start(GTK_BOX(subhbox), label, 
		     FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(subhbox), fl_preview->splines_menu, 
		     TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), subhbox, TRUE, TRUE, 4);
  gtk_widget_set_sensitive (fl_preview->splines_menu, FALSE);
  
  /* ----------------------------------------------------------------
   * vbox[hbox[subhbox[label: scale]  subhbox [label: width] subhbox[label: color picker]]]
   * ---------------------------------------------------------------- */
  hbox = gtk_hbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  /*
   * subhbox[label: scale]
   */
  subhbox = gtk_hbox_new(FALSE, 0);
  label = gtk_label_new (_("Opacity: "));
  fl_preview->splines_opacity = gtk_adjustment_new(0.0, 0.0, 1.0, 0.01, 0.01, 0.001);
  fl_preview->splines_opacity_scale = gtk_hscale_new(GTK_ADJUSTMENT(fl_preview->splines_opacity));
  gtk_scale_set_draw_value(GTK_SCALE(fl_preview->splines_opacity_scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(fl_preview->splines_opacity_scale),
			      GTK_UPDATE_DELAYED);
  gtk_box_pack_start(GTK_BOX(subhbox), label, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(subhbox), fl_preview->splines_opacity_scale, 
		     TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(fl_preview->splines_opacity),
		     "value_changed",
		     GTK_SIGNAL_FUNC(opacity_value_changed_cb),
		     fl_preview);
  gtk_box_pack_start(GTK_BOX(hbox), subhbox, TRUE, TRUE, 4);
  gtk_widget_set_sensitive (fl_preview->splines_opacity_scale, FALSE);

  /*
   * subhbox[label: width]
   */
  subhbox = gtk_hbox_new(FALSE, 0);
  label = gtk_label_new (_("Line width: "));
  fl_preview->line_width = gtk_adjustment_new(FL_DEFAULT_SPLINES_WIDTH, 0.0, 10.0, 1.0, 1.0, 1.0);
  fl_preview->line_width_scale = gtk_hscale_new(GTK_ADJUSTMENT(fl_preview->line_width));
  gtk_scale_set_draw_value(GTK_SCALE(fl_preview->line_width_scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(fl_preview->line_width_scale),
			      GTK_UPDATE_DELAYED);
  gtk_box_pack_start(GTK_BOX(subhbox), label, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(subhbox), fl_preview->line_width_scale, 
		     TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(fl_preview->line_width),
		     "value_changed",
		     GTK_SIGNAL_FUNC(line_width_value_changed_cb),
		     fl_preview);
  gtk_box_pack_start(GTK_BOX(hbox), subhbox, TRUE, TRUE, 4);
  gtk_widget_set_sensitive (fl_preview->line_width_scale, FALSE);
  
  /* 
   * subhbox[label: color picker]
   */
  subhbox                  = gtk_hbox_new(FALSE, 0);
  label 		   = gtk_label_new (_("Color: "));
  fl_preview->splines_static_color = gnome_color_picker_new ();
  gnome_color_picker_set_use_alpha(GNOME_COLOR_PICKER(fl_preview->splines_static_color), 
				   FALSE);
  gnome_color_picker_set_title (GNOME_COLOR_PICKER(fl_preview->splines_static_color), _("Static Color"));
  gtk_box_pack_start(GTK_BOX(subhbox), label, 
		     FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(subhbox), fl_preview->splines_static_color, 
		     FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), subhbox, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(fl_preview->splines_static_color),
		     "color_set",
		     GTK_SIGNAL_FUNC(color_set_cb),
		     fl_preview);
  gtk_widget_set_sensitive (fl_preview->splines_static_color, FALSE);

  fl_preview->appbar = gnome_appbar_new(FALSE, TRUE, GNOME_PREFERENCES_NEVER);
  gtk_box_pack_start(GTK_BOX(vbox), fl_preview->appbar, FALSE, FALSE, 0);
  gtk_widget_show_all(vbox);
}

GtkWidget*
frontline_preview_new      (void)
{
  return gtk_type_new (FRONTLINE_TYPE_PREVIEW);
}

static void
frontline_preview_finalize    (GtkObject * object)
{
  if (FRONTLINE_PREVIEW(object)->tmp_svg_uri)
    frontline_preview_free_tmp_svg(FRONTLINE_PREVIEW (object));
  GTK_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
frontline_preview_free_tmp_svg(FrontlinePreview * fl_preview)
{
  gchar * old_name;
  gchar * tmp;
  
  old_name = g_strdup(fl_preview->tmp_svg_uri + strlen("file://"));
  tmp = strstr(old_name, ".svg");
  g_assert(tmp);
  tmp[0]   = '\0';
  unlink(old_name);
  g_free(old_name);
  g_free(fl_preview->tmp_svg_uri);
  fl_preview->tmp_svg_uri = NULL;
}

gboolean
frontline_preview_set_image_by_file(FrontlinePreview * fl_preview,
				    gchar * img_filename)
{
  GdkImlibImage * im_image = gdk_imlib_load_image (img_filename);
  return frontline_preview_set_image_by_gdk_imlib_image(fl_preview, im_image);
}

gboolean
frontline_preview_set_image_by_bitmap (FrontlinePreview * fl_preview,
				       at_bitmap_type * bitmap)
{
  GdkImlibImage * im_image;
  unsigned short width, height;
  unsigned short x, y;
  
  width  = at_bitmap_get_width(bitmap);
  height = at_bitmap_get_height(bitmap);
  
  if (at_bitmap_get_planes(bitmap) == 1)
    {
      /* Make the monochrome bitmap color bitmap force */
      at_bitmap_type * bitmap3;
      bitmap3 = at_bitmap_new (width, height, 3);
      for (y = 0; y < height; y++)
	for(x = 0; x < width; x++)
	  {
	    bitmap3->bitmap[3*(y * width + x) + 0] = bitmap->bitmap[y * width + x];
	    bitmap3->bitmap[3*(y * width + x) + 1] = bitmap->bitmap[y * width + x];
	    bitmap3->bitmap[3*(y * width + x) + 2] = bitmap->bitmap[y * width + x];
	  }
      im_image = gdk_imlib_create_image_from_data (bitmap3->bitmap, NULL, width, height);
      at_bitmap_free(bitmap3);
    }
  else
    im_image = gdk_imlib_create_image_from_data (bitmap->bitmap, NULL, width, height);
  
  return frontline_preview_set_image_by_gdk_imlib_image(fl_preview, im_image);
}

gboolean
frontline_preview_set_image_by_gdk_imlib_image (FrontlinePreview * fl_preview,
						GdkImlibImage * im_image)
{
  GnomeCanvasGroup * group = gnome_canvas_root(GNOME_CANVAS(fl_preview->canvas));
  gint w, h;
  
  if (!im_image)
    return FALSE;

  if (fl_preview->image)
    {
      gtk_object_destroy(GTK_OBJECT(fl_preview->image));
      fl_preview->image = NULL;
      gtk_signal_emit(GTK_OBJECT (fl_preview),
		      fl_preview_signals[SET_IMAGE],
		      FALSE);
    }
  if (fl_preview->splines)
    {
      gtk_object_destroy(GTK_OBJECT(fl_preview->splines));
      fl_preview->splines = NULL;
      gtk_signal_emit(GTK_OBJECT (fl_preview),
		      fl_preview_signals[SET_SPLINES],
		      FALSE);
    }

  fl_preview->image = gnome_canvas_item_new(group,
					    GNOME_TYPE_CANVAS_IMAGE,
					    "image", im_image,
					    "x", 0.0,
					    "y", 0.0,
					    "width",  (double) (im_image->rgb_width),
					    "height", (double) (im_image->rgb_height),
					    "anchor", GTK_ANCHOR_NORTH_WEST,
					    NULL);
  gnome_canvas_item_lower_to_bottom (fl_preview->image);
  gnome_canvas_set_scroll_region(GNOME_CANVAS(fl_preview->canvas),
				 0, 
				 0, 
				 (int) (im_image->rgb_width), 
				 (int)(im_image->rgb_height));
  w = (int) (im_image->rgb_width) + 24;
  h = (int) (im_image->rgb_height) + 78 + 20;
  w = (w > 300)? w: 300;
  if (w > 600)
    w = 600;
  if (h > 400)
    h = 400;
  
  gtk_window_set_default_size(GTK_WINDOW(fl_preview), w, h);
  gtk_signal_emit(GTK_OBJECT (fl_preview),
		  fl_preview_signals[SET_IMAGE],
		  TRUE);    
  return fl_preview->image? TRUE: FALSE;  
}

static void
frontline_preview_real_set_image (FrontlinePreview * fl_preview,
				  gboolean set)
{
  gtk_widget_set_sensitive (fl_preview->save_button, FALSE);
  gtk_widget_set_sensitive (fl_preview->splines_menu, FALSE);
  gtk_widget_set_sensitive (fl_preview->splines_opacity_scale, FALSE);
  gtk_widget_set_sensitive (fl_preview->splines_static_color, FALSE);
  gtk_widget_set_sensitive (fl_preview->line_width_scale, FALSE);
  
  if (set)
    gtk_widget_set_sensitive (fl_preview->image_toggle, TRUE);
  else
    gtk_widget_set_sensitive (fl_preview->image_toggle, FALSE);
}

gboolean
frontline_preview_set_splines(FrontlinePreview * fl_preview,
			      at_splines_type * splines)
{
  GnomeCanvasGroup * group;
  GTimeVal t1, t2;

  g_return_val_if_fail (fl_preview && FRONTLINE_PREVIEW(fl_preview), 
			FALSE);
  g_return_val_if_fail (splines,
			FALSE);
  
  if (!fl_ask(GTK_WINDOW(fl_preview), splines))
    return FALSE;
  
  g_get_current_time(&t1);
  if (fl_preview->splines)
    {
      gtk_object_destroy(GTK_OBJECT(fl_preview->splines));
      fl_preview->splines = NULL;
      gtk_signal_emit(GTK_OBJECT (fl_preview),
		      fl_preview_signals[SET_SPLINES],
		      FALSE);
    }
  group = gnome_canvas_root(GNOME_CANVAS(fl_preview->canvas));
  fl_preview->splines = frontline_splines_new(group, 
					      splines,
					      GNOME_CANVAS_IMAGE(fl_preview->image)->height);
  gnome_canvas_item_raise_to_top(fl_preview->splines);
  gtk_signal_emit(GTK_OBJECT (fl_preview),
		  fl_preview_signals[SET_SPLINES],
		  TRUE);
  g_get_current_time(&t2);
  g_message ("[frontline_preview_set_splines] %ld", t2.tv_sec - t1.tv_sec);

  /*
   * DND
   */
  gtk_drag_source_set(GTK_WIDGET(fl_preview->save_button), 
		      GDK_BUTTON1_MASK,
		      fl_preview_drop_target_entries,
		      nfl_preview_drop_target_entries,
		      GDK_ACTION_COPY);
  /* Save splines into a tmpdepfile in SVG format */
  {
    char  tmp_name[] = TMPDIR "fl-svg-" "XXXXXX";
    int tmp_fd;
    FILE * tmp_fp;
    at_output_write_func writer;
    
    tmp_fd= mkstemp(tmp_name);
    g_message(tmp_name);

    if (tmp_fd < 0)
      {
	/* TODO */
	g_warning(_("Cannot create temporary file"));
      }
    tmp_fp = fdopen(tmp_fd,"w");
    if (NULL == tmp_fp)
      {
	/* TODO */
	g_warning(_("Cannot do fdopen for temporary file"));
      }
    writer = at_output_get_handler_by_suffix("svg");
    at_splines_write(writer, tmp_fp, tmp_name, NULL, splines, NULL, NULL);
    /* TODO */
    fclose(tmp_fp);
    
    if (fl_preview->tmp_svg_uri)
      frontline_preview_free_tmp_svg(fl_preview);

    fl_preview->tmp_svg_uri = g_strdup_printf("file://%s.svg", tmp_name);
  }

#if 0
  /* TODO: Icon set up:  SVG file icon is needed.
     Copy and pasted from dia/app/interface.c */
 {
   GdkPixmap * icon = NULL;
   GdkBitmap * mask;
   GdkPixbuf * pixbuf;

   pixbuf = gdk_pixbuf_new_from_file(SVG_PIXMAP);
   g_assert(pixbuf);

   gdk_pixbuf_render_pixmap_and_mask(pixbuf, &icon, &mask, 1.0);
   gdk_pixbuf_unref(pixbuf);
   g_assert(icon);
   gtk_drag_source_set_icon(GTK_WIDGET(fl_preview->save_button),
			    gdk_colormap_get_system(),
			    icon, mask);
   gdk_pixmap_unref(icon);
   gdk_bitmap_unref(mask);
 }
#endif /* 0 */
 {
   gchar * message;
   gchar * basefmt = N_("Groups of splines: %d, Splines: %d, Points: %d");
   message = g_strdup_printf(_(basefmt), 
			     at_spline_list_array_count_groups_of_splines(splines),
			     at_spline_list_array_count_splines(splines),
			     at_spline_list_array_count_points(splines));
   frontline_preview_show_message(fl_preview, message);
   g_free(message);
 }
  return TRUE;
}

static void
frontline_preview_real_set_splines (FrontlinePreview * fl_preview,
				    gboolean set)
{
  if (set)
    {
      gtk_widget_set_sensitive (fl_preview->save_button, TRUE);
      gtk_widget_set_sensitive (fl_preview->splines_menu, TRUE);
      gtk_widget_set_sensitive (fl_preview->splines_opacity_scale, TRUE);
      gtk_widget_set_sensitive (fl_preview->line_width_scale, TRUE);
      /* TODO */
    }
  else
    {
      gtk_widget_set_sensitive (fl_preview->save_button, FALSE);
      gtk_widget_set_sensitive (fl_preview->splines_menu, FALSE);
      gtk_widget_set_sensitive (fl_preview->splines_opacity_scale, FALSE);
      gtk_widget_set_sensitive (fl_preview->line_width_scale, FALSE);
    }
}

/* -----------------------------------------------------------------
 * Image 
 * ----------------------------------------------------------------- */
void
frontline_preview_show_image(FrontlinePreview * fl_preview,
			     gboolean show)
{
  g_return_if_fail (fl_preview 
		    && FRONTLINE_IS_PREVIEW(fl_preview));
  g_return_if_fail (fl_preview->image_toggle 
		    && GTK_IS_TOGGLE_BUTTON(fl_preview->image_toggle));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fl_preview->image_toggle),
			       show);
}
static void
image_toggled_cb (GtkToggleButton * toggle, gpointer user_data)
{
  gboolean show;
  g_return_if_fail (toggle && GTK_IS_TOGGLE_BUTTON(toggle));
  g_return_if_fail (user_data && FRONTLINE_IS_PREVIEW(user_data));
  
  show = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle));
  image_show (FRONTLINE_PREVIEW(user_data), show);
}

static void
image_show(FrontlinePreview * fl_preview, gboolean show)
{
  g_assert(fl_preview && FRONTLINE_IS_PREVIEW(fl_preview));
  
  if (!fl_preview->image)
    return ;
  
  if (show)
    gnome_canvas_item_show(fl_preview->image);
  else
    gnome_canvas_item_hide(fl_preview->image);
  gnome_canvas_update_now(GNOME_CANVAS(fl_preview->canvas));
}

/* -----------------------------------------------------------------
 * Save button
 * ----------------------------------------------------------------- */
static void
save_button_clicked_cb (GtkButton * save_button, gpointer user_data)
{
  /* TODO */
  gtk_signal_emit(GTK_OBJECT(user_data),
		  fl_preview_signals[REQUEST_TO_SAVE]);
}


/* -----------------------------------------------------------------
 * Splines
 * ----------------------------------------------------------------- */

void
frontline_preview_show_splines(FrontlinePreview * fl_preview, 
			       FrontlinePreviewSplinesStatus status)
{
  GtkMenuItem * menu_item;
  GTimeVal t1, t2;
  
  g_return_if_fail (fl_preview && FRONTLINE_IS_PREVIEW(fl_preview));

  g_get_current_time(&t1);
  
  if (status == FL_PREVIEW_SHOW_AUTO)
    status = frontline_preview_get_splines_status(fl_preview);
  
  switch (status)
    {
    case FL_PREVIEW_SHOW_IN_MULTIPLE_COLORS:
      menu_item = GTK_MENU_ITEM(fl_preview->splines_muletiple_colors_menu_item);
      gtk_menu_item_activate(menu_item);
      break;
    case FL_PREVIEW_SHOW_IN_STATIC_COLOR:
      menu_item = GTK_MENU_ITEM(fl_preview->splines_static_color_menu_item);
      gtk_menu_item_activate(menu_item);
      break;
    case FL_PREVIEW_HIDE:
      menu_item = GTK_MENU_ITEM(fl_preview->splines_hide_menu_item);
      gtk_menu_item_activate(menu_item);
      break;
    default:
      g_assert_not_reached();
    }
  g_get_current_time(&t2);
  g_message ("[frontline_preview_show_splines] %ld", t2.tv_sec - t1.tv_sec);
}

FrontlinePreviewSplinesStatus
frontline_preview_get_splines_status(FrontlinePreview * fl_preview)
{
#define CHECK(x) GTK_CHECK_MENU_ITEM(fl_preview->x)->active
  if (CHECK(splines_muletiple_colors_menu_item))
    return FL_PREVIEW_SHOW_IN_MULTIPLE_COLORS;
  else if (CHECK(splines_static_color_menu_item))
    return FL_PREVIEW_SHOW_IN_STATIC_COLOR;
  else if (CHECK(splines_hide_menu_item))
    return FL_PREVIEW_HIDE;
  else
    g_assert_not_reached();
#undef CHECK
  return -1;
}

static void
splines_activated_cb(GtkMenuItem *menu_item, gpointer user_data)
{
  gpointer tmp;
  FrontlinePreview * fl_preview;
  FrontlinePreviewSplinesStatus status;
  
  g_return_if_fail (menu_item && GTK_IS_MENU_ITEM(menu_item));
  g_return_if_fail (user_data && FRONTLINE_IS_PREVIEW(user_data));
  
  fl_preview = FRONTLINE_PREVIEW(user_data);
  tmp 	 = gtk_object_get_data(GTK_OBJECT(menu_item), "menu_item_id");
  status = GPOINTER_TO_INT(tmp);
  gtk_option_menu_set_history (GTK_OPTION_MENU(fl_preview->splines_menu), 
			       status);
  splines_show(fl_preview, status);
}

static void
splines_show(FrontlinePreview * fl_preview, FrontlinePreviewSplinesStatus status)
{
  guint8 opacity = opacity_get(fl_preview);
  guint32 color = color_get(fl_preview);
  
  if (!fl_preview->splines)
    return; 
  
  switch (status)
    {
    case FL_PREVIEW_SHOW_IN_MULTIPLE_COLORS:
      frontline_splines_show_in_multiple_colors(GNOME_CANVAS_GROUP(fl_preview->splines),
						opacity);
      gnome_canvas_item_show(fl_preview->splines);
      gtk_widget_set_sensitive (fl_preview->splines_static_color, FALSE);
      break;
    case FL_PREVIEW_SHOW_IN_STATIC_COLOR:
      frontline_splines_show_in_static_color(GNOME_CANVAS_GROUP(fl_preview->splines),
					     color,
					     opacity);
      gnome_canvas_item_show(fl_preview->splines);
      gtk_widget_set_sensitive (fl_preview->splines_static_color, TRUE);
      break;
    case FL_PREVIEW_HIDE:
      gnome_canvas_item_hide(fl_preview->splines);
      gtk_widget_set_sensitive (fl_preview->splines_static_color, FALSE);
      break;
    default:
      g_assert_not_reached();
    }
  gnome_canvas_update_now(GNOME_CANVAS(fl_preview->canvas));
}

/* -----------------------------------------------------------------
 * Opacity
 * ----------------------------------------------------------------- */
void
frontline_preview_set_splines_opacity(FrontlinePreview * fl_preview, 
				      gfloat opacity)
{
  g_return_if_fail (fl_preview && FRONTLINE_IS_PREVIEW(fl_preview));
  g_return_if_fail (0.0 <= opacity && opacity <= 1.0);
  g_return_if_fail (fl_preview->splines_opacity);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(FRONTLINE_PREVIEW(fl_preview)->splines_opacity), 
			   opacity);
}

static void
opacity_value_changed_cb(GtkAdjustment * opacity, gpointer user_data)
{
  guint8  opacity_internal;
  
  g_return_if_fail (opacity && GTK_IS_ADJUSTMENT(opacity));
  g_return_if_fail (user_data && FRONTLINE_IS_PREVIEW(user_data));

  opacity_internal = opacity_get(FRONTLINE_PREVIEW(user_data));
  opacity_set(FRONTLINE_PREVIEW(user_data), opacity_internal);
}

static void
opacity_set(FrontlinePreview * fl_preview, guint8 opacity)
{
  FrontlinePreviewSplinesStatus status;
  g_assert (fl_preview && FRONTLINE_IS_PREVIEW(fl_preview));
  
  if (!fl_preview->splines)
    return;
  
  status = frontline_preview_get_splines_status(fl_preview);
  switch(status)
    {
    case FL_PREVIEW_HIDE:
      return;
    case FL_PREVIEW_SHOW_IN_MULTIPLE_COLORS:
      frontline_splines_set_fill_opacity(GNOME_CANVAS_GROUP(fl_preview->splines),
					 opacity);
      break;
    case FL_PREVIEW_SHOW_IN_STATIC_COLOR:
      frontline_splines_set_stroke_opacity(GNOME_CANVAS_GROUP(fl_preview->splines),
					   opacity);
      break;
    default:
      g_assert_not_reached();
    }
  gnome_canvas_update_now(GNOME_CANVAS(fl_preview->canvas));
}

gfloat
frontline_preview_get_splines_opacity(FrontlinePreview * fl_preview)
{
  return GTK_ADJUSTMENT(fl_preview->splines_opacity)->value;
}

static guint8
opacity_get(FrontlinePreview * fl_preview)
{
  gfloat opacity_external;
  guint8  opacity_internal;
  
  opacity_external = frontline_preview_get_splines_opacity(fl_preview);
  opacity_internal = 255 - ((gchar)(255.0 * opacity_external));
  return opacity_internal;
}

/* -----------------------------------------------------------------
 * Static color
 * ----------------------------------------------------------------- */
static void
color_set_cb (GnomeColorPicker *cp, guint r, guint g, guint b, guint a,
	      gpointer user_data)
{
  guint32 color;

  color = color_get(FRONTLINE_PREVIEW(user_data));
  color_set (FRONTLINE_PREVIEW(user_data), color);
}

static void
color_set    (FrontlinePreview * fl_preview, guint32 color)
{
  FrontlinePreviewSplinesStatus status;
  guint8 opacity;
  status  = frontline_preview_get_splines_status(fl_preview);
  opacity = opacity_get(fl_preview);
  
  if (status == FL_PREVIEW_SHOW_IN_STATIC_COLOR)
    {
      frontline_splines_show_in_static_color (GNOME_CANVAS_GROUP(fl_preview->splines),
					      color,
					      opacity);
    }
}

guint32
frontline_preview_get_splines_static_color(FrontlinePreview * fl_preview)
{
  return color_get(fl_preview);
}

static guint32
color_get (FrontlinePreview * fl_preview)
{
  guint8 r, g, b, a;
  gnome_color_picker_get_i8(GNOME_COLOR_PICKER(fl_preview->splines_static_color), 
			    &r, &g, &b, &a);
  return r << 24 | g << 16 | b << 8 | 0;
}


/* -----------------------------------------------------------------
 * Zooming
 * ----------------------------------------------------------------- */
void
frontline_preview_set_pixels_per_unit(FrontlinePreview * fl_preview,
				      double n)
{
  g_return_if_fail (FRONTLINE_IS_PREVIEW(fl_preview));
  gtk_adjustment_set_value(GTK_ADJUSTMENT(FRONTLINE_PREVIEW(fl_preview)->zoom_factor), 
			   (gfloat)n);
}

double
frontline_preview_get_pixels_per_unit(FrontlinePreview * fl_preview)
{
  g_return_val_if_fail (FRONTLINE_IS_PREVIEW(fl_preview), 0.0);
  return GTK_ADJUSTMENT(fl_preview->zoom_factor)->value;
}

static void
zoom_factor_value_changed_cb(GtkAdjustment * zoom_factor, gpointer user_data)
{
  GnomeCanvas * canvas = GNOME_CANVAS(FRONTLINE_PREVIEW(user_data)->canvas);
  gnome_canvas_set_pixels_per_unit(canvas, zoom_factor->value);
  gnome_canvas_update_now(canvas);
}

/* -----------------------------------------------------------------
 * Line width
 * ----------------------------------------------------------------- */
void
frontline_preview_set_line_width(FrontlinePreview * fl_preview,  guint width)
{
  g_return_if_fail (FRONTLINE_IS_PREVIEW(fl_preview));
  gtk_adjustment_set_value(GTK_ADJUSTMENT(FRONTLINE_PREVIEW(fl_preview)->line_width), 
			   (gfloat)width);
}

guint
frontline_preview_get_line_width(FrontlinePreview * fl_preview)
{
  g_return_val_if_fail (FRONTLINE_IS_PREVIEW(fl_preview), 0);
  return (guint)(GTK_ADJUSTMENT(fl_preview->line_width)->value);
}

static void
line_width_value_changed_cb(GtkAdjustment * line_width, gpointer user_data)
{
  FrontlinePreview * fl_preview = FRONTLINE_PREVIEW(user_data);

  frontline_splines_set_line_width(GNOME_CANVAS_GROUP(fl_preview->splines),
				   line_width->value);
  gnome_canvas_update_now(GNOME_CANVAS(fl_preview->canvas));
}


void
frontline_preview_show_message(FrontlinePreview * fl_preview,
			       const gchar * msg)
{
  g_return_if_fail (fl_preview);
  g_return_if_fail (msg);
  gnome_appbar_set_status(GNOME_APPBAR(fl_preview->appbar), msg);
}
/* -----------------------------------------------------------------
 * Save splines
 * ----------------------------------------------------------------- */
static void
save_button_drag_begin_cb(GtkWidget * widget,
			  GdkDragContext * drag_context,
			  gpointer user_data)
{
  gchar * uri = FRONTLINE_PREVIEW(user_data)->tmp_svg_uri;
  gchar * old_name;
  gchar * new_name;
  gchar * tmp;
  old_name = g_strdup(uri + strlen("file://"));
  tmp 	   = strstr(old_name, ".svg");
  g_assert(tmp);
  tmp[0]   = '\0';
  new_name = uri + strlen("file://");
  link(old_name, new_name);
  g_free(old_name);
}

static void  
save_button_drag_data_get_cb (GtkWidget * widget,
			      GdkDragContext * drag_context,
			      GtkSelectionData * data,
			      guint info,
			      guint time,
			      gpointer user_data)
{
  gchar * uri = FRONTLINE_PREVIEW(user_data)->tmp_svg_uri;
  switch(info) {
  case URI_LIST:
    gtk_selection_data_set(data,
			   gdk_atom_intern(fl_preview_drop_target_entries[0].target,
					   TRUE),
			   8,
			   uri,
			   strlen(uri));
    break;
  }
}
static void
save_button_drag_end_cb (GtkWidget * widget,
			 GdkDragContext * drag_context,
			 gpointer user_data)
{
  gchar * uri = FRONTLINE_PREVIEW(user_data)->tmp_svg_uri;
  gchar * new_name;
  new_name = g_strdup(uri + strlen("file://"));
  unlink (new_name);
}
