/* frontline.h --- Autotrace and Gnome bridge
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

#ifndef FRONTLINE_H
#define FRONTLINE_H

#include <gtk/gtk.h>
#include <popt.h>
#include <libgnomeui/gnome-canvas.h>
#include <libgnome/gnome-i18n.h>
#include <gdk_imlib.h>

#include <autotrace/autotrace.h>

#include <frontline/fl_opt.h>
#include <frontline/fl_dialog.h>
#include <frontline/fl_prog.h>
#include <frontline/fl_fsel.h>
#include <frontline/fl_preview.h>
#include <frontline/fl_ext.h>
#include <frontline/fl_ask.h>

BEGIN_GNOME_DECLS  

/*
 * Bonus functions for autotrace
 */
int at_fitting_opts_save (at_fitting_opts_type * opts, FILE * fp);

at_fitting_opts_type * at_fitting_opts_new_from_file(FILE * fp);
at_fitting_opts_type * at_fitting_opts_new_from_argv(int argc, const char ** argv);

/* Usage of at_fitting_opts_popt_init: 
   struct poptOption * table[at_fitting_opts_popt_table_length];
   at_fitting_opts_type * opts = at_fitting_opts_new();
   at_fitting_opts_popt_init(table, opts); */
#define at_fitting_opts_popt_table_length 16
void at_fitting_opts_popt_table_init(struct poptOption * table, at_fitting_opts_type * opts);

int at_spline_list_array_length         (at_spline_list_array_type * splines); /* flat counts */
#define at_spline_list_array_count_groups_of_splines at_spline_list_array_length
int at_spline_list_array_count_splines  (at_spline_list_array_type * splines); /* deep counts */
int at_spline_list_array_count_points (at_spline_list_array_type * splines); /* deeper counts */

END_GNOME_DECLS

#endif /* Not def: FRONTLINE_H */

