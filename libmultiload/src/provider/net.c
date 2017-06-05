/*
 * Copyright (C) 2017 Mario Cianciolo <mr.udda@gmail.com>
 *
 * This file is part of Multiload-ng.
 *
 * Multiload-ng is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Multiload-ng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Multiload-ng.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <multiload.h>


const MlGraphTypeInterface ML_PROVIDER_NET_IFACE = {
	.name			= "net",
	.label			= N_("Network"),
	.description	= N_("Shows network I/O, split by direction (input/output/local)."),

	.hue			= 53,

	.n_data			= 3,
	.dataset_mode	= ML_DATASET_MODE_ABSOLUTE,

	.init_fn		= ml_provider_net_init,
	.config_fn		= ml_provider_net_config,
	.get_fn			= ml_provider_net_get,
	.destroy_fn		= ml_provider_net_destroy,
	.sizeof_fn		= ml_provider_net_sizeof,
	.unpause_fn		= ml_provider_net_unpause,
	.caption_fn		= ml_provider_net_caption,

	.helptext		= N_(
		"Local traffic includes I/O generated by loopback devices of any type.<br />"
		"For example, ADB (Android Debug Bridge) traffic will be counted as local."
	)
};


enum { NET_IN, NET_OUT, NET_LOCAL, NET_MAX };

typedef struct {
	MlAssocArray *aa_if; // caches MlNetInfo structures (key are the iface names)
	uint64_t last [NET_MAX];

	bool do_count [NET_MAX];
	bool use_packets; // count packets instead of bytes
	char **filter;

	MlGrowBuffer *used_ifaces;
} NETstate;


mlPointer
ml_provider_net_init (MlConfig *config)
{
	NETstate *s = ml_new (NETstate);

	s->aa_if = ml_assoc_array_new (5, (MlDestroyFunc)ml_net_info_destroy);
	s->used_ifaces = ml_grow_buffer_new (32);
	s->do_count [NET_IN] = true;
	s->do_count [NET_OUT] = true;
	s->do_count [NET_LOCAL] = true;
	s->use_packets = false;
	s->filter = NULL;

	ml_config_add_entry (config,
		"count_in",
		ML_VALUE_TYPE_BOOLEAN,
		&s->do_count[NET_IN],
		_("Count inbound traffic"),
		_("Draw cumulative inbound traffic. Enter <b>false</b> to disable.")
	);
	ml_config_add_entry (config,
		"count_out",
		ML_VALUE_TYPE_BOOLEAN,
		&s->do_count[NET_OUT],
		_("Count outbound traffic"),
		_("Draw cumulative outbound traffic. Enter <b>false</b> to disable.")
	);
	ml_config_add_entry (config,
		"count_local",
		ML_VALUE_TYPE_BOOLEAN,
		&s->do_count[NET_LOCAL],
		_("Count local traffic"),
		_("Draw cumulative local traffic. This includes traffic generated by local servers, and by some toolkits like ADB. Enter <b>false</b> to disable.")
	);
	ml_config_add_entry (config,
		"use_packets",
		ML_VALUE_TYPE_BOOLEAN,
		&s->use_packets,
		_("Use packets"),
		_("Calculate values using packets instead of bytes.")
	);
	ml_config_add_entry (config,
		"filter",
		ML_VALUE_TYPE_STRV,
		&s->filter,
		_("Filter"),
		_("Devices that aren't in this list will not be used. If this list is empty, all available devices will be used.")
	);

	return s;
}

void
ml_provider_net_config (MlGraphContext *context)
{
	// every config change invalidates previous data
	ml_graph_context_set_need_data_reset (context);
}

void
ml_provider_net_get (MlGraphContext *context)
{
	NETstate *s = (NETstate*)ml_graph_context_get_provider_data (context);
	ml_graph_context_assert (context, s != NULL);

	MlGrowArray *ifaces = ml_net_info_list_usable_ifaces (s->aa_if);
	// discriminate between empty array and no array at all (useful for debugging)
	ml_graph_context_assert_with_message (context, ifaces != NULL, _("Cannot find any usable network interface"));
	ml_graph_context_assert_with_message (context, !ml_grow_array_is_empty (ifaces), _("Cannot find any usable network interface"));

	uint64_t rate[NET_MAX] = {0, 0, 0};
	uint32_t diff[NET_MAX] = {0, 0, 0};

	ml_grow_buffer_rewind (s->used_ifaces);

	ml_grow_array_for (ifaces, i) {
		MlNetInfo *ni = (MlNetInfo*)ml_grow_array_get (ifaces, i);
		if_unlikely (ni == NULL)
			break;

		bool ignore = false;

		// find devices with same HW address (e.g. ifaces put in monitor mode from airmon-ng)
		for (int j = 0; j < i; j++) {
			MlNetInfo *d_tmp = (MlNetInfo*)ml_grow_array_get (ifaces, j);
			if (ml_string_equals (d_tmp->address, ni->address, true)) {
				ignore = true;
				break;
			}
		}
		if (ignore)
			continue;

		// skip ifaces not present in filter
		if (s->filter != NULL && !ml_strv_contains (s->filter, ni->name))
			continue;

		// build list of used interfaces
		ml_grow_buffer_append_string (s->used_ifaces, ni->name);
		if (!ml_grow_array_is_last_index(ifaces, i))
			ml_grow_buffer_append_string (s->used_ifaces, ", ");


		if (ni->is_loopback) {
			if (s->do_count[NET_LOCAL])
				rate[NET_LOCAL]	+= (s->use_packets) ? ni->pk_read  : ni->byte_read;
		} else {
			if (s->do_count[NET_IN])
				rate[NET_IN]	+= (s->use_packets) ? ni->pk_read  : ni->byte_read;

			if (s->do_count[NET_OUT])
				rate[NET_OUT]	+= (s->use_packets) ? ni->pk_write : ni->byte_write;
		}
	}


	if_unlikely (!ml_graph_context_is_first_call (context)) { // cannot calculate diff on first call
		for (int i = 0; i < NET_MAX; i++) {
			if (s->last[i] > rate[i])
				break;

			diff[i] = (uint32_t)(rate[i]-s->last[i]);
		}
	}

	memcpy (s->last, rate, sizeof(s->last));

	ml_grow_array_destroy (ifaces, true, false);

	ml_graph_context_set_data (context, NET_IN,    diff[NET_IN]);
	ml_graph_context_set_data (context, NET_OUT,   diff[NET_OUT]);
	ml_graph_context_set_data (context, NET_LOCAL, diff[NET_LOCAL]);
	ml_graph_context_set_max  (context, 100); // min ceiling = 100 bps
}

void
ml_provider_net_destroy (mlPointer provider_data)
{
	NETstate *s = (NETstate*)provider_data;

	if_likely (s != NULL) {
		ml_strv_free (s->filter);
		ml_assoc_array_destroy (s->aa_if);
		ml_grow_buffer_destroy (s->used_ifaces, true);
		free (s);
	}
}

size_t
ml_provider_net_sizeof (mlPointer provider_data)
{
	NETstate *s = (NETstate*)provider_data;
	if_unlikely (s == NULL)
		return 0;

	size_t size = sizeof (NETstate);

	size += ml_assoc_array_sizeof (s->aa_if, sizeof (MlNetInfo));
	size += ml_strv_sizeof (s->filter);
	size += ml_grow_buffer_sizeof (s->used_ifaces);

	return size;
}

void
ml_provider_net_unpause (MlGraphContext *context)
{
	ml_graph_context_set_first_call (context); // avoid spikes after unpause
}

void
ml_provider_net_caption (MlCaption *caption, MlDataset *ds, mlPointer provider_data)
{
	NETstate *s = (NETstate*)provider_data;
	if_unlikely (s == NULL)
		return;

	uint32_t diff_in = ml_dataset_get_value (ds, -1, NET_IN);
	uint32_t diff_out = ml_dataset_get_value (ds, -1, NET_OUT);
	uint32_t diff_local = ml_dataset_get_value (ds, -1, NET_LOCAL);

	char buf[24];

	if (s->do_count[NET_IN]) {
		if (s->use_packets) {
			// TRANSLATORS: network packets per seconds
			snprintf (buf, sizeof(buf), _("%u packets/s"), diff_in);
		} else {
			ml_string_format_size_s (diff_in, "Bps", false, buf, sizeof (buf));
		}

		// TRANSLATORS: network speed
		ml_caption_set (caption, ML_CAPTION_COMPONENT_BODY, _("Inbound traffic: %s"), buf);
	} else {
		// TRANSLATORS: network speed measuring for inbound traffic is not active
		ml_caption_set (caption, ML_CAPTION_COMPONENT_BODY, _("Inbound traffic: not measuring"));
	}

	ml_caption_append (caption, ML_CAPTION_COMPONENT_BODY, "\n");

	if (s->do_count[NET_OUT]) {
		if (s->use_packets) {
			// TRANSLATORS: network packets per seconds
			snprintf (buf, sizeof(buf), _("%u packets/s"), diff_out);
		} else {
			ml_string_format_size_s (diff_out, "Bps", false, buf, sizeof (buf));
		}

		// TRANSLATORS: network speed, outbound traffic
		ml_caption_append (caption, ML_CAPTION_COMPONENT_BODY, _("Outbound traffic: %s"), buf);
	} else {
		// TRANSLATORS: network speed measuring for outbound traffic is not active
		ml_caption_append (caption, ML_CAPTION_COMPONENT_BODY, _("Outbound traffic: not measuring"));
	}

	ml_caption_append (caption, ML_CAPTION_COMPONENT_BODY, "\n");

	if (s->do_count[NET_LOCAL]) {
		if (s->use_packets) {
			// TRANSLATORS: network packets per seconds
			snprintf (buf, sizeof(buf), _("%u packets/s"), diff_local);
		} else {
			ml_string_format_size_s (diff_local, "Bps", false, buf, sizeof (buf));
		}

		// TRANSLATORS: network speed, local traffic
		ml_caption_append (caption, ML_CAPTION_COMPONENT_BODY, _("Local traffic: %s"), buf);
	} else {
		// TRANSLATORS: network speed measuring for local traffic is not active
		ml_caption_append (caption, ML_CAPTION_COMPONENT_BODY, _("Local traffic: not measuring"));
	}

	// TRANSLATORS: list of monitored network interfaces
	ml_caption_append (caption, ML_CAPTION_COMPONENT_FOOTER, _("Monitored interfaces: %s"), (const char*)ml_grow_buffer_get_data (s->used_ifaces));
}
