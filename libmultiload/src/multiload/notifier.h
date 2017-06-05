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

#ifndef ML_HEADER__MULTILOAD_NOTIFIER_H__INCLUDED
#define ML_HEADER__MULTILOAD_NOTIFIER_H__INCLUDED
ML_HEADER_BEGIN

typedef struct _MlMultiloadNotifier MlMultiloadNotifier;


MlMultiloadNotifier *
ml_multiload_notifier_new ()
ML_FN_RETURNS_NONNULL;

void
ml_multiload_notifier_destroy (MlMultiloadNotifier *tmn);

size_t
ml_multiload_notifier_sizeof (MlMultiloadNotifier *tmn)
ML_FN_SIZEOF;

void
ml_multiload_notifier_signal (MlMultiloadNotifier *tmn);

void
ml_multiload_notifier_wait (MlMultiloadNotifier *tmn);

bool
ml_multiload_notifier_check (MlMultiloadNotifier *tmn)
ML_FN_READ_DYNAMIC_PROPERTY;


ML_HEADER_END
#endif /* ML_HEADER__MULTILOAD_NOTIFIER_H__INCLUDED */
