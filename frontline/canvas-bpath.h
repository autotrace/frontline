#ifndef __SP_CANVAS_BPATH_H__
#define __SP_CANVAS_BPATH_H__

/*
 * Simple bezier bpath CanvasItem for sodipodi
 *
 * Authors:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define SP_TYPE_CANVAS_BPATH (sp_canvas_bpath_get_type ())
#define SP_CANVAS_BPATH(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_CANVAS_BPATH, SPCanvasBPath))
#define SP_CANVAS_BPATH_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CANVAS_BPATH, SPCanvasBPathClass))
#define SP_IS_CANVAS_BPATH(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_CANVAS_BPATH))
#define SP_IS_CANVAS_BPATH_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CANVAS_BPATH))

typedef struct _SPCanvasBPath SPCanvasBPath;
typedef struct _SPCanvasBPathClass SPCanvasBPathClass;

#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_wind.h>
#include <libart_lgpl/art_svp_vpath_stroke.h>
#include <libgnomeui/gnome-canvas.h>
#include "curve.h"

struct _SPCanvasBPath {
	GnomeCanvasItem item;

	/* Line def */
	SPCurve *curve;

	/* Fill attributes */
	guint32 fill_rgba;
	ArtWindRule fill_rule;

	/* Line attributes */
	guint32 stroke_rgba;
	gdouble stroke_width;
	ArtPathStrokeJoinType stroke_linejoin;
	ArtPathStrokeCapType stroke_linecap;
	gdouble stroke_miterlimit;

	/* State */
	ArtSVP *fill_svp;
	ArtSVP *stroke_svp;
};

struct _SPCanvasBPathClass {
	GnomeCanvasItemClass parent_class;
};

GtkType sp_canvas_bpath_get_type (void);

GnomeCanvasItem *sp_canvas_bpath_new (GnomeCanvasGroup *parent, SPCurve *curve);

void sp_canvas_bpath_set_bpath (SPCanvasBPath *cbp, SPCurve *curve);
void sp_canvas_bpath_set_fill (SPCanvasBPath *cbp, guint32 rgba, ArtWindRule rule);
void sp_canvas_bpath_set_stroke (SPCanvasBPath *cbp, guint32 rgba, gdouble width, ArtPathStrokeJoinType join, ArtPathStrokeCapType cap);

END_GNOME_DECLS

#endif

