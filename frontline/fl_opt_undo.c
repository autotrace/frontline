/* fl_opt_undo.c --- option undo/redo
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

#include "config.h"
#include "private.h"

static void fl_opt_undo_undo(gpointer action_data);
static void fl_opt_undo_redo(gpointer action_data);
static void fl_opt_undo_free(gpointer action_data);

GundoActionType fl_opt_undo_action = { 
  fl_opt_undo_undo, 
  fl_opt_undo_redo, 
  fl_opt_undo_free
};
 
struct _FrontlineOptionUndoData 
{
  at_fitting_opts_type * at_opts;
  FrontlineOption * fl_opt;
};

/*
 * Exported functions
 */
FrontlineOptionUndoData *
fl_opt_undo_data_new(at_fitting_opts_type * at_opts,
		 FrontlineOption * fl_opt)
{
  FrontlineOptionUndoData * data = g_new(FrontlineOptionUndoData, 1);
  data->at_opts       = at_fitting_opts_copy(at_opts);
  data->fl_opt        = fl_opt;
  return data;
}

void
fl_undo_data_free(FrontlineOptionUndoData * data)
{
  at_fitting_opts_free(data->at_opts);
  g_free(data);
}

/*
 * Actions
 */
static void
fl_opt_undo_undo(gpointer action_data)
{
  FrontlineOptionUndoData * data = (FrontlineOptionUndoData * )action_data;
  at_fitting_opts_type * backup;

  backup = frontline_option_get_value (data->fl_opt);
  backup = at_fitting_opts_copy(backup);

  frontline_option_set_value(data->fl_opt, data->at_opts);
  at_fitting_opts_free(data->at_opts);
  
  /* Put the last opts value (before undo opts value) 
     to undo data */
  data->at_opts = backup;
}

static void
fl_opt_undo_redo(gpointer action_data)
{
  fl_opt_undo_undo(action_data);
}

static void
fl_opt_undo_free(gpointer action_data)
{
  FrontlineOptionUndoData * data = (FrontlineOptionUndoData * )action_data;
  fl_undo_data_free (data);
}
