/* fl_ask.c --- 
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
#include <libgnomeui/libgnomeui.h>

static void fl_ask_cb(gint reply, gpointer data);

gboolean
fl_ask(GtkWindow * parent, at_splines_type * splines)
{
  const gint answer_default = 2;
  gint answer = answer_default;
  gchar * message;
  gchar * basefmt;
  GtkWidget * dialog;
      
  if (at_spline_list_array_count_groups_of_splines(splines) < 500)
    return TRUE;

  basefmt =
    "Draw the result? In some case, it takes long time and large memory area.\n"
    "- the total number of groups of splines: %d\n"
    "- the total number splines: %d\n"
    "- the total number points: %d";
  
  message = g_strdup_printf(basefmt, 
			    at_spline_list_array_count_groups_of_splines(splines),
			    at_spline_list_array_count_splines(splines),
			    at_spline_list_array_count_points(splines));
  
  if (parent)
    dialog = gnome_ok_cancel_dialog_modal_parented (message,
						    fl_ask_cb,
						    &answer,
						    parent);
  else
    dialog = gnome_ok_cancel_dialog_modal (message,
					   fl_ask_cb,
					   &answer);

  while(answer_default == answer)
    gtk_main_iteration ();
  g_free(message);
  
  if (answer == 0)
    return TRUE;
  else if (answer == 1)
    return FALSE;
  else
    {
      g_assert_not_reached();
      return TRUE;
    }
}


static void
fl_ask_cb(gint reply, gpointer data)
{
  *((gint *)data) = reply;
}
