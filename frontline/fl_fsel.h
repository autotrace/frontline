/* fl_fsel.h --- Frontline File selection widget (fl_fsel in short)
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

#ifndef FL_FSEL_H
#define FL_FSEL_H 

BEGIN_GNOME_DECLS  

#define FRONTLINE_TYPE_FILE_SELECTION            (frontline_file_selection_get_type ())
#define FRONTLINE_FILE_SELECTION(obj)            (GTK_CHECK_CAST ((obj), FRONTLINE_TYPE_FILE_SELECTION, FrontlineFileSelection))
#define FRONTLINE_FILE_SELECTION_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), FRONTLINE_TYPE_FILE_SELECTION, FrontlineFileSelectionClass))
#define FRONTLINE_IS_FILE_SELECTION(obj)         (GTK_CHECK_TYPE ((obj), FRONTLINE_TYPE_FILE_SELECTION))
#define FRONTLINE_IS_FILE_SELECTION_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), FRONTLINE_TYPE_FILE_SELECTION))
  
typedef struct _FrontlineFileSelection FrontlineFileSelection;
typedef struct _FrontlineFileSelectionClass FrontlineFileSelectionClass;
struct _FrontlineFileSelection
{
  GtkVBox hbox;
  GtkWidget * ifentry;
  at_input_opts_type * opts;
};

struct _FrontlineFileSelectionClass
{
  GtkVBoxClass parent_class;
  void (* loaded)   (FrontlineFileSelection * fl_fsel,
		     gchar * filename,
		     at_bitmap_type * bitmap);
};
  
GtkType    frontline_file_selection_get_type (void);
GtkWidget* frontline_file_selection_new      (void);
gboolean   frontline_file_selection_load_file (FrontlineFileSelection * fsel,
					       const gchar * filename);

/* After setting, OPTS should be freed by the client. */
void       frontline_file_selection_set_input_option(FrontlineFileSelection * fsel,
						     at_input_opts_type * opts);

END_GNOME_DECLS

#endif /* Not def: FL_FSEL_H */
