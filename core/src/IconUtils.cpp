/*
 * IconUtils.cpp - helper functions for loading icons
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

#include <QFile>
#include <QImageReader>

#include "IconUtils.h"

namespace
{
bool isSvgUrl( const QString& url )
{
	return url.endsWith( QLatin1String( ".svg" ), Qt::CaseInsensitive );
}



QStringList supportedImageFormats()
{
	QStringList formats;
	for( const auto& format : QImageReader::supportedImageFormats() )
	{
		formats.append( QString::fromLatin1( format ) );
	}
	return formats;
}



void logSvgLoadFailure( const QString& url, const QImageReader& reader, const QPixmap& pixmap )
{
	vWarning() << "could not load SVG icon" << url
			   << "resource/file exists:" << QFile::exists( url )
			   << "reader can read:" << reader.canRead()
			   << "reader size:" << reader.size()
			   << "reader error:" << reader.errorString()
			   << "pixmap null:" << pixmap.isNull()
			   << "supported image formats:" << supportedImageFormats().join( QLatin1Char( ',' ) );
}
}



QIcon IconUtils::iconFromUrl( const QString& url )
{
	if( url.isEmpty() )
	{
		return {};
	}

	if( isSvgUrl( url ) )
	{
		const auto pixmap = pixmapFromUrl( url );
		if( pixmap.isNull() == false )
		{
			return QIcon( pixmap );
		}
	}

	auto icon = QIcon( url );
	if( isSvgUrl( url ) && icon.pixmap( QSize( 24, 24 ) ).isNull() )
	{
		vWarning() << "SVG icon engine returned an empty icon" << url;
	}

	return icon;
}



QPixmap IconUtils::pixmapFromUrl( const QString& url, const QSize& size )
{
	if( url.isEmpty() )
	{
		return {};
	}

	if( isSvgUrl( url ) )
	{
		QImageReader reader( url );
		if( size.isValid() )
		{
			reader.setScaledSize( size );
		}

		const auto image = reader.read();
		const auto pixmap = QPixmap::fromImage( image );
		if( pixmap.isNull() )
		{
			logSvgLoadFailure( url, reader, pixmap );
		}
		else
		{
			vDebug() << "loaded SVG icon" << url
					 << "resource/file exists:" << QFile::exists( url )
					 << "reader can read:" << reader.canRead()
					 << "reader size:" << reader.size()
					 << "pixmap size:" << pixmap.size();
		}

		return pixmap;
	}

	QPixmap pixmap( url );

	if( pixmap.isNull() == false && size.isValid() )
	{
		pixmap = pixmap.scaled( size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
	}

	return pixmap;
}
