/* fl_splines.c --- building splines canvas item from at_splines_type
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

#include "frontline.h"
#include "private.h"
#include "curve.h"
#include "canvas-bpath.h"
#include <autotrace/output.h>

static const ArtWindRule fl_splines_wind_rule 	   = ART_WIND_RULE_NONZERO;
static const ArtPathStrokeJoinType fl_splines_join = ART_PATH_STROKE_JOIN_ROUND;
static const ArtPathStrokeCapType  fl_splines_cap  = ART_PATH_STROKE_CAP_ROUND;
static const gfloat fl_splines_width 		   = 1.0;

static GnomeCanvasItem * spline_new  (GnomeCanvasGroup * group,
				      at_spline_list_type * spline,
				      at_bool centerline,
				      gfloat image_height);
static void spline_set_color (GnomeCanvasItem * spline,
			      guint32 fill_color,
			      guint32 stroke_color);

/* Callbacks */
static void splines_set_fill_opacity_cb        (gpointer item, gpointer user_data);
static void splines_set_stroke_opacity_cb      (gpointer item, gpointer user_data);
static void splines_show_in_multiple_colors_cb (gpointer item, gpointer user_data);
static void splines_show_in_static_color_cb    (gpointer item, gpointer user_data);


struct list_array_ctx
{
  GnomeCanvasItem * splines_item;
  gfloat image_height;  
};
static void list_array_foreach_cb               (at_spline_list_array_type * spline_list_array, 
						 at_spline_list_type * spline_list, 
						 int index, 
						 at_address user_data);

struct list_ctx
{
  SPCurve * curve;
  double * matrix;
};
static void list_foreach_cb                     (at_spline_list_type * spline_list, 
						 at_spline_type * spline, 
						 int index, 
						 at_address user_data);

GnomeCanvasItem *
frontline_splines_new (GnomeCanvasGroup * group,
		       at_splines_type * splines,
		       gfloat image_height)

{
  struct list_array_ctx ctx;
  
  ctx.splines_item = gnome_canvas_item_new(group,
					   GNOME_TYPE_CANVAS_GROUP,
					   "x", 0.0,
					   "y", 0.0,
					   NULL);
  ctx.image_height = image_height;
  
  at_spline_list_array_foreach (splines, list_array_foreach_cb, &ctx);
  
  return ctx.splines_item;
}

static void
list_array_foreach_cb (at_spline_list_array_type * spline_list_array,
		       at_spline_list_type * spline_list,
		       int index,
		       at_address user_data)
{
  GnomeCanvasItem * spline_item;
  struct list_array_ctx * ctx = user_data;

  spline_item = spline_new(GNOME_CANVAS_GROUP(ctx->splines_item),
			   spline_list, 
			   AT_SPLINE_LIST_ARRAY_IS_CENTERLINE(spline_list_array),
			   ctx->image_height);
  gnome_canvas_item_show(spline_item);
}

static GnomeCanvasItem *
spline_new (GnomeCanvasGroup * group,
	    at_spline_list_type * spline,
	    at_bool centerline,
	    gfloat image_height)
{
  struct list_ctx ctx;
  double matrix[] = {1.0, 0.0, 0.0, -1.0, 0.0, 0.0};
 
  GnomeCanvasItem * spline_item;
  guint32 color;
 
  matrix[5]  =  image_height;
  ctx.matrix = matrix;
  ctx.curve  =  sp_curve_new();
  
  at_spline_list_foreach(spline, list_foreach_cb, &ctx);

  if (!AT_SPLINE_LIST_IS_OPENED(spline))
    sp_curve_closepath(ctx.curve);

  spline_item = sp_canvas_bpath_new(group, ctx.curve);
  
  color = AT_SPLINE_LIST_COLOR(spline)->r << 24
    | AT_SPLINE_LIST_COLOR(spline)->g     << 16
    | AT_SPLINE_LIST_COLOR(spline)->b     <<  8
    | 0x000000FF;

  if (AT_SPLINE_LIST_IS_OPENED(spline) || centerline)
    {
      spline_set_color(spline_item, 0x00000000, color);
      color = (color & 0xFFFFFF00) | 0x00000001;

    }
  else
    {
      spline_set_color(spline_item, color, 0x00000000);
      color = (color & 0xFFFFFF00) | 0x00000000;
    }
  gtk_object_set_data(GTK_OBJECT(spline_item),
		      "color",
		      GINT_TO_POINTER(color));
  return spline_item;
}

static void
list_foreach_cb                     (at_spline_list_type * spline_list, 
				     at_spline_type * spline, 
				     int index, 
				     at_address user_data)
{
  struct list_ctx * ctx = user_data;
  ArtPoint p[4];

#define START 0
#define CTRL1 1
#define CTRL2 2
#define END   3
 
  if (index == 0)
    {
      p[START].x  = AT_SPLINE_START_POINT(spline)->x;
      p[START].y  = AT_SPLINE_START_POINT(spline)->y;
      art_affine_point(p+START, p+START, ctx->matrix);
      sp_curve_moveto(ctx->curve, p[START].x, p[START].y);
    }

  p[END].x = AT_SPLINE_END_POINT(spline)->x;
  p[END].y = AT_SPLINE_END_POINT(spline)->y;
  art_affine_point(p+END, p+END, ctx->matrix);
  switch (AT_SPLINE_DEGREE(spline))
    {
    case AT_LINEARTYPE:
      sp_curve_lineto(ctx->curve, p[END].x, p[END].y);
      break;
    case AT_CUBICTYPE:
      p[CTRL1].x = AT_SPLINE_CONTROL1(spline)->x;
      p[CTRL1].y = AT_SPLINE_CONTROL1(spline)->y;
      art_affine_point(p+CTRL1, p+CTRL1, ctx->matrix);
      p[CTRL2].x = AT_SPLINE_CONTROL2(spline)->x;
      p[CTRL2].y = AT_SPLINE_CONTROL2(spline)->y;
      art_affine_point(p+CTRL2, p+CTRL2, ctx->matrix);
      sp_curve_curveto(ctx->curve, 
		       p[CTRL1].x, p[CTRL1].y,
		       p[CTRL2].x, p[CTRL2].y,
		       p[END].x, p[END].y);
      break;
    default:
      g_assert_not_reached();
    }
#undef START
#undef CTRL1
#undef CTRL2
#undef END
}


static void
spline_set_color (GnomeCanvasItem * spline,
		  guint32 fill_color,
		  guint32 stroke_color)
{
  sp_canvas_bpath_set_stroke (SP_CANVAS_BPATH(spline),
			      stroke_color,
			      fl_splines_width,
			      fl_splines_join,
			      fl_splines_cap);
  sp_canvas_bpath_set_fill (SP_CANVAS_BPATH(spline),
			    fill_color,
			    fl_splines_wind_rule);
}

void
frontline_splines_set_fill_opacity             (GnomeCanvasGroup  * splines,
						guint8 opacity)
{
   g_list_foreach(splines->item_list, splines_set_fill_opacity_cb, &opacity);
}

void
frontline_splines_set_stroke_opacity             (GnomeCanvasGroup  * splines,
						  guint8 opacity)
{
  g_list_foreach(splines->item_list, splines_set_stroke_opacity_cb, &opacity);
}

static void
splines_set_fill_opacity_cb(gpointer item, gpointer user_data)
{
  SPCanvasBPath * bpath = SP_CANVAS_BPATH(item);
  guint8 opacity 	= *(guint8 *)user_data;
  guint32 fill_color;

  fill_color = ((bpath->fill_rgba) & 0xFFFFFF00)
    |                               (0x000000FF & (guint32)opacity);
  spline_set_color(GNOME_CANVAS_ITEM(bpath), fill_color, bpath->stroke_rgba);
}

static void
splines_set_stroke_opacity_cb(gpointer item, gpointer user_data)
{
  SPCanvasBPath * bpath = SP_CANVAS_BPATH(item);
  guint8 opacity 	= *(guint8 *)user_data;
  guint32 stroke_color;

  stroke_color = ((bpath->stroke_rgba)&0xFFFFFF00)
    |                                 (0x000000FF & (guint32)opacity);  
  spline_set_color(GNOME_CANVAS_ITEM(bpath), bpath->fill_rgba, stroke_color);
}

void
frontline_splines_show_in_multiple_colors (GnomeCanvasGroup * splines,
					   guint8 opacity)					   
{
  g_list_foreach(splines->item_list, splines_show_in_multiple_colors_cb, &opacity);
}

static void
splines_show_in_multiple_colors_cb(gpointer item, gpointer user_data)
{
  SPCanvasBPath * bpath = SP_CANVAS_BPATH(item);
  gint open;
  gpointer original_color;
  guint32 color;
  guint8 opacity = *(guint8 *)user_data;

  original_color = gtk_object_get_data(GTK_OBJECT(bpath), "color");
  open 		 = (GPOINTER_TO_INT(original_color) & 0x00000001);

  color = ((guint32)opacity&0x000000FF)
    |                      (0xFFFFFF00 & GPOINTER_TO_INT(original_color));  
  if (open)
    spline_set_color (GNOME_CANVAS_ITEM(bpath), 0xFFFFFF00, color);
  else
    spline_set_color (GNOME_CANVAS_ITEM(bpath), color, 0xFFFFFF00);
}

void
frontline_splines_show_in_static_color    (GnomeCanvasGroup * splines,
					   guint32 color,
					   guint8 opacity)
{
  color = (color & 0xFFFFFF00) | (opacity & 0x000000FF);
  g_list_foreach(splines->item_list, splines_show_in_static_color_cb, &color);
}
					   
static void
splines_show_in_static_color_cb    (gpointer item, gpointer user_data)
{
  SPCanvasBPath * bpath = SP_CANVAS_BPATH(item);
  gint open;
  guint32 color;

  open  = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(bpath), "color")) & 0x00000001;
  color = *(guint32 *)user_data;

  spline_set_color(GNOME_CANVAS_ITEM(bpath), 0xFFFFFF00, color);
}
