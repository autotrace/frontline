/* private.h --- frontline library private code
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

#ifndef FL_PRIVATE_H
#define FL_PRIVATE_H 

#include <gtk/gtk.h>
#include <gundo/gundo.h>
#include "frontline.h"

/*
 * Option private datum
 */
FrontlineOptionPriv * fl_opt_priv_new (FrontlineOption * fl_opt);
void fl_opt_priv_free (FrontlineOptionPriv * priv);
at_fitting_opts_type * fl_opt_priv_get_value (FrontlineOptionPriv * priv);
void fl_opt_priv_set_value(FrontlineOptionPriv * priv, 
			   at_fitting_opts_type * new_value);

/*
 * Option undo related
 */
typedef struct _FrontlineOptionUndoData  FrontlineOptionUndoData ;
extern GundoActionType fl_opt_undo_action;
FrontlineOptionUndoData * fl_opt_undo_data_new(at_fitting_opts_type * at_opts,
					FrontlineOption * fl_opt);
void fl_opt_undo_data_free(FrontlineOptionUndoData * data);

/*
 * Build spliens
 */
GnomeCanvasItem * frontline_splines_new                     (GnomeCanvasGroup * group,
							     at_splines_type * splines,
							     gfloat image_height);
void              frontline_splines_set_fill_opacity        (GnomeCanvasGroup  * splines,
							     guchar opacity);
void              frontline_splines_set_stroke_opacity      (GnomeCanvasGroup  * splines,
							     guchar opacity);
void              frontline_splines_show_in_multiple_colors (GnomeCanvasGroup * splines,
							     guchar opacity);
void              frontline_splines_show_in_static_color    (GnomeCanvasGroup * splines,
							     guint32 color,
							     guchar opacity);

/*
 * Utils
 */
void fl_not_implemented(GtkButton * button, gpointer data);
void fl_str_replace_underscore(gchar * str, gchar replaced_with);
#endif /* Not def: FL_PRIVATE_H */
