/* fl_opt_priv.c --- frontline option class private code
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

/* TODO: justification */

#include "config.h"
#include "private.h"

#include <autotrace/autotrace.h>
#include <libgnomeui/gnome-color-picker.h>
#include <ctype.h>
#include <string.h>

static GtkWidget * fl_opt_priv_color_new (at_color_type ** value,
					  unsigned dummy_min,
					  unsigned dummy_max,
					  const char * label,
					  const char * tips,
					  FrontlineOption * fl_opt);
static void fl_opt_priv_color_set_sensitive (GtkWidget *toggle, 
					     gpointer color);
static void fl_opt_priv_color_update_via_state (GtkWidget * widget,
						GtkStateType	previous_state,
						gpointer value);
static void fl_opt_priv_color_update_via_color (GnomeColorPicker *cp, 
						guint r, guint g, guint b, guint a,
						gpointer value);
static void fl_opt_priv_color_lock_unlock(GtkButton * button,
					  gpointer user_data);
static void fl_opt_priv_color_set(GtkWidget *widget, at_color_type * color);

static GtkWidget * fl_opt_priv_unsigned_new (unsigned * value,
					     unsigned min,
					     unsigned max,
					     const char * label,
					     const char * tips,
					     FrontlineOption * fl_opt);
static void fl_opt_priv_unsigned_update     (GtkAdjustment *adjustment, 
					     unsigned * value);
static void fl_opt_priv_unsigned_set        (GtkWidget *widget, 
					     unsigned value);

static GtkWidget * fl_opt_priv_real_new     (at_real * value,
					     at_real min,
					     at_real max,
					     const char * label,
					     const char * tips,
					     FrontlineOption * fl_opt);
static void fl_opt_priv_real_update         (GtkAdjustment *adjustment, 
				             at_real * value);
static void fl_opt_priv_real_set            (GtkWidget *widget, 
					     at_real value);

static GtkWidget * fl_opt_priv_bool_new     (at_bool * value,
					     at_bool dummy_min,
					     at_bool dummy_max,
					     const char * label,
					     const char * tips,
					     FrontlineOption * fl_opt);
static void fl_opt_priv_bool_update         (GtkCheckButton *check, 
				             at_bool * value);
static void fl_opt_priv_bool_set            (GtkWidget *widget, 
					     at_bool value);

static void fl_opt_priv_propagate (GtkObject *object, 
				   FrontlineOption * fl_opt);
static void fl_opt_priv_propagate_via_state (GtkWidget * widget,
					     GtkStateType	previous_state,
					     FrontlineOption * fl_opt);
static void fl_opt_priv_propagate_via_color (GnomeColorPicker *cp, 
					     guint r, guint g, guint b, guint a,
					     FrontlineOption * fl_opt);

#if 0
static gchar * strdup_format_label_string(const gchar * base);
#endif /* 0 */

#define VALUE_HOLDER_KEY "value_holder"
#define VALUE_HOLDER_COLOR_KEY "value_holder_color"
#define VALUE_HOLDER_CHECK_KEY "value_holder_check"

struct _FrontlineOptionPriv
{
  at_fitting_opts_type * value;
  GtkWidget * background_color;
  GtkWidget * color_count;
  GtkWidget * corner_always_threshold;
  GtkWidget * corner_surround;
  GtkWidget * corner_threshold;
  GtkWidget * error_threshold;
  GtkWidget * filter_iterations;
  GtkWidget * line_reversion_threshold;
  GtkWidget * line_threshold;
  GtkWidget * remove_adjacent_corners;
  GtkWidget * tangent_surround;
  GtkWidget * despeckle_level;
  GtkWidget * despeckle_tightness;
  GtkWidget * centerline;
  GtkWidget * preserve_width;
  GtkWidget * width_weight_factor;
  gint propagate_lock;
};

FrontlineOptionPriv *
fl_opt_priv_new (FrontlineOption * fl_opt, GtkBox * box)
{
  GtkWidget * vbox;

#define member_construct(label, member_symbol, min, max, type)	\
priv->member_symbol =						\
  fl_opt_priv_##type##_new(&(priv->value->member_symbol),	\
			 min,					\
			 max,					\
                         label,					\
			 at_fitting_opts_doc(member_symbol),	\
			 fl_opt);                               \
  gtk_container_add(GTK_CONTAINER(vbox), priv->member_symbol);

  FrontlineOptionPriv * priv 	 = g_new(FrontlineOptionPriv, 1);
  priv->value 			 = at_fitting_opts_new();
  priv->propagate_lock           = 0;
  vbox = gtk_vbox_new(TRUE, 1);
  
  member_construct(_("Background Color"), background_color, 0, 256, color);
  member_construct(_("Color Count"), color_count, 0, 256, unsigned);
  member_construct(_("Corner Always Threshold"), corner_always_threshold, 0, 180, real);
  member_construct(_("Corner Surround"), corner_surround, 0, 16, unsigned);
  member_construct(_("Corner Threshold"), corner_threshold, 30, 120, real);
  member_construct(_("Error Threshold"), error_threshold, 0.0, 16.0, real);
  member_construct(_("Filter Iterations"), filter_iterations, 1, 12, unsigned);
  member_construct(_("Line Reversion Threshold"), line_reversion_threshold, .01, 1, real);
  member_construct(_("Line Threshold"), line_threshold, 1, 10, real);
  member_construct(_("Remove Adjacent Corners"), remove_adjacent_corners, false, true, bool);
  member_construct(_("Tangent Surround"), tangent_surround, 3, 12, unsigned);
  member_construct(_("Despeckle Level"), despeckle_level, 0, 20, unsigned);
  member_construct(_("Despeckle Tightness"), despeckle_tightness, 0.0, 8.0, real);
  member_construct(_("Centerline"), centerline, false, true, bool);
  member_construct(_("Preserve Width"), preserve_width, false, true, bool);
  member_construct(_("Width Weight Factor"), width_weight_factor, 0.1, 10.0, real);
#undef member_construct

  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(box), vbox);
  return priv;
}

void
fl_opt_priv_free (FrontlineOptionPriv * priv)
{
  g_return_if_fail(priv);
  g_return_if_fail(priv->value);

  at_fitting_opts_free(priv->value);
  priv->value = NULL;

  g_free(priv);
}

at_fitting_opts_type *
fl_opt_priv_get_value (FrontlineOptionPriv * priv)
{
  g_return_val_if_fail(priv, NULL);
  g_return_val_if_fail(priv->value, NULL);
  
  return priv->value;
}

void
fl_opt_priv_set_value(FrontlineOptionPriv * priv, 
		      at_fitting_opts_type * new_value)
{
  g_return_if_fail(priv);
  g_return_if_fail(priv->value);
  g_return_if_fail(new_value);

#define member_set(member_symbol, type) \
  fl_opt_priv_##type##_set(priv->member_symbol, new_value->member_symbol);
  
  member_set(background_color, color);
  member_set(color_count, unsigned);
  member_set(corner_always_threshold, real);
  member_set(corner_surround, unsigned);
  member_set(corner_threshold, real);
  member_set(error_threshold, real);
  member_set(filter_iterations, unsigned);
  member_set(line_reversion_threshold, real);
  member_set(line_threshold, real);
  member_set(remove_adjacent_corners, bool);
  member_set(tangent_surround, unsigned);
  member_set(despeckle_level, unsigned);
  member_set(despeckle_tightness, real);
  member_set(centerline, bool);
  member_set(preserve_width, bool);
  member_set(width_weight_factor, real);
#undef member_value_set
}

static GtkWidget *
fl_opt_priv_color_new (at_color_type ** value,
		       unsigned dummy_min,
		       unsigned dummy_max,
		       const char * label,
		       const char * tips,
		       FrontlineOption * fl_opt)
{
  /* Widget layout: 
     ebox:[hbox:[label_widget hhbox:[check color]]] */

  GtkWidget* hbox;
  GtkWidget* hhbox;
  GtkWidget* label_widget;
  GtkWidget* color;
  GtkWidget* ebox;
  GtkTooltips * tooltips;
  GtkWidget * check;

  hbox = gtk_hbox_new(TRUE, 4);

  /* Label */
  label_widget = gtk_label_new(label);
  gtk_label_set_justify(GTK_LABEL(label_widget), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment(GTK_MISC(label_widget), 0.0, 0.5);

  gtk_box_pack_start(GTK_BOX(hbox), label_widget, FALSE, FALSE, 0);

  hhbox = gtk_hbox_new(FALSE, 4);

  /* Check */
  check = gtk_check_button_new();  
  gtk_box_pack_start(GTK_BOX(hhbox), check, FALSE, FALSE, 0);

  /* Color picker */
  color = gnome_color_picker_new ();
  gnome_color_picker_set_use_alpha(GNOME_COLOR_PICKER(color), FALSE);
  gnome_color_picker_set_title (GNOME_COLOR_PICKER(color), _("Background color"));
  gtk_box_pack_start_defaults(GTK_BOX(hhbox), color);
  
  
  /* Set initial value to widgets */
  if (*value)
    {
      gnome_color_picker_set_i8(GNOME_COLOR_PICKER(color), (*value)->r, (*value)->g, (*value)->b, 0);
      gtk_widget_set_sensitive (GTK_WIDGET(color),
				TRUE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check), TRUE);
    }
  else
    {
      gnome_color_picker_set_i8(GNOME_COLOR_PICKER(color), 0xFF, 0xFF, 0xFF, 0);
      gtk_widget_set_sensitive (GTK_WIDGET(color),
				FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check), FALSE);
    }

  /* Connect */
  gtk_signal_connect(GTK_OBJECT(check), 
		     "toggled",
		     GTK_SIGNAL_FUNC(fl_opt_priv_color_set_sensitive),
		     color);
  gtk_signal_connect(GTK_OBJECT(color), 
		     "state_changed",
		     GTK_SIGNAL_FUNC(fl_opt_priv_color_update_via_state),
		     value);
  gtk_signal_connect(GTK_OBJECT(color),
		     "color_set",
		     GTK_SIGNAL_FUNC(fl_opt_priv_color_update_via_color),
		     value);
  gtk_signal_connect(GTK_OBJECT(color), 
		     "state_changed",
		     GTK_SIGNAL_FUNC(fl_opt_priv_propagate_via_state),
		     fl_opt);
  gtk_signal_connect(GTK_OBJECT(color), 
		     "color_set",
		     GTK_SIGNAL_FUNC(fl_opt_priv_propagate_via_color),
		     fl_opt);
  gtk_signal_connect(GTK_OBJECT(color),
		     "clicked",
		     GTK_SIGNAL_FUNC(fl_opt_priv_color_lock_unlock),
		     NULL);
  
  gtk_box_pack_start_defaults(GTK_BOX(hbox), hhbox);
 
  ebox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(ebox), hbox);
  gtk_widget_show_all(ebox);

  /* TODO: Where should I free the tooltips? */
  tooltips = gtk_tooltips_new();
  gtk_tooltips_set_tip(tooltips, GTK_WIDGET(ebox), tips, tips);
  gtk_tooltips_enable (tooltips);

  gtk_object_set_data(GTK_OBJECT(ebox), VALUE_HOLDER_COLOR_KEY, color);
  gtk_object_set_data(GTK_OBJECT(ebox), VALUE_HOLDER_CHECK_KEY, check);
  return ebox; 
}

static GtkWidget *
fl_opt_priv_unsigned_new(unsigned * value,
			 unsigned min,
			 unsigned max,
			 const char * label,
			 const char * tips,
			 FrontlineOption * fl_opt)
{
  GtkWidget* hbox;
  GtkWidget* label_widget;
  GtkWidget* scale;
  GtkObject * adj;
  GtkWidget* ebox;
  GtkTooltips * tooltips;

  hbox = gtk_hbox_new(TRUE, 4);

  label_widget = gtk_label_new(label);
  /* gtk_label_set_justify(GTK_LABEL(label_widget), GTK_JUSTIFY_LEFT); */
  gtk_misc_set_alignment(GTK_MISC(label_widget), 0.0, 0.5);

  gtk_box_pack_start(GTK_BOX(hbox), label_widget, FALSE, FALSE, 0);

  adj = gtk_adjustment_new((gfloat)*value, min, max, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT(adj));
  gtk_scale_set_digits(GTK_SCALE(scale), 0);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);

  gtk_signal_connect(adj, 
		     "value_changed",
		     GTK_SIGNAL_FUNC(fl_opt_priv_unsigned_update),
		     value);
  gtk_signal_connect(adj,
		     "value_changed",
		     GTK_SIGNAL_FUNC(fl_opt_priv_propagate),
		     fl_opt);
  gtk_box_pack_start_defaults(GTK_BOX(hbox), scale);
 
  ebox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(ebox), hbox);
  gtk_widget_show_all(ebox);

  /* TODO: Where should I free the tooltips? */
  tooltips = gtk_tooltips_new();
  gtk_tooltips_set_tip(tooltips, GTK_WIDGET(ebox), tips, tips);
  gtk_tooltips_enable (tooltips);

  gtk_object_set_data(GTK_OBJECT(ebox), VALUE_HOLDER_KEY, adj);
  return ebox; 
}

static GtkWidget *
fl_opt_priv_real_new     (at_real * value,
			  at_real min,
			  at_real max,
			  const char * label,
			  const char * tips,
			  FrontlineOption * fl_opt)
{
  GtkWidget* hbox;
  GtkWidget* label_widget;
  GtkWidget* scale;
  GtkObject * adj;
  GtkWidget* ebox;
  GtkTooltips * tooltips;

  hbox = gtk_hbox_new(TRUE, 4);

  label_widget = gtk_label_new(label);
  /* gtk_label_set_justify(GTK_LABEL(label_widget), GTK_JUSTIFY_LEFT); */
  gtk_misc_set_alignment(GTK_MISC(label_widget), 0.0, 0.5);

  gtk_box_pack_start(GTK_BOX(hbox), label_widget, TRUE, FALSE, 4);

  adj = gtk_adjustment_new((gfloat)*value, min, max, 
			   (max-min)/10.0, 
			   (max-min)/10.0,
			   0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT(adj));
  gtk_scale_set_digits(GTK_SCALE(scale), 2);  
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
  
  gtk_signal_connect(adj, 
		     "value_changed",
		     GTK_SIGNAL_FUNC(fl_opt_priv_real_update),
		     value);
  gtk_signal_connect(adj,
		     "value_changed",
		     GTK_SIGNAL_FUNC(fl_opt_priv_propagate),
		     fl_opt);
  
  gtk_box_pack_start_defaults(GTK_BOX(hbox), scale);
  
  ebox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(ebox), hbox);
  gtk_widget_show_all(ebox);

  /* TODO: Where should I free the tooltips? */
  tooltips = gtk_tooltips_new();
  gtk_tooltips_set_tip(tooltips, GTK_WIDGET(ebox), tips, tips);
  gtk_tooltips_enable (tooltips);
  
  gtk_object_set_data(GTK_OBJECT(ebox), VALUE_HOLDER_KEY, adj);
  
  return ebox;
}

static GtkWidget *
fl_opt_priv_bool_new     (at_bool * value,
			  at_bool dummy_min,
			  at_bool dummy_max,
			  const char * label,
			  const char * tips,
			  FrontlineOption * fl_opt)
{
  GtkWidget* hbox;
  GtkWidget* label_widget;
  GtkWidget* check;
  GtkWidget* ebox;
  GtkTooltips * tooltips;

  hbox = gtk_hbox_new(TRUE, 4);

  label_widget = gtk_label_new(label);
  gtk_label_set_justify(GTK_LABEL(label_widget), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment(GTK_MISC(label_widget), 0.0, 0.5);

  gtk_box_pack_start(GTK_BOX(hbox), label_widget, TRUE, FALSE, 4);

  check = gtk_check_button_new();
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check), *value);
  
  gtk_signal_connect(GTK_OBJECT(check), 
		     "toggled",
		     GTK_SIGNAL_FUNC(fl_opt_priv_bool_update),
		     value);
  gtk_signal_connect(GTK_OBJECT(check),
		     "toggled",
		     GTK_SIGNAL_FUNC(fl_opt_priv_propagate),
		     fl_opt);
  
  gtk_box_pack_start_defaults(GTK_BOX(hbox), check);
  
  ebox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(ebox), hbox);
  gtk_widget_show_all(ebox);

  /* TODO: Where should I free the tooltips? */
  tooltips = gtk_tooltips_new();
  gtk_tooltips_set_tip(tooltips, GTK_WIDGET(ebox), tips, tips);
  gtk_tooltips_enable (tooltips);

  gtk_object_set_data(GTK_OBJECT(ebox), VALUE_HOLDER_KEY, check);

  return ebox;
}

static void
fl_opt_priv_color_set_sensitive (GtkWidget *toggle, 
				 gpointer color)
{
  gboolean active;
  gboolean sensitive;
  
  g_return_if_fail (toggle);
  g_return_if_fail (color);
  
  active    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle));
  sensitive = GTK_WIDGET_SENSITIVE(GTK_WIDGET(color));

  g_assert (active != sensitive);

  gtk_widget_set_sensitive (GTK_WIDGET(color), active);
}

/* Widget -> opts */
static void
fl_opt_priv_color_update_via_state (GtkWidget * widget,
				    GtkStateType previous_state,
				    gpointer value)
{
  
  gpointer * value_ptr = (gpointer *)value;
  gint8 r, g, b, tmp;

  if (GTK_WIDGET_SENSITIVE (widget))
    {
      gnome_color_picker_get_i8 (GNOME_COLOR_PICKER(widget), &r, &g, &b, &tmp);
      if (*value_ptr == NULL)
	{
	  *value_ptr = at_color_new(r, g, b);
	}
      else
	{
	  at_color_type * color_ptr = *value_ptr;
	  color_ptr->r = r;
	  color_ptr->g = g;
	  color_ptr->b = b;
	}
    }
  else
    {
      if (*value_ptr)
	{
	  at_color_free(*value_ptr);
	  *value_ptr = NULL;
	}
    }
}

/* Widget -> opts */
static void
fl_opt_priv_color_update_via_color (GnomeColorPicker *cp, 
				    guint r, guint g, guint b, guint a,
				    gpointer value)
{
  gpointer * value_ptr 	    = (gpointer *)value;
  at_color_type * color_ptr = *value_ptr;

  if (color_ptr)
    {
      color_ptr->r = 255*r/65535;
      color_ptr->g = 255*g/65535;
      color_ptr->b = 255*b/65535;
    }
}

static void
fl_opt_priv_unsigned_update (GtkAdjustment *adjustment, 
			     unsigned * value)
{
  g_return_if_fail (adjustment);
  g_return_if_fail (GTK_IS_ADJUSTMENT(adjustment));
  g_return_if_fail (value);
  *value = (unsigned)(adjustment->value);
}

static void
fl_opt_priv_real_update (GtkAdjustment *adjustment, 
			 at_real * value)
{
  g_return_if_fail (adjustment);
  g_return_if_fail (GTK_IS_ADJUSTMENT(adjustment));
  g_return_if_fail (value);
  *value = (at_real)(adjustment->value);
}

static void 
fl_opt_priv_bool_update (GtkCheckButton *check, 
			 at_bool * value)
{
  g_return_if_fail (check);
  g_return_if_fail (GTK_IS_CHECK_BUTTON(check));
  g_return_if_fail (value);
  *value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
}

static void
fl_opt_priv_propagate (GtkObject *object_not_used, 
		       FrontlineOption * fl_opt)
{
  g_return_if_fail (fl_opt);
  g_return_if_fail (FRONTLINE_IS_OPTION(fl_opt));
  
  frontline_option_value_changed(fl_opt);
}

static void
fl_opt_priv_propagate_via_state (GtkWidget * widget,
				 GtkStateType	previous_state,
				 FrontlineOption * fl_opt)
{
  g_return_if_fail (fl_opt);
  g_return_if_fail (FRONTLINE_IS_OPTION(fl_opt));

  if (previous_state == GTK_STATE_INSENSITIVE
      && GTK_WIDGET_SENSITIVE(widget))
    frontline_option_value_changed(fl_opt);
  else if (previous_state == GTK_STATE_NORMAL
	   && !GTK_WIDGET_SENSITIVE(widget))
    frontline_option_value_changed(fl_opt);
}

static void 
fl_opt_priv_propagate_via_color (GnomeColorPicker *cp, 
				 guint r, guint g, guint b, guint a,
				 FrontlineOption * fl_opt)
{
  gboolean sensitive;
  g_return_if_fail (fl_opt);
  g_return_if_fail (FRONTLINE_IS_OPTION(fl_opt));
  
  sensitive = GTK_WIDGET_SENSITIVE(GTK_WIDGET(cp));
  if (sensitive)
    frontline_option_value_changed(fl_opt);
}

static void
fl_opt_priv_color_set(GtkWidget *widget, at_color_type * color)
{
  GtkObject * cp;
  GtkObject * check;

  g_return_if_fail (widget);

  cp = gtk_object_get_data(GTK_OBJECT(widget), VALUE_HOLDER_COLOR_KEY);
  check = gtk_object_get_data(GTK_OBJECT(widget), VALUE_HOLDER_CHECK_KEY);
  g_return_if_fail (cp);
  g_return_if_fail (check);
  
  if (color)
    {
      /* TODO: Bug: Order of following two invocations could not swap with each other */
      gnome_color_picker_set_i8 (GNOME_COLOR_PICKER(cp), color->r, color->g, color->b, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check), TRUE);
    }
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check), FALSE);
}

static void
fl_opt_priv_unsigned_set        (GtkWidget *widget, 
				 unsigned value)
{
  GtkObject * adj;
  g_return_if_fail (widget);
  adj = gtk_object_get_data(GTK_OBJECT(widget), VALUE_HOLDER_KEY);
  g_return_if_fail (adj);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), (gfloat)value);
}

static void
fl_opt_priv_real_set        (GtkWidget *widget, 
			     at_real value)
{
  GtkObject * adj;
  g_return_if_fail (widget);
  adj = gtk_object_get_data(GTK_OBJECT(widget), VALUE_HOLDER_KEY);
  g_return_if_fail (adj);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), value);
}

static void
fl_opt_priv_bool_set            (GtkWidget *widget, 
				 at_bool value)
{
  GtkObject * check;
  g_return_if_fail (widget);
  check = gtk_object_get_data(GTK_OBJECT(widget), VALUE_HOLDER_KEY);
  g_return_if_fail (check);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), value);
}

static void
fl_opt_priv_color_lock_unlock(GtkButton * button,
			      gpointer user_data)
{
  GnomeColorPicker * color = GNOME_COLOR_PICKER(button);
  GtkWidget * color_dialog = color->cs_dialog;
  
  if (color_dialog)
    gtk_window_set_modal(GTK_WINDOW(color_dialog), TRUE);
}

void
fl_str_replace_underscore(gchar * str,
			      gchar replaced_with)
{
  gchar * tmp = str;
  while (*tmp != '\0')
    {
      if (*tmp == '_')
	*tmp = replaced_with;
      tmp++;
    }
}

#if 0
static void str_capitalize(gchar * str);
static gchar *
strdup_format_label_string(const gchar * base)
{
  gchar * copy 	    = g_strdup(base);
  fl_str_replace_underscore(copy, ' ');
  str_capitalize(copy);
  return copy;
}
static void
str_capitalize(gchar * str)
{
  gchar * tmp = str;
  gboolean last_char_is_space = FALSE;
  
  if (0 == strlen(str))
    return;

  *tmp = toupper(*tmp);
  while (*tmp != '\0')
    {
      if (last_char_is_space == TRUE)
	{
	  *tmp = toupper(*tmp);
	  last_char_is_space = FALSE;
	};
      if (isspace(*tmp))
	last_char_is_space = TRUE;
      tmp++;
    }
}
#endif /* 0 */
