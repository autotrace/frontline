#define CURVE_C

#include <string.h>
#include <libart_lgpl/art_misc.h>
#include "curve.h"

#define SP_CURVE_LENSTEP 32

static gboolean sp_bpath_good (ArtBpath * bpath);
ArtBpath * sp_bpath_check_subpath (ArtBpath * bpath);
static gint sp_bpath_length (ArtBpath * bpath);
static gboolean sp_bpath_closed (ArtBpath * bpath);

/* Constructors */

SPCurve *
sp_curve_new (void)
{
	SPCurve * curve;

	curve = sp_curve_new_sized (SP_CURVE_LENSTEP);

	return curve;
}

SPCurve *
sp_curve_new_sized (gint length)
{
	SPCurve * curve;

	g_return_val_if_fail (length > 0, NULL);

	curve = g_new (SPCurve, 1);

	curve->refcount = 1;
	curve->bpath = art_new (ArtBpath, length);
	curve->end = 0;
	curve->bpath[curve->end].code = ART_END;
	curve->length = length;
	curve->sbpath = FALSE;
	curve->hascpt = FALSE;
	curve->posset = FALSE;
	curve->moving = FALSE;
	curve->closed = FALSE;

	return curve;
}

SPCurve *
sp_curve_new_from_bpath (ArtBpath * bpath)
{
	SPCurve * curve;

	g_return_val_if_fail (sp_bpath_good (bpath), NULL);

	curve = g_new (SPCurve, 1);

	curve->refcount = 1;
	curve->bpath = bpath;
	curve->length = sp_bpath_length (bpath);
	curve->end = curve->length - 1;
	curve->sbpath = FALSE;
	curve->hascpt = FALSE;
	curve->posset = FALSE;
	curve->moving = FALSE;
	curve->closed = sp_bpath_closed (bpath);

	return curve;
}

SPCurve *
sp_curve_new_from_static_bpath (ArtBpath * bpath)
{
	SPCurve * curve;

	g_return_val_if_fail (sp_bpath_good (bpath), NULL);

	curve = g_new (SPCurve, 1);

	curve->refcount = 1;
	curve->bpath = bpath;
	curve->length = sp_bpath_length (bpath);
	curve->end = curve->length - 1;
	curve->sbpath = TRUE;
	curve->hascpt = FALSE;
	curve->posset = FALSE;
	curve->moving = FALSE;
	curve->closed = sp_bpath_closed (bpath);

	return curve;
}

SPCurve *
sp_curve_new_from_foreign_bpath (ArtBpath * bpath)
{
	SPCurve * curve;
	gint length;

	g_return_val_if_fail (sp_bpath_good (bpath), NULL);

	length = sp_bpath_length (bpath);

	curve = sp_curve_new_sized (length);
	memcpy (curve->bpath, bpath, sizeof (ArtBpath) * length);
	curve->end = length - 1;
	curve->closed = sp_bpath_closed (bpath);

	return curve;
}

SPCurve *
sp_curve_ref (SPCurve * curve)
{
	g_return_val_if_fail (curve != NULL, NULL);

	curve->refcount += 1;

	return curve;
}

SPCurve *
sp_curve_unref (SPCurve * curve)
{
	g_return_val_if_fail (curve != NULL, NULL);

	curve->refcount -= 1;

	if (curve->refcount < 1) {
		if ((!curve->sbpath) && (curve->bpath)) art_free (curve->bpath);
		g_free (curve);
	}

	return NULL;
}


void
sp_curve_finish (SPCurve * curve)
{
	ArtBpath * bp;

	g_return_if_fail (curve != NULL);
	g_return_if_fail (curve->sbpath);

	if (curve->end > 0) {
		bp = curve->bpath + curve->end - 1;
		if (bp->code == ART_LINETO) {
			curve->end--;
			bp->code = ART_END;
		}
	}

	if (curve->end < (curve->length - 1)) {
		curve->bpath = art_renew (curve->bpath, ArtBpath, curve->end);
	}

	curve->hascpt = FALSE;
	curve->posset = FALSE;
	curve->moving = FALSE;
}

void
sp_curve_ensure_space (SPCurve * curve, gint space)
{
	g_return_if_fail (curve != NULL);
	g_return_if_fail (space > 0);

	if (curve->end + space < curve->length) return;

	if (space < SP_CURVE_LENSTEP) space = SP_CURVE_LENSTEP;

	curve->bpath = art_renew (curve->bpath, ArtBpath, curve->length + space);

	curve->length += space;
}

SPCurve *
sp_curve_copy (SPCurve * curve)
{
	SPCurve * new;

	g_return_val_if_fail (curve != NULL, NULL);

	new = sp_curve_new_from_foreign_bpath (curve->bpath);

	return new;
}

SPCurve *
sp_curve_concat (const GSList * list)
{
	SPCurve * c, * new;
	ArtBpath * bp;
	const GSList * l;
	gint length;

	g_return_val_if_fail (list != NULL, NULL);

	length = 1;

	for (l = list; l != NULL; l = l->next) {
		c = (SPCurve *) l->data;
		length += c->end;
	}

	new = sp_curve_new_sized (length);

	bp = new->bpath;

	for (l = list; l != NULL; l = l->next) {
		c = (SPCurve *) l->data;
		memcpy (bp, c->bpath, c->end);
		bp += c->end;
	}

	bp->code = ART_END;

	new->end = length - 1;

	return new;
}

GSList *
sp_curve_split (SPCurve * curve)
{
	SPCurve * new;
	GSList * l;
	gint p, i;

	g_return_val_if_fail (curve != NULL, NULL);

	p = 0;
	l = NULL;

	while (p < curve->end) {
		i = 1;
		while ((curve->bpath[p + i].code == ART_LINETO) || (curve->bpath[p + i].code == ART_CURVETO)) i++;
		new = sp_curve_new_sized (i + 1);
		memcpy (new->bpath, curve->bpath + p, i * sizeof (ArtBpath));
		new->end = i;
		new->bpath[i].code = ART_END;
		new->closed = (new->bpath->code == ART_MOVETO);
		l = g_slist_append (l, new);
		p += i;
	}

	return l;
}

/* Methods */

void
sp_curve_reset (SPCurve * curve)
{
	g_return_if_fail (curve != NULL);
	g_return_if_fail (!curve->sbpath);

	curve->bpath->code = ART_END;
	curve->end = 0;
	curve->hascpt = FALSE;
	curve->posset = FALSE;
	curve->moving = FALSE;
	curve->closed = FALSE;
}

/* Several conequtive movetos are ALLOWED */

void
sp_curve_moveto (SPCurve * curve, gdouble x, gdouble y)
{
	g_return_if_fail (curve != NULL);
	g_return_if_fail (!curve->sbpath);
	g_return_if_fail (!curve->moving);

	curve->substart = curve->end;
	curve->hascpt = TRUE;
	curve->posset = TRUE;
	curve->x = x;
	curve->y = y;
}

void
sp_curve_lineto (SPCurve * curve, gdouble x, gdouble y)
{
	ArtBpath * bp;

	g_return_if_fail (curve != NULL);
	g_return_if_fail (!curve->sbpath);
	g_return_if_fail (curve->hascpt);

	if (curve->moving) {
		/* simply fix endpoint */
		g_return_if_fail (!curve->posset);
		g_return_if_fail (curve->end > 1);
		bp = curve->bpath + curve->end - 1;
		g_return_if_fail (bp->code == ART_LINETO);
		bp->x3 = x;
		bp->y3 = y;
		curve->moving = FALSE;
		return;
	}

	if (curve->posset) {
		/* start a new segment */
		sp_curve_ensure_space (curve, 2);
		bp = curve->bpath + curve->end;
		bp->code = ART_MOVETO_OPEN;
		bp->x3 = curve->x;
		bp->y3 = curve->y;
		bp++;
		bp->code = ART_LINETO;
		bp->x3 = x;
		bp->y3 = y;
		bp++;
		bp->code = ART_END;
		curve->end += 2;
		curve->posset = FALSE;
		curve->closed = FALSE;
		return;
	}

	/* Simply add line */

	g_return_if_fail (curve->end > 1);
	sp_curve_ensure_space (curve, 1);
	bp = curve->bpath + curve->end;
	bp->code = ART_LINETO;
	bp->x3 = x;
	bp->y3 = y;
	bp++;
	bp->code = ART_END;
	curve->end++;
}

void
sp_curve_lineto_moving (SPCurve * curve, gdouble x, gdouble y)
{
	ArtBpath * bp;

	g_return_if_fail (curve != NULL);
	g_return_if_fail (!curve->sbpath);
	g_return_if_fail (curve->hascpt);

	if (curve->moving) {
		/* simply change endpoint */
		g_return_if_fail (!curve->posset);
		g_return_if_fail (curve->end > 1);
		bp = curve->bpath + curve->end - 1;
		g_return_if_fail (bp->code == ART_LINETO);
		bp->x3 = x;
		bp->y3 = y;
		return;
	}

	if (curve->posset) {
		/* start a new segment */
		sp_curve_ensure_space (curve, 2);
		bp = curve->bpath + curve->end;
		bp->code = ART_MOVETO_OPEN;
		bp->x3 = curve->x;
		bp->y3 = curve->y;
		bp++;
		bp->code = ART_LINETO;
		bp->x3 = x;
		bp->y3 = y;
		bp++;
		bp->code = ART_END;
		curve->end += 2;
		curve->posset = FALSE;
		curve->moving = TRUE;
		curve->closed = FALSE;
		return;
	}

	/* Simply add line */

	g_return_if_fail (curve->end > 1);
	sp_curve_ensure_space (curve, 1);
	bp = curve->bpath + curve->end;
	bp->code = ART_LINETO;
	bp->x3 = x;
	bp->y3 = y;
	bp++;
	bp->code = ART_END;
	curve->end++;
	curve->moving = TRUE;
}

void
sp_curve_curveto (SPCurve * curve, gdouble x0, gdouble y0, gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
	ArtBpath * bp;

	g_return_if_fail (curve != NULL);
	g_return_if_fail (!curve->sbpath);
	g_return_if_fail (curve->hascpt);
	g_return_if_fail (!curve->moving);

	if (curve->posset) {
		/* start a new segment */
		sp_curve_ensure_space (curve, 2);
		bp = curve->bpath + curve->end;
		bp->code = ART_MOVETO_OPEN;
		bp->x3 = curve->x;
		bp->y3 = curve->y;
		bp++;
		bp->code = ART_CURVETO;
		bp->x1 = x0;
		bp->y1 = y0;
		bp->x2 = x1;
		bp->y2 = y1;
		bp->x3 = x2;
		bp->y3 = y2;
		bp++;
		bp->code = ART_END;
		curve->end += 2;
		curve->posset = FALSE;
		curve->closed = FALSE;
		return;
	}

	/* Simply add curve */

	g_return_if_fail (curve->end > 1);
	sp_curve_ensure_space (curve, 1);
	bp = curve->bpath + curve->end;
	bp->code = ART_CURVETO;
	bp->x1 = x0;
	bp->y1 = y0;
	bp->x2 = x1;
	bp->y2 = y1;
	bp->x3 = x2;
	bp->y3 = y2;
	bp++;
	bp->code = ART_END;
	curve->end++;
}

void
sp_curve_closepath (SPCurve * curve)
{
	ArtBpath * bs, * be;

	g_return_if_fail (curve != NULL);
	g_return_if_fail (!curve->sbpath);
	g_return_if_fail (curve->hascpt);
	g_return_if_fail (!curve->posset);
	g_return_if_fail (!curve->moving);
	g_return_if_fail (!curve->closed);
	/* We need at last M + C + E */
	g_return_if_fail (curve->end - curve->substart > 1);

	bs = curve->bpath + curve->substart;
	be = curve->bpath + curve->end - 1;

	if ((bs->x3 != be->x3) || (bs->y3 != be->y3)) {
		sp_curve_lineto (curve, bs->x3, bs->y3);
	}

	bs->code = ART_MOVETO;

	curve->closed = TRUE;

	for (bs = curve->bpath; bs->code != ART_END; bs++)
		if (bs->code == ART_MOVETO_OPEN) curve->closed = FALSE;

	curve->hascpt = FALSE;
}

void
sp_curve_closepath_current (SPCurve * curve)
{
	ArtBpath * bs, * be;

	g_return_if_fail (curve != NULL);
	g_return_if_fail (!curve->sbpath);
	g_return_if_fail (curve->hascpt);
	g_return_if_fail (!curve->posset);
	g_return_if_fail (!curve->closed);
	/* We need at last M + L + L + E */
	g_return_if_fail (curve->end - curve->substart > 2);

	bs = curve->bpath + curve->substart;
	be = curve->bpath + curve->end - 1;

	be->x3 = bs->x3;
	be->y3 = bs->y3;

	bs->code = ART_MOVETO;

	curve->closed = TRUE;

	for (bs = curve->bpath; bs->code != ART_END; bs++)
		if (bs->code == ART_MOVETO_OPEN) curve->closed = FALSE;

	curve->hascpt = FALSE;
	curve->moving = FALSE;
}

gboolean
sp_curve_empty (SPCurve * curve)
{
	g_return_val_if_fail (curve != NULL, TRUE);

	return (curve->bpath->code == ART_END);
}

ArtBpath *
sp_curve_last_bpath (SPCurve * curve)
{
	g_return_val_if_fail (curve != NULL, NULL);

	if (curve->end == 0) return NULL;

	return curve->bpath + curve->end - 1;
}

ArtBpath *
sp_curve_first_bpath (SPCurve * curve)
{
	g_return_val_if_fail (curve != NULL, NULL);

	if (curve->end == 0) return NULL;

	return curve->bpath;
}

SPCurve *
sp_curve_reverse (SPCurve *curve)
{
  ArtBpath *bs, *be, *bp;
  SPCurve  *new_curve;

#if 0
  g_return_val_if_fail (curve != NULL, NULL);
  g_return_val_if_fail (!curve->sbpath, NULL);
  g_return_val_if_fail (curve->hascpt, NULL);
  g_return_val_if_fail (!curve->posset, NULL);
  g_return_val_if_fail (!curve->moving, NULL);
  g_return_val_if_fail (!curve->closed, NULL);
#endif
  /* We need at last M + C + E */
  g_return_val_if_fail (curve->end - curve->substart > 1, NULL);

  bs = curve->bpath + curve->substart;
  be = curve->bpath + curve->end - 1;

  new_curve = sp_curve_new_sized (curve->length);

  g_assert (bs->code == ART_MOVETO_OPEN);
  g_assert ((be+1)->code == ART_END);

  sp_curve_moveto (new_curve, be->x3, be->y3);

  for (bp = be; bp != bs; bp--)
    {
      switch (bp->code)
        {
        case ART_MOVETO_OPEN:
          sp_curve_moveto (new_curve, (bp-1)->x3, (bp-1)->y3);
          break;
        case ART_MOVETO:
          sp_curve_moveto (new_curve, (bp-1)->x3, (bp-1)->y3);
          break;
        case ART_LINETO:
          sp_curve_lineto (new_curve, (bp-1)->x3, (bp-1)->y3);
          break;
        case ART_CURVETO:
          sp_curve_curveto (new_curve, bp->x2, bp->y2, bp->x1, bp->y1, (bp-1)->x3, (bp-1)->y3);
          break;
        case ART_END:
          g_assert_not_reached ();
        }
    }

  return new_curve;
}

void
sp_curve_append (SPCurve *curve,
                 SPCurve *curve2,
                 gboolean use_lineto)
{
	ArtBpath *bs, *bp;

	g_return_if_fail (curve != NULL);
	g_return_if_fail (curve2 != NULL);

	if (curve2->end < 1) return;

	bs = curve2->bpath + curve2->substart;

	if (use_lineto && (curve->end > 0)) {
		sp_curve_lineto (curve, bs->x3, bs->y3);
	} else {
		sp_curve_moveto (curve, bs->x3, bs->y3);
	}

	for (bp = bs+1; bp->code != ART_END; bp++) {
		switch (bp->code) {
		case ART_MOVETO_OPEN:
			sp_curve_moveto (curve, bp->x3, bp->y3);
			break;
		case ART_MOVETO:
			sp_curve_moveto (curve, bp->x3, bp->y3);
			break;
		case ART_LINETO:
			sp_curve_lineto (curve, bp->x3, bp->y3);
			break;
		case ART_CURVETO:
			sp_curve_curveto (curve, bp->x1, bp->y1, bp->x2, bp->y2, bp->x3, bp->y3);
			break;
		case ART_END:
			g_assert_not_reached ();
		}
	}
}

/* Private methods */

static
gboolean sp_bpath_good (ArtBpath * bpath)
{
	ArtBpath * bp;

	g_return_val_if_fail (bpath != NULL, FALSE);

	if (bpath->code == ART_END) return TRUE;

	bp = bpath;

	while (bp->code != ART_END) {
		bp = sp_bpath_check_subpath (bp);
		if (bp == NULL) return FALSE;
	}

	return TRUE;
}

ArtBpath *
sp_bpath_check_subpath (ArtBpath * bpath)
{
	gint i, len;
	gboolean closed;

	g_return_val_if_fail (bpath != NULL, NULL);

	if (bpath->code == ART_MOVETO) {
		closed = TRUE;
	} else if (bpath->code == ART_MOVETO_OPEN) {
		closed = FALSE;
	} else {
		return NULL;
	}

	len = 0;

	for (i = 1; (bpath[i].code != ART_END) && (bpath[i].code != ART_MOVETO) && (bpath[i].code != ART_MOVETO_OPEN); i++) {
		switch (bpath[i].code) {
			case ART_LINETO:
			case ART_CURVETO:
				len++;
				break;
			default:
				return NULL;
		}
	}

	if (closed) {
		if (len < 1) return NULL;
		if ((bpath->x3 != bpath[i-1].x3) || (bpath->y3 != bpath[i-1].y3)) return NULL;
	} else {
		if (len < 1) return NULL;
	}

	return bpath + i;
}

static gint
sp_bpath_length (ArtBpath * bpath)
{
	gint l;

	g_return_val_if_fail (bpath != NULL, FALSE);

	for (l = 0; bpath[l].code != ART_END; l++) ;

	l++;

	return l;
}

static gboolean
sp_bpath_closed (ArtBpath * bpath)
{
	ArtBpath * bp;

	g_return_val_if_fail (bpath != NULL, FALSE);

	for (bp = bpath; bp->code != ART_END; bp++)
		if (bp->code == ART_MOVETO_OPEN) return FALSE;

	return TRUE;
}
