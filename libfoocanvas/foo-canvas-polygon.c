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
/* Polygon item type for FooCanvas widget
 *
 * FooCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include "libfoocanvas.h"


#define NUM_STATIC_POINTS 256	/* Number of static points to use to avoid allocating arrays */


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
	PROP_OUTLINE_COLOR,
	PROP_OUTLINE_COLOR_GDK,
	PROP_OUTLINE_COLOR_RGBA,
	PROP_FILL_STIPPLE,
	PROP_OUTLINE_STIPPLE,
	PROP_WIDTH_PIXELS,
	PROP_WIDTH_UNITS,
	PROP_AA
};


static void foo_canvas_polygon_class_init (FooCanvasPolygonClass *klass);
static void foo_canvas_polygon_init       (FooCanvasPolygon      *poly);
static void foo_canvas_polygon_destroy    (GtkObject               *object);
static void foo_canvas_polygon_set_property (GObject              *object,
					       guint                 param_id,
					       const GValue         *value,
					       GParamSpec           *pspec);
static void foo_canvas_polygon_get_property (GObject              *object,
					       guint                 param_id,
					       GValue               *value,
					       GParamSpec           *pspec);

static void   foo_canvas_polygon_update      (FooCanvasItem *item,
						double i2w_dx, double i2w_dy,
						int flags);
static void   foo_canvas_polygon_draw        (FooCanvasItem *item, GdkDrawable *drawable,
						GdkEventExpose *expose);
static double foo_canvas_polygon_point       (FooCanvasItem *item, double x, double y,
						int cx, int cy, FooCanvasItem **actual_item);
static void   foo_canvas_polygon_translate   (FooCanvasItem *item, double dx, double dy);
static void   foo_canvas_polygon_bounds      (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2);


static FooCanvasItemClass *parent_class;

G_DEFINE_TYPE (FooCanvasPolygon, foo_canvas_polygon, FOO_TYPE_CANVAS_ITEM)

static void
foo_canvas_polygon_class_init (FooCanvasPolygonClass *klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	FooCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) klass;
	object_class = (GtkObjectClass *) klass;
	item_class = (FooCanvasItemClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->set_property = foo_canvas_polygon_set_property;
	gobject_class->get_property = foo_canvas_polygon_get_property;

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
                 PROP_OUTLINE_COLOR,
                 g_param_spec_string ("outline-color", NULL, NULL,
                                      NULL,
                                      G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_OUTLINE_COLOR_GDK,
                 g_param_spec_boxed ("outline-color-gdk", NULL, NULL,
				     GDK_TYPE_COLOR,
				     G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_OUTLINE_COLOR_RGBA,
                 g_param_spec_uint ("outline-color-rgba", NULL, NULL,
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
                 PROP_OUTLINE_STIPPLE,
                 g_param_spec_object ("outline-stipple", NULL, NULL,
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
                 PROP_AA,
                 g_param_spec_boolean ("aa", NULL, NULL,
				       FALSE,
				       G_PARAM_READWRITE));

	object_class->destroy = foo_canvas_polygon_destroy;

	item_class->update = foo_canvas_polygon_update;
	item_class->draw = foo_canvas_polygon_draw;
	item_class->point = foo_canvas_polygon_point;
	item_class->translate = foo_canvas_polygon_translate;
	item_class->bounds = foo_canvas_polygon_bounds;
}

static void
foo_canvas_polygon_init (FooCanvasPolygon *poly)
{
	poly->width = 1.0;
	poly->aa = TRUE;
}

static void
foo_canvas_polygon_destroy (GtkObject *object)
{
	FooCanvasPolygon *poly;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FOO_IS_CANVAS_POLYGON (object));

	poly = FOO_CANVAS_POLYGON (object);

	/* remember, destroy can be run multiple times! */

	if (poly->coords)
		g_free (poly->coords);
	poly->coords = NULL;

	if (poly->fill_stipple)
		g_object_unref (poly->fill_stipple);
	poly->fill_stipple = NULL;

	if (poly->outline_stipple)
		g_object_unref (poly->outline_stipple);
	poly->outline_stipple = NULL;

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/* Computes the bounding box of the polygon.  Assumes that the number of points in the polygon is
 * not zero.
 */
static gboolean
get_bounds (FooCanvasPolygon *poly, double *bx1, double *by1, double *bx2, double *by2)
{
	double *coords;
	double x1, y1, x2, y2;
	double width;
	int i;

	if (poly->num_points == 0)
		return FALSE;

	/* Compute bounds of vertices */

	x1 = x2 = poly->coords[0];
	y1 = y2 = poly->coords[1];

	for (i = 1, coords = poly->coords + 2; i < poly->num_points; i++, coords += 2) {
		GROW_BOUNDS (x1, y1, x2, y2, coords[0], coords[1]);
	}

	/* Add outline width */

	if (poly->width_pixels)
		width = poly->width / poly->item.canvas->pixels_per_unit;
	else
		width = poly->width;

	width /= 2.0;

	x1 -= width;
	y1 -= width;
	x2 += width;
	y2 += width;

	/* Done */

	*bx1 = x1;
	*by1 = y1;
	*bx2 = x2;
	*by2 = y2;
	return TRUE;
}

/* Computes the bounding box of the polygon, in canvas coordinates.  Assumes that the number of points in the polygon is
 * not zero.
 */
static gboolean
get_bounds_canvas (FooCanvasPolygon *poly,
		   double *bx1, double *by1, double *bx2, double *by2,
		   double i2w_dx, double i2w_dy)
{
	FooCanvasItem *item;
	double bbox_x0, bbox_y0, bbox_x1, bbox_y1;

	item = FOO_CANVAS_ITEM (poly);

	if (!get_bounds (poly, &bbox_x0, &bbox_y0, &bbox_x1, &bbox_y1))
		return FALSE;

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
	return TRUE;
}

/* Sets the points of the polygon item to the specified ones.  If needed, it will add a point to
 * close the polygon.
 */
static void
set_points (FooCanvasPolygon *poly, FooCanvasPoints *points)
{
	int duplicate;

	/* See if we need to duplicate the first point */

	duplicate = ((points->coords[0] != points->coords[2 * points->num_points - 2])
		     || (points->coords[1] != points->coords[2 * points->num_points - 1]));

	if (duplicate)
		poly->num_points = points->num_points + 1;
	else
		poly->num_points = points->num_points;

	poly->coords = g_new (double, 2 * poly->num_points);
	memcpy (poly->coords, points->coords, 2 * points->num_points * sizeof (double));

	if (duplicate) {
		poly->coords[2 * poly->num_points - 2] = poly->coords[0];
		poly->coords[2 * poly->num_points - 1] = poly->coords[1];
	}
}

static void
foo_canvas_polygon_set_property (GObject              *object,
				   guint                 param_id,
				   const GValue         *value,
				   GParamSpec           *pspec)
{
	FooCanvasItem *item;
	FooCanvasPolygon *poly;
	FooCanvasPoints *points;
	GdkColor color = { 0, 0, 0, 0, };
	GdkColor *pcolor;
	int have_pixel;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FOO_IS_CANVAS_POLYGON (object));

	item = FOO_CANVAS_ITEM (object);
	poly = FOO_CANVAS_POLYGON (object);
	have_pixel = FALSE;

	switch (param_id) {
	case PROP_POINTS:
		points = g_value_get_boxed (value);

		if (poly->coords) {
			g_free (poly->coords);
			poly->coords = NULL;
		}

		if (!points)
			poly->num_points = 0;
		else
			set_points (poly, points);

		foo_canvas_item_request_update (item);
		break;

        case PROP_FILL_COLOR:
	case PROP_FILL_COLOR_GDK:
	case PROP_FILL_COLOR_RGBA:
		switch (param_id) {
		case PROP_FILL_COLOR:
			if (g_value_get_string (value) &&
			    gdk_color_parse (g_value_get_string (value), &color))
				poly->fill_set = TRUE;
			else
				poly->fill_set = FALSE;

			poly->fill_color = ((color.red & 0xff00) << 16 |
					    (color.green & 0xff00) << 8 |
					    (color.blue & 0xff00) |
					    0xff);
			break;

		case PROP_FILL_COLOR_GDK:
			pcolor = g_value_get_boxed (value);
			poly->fill_set = pcolor != NULL;

			if (pcolor) {
				GdkColormap *colormap;

				color = *pcolor;
				colormap = gtk_widget_get_colormap (GTK_WIDGET (item->canvas));
				gdk_rgb_find_color (colormap, &color);
				have_pixel = TRUE;
			}

			poly->fill_color = ((color.red & 0xff00) << 16 |
					    (color.green & 0xff00) << 8 |
					    (color.blue & 0xff00) |
					    0xff);
			break;

		case PROP_FILL_COLOR_RGBA:
			poly->fill_set = TRUE;
			poly->fill_color = g_value_get_uint (value);
			break;
		}
#ifdef VERBOSE
		g_print ("poly fill color = %08x\n", poly->fill_color);
#endif
		if (have_pixel)
			poly->fill_pixel = color.pixel;
		else
			poly->fill_pixel = foo_canvas_get_color_pixel (item->canvas,
									 poly->fill_color);

		foo_canvas_item_request_redraw (item);		
		break;

        case PROP_OUTLINE_COLOR:
	case PROP_OUTLINE_COLOR_GDK:
	case PROP_OUTLINE_COLOR_RGBA:
		switch (param_id) {
		case PROP_OUTLINE_COLOR:
			if (g_value_get_string (value) &&
			    gdk_color_parse (g_value_get_string (value), &color))
				poly->outline_set = TRUE;
			else
				poly->outline_set = FALSE;

			poly->outline_color = ((color.red & 0xff00) << 16 |
					       (color.green & 0xff00) << 8 |
					       (color.blue & 0xff00) |
					       0xff);
			break;

		case PROP_OUTLINE_COLOR_GDK:
			pcolor = g_value_get_boxed (value);
			poly->outline_set = pcolor != NULL;

			if (pcolor) {
				GdkColormap *colormap;

				color = *pcolor;
				colormap = gtk_widget_get_colormap (GTK_WIDGET (item->canvas));
				gdk_rgb_find_color (colormap, &color);
				have_pixel = TRUE;
			}

			poly->outline_color = ((color.red & 0xff00) << 16 |
					       (color.green & 0xff00) << 8 |
					       (color.blue & 0xff00) |
					       0xff);
			break;

		case PROP_OUTLINE_COLOR_RGBA:
			poly->outline_set = TRUE;
			poly->outline_color = g_value_get_uint (value);
			break;
		}
#ifdef VERBOSE
		g_print ("poly outline color = %08x\n", poly->outline_color);
#endif
		if (have_pixel)
			poly->outline_pixel = color.pixel;
		else
			poly->outline_pixel = foo_canvas_get_color_pixel (item->canvas,
									    poly->outline_color);

		foo_canvas_item_request_redraw (item);		
		break;

	case PROP_FILL_STIPPLE:
		if (poly->fill_stipple)
			g_object_unref (poly->fill_stipple);
		poly->fill_stipple = (GdkBitmap *) g_value_get_object (value);
		g_object_ref (poly->fill_stipple);
		foo_canvas_item_request_update (item);
		break;

	case PROP_OUTLINE_STIPPLE:
		if (poly->outline_stipple)
			g_object_unref (poly->outline_stipple);
		poly->outline_stipple = (GdkBitmap *) g_value_get_object (value);
		g_object_ref (poly->outline_stipple);
		foo_canvas_item_request_update (item);
		break;

	case PROP_WIDTH_PIXELS:
		poly->width = g_value_get_uint (value);
		poly->width_pixels = TRUE;
#ifdef OLD_XFORM
		recalc_bounds (poly);
#else
		foo_canvas_item_request_update (item);
#endif
		break;

	case PROP_WIDTH_UNITS:
		poly->width = fabs (g_value_get_double (value));
		poly->width_pixels = FALSE;
#ifdef OLD_XFORM
		recalc_bounds (poly);
#else
		foo_canvas_item_request_update (item);
#endif
		break;
	case PROP_AA:
		poly->aa = g_value_get_boolean (value);
		foo_canvas_item_request_update (item);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

/* Allocates a GdkColor structure filled with the specified pixel, and puts it into the specified
 * value for returning it in the get_property method.
 */
static void
get_color_value (FooCanvasPolygon *poly, gulong pixel, GValue *value)
{
	GdkColor *color;
	GdkColormap *colormap;

	color = g_new (GdkColor, 1);
	color->pixel = pixel;

	colormap = gtk_widget_get_colormap (GTK_WIDGET (poly));
	gdk_rgb_find_color (colormap, color);
	g_value_set_boxed (value, color);
}

static void
foo_canvas_polygon_get_property (GObject              *object,
				   guint                 param_id,
				   GValue               *value,
				   GParamSpec           *pspec)
{
	FooCanvasPolygon *poly;
	FooCanvasPoints *points;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FOO_IS_CANVAS_POLYGON (object));

	poly = FOO_CANVAS_POLYGON (object);

	switch (param_id) {
	case PROP_POINTS:
		if (poly->num_points != 0) {
			points = foo_canvas_points_new (poly->num_points);
			memcpy (points->coords, poly->coords, 2 * poly->num_points * sizeof (double));
			g_value_set_boxed (value, points);
		} else
			g_value_set_boxed (value, NULL);
		break;

	case PROP_FILL_COLOR_GDK:
		get_color_value (poly, poly->fill_pixel, value);
		break;

	case PROP_OUTLINE_COLOR_GDK:
		get_color_value (poly, poly->outline_pixel, value);
		break;

	case PROP_FILL_COLOR_RGBA:
		g_value_set_uint (value, poly->fill_color);
		break;

	case PROP_OUTLINE_COLOR_RGBA:
		g_value_set_uint (value, poly->outline_color);
		break;

	case PROP_FILL_STIPPLE:
		g_value_set_object (value, (GObject *) poly->fill_stipple);
		break;

	case PROP_OUTLINE_STIPPLE:
		g_value_set_object (value, (GObject *) poly->outline_stipple);
		break;

	case PROP_AA:
		g_value_set_boolean (value, poly->aa);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
foo_canvas_polygon_update (FooCanvasItem *item,
			     double i2w_dx, double i2w_dy,
			     int flags)
{
	FooCanvasPolygon *poly;
	double x1, y1, x2, y2;

	poly = FOO_CANVAS_POLYGON (item);

	if (parent_class->update)
		(* parent_class->update) (item, i2w_dx, i2w_dy, flags);

	if (get_bounds_canvas (poly, &x1, &y1, &x2, &y2, i2w_dx, i2w_dy))
		foo_canvas_update_bbox (item, x1, y1, x2, y2);
}

/* Converts an array of world coordinates into an array of canvas pixel coordinates.  Takes in the
 * item->world deltas and the drawable deltas.
 */
static void
item_to_canvas (FooCanvas *canvas, double *item_coords, GdkPoint *canvas_coords, int num_points,
		double i2w_dx, double i2w_dy)
{
	int i;

	for (i = 0; i < num_points; i++) {
		foo_canvas_w2c (canvas,
				  item_coords[i*2] + i2w_dx,
				  item_coords[i*2+1] + i2w_dy,
				  &canvas_coords->x, &canvas_coords->y);
		canvas_coords++;
	}
}

static void
foo_canvas_polygon_draw (FooCanvasItem *item, GdkDrawable *drawable,
			   GdkEventExpose *expose)
{
	cairo_t *cr;
	FooCanvasPolygon *poly;
	GdkPoint static_points[NUM_STATIC_POINTS];
	GdkPoint *points;
	double i2w_dx, i2w_dy;
	double width;
	gint i;
	
	poly = FOO_CANVAS_POLYGON (item);

	if (poly->num_points == 0)
		return;

	/* Build array of canvas pixel coordinates */

	if (poly->num_points <= NUM_STATIC_POINTS)
		points = static_points;
	else
		points = g_new (GdkPoint, poly->num_points);

	i2w_dx = 0.0;
	i2w_dy = 0.0;
	foo_canvas_item_i2w (item, &i2w_dx, &i2w_dy);

	item_to_canvas (item->canvas,
			poly->coords, points, poly->num_points,
			i2w_dx, i2w_dy);

	if (poly->width_pixels)
		width = poly->width;
	else
		width = poly->width * poly->item.canvas->pixels_per_unit;

	cr = gdk_cairo_create (drawable);
	if (!poly->aa)
		cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	cairo_set_line_width (cr, width);

	cairo_move_to (cr, points[0].x, points[0].y);
	for(i = 1; i < poly->num_points - 1; i++)
	{
		cairo_line_to (cr, points[i].x, points[i].y);
	}
	cairo_close_path (cr);
	
	if (poly->fill_set) {
		/* FIXME: Set stripple */
		cairo_set_source_rgba (cr,
		                       ((double) ((poly->fill_color & 0xff000000) >> 24)) / 255,
		                       ((double) ((poly->fill_color & 0xff0000) >> 16)) / 255,
		                       ((double) ((poly->fill_color & 0xff00) >> 8)) / 255,
		                       ((double) ((poly->fill_color & 0xff))) / 255);
		cairo_fill_preserve (cr);
	}

	if (poly->outline_set) {
		/* FIXME: Set stripple */
		cairo_set_source_rgba (cr,
		                       ((double) ((poly->outline_color & 0xff000000) >> 24)) / 255,
		                       ((double) ((poly->outline_color & 0xff0000) >> 16)) / 255,
		                       ((double) ((poly->outline_color & 0xff00) >> 8)) / 255,
		                       ((double) ((poly->outline_color & 0xff))) / 255);
		cairo_stroke (cr);
	}

	/* Done */

	if (points != static_points)
		g_free (points);
}

static double
foo_canvas_polygon_point (FooCanvasItem *item, double x, double y,
			    int cx, int cy, FooCanvasItem **actual_item)
{
	FooCanvasPolygon *poly;
	double dist;
	double width;

	poly = FOO_CANVAS_POLYGON (item);

	*actual_item = item;

	dist = foo_canvas_polygon_to_point (poly->coords, poly->num_points, x, y);

	if (poly->outline_set) {
		if (poly->width_pixels)
			width = poly->width / item->canvas->pixels_per_unit;
		else
			width = poly->width;

		dist -= width / 2.0;

		if (dist < 0.0)
			dist = 0.0;
	}

	return dist;
}

static void
foo_canvas_polygon_translate (FooCanvasItem *item, double dx, double dy)
{
        FooCanvasPolygon *poly;
        int i;
        double *coords;

        poly = FOO_CANVAS_POLYGON (item);

        for (i = 0, coords = poly->coords; i < poly->num_points; i++, coords += 2) {
                coords[0] += dx;
                coords[1] += dy;
        }

}

static void
foo_canvas_polygon_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	FooCanvasPolygon *poly;

	g_return_if_fail (item != NULL);
	g_return_if_fail (FOO_IS_CANVAS_POLYGON (item));

	poly = FOO_CANVAS_POLYGON (item);

	if (poly->num_points == 0) {
		*x1 = *y1 = *x2 = *y2 = 0.0;
		return;
	}

	get_bounds (poly, x1, y1, x2, y2);
}
