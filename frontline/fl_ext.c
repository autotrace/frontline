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

#include "frontline.h"
#include <string.h>

#define EXTENSION_KEY   "FL_FORMAT_EXTENSION_KEY"
#define DESCRIPTION_KEY "FL_FORMAT_DESCRIPTION_KEY"
#define MENU_WIDGET_KEY "FL_EXT_MENU_WIDGET_KEY"

static void fl_save_file_activate_cb (GtkMenuItem *menu_item, gpointer user_data);

static gchar * g_extension (gchar * filename);

GtkWidget *
fl_format_option_menu_new_with_type (FlFormatMenuType type,
				     GtkSignalFunc activate_cb,
				     gpointer user_data)
{
  char ** list;
  GtkWidget * opt_menu;
  GtkWidget * menu;
  GtkWidget * menu_item;
  int i;
  GtkTooltips* tooltips;
  
  if (FL_FORMAT_MENU_INPUT == type)
    list = at_input_list_new();
  else
    list = at_output_list_new();
  
  menu = gtk_menu_new ();
  
  menu_item = gtk_menu_item_new_with_label("By extension");
  gtk_object_set_data(GTK_OBJECT(menu_item),
		      EXTENSION_KEY,
		      NULL);
  gtk_object_set_data(GTK_OBJECT(menu_item),
		      DESCRIPTION_KEY,
		      NULL);
  if (activate_cb)
    gtk_signal_connect(GTK_OBJECT(menu_item),
		       "activate",
		       activate_cb,
		       user_data);
  gtk_menu_append(GTK_MENU(menu), menu_item);
  
  menu_item = gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), menu_item);

  for (i = 0; list[2*i] != NULL; i++)
    {
      menu_item = gtk_menu_item_new_with_label(list[2 * i]);
      gtk_object_set_data_full(GTK_OBJECT(menu_item),
			       EXTENSION_KEY,
			       g_strdup(list[2 * i]),
			       g_free);
      tooltips = gtk_tooltips_new();
      gtk_tooltips_set_tip(tooltips, menu_item, list[2 * i + 1], list[2 * i + 1]);
      gtk_object_set_data_full(GTK_OBJECT(menu_item),
			       DESCRIPTION_KEY,
			       g_strdup(list[2 * i + 1]),
			       g_free);
      if (activate_cb)
	gtk_signal_connect(GTK_OBJECT(menu_item),
			   "activate",
			   activate_cb,
			   user_data);
      gtk_menu_append(GTK_MENU(menu), menu_item);
    }
  
  if (FL_FORMAT_MENU_INPUT == type)
    at_input_list_free(list);
  else
    at_output_list_free(list);

  gtk_widget_show_all(menu);
  opt_menu = gtk_option_menu_new ();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(opt_menu), menu);
  return opt_menu;
}

gchar *
fl_format_option_menu_get_extension (GtkWidget * format_opt_menu)
{
  GtkWidget * menu_item = fl_format_option_menu_get_active (format_opt_menu);
  return gtk_object_get_data(GTK_OBJECT(menu_item), EXTENSION_KEY);
}

gchar *
fl_format_option_menu_get_description(GtkWidget * format_opt_menu)
{
  GtkWidget * menu_item = fl_format_option_menu_get_active (format_opt_menu);
  return gtk_object_get_data(GTK_OBJECT(menu_item), DESCRIPTION_KEY);
}

GtkWidget *
fl_format_option_menu_get_active(GtkWidget * format_opt_menu)
{
  GtkWidget * menu;
  GtkWidget * menu_item;

  g_return_val_if_fail(format_opt_menu, NULL);
  g_return_val_if_fail(GTK_IS_OPTION_MENU(format_opt_menu), NULL);
  
  menu 	    = gtk_option_menu_get_menu(GTK_OPTION_MENU(format_opt_menu));
  menu_item = gtk_menu_get_active(GTK_MENU(menu));
  return menu_item;
}

GtkWidget *
fl_save_file_selection_new (void)
{
  GtkWidget *file_selector;
  GtkWidget * extmenu;
  GtkWidget * frame;
  
  file_selector = gtk_file_selection_new("Save splines to");
  frame = gtk_frame_new ("Output file format");
  extmenu = fl_format_option_menu_new_with_type(FL_FORMAT_MENU_OUTPUT,
						GTK_SIGNAL_FUNC(fl_save_file_activate_cb),
						file_selector);
  gtk_container_set_border_width(GTK_CONTAINER(extmenu), 8);
  gtk_container_add(GTK_CONTAINER(frame), extmenu);
  gtk_box_pack_start_defaults(GTK_BOX(GTK_FILE_SELECTION(file_selector)->main_vbox),
			      frame);
  gtk_object_set_data(GTK_OBJECT(file_selector),
		      MENU_WIDGET_KEY,
		      extmenu);
  return file_selector;
}

gchar *
fl_save_file_selection_get_extension (GtkFileSelection * selection)
{
  GtkWidget * extmenu;
  g_return_val_if_fail (selection, NULL);
  g_return_val_if_fail (GTK_IS_FILE_SELECTION(selection), NULL);
  extmenu = gtk_object_get_data(GTK_OBJECT(selection), MENU_WIDGET_KEY);
  g_return_val_if_fail (extmenu, NULL);
  return fl_format_option_menu_get_extension(extmenu);  
}

static void
fl_save_file_activate_cb (GtkMenuItem *menu_item, gpointer user_data)
{
  GtkWidget * entry;
  gchar * new_text, * new_ext;
  gchar * text, * ext;
  GtkFileSelection * selection;
  
  g_return_if_fail (menu_item);
  g_return_if_fail (user_data);
  
  selection = GTK_FILE_SELECTION(user_data);
  new_ext    = fl_save_file_selection_get_extension(selection);

  if (new_ext)
    {
      entry = selection->selection_entry;
      text  = gtk_entry_get_text(GTK_ENTRY(entry));
      text  = g_strdup(text);
      ext   = g_extension(text);
      if (ext)
	{
	  ext[0] = '\0';
	  new_text = g_strconcat(text, new_ext, NULL);
	}
      else
	new_text = g_strconcat(text, ".", new_ext, NULL);
      gtk_file_selection_set_filename(selection, new_text);
      g_free(new_text);
      g_free(text);
    }
}

static gchar *
g_extension (gchar * filename)
{
  gchar * ext;
  if (NULL == filename || '\0' == filename)
    return NULL;
  
  ext = strrchr(filename, '.');
  if (ext)
    return ext + 1;
  else
    return NULL;
}
