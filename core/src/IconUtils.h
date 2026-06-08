/*
 * IconUtils.h - helper functions for loading icons
 *
 * Copyright (c) 2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#pragma once

#include <QIcon>
#include <QPixmap>
#include <QSize>

#include "VeyonCore.h"

namespace IconUtils
{
	VEYON_CORE_EXPORT QIcon iconFromUrl( const QString& url );
	VEYON_CORE_EXPORT QPixmap pixmapFromUrl( const QString& url, const QSize& size = QSize() );
}
