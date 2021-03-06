/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* Line/curve item type for FooCanvas widget
 *
 * FooCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include "libfoocanvas.h"

#define noVERBOSE

#define DEFAULT_SPLINE_STEPS 12		/* this is what Tk uses */
#define NUM_ARROW_POINTS     6		/* number of points in an arrowhead */
#define NUM_STATIC_POINTS    256	/* number of static points to use to avoid allocating arrays */
#define GDK_TO_CAIRO_LINE_CAP(cap) ((cap == GDK_CAP_NOT_LAST)? CAIRO_LINE_CAP_BUTT : cap - 1)
#define GDK_TO_CAIRO_LINE_JOIN(join) (join)

#define GROW_BOUNDS(bx1, by1, bx2, by2, x, y) {	\
	if (x < bx1)				\
		bx1 = x;			\
						\
	if (x > bx2)				\
		bx2 = x;			\
						\
	if (y < by1)				\
		by1 = y;			\
						\
	if (y > by2)				\
		by2 = y;			\
}

enum {
	PROP_0,
	PROP_POINTS,
	PROP_FILL_COLOR,
	PROP_FILL_COLOR_GDK,
	PROP_FILL_COLOR_RGBA,
	PROP_FILL_STIPPLE,
	PROP_WIDTH_PIXELS,
	PROP_WIDTH_UNITS,
	PROP_CAP_STYLE,
	PROP_JOIN_STYLE,
	PROP_LINE_STYLE,
	PROP_FIRST_ARROWHEAD,
	PROP_LAST_ARROWHEAD,
	PROP_SMOOTH,
	PROP_AA,
	PROP_SPLINE_STEPS,
	PROP_ARROW_SHAPE_A,
	PROP_ARROW_SHAPE_B,
	PROP_ARROW_SHAPE_C
};


static void foo_canvas_line_class_init   (FooCanvasLineClass *klass);
static void foo_canvas_line_init         (FooCanvasLine      *line);
static void foo_canvas_line_destroy      (GtkObject            *object);
static void foo_canvas_line_set_property (GObject              *object,
					    guint                 param_id,
					    const GValue         *value,
					    GParamSpec           *pspec);
static void foo_canvas_line_get_property (GObject              *object,
					    guint                 param_id,
					    GValue               *value,
					    GParamSpec           *pspec);

static void   foo_canvas_line_update      (FooCanvasItem *item,
					     double i2w_dx, double i2w_dy,
					     int flags);
static void   foo_canvas_line_realize     (FooCanvasItem *item);
static void   foo_canvas_line_unrealize   (FooCanvasItem *item);
static void   foo_canvas_line_draw        (FooCanvasItem *item, GdkDrawable *drawable,
					     GdkEventExpose   *event);
static double foo_canvas_line_point       (FooCanvasItem *item, double x, double y,
					     int cx, int cy, FooCanvasItem **actual_item);
static void   foo_canvas_line_translate   (FooCanvasItem *item, double dx, double dy);
static void   foo_canvas_line_bounds      (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2);


static FooCanvasItemClass *parent_class;

G_DEFINE_TYPE (FooCanvasLine, foo_canvas_line, FOO_TYPE_CANVAS_ITEM)

static void
foo_canvas_line_class_init (FooCanvasLineClass *klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	FooCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) klass;
	object_class = (GtkObjectClass *) klass;
	item_class = (FooCanvasItemClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->set_property = foo_canvas_line_set_property;
	gobject_class->get_property = foo_canvas_line_get_property;

        g_object_class_install_property
                (gobject_class,
                 PROP_POINTS,
                 g_param_spec_boxed ("points", NULL, NULL,
				     FOO_TYPE_CANVAS_POINTS,
				     G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR,
                 g_param_spec_string ("fill-color", NULL, NULL,
                                      NULL,
                                      G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR_GDK,
                 g_param_spec_boxed ("fill-color-gdk", NULL, NULL,
				     GDK_TYPE_COLOR,
				     G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR_RGBA,
                 g_param_spec_uint ("fill-color-rgba", NULL, NULL,
				    0, G_MAXUINT, 0,
				    G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_STIPPLE,
                 g_param_spec_object ("fill-stipple", NULL, NULL,
                                      GDK_TYPE_DRAWABLE,
                                      G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_WIDTH_PIXELS,
                 g_param_spec_uint ("width-pixels", NULL, NULL,
				    0, G_MAXUINT, 0,
				    G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_WIDTH_UNITS,
                 g_param_spec_double ("width-units", NULL, NULL,
				      0.0, G_MAXDOUBLE, 0.0,
				      G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_CAP_STYLE,
                 g_param_spec_enum ("cap-style", NULL, NULL,
                                    GDK_TYPE_CAP_STYLE,
                                    GDK_CAP_BUTT,
                                    G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_JOIN_STYLE,
                 g_param_spec_enum ("join-style", NULL, NULL,
                                    GDK_TYPE_JOIN_STYLE,
                                    GDK_JOIN_MITER,
                                    G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_LINE_STYLE,
                 g_param_spec_enum ("line-style", NULL, NULL,
                                    GDK_TYPE_LINE_STYLE,
                                    GDK_LINE_SOLID,
                                    G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_FIRST_ARROWHEAD,
                 g_param_spec_boolean ("first-arrowhead", NULL, NULL,
				       FALSE,
				       G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_LAST_ARROWHEAD,
                 g_param_spec_boolean ("last-arrowhead", NULL, NULL,
				       FALSE,
				       G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_SMOOTH,
                 g_param_spec_boolean ("smooth", NULL, NULL,
				       FALSE,
				       G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_AA,
                 g_param_spec_boolean ("aa", NULL, NULL,
				       FALSE,
				       G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_ARROW_SHAPE_A,
                 g_param_spec_double ("arrow-shape-a", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_ARROW_SHAPE_B,
                 g_param_spec_double ("arrow-shape-b", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_ARROW_SHAPE_C,
                 g_param_spec_double ("arrow-shape-c", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      G_PARAM_READWRITE));

	object_class->destroy = foo_canvas_line_destroy;

	item_class->update = foo_canvas_line_update;
	item_class->realize = foo_canvas_line_realize;
	item_class->unrealize = foo_canvas_line_unrealize;
	item_class->draw = foo_canvas_line_draw;
	item_class->point = foo_canvas_line_point;
	item_class->translate = foo_canvas_line_translate;
	item_class->bounds = foo_canvas_line_bounds;
}

static void
foo_canvas_line_init (FooCanvasLine *line)
{
	line->width = 1.0;
	line->cap = GDK_CAP_BUTT;
	line->join = GDK_JOIN_MITER;
	line->line_style = GDK_LINE_SOLID;
	line->shape_a = 0.0;
	line->shape_b = 0.0;
	line->shape_c = 0.0;
	line->aa = TRUE;
	line->smooth = FALSE;
}

static void
foo_canvas_line_destroy (GtkObject *object)
{
	FooCanvasLine *line;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FOO_IS_CANVAS_LINE (object));

	line = FOO_CANVAS_LINE (object);

	/* remember, destroy can be run multiple times! */

	if (line->coords)
		g_free (line->coords);
	line->coords = NULL;

	if (line->first_coords)
		g_free (line->first_coords);
	line->first_coords = NULL;

	if (line->last_coords)
		g_free (line->last_coords);
	line->last_coords = NULL;

	if (line->stipple)
		g_object_unref (line->stipple);
	line->stipple = NULL;

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/* Computes the bounding box of the line, including its arrow points.  Assumes that the number of
 * points in the line is not zero.
 */
static void
get_bounds (FooCanvasLine *line, double *bx1, double *by1, double *bx2, double *by2)
{
	double *coords;
	double x1, y1, x2, y2;
	double width;
	int i;

	if (!line->coords) {
	    *bx1 = *by1 = *bx2 = *by2 = 0.0;
	    return;
	}
	
	/* Find bounding box of line's points */

	x1 = x2 = line->coords[0];
	y1 = y2 = line->coords[1];

	for (i = 1, coords = line->coords + 2; i < line->num_points; i++, coords += 2)
		GROW_BOUNDS (x1, y1, x2, y2, coords[0], coords[1]);

	/* Add possible over-estimate for wide lines */

	if (line->width_pixels)
		width = line->width / line->item.canvas->pixels_per_unit;
	else
		width = line->width;

	x1 -= width;
	y1 -= width;
	x2 += width;
	y2 += width;

	/* For mitered lines, make a second pass through all the points.  Compute the location of
	 * the two miter vertex points and add them to the bounding box.
	 */

	if (line->join == GDK_JOIN_MITER)
		for (i = line->num_points, coords = line->coords; i >= 3; i--, coords += 2) {
			double mx1, my1, mx2, my2;

			if (foo_canvas_get_miter_points (coords[0], coords[1],
							   coords[2], coords[3],
							   coords[4], coords[5],
							   width,
							   &mx1, &my1, &mx2, &my2)) {
				GROW_BOUNDS (x1, y1, x2, y2, mx1, my1);
				GROW_BOUNDS (x1, y1, x2, y2, mx2, my2);
			}
		}

	/* Add the arrow points, if any */

	if (line->first_arrow && line->first_coords)
		for (i = 0, coords = line->first_coords; i < NUM_ARROW_POINTS; i++, coords += 2)
			GROW_BOUNDS (x1, y1, x2, y2, coords[0], coords[1]);

	if (line->last_arrow && line->last_coords)
		for (i = 0, coords = line->last_coords; i < NUM_ARROW_POINTS; i++, coords += 2)
			GROW_BOUNDS (x1, y1, x2, y2, coords[0], coords[1]);

	/* Done */

	*bx1 = x1;
	*by1 = y1;
	*bx2 = x2;
	*by2 = y2;
}

/* Computes the bounding box of the line, in canvas coordinates.  Assumes that the number of points in the polygon is
 * not zero. 
 */
static void
get_bounds_canvas (FooCanvasLine *line,
		   double *bx1, double *by1, double *bx2, double *by2,
		   double i2w_dx, double i2w_dy)
{
	FooCanvasItem *item;
	double bbox_x0, bbox_y0, bbox_x1, bbox_y1;

	item = FOO_CANVAS_ITEM (line);

	get_bounds (line, &bbox_x0, &bbox_y0, &bbox_x1, &bbox_y1);

	bbox_x0 += i2w_dx; 
	bbox_y0 += i2w_dy; 
	bbox_x1 += i2w_dx; 
	bbox_y1 += i2w_dy; 

	foo_canvas_w2c_rect_d (item->canvas,
				 &bbox_x0, &bbox_y0, &bbox_x1, &bbox_y1);
	
	/* include 1 pixel of fudge */
	*bx1 = bbox_x0 - 1;
	*by1 = bbox_y0 - 1;
	*bx2 = bbox_x1 + 1;
	*by2 = bbox_y1 + 1;
}

/* Recalculates the arrow polygons for the line */
static void
reconfigure_arrows (FooCanvasLine *line)
{
	double *poly, *coords;
	double dx, dy, length;
	double sin_theta, cos_theta, tmp;
	double frac_height;	/* Line width as fraction of arrowhead width */
	double backup;		/* Distance to backup end points so the line ends in the middle of the arrowhead */
	double vx, vy;		/* Position of arrowhead vertex */
	double shape_a, shape_b, shape_c;
	double width;
	int i;

	if (line->num_points == 0)
		return;

	/* Set up things */

	if (line->first_arrow) {
		if (line->first_coords) {
			line->coords[0] = line->first_coords[0];
			line->coords[1] = line->first_coords[1];
		} else
			line->first_coords = g_new (double, 2 * NUM_ARROW_POINTS);
	} else if (line->first_coords) {
		line->coords[0] = line->first_coords[0];
		line->coords[1] = line->first_coords[1];

		g_free (line->first_coords);
		line->first_coords = NULL;
	}

	i = 2 * (line->num_points - 1);

	if (line->last_arrow) {
		if (line->last_coords) {
			line->coords[i] = line->last_coords[0];
			line->coords[i + 1] = line->last_coords[1];
		} else
			line->last_coords = g_new (double, 2 * NUM_ARROW_POINTS);
	} else if (line->last_coords) {
		line->coords[i] = line->last_coords[0];
		line->coords[i + 1] = line->last_coords[1];

		g_free (line->last_coords);
		line->last_coords = NULL;
	}

	if (!line->first_arrow && !line->last_arrow)
		return;

	if (line->width_pixels)
		width = line->width / line->item.canvas->pixels_per_unit;
	else
		width = line->width;

	/* Add fudge value for better-looking results */

	shape_a = line->shape_a;
	shape_b = line->shape_b;
	shape_c = line->shape_c + width / 2.0;

	if (line->width_pixels) {
		shape_a /= line->item.canvas->pixels_per_unit;
		shape_b /= line->item.canvas->pixels_per_unit;
		shape_c /= line->item.canvas->pixels_per_unit;
	}

	shape_a += 0.001;
	shape_b += 0.001;
	shape_c += 0.001;

	/* Compute the polygon for the first arrowhead and adjust the first point in the line so
	 * that the line does not stick out past the leading edge of the arrowhead.
	 */

	frac_height = (line->width / 2.0) / shape_c;
	backup = frac_height * shape_b + shape_a * (1.0 - frac_height) / 2.0;

	if (line->first_arrow) {
		poly = line->first_coords;
		poly[0] = poly[10] = line->coords[0];
		poly[1] = poly[11] = line->coords[1];

		dx = poly[0] - line->coords[2];
		dy = poly[1] - line->coords[3];
		length = sqrt (dx * dx + dy * dy);
		if (length < FOO_CANVAS_EPSILON)
			sin_theta = cos_theta = 0.0;
		else {
			sin_theta = dy / length;
			cos_theta = dx / length;
		}

		vx = poly[0] - shape_a * cos_theta;
		vy = poly[1] - shape_a * sin_theta;

		tmp = shape_c * sin_theta;

		poly[2] = poly[0] - shape_b * cos_theta + tmp;
		poly[8] = poly[2] - 2.0 * tmp;

		tmp = shape_c * cos_theta;

		poly[3] = poly[1] - shape_b * sin_theta - tmp;
		poly[9] = poly[3] + 2.0 * tmp;

		poly[4] = poly[2] * frac_height + vx * (1.0 - frac_height);
		poly[5] = poly[3] * frac_height + vy * (1.0 - frac_height);
		poly[6] = poly[8] * frac_height + vx * (1.0 - frac_height);
		poly[7] = poly[9] * frac_height + vy * (1.0 - frac_height);

		/* Move the first point towards the second so that the corners at the end of the
		 * line are inside the arrowhead.
		 */

		line->coords[0] = poly[0] - backup * cos_theta;
		line->coords[1] = poly[1] - backup * sin_theta;
	}

	/* Same process for last arrowhead */

	if (line->last_arrow) {
		coords = line->coords + 2 * (line->num_points - 2);
		poly = line->last_coords;
		poly[0] = poly[10] = coords[2];
		poly[1] = poly[11] = coords[3];

		dx = poly[0] - coords[0];
		dy = poly[1] - coords[1];
		length = sqrt (dx * dx + dy * dy);
		if (length < FOO_CANVAS_EPSILON)
			sin_theta = cos_theta = 0.0;
		else {
			sin_theta = dy / length;
			cos_theta = dx / length;
		}

		vx = poly[0] - shape_a * cos_theta;
		vy = poly[1] - shape_a * sin_theta;

		tmp = shape_c * sin_theta;

		poly[2] = poly[0] - shape_b * cos_theta + tmp;
		poly[8] = poly[2] - 2.0 * tmp;

		tmp = shape_c * cos_theta;

		poly[3] = poly[1] - shape_b * sin_theta - tmp;
		poly[9] = poly[3] + 2.0 * tmp;

		poly[4] = poly[2] * frac_height + vx * (1.0 - frac_height);
		poly[5] = poly[3] * frac_height + vy * (1.0 - frac_height);
		poly[6] = poly[8] * frac_height + vx * (1.0 - frac_height);
		poly[7] = poly[9] * frac_height + vy * (1.0 - frac_height);

		coords[2] = poly[0] - backup * cos_theta;
		coords[3] = poly[1] - backup * sin_theta;
	}
}

static void
foo_canvas_line_set_property (GObject              *object,
				guint                 param_id,
				const GValue         *value,
				GParamSpec           *pspec)
{
	FooCanvasItem *item;
	FooCanvasLine *line;
	FooCanvasPoints *points;
	GdkColor color = { 0, 0, 0, 0, };
	GdkColor *pcolor;
	gboolean color_changed;
	int have_pixel;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FOO_IS_CANVAS_LINE (object));

	item = FOO_CANVAS_ITEM (object);
	line = FOO_CANVAS_LINE (object);

	color_changed = FALSE;
	have_pixel = FALSE;

	switch (param_id) {
	case PROP_POINTS:
		points = g_value_get_boxed (value);

		if (line->coords) {
			g_free (line->coords);
			line->coords = NULL;
		}

		if (!points)
			line->num_points = 0;
		else {
			line->num_points = points->num_points;
			line->coords = g_new (double, 2 * line->num_points);
			memcpy (line->coords, points->coords, 2 * line->num_points * sizeof (double));
		}

		/* Drop the arrowhead polygons if they exist -- they will be regenerated */

		if (line->first_coords) {
			g_free (line->first_coords);
			line->first_coords = NULL;
		}

		if (line->last_coords) {
			g_free (line->last_coords);
			line->last_coords = NULL;
		}

		/* Since the line's points have changed, we need to re-generate arrowheads in
		 * addition to recalculating the bounds.
		 */
		foo_canvas_item_request_update (item);
		break;

	case PROP_FILL_COLOR:
		if (g_value_get_string (value))
			gdk_color_parse (g_value_get_string (value), &color);
		line->fill_rgba = ((color.red & 0xff00) << 16 |
				   (color.green & 0xff00) << 8 |
				   (color.blue & 0xff00) |
				   0xff);
		color_changed = TRUE;
		break;

	case PROP_FILL_COLOR_GDK:
		pcolor = g_value_get_boxed (value);
		if (pcolor) {
			GdkColormap *colormap;
			color = *pcolor;

			colormap = gtk_widget_get_colormap (GTK_WIDGET (item->canvas));
			gdk_rgb_find_color (colormap, &color);

			have_pixel = TRUE;
		}

		line->fill_rgba = ((color.red & 0xff00) << 16 |
				   (color.green & 0xff00) << 8 |
				   (color.blue & 0xff00) |
				   0xff);
		color_changed = TRUE;
		break;

	case PROP_FILL_COLOR_RGBA:
		line->fill_rgba = g_value_get_uint (value);
		color_changed = TRUE;
		break;

	case PROP_FILL_STIPPLE:
		line->stipple = (GdkBitmap *) g_value_get_object (value);
		g_object_ref (line->stipple);
		foo_canvas_item_request_redraw (item);		
		break;

	case PROP_WIDTH_PIXELS:
		line->width = g_value_get_uint (value);
		line->width_pixels = TRUE;
		foo_canvas_item_request_update (item);
		break;

	case PROP_WIDTH_UNITS:
		line->width = fabs (g_value_get_double (value));
		line->width_pixels = FALSE;
		foo_canvas_item_request_update (item);
		break;

	case PROP_CAP_STYLE:
		line->cap = g_value_get_enum (value);
		foo_canvas_item_request_update (item);
		break;

	case PROP_JOIN_STYLE:
		line->join = g_value_get_enum (value);
		foo_canvas_item_request_update (item);
		break;

	case PROP_LINE_STYLE:
		line->line_style = g_value_get_enum (value);
		foo_canvas_item_request_update (item);
		break;

	case PROP_FIRST_ARROWHEAD:
		line->first_arrow = g_value_get_boolean (value);
		foo_canvas_item_request_update (item);
		break;

	case PROP_LAST_ARROWHEAD:
		line->last_arrow = g_value_get_boolean (value);
		foo_canvas_item_request_update (item);
		break;

	case PROP_SMOOTH:
		line->smooth = g_value_get_boolean (value);
		foo_canvas_item_request_update (item);
		break;

	case PROP_AA:
		line->aa = g_value_get_boolean (value);
		foo_canvas_item_request_update (item);
		break;

	case PROP_ARROW_SHAPE_A:
		line->shape_a = fabs (g_value_get_double (value));
		foo_canvas_item_request_update (item);
		break;

	case PROP_ARROW_SHAPE_B:
		line->shape_b = fabs (g_value_get_double (value));
		foo_canvas_item_request_update (item);
		break;

	case PROP_ARROW_SHAPE_C:
		line->shape_c = fabs (g_value_get_double (value));
		foo_canvas_item_request_update (item);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}

	if (color_changed) {
		if (have_pixel)
			line->fill_pixel = color.pixel;
		else
			line->fill_pixel = foo_canvas_get_color_pixel (item->canvas,
									 line->fill_rgba);

		foo_canvas_item_request_redraw (item);		
	}
}

/* Returns a copy of the line's points without the endpoint adjustments for
 * arrowheads.
 */
static FooCanvasPoints *
get_points (FooCanvasLine *line)
{
	FooCanvasPoints *points;
	int start_ofs, end_ofs;

	if (line->num_points == 0)
		return NULL;

	start_ofs = end_ofs = 0;

	points = foo_canvas_points_new (line->num_points);

	/* Invariant:  if first_coords or last_coords exist, then the line's
	 * endpoints have been adjusted.
	 */

	if (line->first_coords) {
		start_ofs = 1;

		points->coords[0] = line->first_coords[0];
		points->coords[1] = line->first_coords[1];
	}

	if (line->last_coords) {
		end_ofs = 1;

		points->coords[2 * (line->num_points - 1)] = line->last_coords[0];
		points->coords[2 * (line->num_points - 1) + 1] = line->last_coords[1];
	}

	memcpy (points->coords + 2 * start_ofs,
		line->coords + 2 * start_ofs,
		2 * (line->num_points - (start_ofs + end_ofs)) * sizeof (double));

	return points;
}

static void
foo_canvas_line_get_property (GObject              *object,
				guint                 param_id,
				GValue               *value,
				GParamSpec           *pspec)
{
	FooCanvasLine *line;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FOO_IS_CANVAS_LINE (object));

	line = FOO_CANVAS_LINE (object);

	switch (param_id) {
	case PROP_POINTS:
		g_value_set_boxed (value, get_points (line));
		break;

	case PROP_FILL_COLOR:
		g_value_take_string (value,
				     g_strdup_printf ("#%02x%02x%02x",
						      line->fill_rgba >> 24,
						      (line->fill_rgba >> 16) & 0xff,
						      (line->fill_rgba >> 8) & 0xff));
		break;

	case PROP_FILL_COLOR_GDK: {
		FooCanvas *canvas = FOO_CANVAS_ITEM (line)->canvas;
		GdkColormap *colormap = gtk_widget_get_colormap (GTK_WIDGET (canvas));
		GdkColor color;

		gdk_colormap_query_color (colormap, line->fill_pixel, &color);
		g_value_set_boxed (value, &color);
		break;
	}
			
	case PROP_FILL_COLOR_RGBA:
		g_value_set_uint (value, line->fill_rgba);
		break;

	case PROP_FILL_STIPPLE:
		g_value_set_object (value, line->stipple);
		break;

	case PROP_WIDTH_PIXELS:
		g_value_set_uint (value, line->width);
		break;
		
	case PROP_WIDTH_UNITS:
		g_value_set_double (value, line->width);
		break;
		
	case PROP_CAP_STYLE:
		g_value_set_enum (value, line->cap);
		break;

	case PROP_JOIN_STYLE:
		g_value_set_enum (value, line->join);
		break;

	case PROP_LINE_STYLE:
		g_value_set_enum (value, line->line_style);
		break;

	case PROP_FIRST_ARROWHEAD:
		g_value_set_boolean (value, line->first_arrow);
		break;

	case PROP_LAST_ARROWHEAD:
		g_value_set_boolean (value, line->last_arrow);
		break;

	case PROP_SMOOTH:
		g_value_set_boolean (value, line->smooth);
		break;

	case PROP_AA:
		g_value_set_boolean (value, line->aa);
		break;

	case PROP_ARROW_SHAPE_A:
		g_value_set_double (value, line->shape_a);
		break;

	case PROP_ARROW_SHAPE_B:
		g_value_set_double (value, line->shape_b);
		break;

	case PROP_ARROW_SHAPE_C:
		g_value_set_double (value, line->shape_c);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
foo_canvas_line_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{
	FooCanvasLine *line;
	double x1, y1, x2, y2;

	line = FOO_CANVAS_LINE (item);

	if (parent_class->update)
		(* parent_class->update) (item, i2w_dx, i2w_dy, flags);

	reconfigure_arrows (line);

	get_bounds_canvas (line, &x1, &y1, &x2, &y2, i2w_dx, i2w_dy);
	foo_canvas_update_bbox (item, x1, y1, x2, y2);
}

static void
foo_canvas_line_realize (FooCanvasItem *item)
{
	FooCanvasLine *line;

	line = FOO_CANVAS_LINE (item);

	if (parent_class->realize)
		(* parent_class->realize) (item);
}

static void
foo_canvas_line_unrealize (FooCanvasItem *item)
{
	FooCanvasLine *line;

	line = FOO_CANVAS_LINE (item);

	if (parent_class->unrealize)
		(* parent_class->unrealize) (item);
}

static void
item_to_canvas (FooCanvas *canvas, double *item_coords, GdkPoint *canvas_coords, int num_points,
		int *num_drawn_points, double i2w_dx, double i2w_dy)
{
	int i;
	int old_cx, old_cy;
	int cx, cy;

	/* the first point is always drawn */
	foo_canvas_w2c (canvas,
			  item_coords[0] + i2w_dx,
			  item_coords[1] + i2w_dy,
			  &canvas_coords->x, &canvas_coords->y);
	old_cx = canvas_coords->x;
	old_cy = canvas_coords->y;
	canvas_coords++;
	*num_drawn_points = 1;

	for (i = 1; i < num_points; i++) {
		foo_canvas_w2c (canvas,
				  item_coords[i*2] + i2w_dx,
				  item_coords[i*2+1] + i2w_dy,
				  &cx, &cy);
		if (old_cx != cx || old_cy != cy) {
			canvas_coords->x = cx;
			canvas_coords->y = cy;
			old_cx = cx;
			old_cy = cy;
			canvas_coords++;
			(*num_drawn_points)++;
		}
	}
}

static void
foo_canvas_line_draw (FooCanvasItem *item, GdkDrawable *drawable,
			GdkEventExpose *event)
{
	gint i;
	cairo_t *cr;
	FooCanvasLine *line;
	GdkPoint static_points[NUM_STATIC_POINTS];
	GdkPoint *points;
	int actual_num_points_drawn;
	double i2w_dx, i2w_dy;
	double width;
	
	line = FOO_CANVAS_LINE (item);

	if (line->num_points == 0)
		return;

	/* Build array of canvas pixel coordinates */

	if (line->num_points <= NUM_STATIC_POINTS)
		points = static_points;
	else
		points = g_new (GdkPoint, line->num_points);

	i2w_dx = 0.0;
	i2w_dy = 0.0;
	foo_canvas_item_i2w (item, &i2w_dx, &i2w_dy);
 
	item_to_canvas (item->canvas, line->coords, points, line->num_points,
			&actual_num_points_drawn, i2w_dx, i2w_dy);

	if (line->width_pixels)
		width = line->width;
	else
		width = line->width * line->item.canvas->pixels_per_unit;

	cr = gdk_cairo_create (drawable);
	cairo_set_line_width (cr, width);
	cairo_set_line_cap (cr, GDK_TO_CAIRO_LINE_CAP (line->cap));
	cairo_set_line_join (cr, GDK_TO_CAIRO_LINE_JOIN (line->join));
	/* FIXME: Set line dash */
	cairo_set_source_rgba (cr,
	                       ((double) ((line->fill_rgba & 0xff000000) >> 24)) / 255,
						   ((double) ((line->fill_rgba & 0xff0000) >> 16)) / 255,
						   ((double) ((line->fill_rgba & 0xff00) >> 8)) / 255,
						   ((double) ((line->fill_rgba & 0xff))) / 255);
	if (!line->aa)
		cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);

    cairo_move_to (cr, points[0].x, points[0].y);
	i = 1;
	if (line->smooth)
	{
		for (; (i+2) < actual_num_points_drawn; i += 3)
		{
			cairo_curve_to (cr,
			                points[i].x, points[i].y,
			                points[i+1].x, points[i+1].y,
			                points[i+2].x, points[i+2].y);
		}
	}
	for (; i < actual_num_points_drawn; i++)
		cairo_line_to (cr, points[i].x, points[i].y);
	
	cairo_stroke (cr);

	if (points != static_points)
		g_free (points);

	/* Draw arrowheads */

	points = static_points;

	if (line->first_arrow) {
		item_to_canvas (item->canvas, line->first_coords, points, NUM_ARROW_POINTS,
				&actual_num_points_drawn, i2w_dx, i2w_dy);
		cairo_move_to (cr, points[0].x, points[0].y);
		for (i = 1; i < actual_num_points_drawn - 1; i++)
			cairo_line_to (cr, points[i].x, points[i].y);
		cairo_close_path (cr);
		cairo_fill (cr);
	}

	if (line->last_arrow) {
		item_to_canvas (item->canvas, line->last_coords, points, NUM_ARROW_POINTS,
				&actual_num_points_drawn, i2w_dx, i2w_dy);
		cairo_move_to (cr, points[0].x, points[0].y);
		for (i = 1; i < actual_num_points_drawn - 1; i++)
			cairo_line_to (cr, points[i].x, points[i].y);
		cairo_close_path (cr);
		cairo_fill (cr);
	}
	cairo_destroy (cr);
}

static double
foo_canvas_line_point (FooCanvasItem *item, double x, double y,
			 int cx, int cy, FooCanvasItem **actual_item)
{
	FooCanvasLine *line;
	double *line_points = NULL, *coords;
	double static_points[2 * NUM_STATIC_POINTS];
	double poly[10];
	double best, dist;
	double dx, dy;
	double width;
	int num_points = 0, i;
	int changed_miter_to_bevel;

#ifdef VERBOSE
	g_print ("foo_canvas_line_point x, y = (%g, %g); cx, cy = (%d, %d)\n", x, y, cx, cy);
#endif

	line = FOO_CANVAS_LINE (item);

	*actual_item = item;

	best = 1.0e36;

	/* Handle smoothed lines by generating an expanded set ot points */

	if (line->smooth && (line->num_points > 2)) {
		/* FIXME */
	} else {
		num_points = line->num_points;
		line_points = line->coords;
	}

	/* Compute a polygon for each edge of the line and test the point against it.  The effective
	 * width of the line is adjusted so that it will be at least one pixel thick (so that zero
	 * pixel-wide lines can be pickedup as well).
	 */

	if (line->width_pixels)
		width = line->width / item->canvas->pixels_per_unit;
	else
		width = line->width;

	if (width < (1.0 / item->canvas->pixels_per_unit))
		width = 1.0 / item->canvas->pixels_per_unit;

	changed_miter_to_bevel = 0;

	for (i = num_points, coords = line_points; i >= 2; i--, coords += 2) {
		/* If rounding is done around the first point, then compute distance between the
		 * point and the first point.
		 */

		if (((line->cap == GDK_CAP_ROUND) && (i == num_points))
		    || ((line->join == GDK_JOIN_ROUND) && (i != num_points))) {
			dx = coords[0] - x;
			dy = coords[1] - y;
			dist = sqrt (dx * dx + dy * dy) - width / 2.0;
			if (dist < FOO_CANVAS_EPSILON) {
				best = 0.0;
				goto done;
			} else if (dist < best)
				best = dist;
		}

		/* Compute the polygonal shape corresponding to this edge, with two points for the
		 * first point of the edge and two points for the last point of the edge.
		 */

		if (i == num_points)
			foo_canvas_get_butt_points (coords[2], coords[3], coords[0], coords[1],
						      width, (line->cap == GDK_CAP_PROJECTING),
						      poly, poly + 1, poly + 2, poly + 3);
		else if ((line->join == GDK_JOIN_MITER) && !changed_miter_to_bevel) {
			poly[0] = poly[6];
			poly[1] = poly[7];
			poly[2] = poly[4];
			poly[3] = poly[5];
		} else {
			foo_canvas_get_butt_points (coords[2], coords[3], coords[0], coords[1],
						      width, FALSE,
						      poly, poly + 1, poly + 2, poly + 3);

			/* If this line uses beveled joints, then check the distance to a polygon
			 * comprising the last two points of the previous polygon and the first two
			 * from this polygon; this checks the wedges that fill the mitered point.
			 */

			if ((line->join == GDK_JOIN_BEVEL) || changed_miter_to_bevel) {
				poly[8] = poly[0];
				poly[9] = poly[1];

				dist = foo_canvas_polygon_to_point (poly, 5, x, y);
				if (dist < FOO_CANVAS_EPSILON) {
					best = 0.0;
					goto done;
				} else if (dist < best)
					best = dist;

				changed_miter_to_bevel = FALSE;
			}
		}

		if (i == 2)
			foo_canvas_get_butt_points (coords[0], coords[1], coords[2], coords[3],
						      width, (line->cap == GDK_CAP_PROJECTING),
						      poly + 4, poly + 5, poly + 6, poly + 7);
		else if (line->join == GDK_JOIN_MITER) {
			if (!foo_canvas_get_miter_points (coords[0], coords[1],
							    coords[2], coords[3],
							    coords[4], coords[5],
							    width,
							    poly + 4, poly + 5, poly + 6, poly + 7)) {
				changed_miter_to_bevel = TRUE;
				foo_canvas_get_butt_points (coords[0], coords[1], coords[2], coords[3],
							      width, FALSE,
							      poly + 4, poly + 5, poly + 6, poly + 7);
			}
		} else
			foo_canvas_get_butt_points (coords[0], coords[1], coords[2], coords[3],
						      width, FALSE,
						      poly + 4, poly + 5, poly + 6, poly + 7);

		poly[8] = poly[0];
		poly[9] = poly[1];

		dist = foo_canvas_polygon_to_point (poly, 5, x, y);
		if (dist < FOO_CANVAS_EPSILON) {
			best = 0.0;
			goto done;
		} else if (dist < best)
			best = dist;
	}

	/* If caps are rounded, check the distance to the cap around the final end point of the line */

	if (line->cap == GDK_CAP_ROUND) {
		dx = coords[0] - x;
		dy = coords[1] - y;
		dist = sqrt (dx * dx + dy * dy) - width / 2.0;
		if (dist < FOO_CANVAS_EPSILON) {
			best = 0.0;
			goto done;
		} else
			best = dist;
	}

	/* sometimes the FooCanvasItem::update signal will not have
           been processed between deleting the arrow points and a call
           to this routine -- this can cause a segfault here */
	if ((line->first_arrow && !line->first_coords) ||
	    (line->last_arrow && !line->last_coords))
		reconfigure_arrows(line);

	/* If there are arrowheads, check the distance to them */

	if (line->first_arrow) {
		dist = foo_canvas_polygon_to_point (line->first_coords, NUM_ARROW_POINTS, x, y);
		if (dist < FOO_CANVAS_EPSILON) {
			best = 0.0;
			goto done;
		} else
			best = dist;
	}

	if (line->last_arrow) {
		dist = foo_canvas_polygon_to_point (line->last_coords, NUM_ARROW_POINTS, x, y);
		if (dist < FOO_CANVAS_EPSILON) {
			best = 0.0;
			goto done;
		} else
			best = dist;
	}

done:

	if ((line_points != static_points) && (line_points != line->coords))
		g_free (line_points);

	return best;
}

static void
foo_canvas_line_translate (FooCanvasItem *item, double dx, double dy)
{
        FooCanvasLine *line;
        int i;
        double *coords;

        line = FOO_CANVAS_LINE (item);

        for (i = 0, coords = line->coords; i < line->num_points; i++, coords += 2) {
                coords[0] += dx;
                coords[1] += dy;
        }

        if (line->first_arrow)
                for (i = 0, coords = line->first_coords; i < NUM_ARROW_POINTS; i++, coords += 2) {
                        coords[0] += dx;
                        coords[1] += dy;
                }

        if (line->last_arrow)
                for (i = 0, coords = line->last_coords; i < NUM_ARROW_POINTS; i++, coords += 2) {
                        coords[0] += dx;
                        coords[1] += dy;
                }
}

static void
foo_canvas_line_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	FooCanvasLine *line;

	line = FOO_CANVAS_LINE (item);

	if (line->num_points == 0) {
		*x1 = *y1 = *x2 = *y2 = 0.0;
		return;
	}

	get_bounds (line, x1, y1, x2, y2);
}
