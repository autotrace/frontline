/* fl_ext.h --- 
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
#ifndef FL_MISC_H
#define FL_MISC_H 

BEGIN_GNOME_DECLS  

typedef enum 
{
  FL_FORMAT_MENU_INPUT, 
  FL_FORMAT_MENU_OUTPUT
} FlFormatMenuType;

GtkWidget * fl_format_option_menu_new_with_type (FlFormatMenuType type,
						 GtkSignalFunc activate_cb,
						 gpointer user_data);
gchar * fl_format_option_menu_get_extension (GtkWidget * format_opt_menu);
gchar * fl_format_option_menu_get_description(GtkWidget * format_opt_menu);
GtkWidget * fl_format_option_menu_get_active(GtkWidget * format_opt_menu);

GtkWidget * fl_save_file_selection_new (void);
gchar * fl_save_file_selection_get_extension (GtkFileSelection * selection);

END_GNOME_DECLS

#endif /* Not def: FL_MISC_H */
