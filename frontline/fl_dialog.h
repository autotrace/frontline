/* fl_dialog.h --- Frontline Dialog Widget (fl_dialog in short)
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


#ifndef FL_DIALOG_H
#define FL_DIALOG_H

BEGIN_GNOME_DECLS

#define FRONTLINE_TYPE_DIALOG                (frontline_dialog_get_type ())
#define FRONTLINE_DIALOG(obj)                (GTK_CHECK_CAST ((obj), FRONTLINE_TYPE_DIALOG, FrontlineDialog))
#define FRONTLINE_DIALOG_CLASS(klass)        (GTK_CHECK_CLASS_CAST ((klass), FRONTLINE_TYPE_DIALOG, FrontlineDialogClass))
#define FRONTLINE_IS_DIALOG(obj)             (GTK_CHECK_TYPE ((obj), FRONTLINE_TYPE_DIALOG))
#define FRONTLINE_IS_DIALOG_CLASS(klass)     (GTK_CHECK_CLASS_TYPE ((klass), FRONTLINE_TYPE_DIALOG))

typedef struct _FrontlineDialog FrontlineDialog;
typedef struct _FrontlineDialogClass FrontlineDialogClass;
struct _FrontlineDialog
{
  GtkDialog dialog;
  GtkWidget * option;		/* FrontlineOption */
  GtkWidget * header_area;	/* GtkVBox */
  GtkWidget * wait_actions;	/* Private */
  GtkWidget * trace_button;
  GtkWidget * close_button;
  GtkWidget * run_actions;

  GtkWidget * error_action;

  at_bitmap_type * bitmap;
  at_splines_type * splines;
};

struct _FrontlineDialogClass
{
  GtkDialogClass parent_class;
  void (* set_bitmap)   (FrontlineDialog  *fl_dialog);
  void (* trace_started)   (FrontlineDialog  *fl_dialog);
  void (* trace_done)   (FrontlineDialog  *fl_dialog);
};

GtkType    frontline_dialog_get_type      (void);
GtkWidget* frontline_dialog_new           (void);
GtkWidget* frontline_dialog_new_with_opts (at_fitting_opts_type * value);

/* FrontlineDialog becomes the owner of bitmap.
   Don't free the bitmap */
void frontline_dialog_set_bitmap(FrontlineDialog  *fl_dialog,
				 at_bitmap_type * bitmap);

END_GNOME_DECLS

#endif /* Not def: FL_DIALOG_H */
