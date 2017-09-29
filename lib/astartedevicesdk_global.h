/*
 * Copyright (C) 2017 Ispirata Srl
 *
 * This file is part of Astarte.
 * Astarte is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * Astarte is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Astarte.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QT4SDK_GLOBAL_H
#define QT4SDK_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QT4SDK_LIBRARY)
#  define ASTARTEQT4SDKSHARED_EXPORT Q_DECL_EXPORT
#else
#  define ASTARTEQT4SDKSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // QT4SDK_GLOBAL_H
