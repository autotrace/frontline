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

#include "frontline.h"
#include "private.h"
#include <libgnomeui/gnome-window-icon.h>
#include <gnome.h>
#include <gtk/gtk.h>

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

  fl_preview->image = NULL;
  fl_preview->splines = NULL;
  gnome_window_icon_set_from_file(GTK_WINDOW(fl_preview), 
				  GNOME_ICONDIR "/frontline.png");  
  
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(fl_preview), vbox);
  
  /* ----------------------------------------------------------------
   * vbox[scrolled window[canvas]]
   * ---------------------------------------------------------------- */
  fl_preview->save_button = gtk_button_new();
  gtk_box_pack_start(GTK_BOX(vbox), fl_preview->save_button, TRUE, TRUE, 4);
  
  fl_preview->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(fl_preview->save_button),
		    fl_preview->scrolled_window);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(fl_preview->scrolled_window),
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);
  gtk_widget_push_visual(gdk_imlib_get_visual());
  gtk_widget_push_colormap(gdk_imlib_get_colormap());
  fl_preview->canvas = gnome_canvas_new_aa();
  gtk_widget_pop_colormap();
  gtk_widget_pop_visual();
  gtk_container_add(GTK_CONTAINER(fl_preview->scrolled_window), 
		    fl_preview->canvas);

  gtk_signal_connect(GTK_OBJECT(fl_preview->save_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(save_button_clicked_cb),
		     fl_preview);

  gtk_widget_set_sensitive (fl_preview->save_button, FALSE);
  
  /* ----------------------------------------------------------------
   * vbox[hbox[subhbox[[label: toggle]] subhbox[[label: opts menu]]]]
   * ---------------------------------------------------------------- */
  hbox = gtk_hbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  
  /* 
   * subhbox[[label: toggle]] 
   */
  subhbox                  = gtk_hbox_new(FALSE, 0);
  label 		   = gtk_label_new ("Image: ");
  fl_preview->image_toggle = gtk_toggle_button_new_with_label("Show");
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
  label 		   = gtk_label_new ("Splines: ");
  fl_preview->splines_menu = gtk_option_menu_new ();
  menu 			   = gtk_menu_new();
  menu_item = gtk_radio_menu_item_new_with_label(NULL, "Multiple Colors");
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
  
  menu_item = gtk_radio_menu_item_new_with_label(group, "Static Color");
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
  
  menu_item = gtk_radio_menu_item_new_with_label(group, "Hide");
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
   * vbox[hbox[subhbox[label: scale]  subhbox[label: color picker]]]
   * ---------------------------------------------------------------- */
  hbox = gtk_hbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  /*
   * subhbox[label: scale]
   */
  subhbox = gtk_hbox_new(FALSE, 0);
  label = gtk_label_new ("Opacity: ");
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
   * subhbox[label: color picker]
   */
  subhbox                  = gtk_hbox_new(FALSE, 0);
  label 		   = gtk_label_new ("Color: ");
  fl_preview->splines_static_color = gnome_color_picker_new ();
  gnome_color_picker_set_use_alpha(GNOME_COLOR_PICKER(fl_preview->splines_static_color), 
				   FALSE);
  gnome_color_picker_set_title (GNOME_COLOR_PICKER(fl_preview->splines_static_color), "Static Color");
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
  GTK_OBJECT_CLASS(parent_class)->finalize(object);
}

gboolean
frontline_preview_set_image(FrontlinePreview * fl_preview,
			    gchar * img_filename)
{
  GdkImlibImage * im_image = gdk_imlib_load_image (img_filename);
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
  w = (int) (im_image->rgb_width) + 20;
  h = (int) (im_image->rgb_height) + 64;
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

  g_return_val_if_fail (fl_preview && FRONTLINE_PREVIEW(fl_preview), 
			FALSE);
  g_return_val_if_fail (splines,
			FALSE);

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
      /* TODO */
    }
  else
    {
      gtk_widget_set_sensitive (fl_preview->save_button, FALSE);
      gtk_widget_set_sensitive (fl_preview->splines_menu, FALSE);
      gtk_widget_set_sensitive (fl_preview->splines_opacity_scale, FALSE);
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

  g_return_if_fail (fl_preview && FRONTLINE_IS_PREVIEW(fl_preview));

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
  FrontlinePreviewSplinesStatus status;
  
  g_return_if_fail (menu_item && GTK_IS_MENU_ITEM(menu_item));
  g_return_if_fail (user_data && FRONTLINE_IS_PREVIEW(user_data));
  
  tmp 	 = gtk_object_get_data(GTK_OBJECT(menu_item), "menu_item_id");
  status = GPOINTER_TO_INT(tmp);
  
  splines_show(FRONTLINE_PREVIEW(user_data), status);
}

static void
splines_show(FrontlinePreview * fl_preview, FrontlinePreviewSplinesStatus status)
{
  guint8 opacity = opacity_get(fl_preview);
  guint32 color = color_get(fl_preview);
  
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

  GTK_ADJUSTMENT(fl_preview->splines_opacity)->value = opacity;
  gtk_adjustment_value_changed(GTK_ADJUSTMENT(fl_preview->splines_opacity));
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
