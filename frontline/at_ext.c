/* at_ext.c --- 
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
#include <autotrace/output.h>

static void at_spline_list_array_flat_counter(at_spline_list_array_type * spline_list_array, 
					      at_spline_list_type * spline_list, 
					      int index, 
					      at_address user_data);

static void at_spline_list_array_deep_counter(at_spline_list_array_type * spline_list_array, 
					      at_spline_list_type * spline_list, 
					      int index, 
					      at_address user_data);

static void at_spline_list_array_deeper_counter(at_spline_list_array_type * spline_list_array, 
						at_spline_list_type * spline_list, 
						int index, 
						at_address user_data);

static void at_spline_list_counter (at_spline_list_type * spline_list,
				    at_spline_type * spline,
				    int index,
				    at_address user_data);

static void at_spline_list_counter_deep (at_spline_list_type * spline_list,
					 at_spline_type * spline,
					 int index,
					 at_address user_data);
int
at_spline_list_array_count_splines (at_spline_list_array_type * splines)
{
  int length = 0;
  at_spline_list_array_foreach(splines, at_spline_list_array_deep_counter, &length);
  return length;
}

int
at_spline_list_array_length (at_spline_list_array_type * splines)
{
  int length = 0;
  at_spline_list_array_foreach(splines, at_spline_list_array_flat_counter, &length);
  return length;
}

static void
at_spline_list_array_flat_counter(at_spline_list_array_type * spline_list_array, 
				  at_spline_list_type * spline_list, 
				  int index, 
				  at_address user_data)
{
  int * length = (int *)user_data;
  *length = index;
}

static void
at_spline_list_array_deep_counter(at_spline_list_array_type * spline_list_array, 
				  at_spline_list_type * spline_list, 
				  int index, 
				  at_address user_data)
{
  at_spline_list_foreach (spline_list, at_spline_list_counter, user_data); 
}

static void
at_spline_list_counter (at_spline_list_type * spline_list,
			at_spline_type * spline,
			int index,
			at_address user_data)
{
  int * i = user_data;
  (*i)++;
}

int
at_spline_list_array_count_points (at_spline_list_array_type * splines)
{
  int length = 0;
  at_spline_list_array_foreach(splines, at_spline_list_array_deeper_counter, &length);
  return length;
}

static void
at_spline_list_array_deeper_counter(at_spline_list_array_type * spline_list_array, 
				    at_spline_list_type * spline_list, 
				    int index, 
				    at_address user_data)
{
  at_spline_list_foreach (spline_list, at_spline_list_counter_deep, user_data); 
}

static void
at_spline_list_counter_deep (at_spline_list_type * spline_list,
			     at_spline_type * spline,
			     int index,
			     at_address user_data)
{
  int * i = user_data;
  if (spline->degree == AT_CUBICTYPE)
    (*i) += 3;
  else
    (*i) += 1;
}
