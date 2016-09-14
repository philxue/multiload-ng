/*
 * Copyright (C) 2016 Mario Cianciolo <mr.udda@gmail.com>
 *
 * This file is part of multiload-ng.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#ifndef __GRAPH_DATA_H__
#define __GRAPH_DATA_H__

#include "autoscaler.h"


G_BEGIN_DECLS

/* Requirements for filter separators: must characters/sequences that never appear in
 * filter elements. As filter elements by now are all files in /sys or /dev, comma
 * colon, newline, etc are good separators. To play safe, longer sequences can be used. */
// separator for filter returned by multiload_graph_*_get_filter() to use in program
#define MULTILOAD_FILTER_SEPARATOR "\n"
// separator for filter saved on settings. Must not contain newlines or invalid UTF-8 sequences.
#define MULTILOAD_FILTER_SEPARATOR_INLINE ","


typedef struct _CpuData {
	guint64 last [5];

	gfloat user;
	gfloat nice;
	gfloat system;
	gfloat iowait;
	gfloat total_use;
	gdouble uptime;

	gulong num_cpu;
	gchar cpu0_name[64];
	// use oversized buffers (just to be sure)
	gchar cpu0_governor[32];
	double cpu0_mhz;
} CpuData;

typedef struct _MemoryData {
	guint64 user;
	guint64 shared;
	guint64 buffers;
	guint64 cache;
	guint64 total;
} MemoryData;

typedef struct _NetData {
	guint64 last [3];
	AutoScaler scaler;

	guint64 in_speed;
	guint64 out_speed;
	guint64 local_speed;

	gchar ifaces[64];
} NetData;

typedef struct _SwapData {
	guint64 used;
	guint64 total;
} SwapData;

typedef struct _LoadData {
	double loadavg[3];

	AutoScaler scaler;

	guint proc_active;
	guint proc_count;
	// use oversized buffers (just to be sure)
	gchar uname[512];
} LoadData;

typedef struct _DiskData {
	guint64 last_read;
	guint64 last_write;
	AutoScaler scaler;

	guint64 read_speed;
	guint64 write_speed;

	gchar partitions[128];
} DiskData;

typedef struct _TemperatureData {
	gchar name[30];
	double value;
	double max;
	AutoScaler scaler;
} TemperatureData;

typedef struct _ParametricData {
	gchar command[512];
	gchar message[512];
	gboolean error;
	gdouble result[4];
	int nvalues;
	AutoScaler scaler;
} ParametricData;


G_GNUC_INTERNAL void
multiload_graph_cpu_get_data (int Maximum, int data [5], LoadGraph *g, CpuData *xd);
G_GNUC_INTERNAL void
multiload_graph_cpu_cmdline_output (LoadGraph *g, CpuData *xd);
G_GNUC_INTERNAL void
multiload_graph_cpu_tooltip_update (char **title, char **text, LoadGraph *g, CpuData *xd);

G_GNUC_INTERNAL void
multiload_graph_mem_get_data (int Maximum, int data [4], LoadGraph *g, MemoryData *xd);
G_GNUC_INTERNAL void
multiload_graph_mem_cmdline_output (LoadGraph *g, MemoryData *xd);
G_GNUC_INTERNAL void
multiload_graph_mem_tooltip_update (char **title, char **text, LoadGraph *g, MemoryData *xd);

G_GNUC_INTERNAL gchar *
multiload_graph_net_get_filter (LoadGraph *g, NetData *xd);
G_GNUC_INTERNAL void
multiload_graph_net_get_data (int Maximum, int data [4], LoadGraph *g, NetData *xd);
G_GNUC_INTERNAL void
multiload_graph_net_cmdline_output (LoadGraph *g, NetData *xd);
G_GNUC_INTERNAL void
multiload_graph_net_tooltip_update (char **title, char **text, LoadGraph *g, NetData *xd);

G_GNUC_INTERNAL void
multiload_graph_swap_get_data (int Maximum, int data [2], LoadGraph *g, SwapData *xd);
G_GNUC_INTERNAL void
multiload_graph_swap_cmdline_output (LoadGraph *g, SwapData *xd);
G_GNUC_INTERNAL void
multiload_graph_swap_tooltip_update (char **title, char **text, LoadGraph *g, SwapData *xd);

G_GNUC_INTERNAL void
multiload_graph_load_get_data (int Maximum, int data [2], LoadGraph *g, LoadData *xd);
G_GNUC_INTERNAL void
multiload_graph_load_cmdline_output (LoadGraph *g, LoadData *xd);
G_GNUC_INTERNAL void
multiload_graph_load_tooltip_update (char **title, char **text, LoadGraph *g, LoadData *xd);

G_GNUC_INTERNAL gchar *
multiload_graph_disk_get_filter (LoadGraph *g, DiskData *xd);
G_GNUC_INTERNAL void
multiload_graph_disk_get_data (int Maximum, int data [3], LoadGraph *g, DiskData *xd);
G_GNUC_INTERNAL void
multiload_graph_disk_cmdline_output (LoadGraph *g, DiskData *xd);
G_GNUC_INTERNAL void
multiload_graph_disk_tooltip_update (char **title, char **text, LoadGraph *g, DiskData *xd);

G_GNUC_INTERNAL gchar *
multiload_graph_temp_get_filter (LoadGraph *g, TemperatureData *xd);
G_GNUC_INTERNAL void
multiload_graph_temp_get_data (int Maximum, int data [2], LoadGraph *g, TemperatureData *xd);
G_GNUC_INTERNAL void
multiload_graph_temp_cmdline_output (LoadGraph *g, TemperatureData *xd);
G_GNUC_INTERNAL void
multiload_graph_temp_tooltip_update (char **title, char **text, LoadGraph *g, TemperatureData *xd);

G_GNUC_INTERNAL void
multiload_graph_parm_get_data (int Maximum, int data[1], LoadGraph *g, ParametricData *xd);
G_GNUC_INTERNAL void
multiload_graph_parm_cmdline_output (LoadGraph *g, ParametricData *xd);
G_GNUC_INTERNAL void
multiload_graph_parm_tooltip_update (char **title, char **text, LoadGraph *g, ParametricData *xd);

G_END_DECLS

#endif /* __GRAPH_DATA_H__ */
