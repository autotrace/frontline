/* fl_preview.h --- Frontline Preview window (fl_preview in short)
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


#ifndef FL_PREVIEW_H
#define FL_PREVIEW_H 

BEGIN_GNOME_DECLS  

#define FRONTLINE_TYPE_PREVIEW                (frontline_preview_get_type ())
#define FRONTLINE_PREVIEW(obj)                (GTK_CHECK_CAST ((obj), FRONTLINE_TYPE_PREVIEW, FrontlinePreview))
#define FRONTLINE_PREVIEW_CLASS(klass)        (GTK_CHECK_CLASS_CAST ((klass), FRONTLINE_TYPE_PREVIEW, FrontlinePreviewClass))
#define FRONTLINE_IS_PREVIEW(obj)             (GTK_CHECK_TYPE ((obj), FRONTLINE_TYPE_PREVIEW))
#define FRONTLINE_IS_PREVIEW_CLASS(klass)     (GTK_CHECK_CLASS_TYPE ((klass), FRONTLINE_TYPE_PREVIEW))

typedef struct _FrontlinePreview FrontlinePreview;
typedef struct _FrontlinePreviewClass FrontlinePreviewClass;
typedef enum   _FrontlinePreviewSplinesStatus FrontlinePreviewSplinesStatus;

struct _FrontlinePreview 
{
  GtkWindow window;
  
  GtkWidget * scrolled_window;
  GtkWidget * canvas;
  
  GnomeCanvasItem * image;
  GtkWidget * image_toggle;
  
  GnomeCanvasItem * splines;
  GtkWidget * splines_menu;
  GtkWidget * splines_muletiple_colors_menu_item;
  GtkWidget * splines_static_color_menu_item;
  GtkWidget * splines_hide_menu_item;
  GtkObject * splines_opacity;
  GtkWidget * splines_opacity_scale;
  GtkWidget * splines_static_color;

  GtkWidget * save_button;
};

struct _FrontlinePreviewClass
{
  GtkWindowClass parent_class;
  void (* request_to_save) (FrontlinePreview * fl_preview);
  void (* set_image)       (FrontlinePreview * fl_preview, gboolean set);
  void (* set_splines)     (FrontlinePreview * fl_preview, gboolean set);
};

enum _FrontlinePreviewSplinesStatus {
  FL_PREVIEW_SHOW_AUTO,
  FL_PREVIEW_SHOW_IN_MULTIPLE_COLORS,
  FL_PREVIEW_SHOW_IN_STATIC_COLOR,
  FL_PREVIEW_HIDE,
};

GtkType    frontline_preview_get_type (void);
GtkWidget* frontline_preview_new      (void);

/* Image */
gboolean   frontline_preview_set_image (FrontlinePreview * fl_preview,
					gchar * img_filename);
void       frontline_preview_show_image (FrontlinePreview * fl_preview,
					 gboolean show);

/* Splines */
gboolean   frontline_preview_set_splines (FrontlinePreview * fl_preview,
					  at_splines_type * splines);
void       frontline_preview_show_splines (FrontlinePreview * fl_preview,
					   FrontlinePreviewSplinesStatus status);

FrontlinePreviewSplinesStatus frontline_preview_get_splines_status(FrontlinePreview * fl_preview);


void       frontline_preview_set_splines_static_color(FrontlinePreview * fl_preview, 
						      at_color_type * color);
guint32    frontline_preview_get_splines_static_color(FrontlinePreview * fl_preview);
							 

/* frontline_preview_set_splines_opacity:
   0.0 <= option <= 1.0 */
void       frontline_preview_set_splines_opacity(FrontlinePreview * fl_preview, 
						 gfloat opacity);

gfloat     frontline_preview_get_splines_opacity(FrontlinePreview * fl_preview);


END_GNOME_DECLS

#endif /* Not def: FL_PREVIEW_H */
