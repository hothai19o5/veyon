/*
 * main.cpp - startup routine for Veyon Master Application
 *
 * Copyright (c) 2004-2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QPalette>
#include <QSplashScreen>
#include <QTextStream>
#include <QToolTip>
#include <QWidget>

#include "DocumentationFigureCreator.h"
#include "VeyonMaster.h"
#include "MainWindow.h"


class RoundedPopupEventFilter : public QObject
{
public:
	explicit RoundedPopupEventFilter( QObject* parent = nullptr ) :
		QObject( parent )
	{
	}

protected:
	bool eventFilter( QObject* obj, QEvent* event ) override
	{
		if( event->type() == QEvent::Show ||
			event->type() == QEvent::Create ||
			event->type() == QEvent::Resize )
		{
			if( auto widget = qobject_cast<QWidget *>( obj ) )
			{
				const auto className = widget->metaObject()->className();
				if( widget->inherits( "QTipLabel" ) || qstrcmp( className, "QTipLabel" ) == 0 ||
					widget->inherits( "QMenu" ) || qstrcmp( className, "QMenu" ) == 0 )
				{
					widget->setWindowFlag( Qt::NoDropShadowWindowHint, true );
				}
			}
		}

		return QObject::eventFilter( obj, event );
	}
};


int main( int argc, char** argv )
{
	VeyonCore::setupApplicationParameters();

	QApplication app( argc, argv );
	RoundedPopupEventFilter roundedPopupEventFilter( &app );
	app.installEventFilter( &roundedPopupEventFilter );
	app.connect( &app, &QApplication::lastWindowClosed, &QApplication::quit );

	VeyonCore core( &app, VeyonCore::Component::Master, QStringLiteral("Master") );

	QFontDatabase::addApplicationFont( QStringLiteral( ":/master/style/fonts/JetBrainsMono-Regular.ttf" ) );
	QFontDatabase::addApplicationFont( QStringLiteral( ":/master/style/fonts/JetBrainsMono-Bold.ttf" ) );
	app.setFont( QFont( QStringLiteral( "JetBrains Mono" ) ) );

	QFile styleFile( QStringLiteral( ":/master/style/modern-master.qss" ) );
	if( styleFile.open( QFile::ReadOnly | QFile::Text ) )
	{
		QTextStream stream( &styleFile );
		app.setStyleSheet( stream.readAll() );
	}

	auto toolTipPalette = QToolTip::palette();
	toolTipPalette.setColor( QPalette::Window, QColor( QStringLiteral( "#ffffff" ) ) );
	toolTipPalette.setColor( QPalette::ToolTipBase, QColor( QStringLiteral( "#ffffff" ) ) );
	toolTipPalette.setColor( QPalette::ToolTipText, QColor( QStringLiteral( "#202124" ) ) );
	QToolTip::setPalette( toolTipPalette );

#ifdef VEYON_DEBUG
	if( qEnvironmentVariableIsSet( "VEYON_MASTER_CREATE_DOC_FIGURES") )
	{
		DocumentationFigureCreator().run();
		return 0;
	}
#endif

	QSplashScreen splashScreen( QPixmap( QStringLiteral(":/master/splash.png") ) );
	splashScreen.show();

	if( MainWindow::initAuthentication() == false ||
			MainWindow::initAccessControl() == false )
	{
		return -1;
	}

	VeyonMaster masterCore( &core );

	// hide splash-screen as soon as main-window is shown
	splashScreen.finish( masterCore.mainWindow() );

	masterCore.mainWindow()->show();

	return core.exec();
}
