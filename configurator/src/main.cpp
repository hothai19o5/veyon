/*
 * main.cpp - main file for Veyon Configurator
 *
 * Copyright (c) 2010-2026 Tobias Junghans <tobydox@veyon.io>
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
#include <QFontDatabase>
#include <QIcon>
#include <QMessageBox>
#include <QFile>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOptionComboBox>
#include <QTextStream>

#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "MainWindow.h"
#include "PlatformCoreFunctions.h"
#include "Logger.h"


class ConfiguratorStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    QRect subControlRect( ComplexControl control, const QStyleOptionComplex* option, SubControl subControl,
                          const QWidget* widget = nullptr ) const override
    {
        if( control == CC_ComboBox && subControl == SC_ComboBoxArrow )
        {
            return QRect();
        }

        return QProxyStyle::subControlRect( control, option, subControl, widget );
    }

    void drawComplexControl( ComplexControl control, const QStyleOptionComplex* option, QPainter* painter,
                             const QWidget* widget = nullptr ) const override
    {
        if( control == CC_ComboBox )
        {
            if( auto comboBoxOption = qstyleoption_cast<const QStyleOptionComboBox*>( option ) )
            {
                auto optionWithoutArrow = *comboBoxOption;
                optionWithoutArrow.subControls &= ~SC_ComboBoxArrow;
                optionWithoutArrow.activeSubControls &= ~SC_ComboBoxArrow;

                QProxyStyle::drawComplexControl( control, &optionWithoutArrow, painter, widget );
                return;
            }
        }

        QProxyStyle::drawComplexControl( control, option, painter, widget );
    }

    int styleHint( StyleHint hint, const QStyleOption* option = nullptr,
                   const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr ) const override
    {
        if( hint == SH_ComboBox_Popup )
        {
            return 0;
        }

        return QProxyStyle::styleHint( hint, option, widget, returnData );
    }
};



int main( int argc, char **argv )
{
	VeyonCore::setupApplicationParameters();

	QApplication app( argc, argv );
	auto configuratorBaseStyle = QStyleFactory::create( app.style()->objectName() );
	if( configuratorBaseStyle == nullptr )
	{
		configuratorBaseStyle = QStyleFactory::create( QStringLiteral( "Fusion" ) );
	}
	app.setStyle( new ConfiguratorStyle( configuratorBaseStyle ) );

	VeyonCore core( &app, VeyonCore::Component::Configurator, QStringLiteral("Configurator") );

	QFontDatabase::addApplicationFont( QStringLiteral( ":/configurator/style/fonts/JetBrainsMono-Regular.ttf" ) );
	QFontDatabase::addApplicationFont( QStringLiteral( ":/configurator/style/fonts/JetBrainsMono-Bold.ttf" ) );

	// Load and apply modern UI stylesheet
	QFile styleFile( QStringLiteral(":/configurator/style/modern.qss") );
	if( styleFile.open( QFile::ReadOnly | QFile::Text ) )
	{
		QTextStream ts( &styleFile );
		app.setStyleSheet( ts.readAll() );
	}


	// make sure to run as admin
	if( qEnvironmentVariableIntValue( "VEYON_CONFIGURATOR_NO_ELEVATION" ) == 0 &&
		VeyonCore::platform().coreFunctions().isRunningAsAdmin() == false &&
		app.arguments().size() <= 1 )
	{
		if( VeyonCore::platform().coreFunctions().runProgramAsAdmin( QCoreApplication::applicationFilePath(), {
																	 QStringLiteral("-elevated") } ) )
		{
			return 0;
		}

		QMessageBox msgBox( QMessageBox::Warning, MainWindow::tr( "Insufficient privileges" ),
							MainWindow::tr( "Could not start with administrative privileges. "
											"Please make sure a sudo-like program is installed "
											"for your desktop environment! The program will "
											"be run with normal user privileges."),
							QMessageBox::Ok, nullptr );
		msgBox.setIcon( QMessageBox::NoIcon );
		msgBox.button( QMessageBox::Ok )->setIcon( QIcon() );
		msgBox.exec();
	}

	if( VeyonConfiguration().isStoreWritable() == false &&
		VeyonCore::config().logLevel() != Logger::LogLevel::Debug )
	{
		QMessageBox msgBox( QMessageBox::Critical, MainWindow::tr("Configuration not writable"),
							MainWindow::tr("The local configuration backend reported that the "
										   "configuration is not writable! Please run EduMonitor "
										   "Configurator with higher privileges."),
							QMessageBox::Ok, nullptr );
		msgBox.setIcon( QMessageBox::NoIcon );
		msgBox.button( QMessageBox::Ok )->setIcon( QIcon() );
		msgBox.exec();
		return -1;
	}

	// now create the main window
	auto mainWindow = new MainWindow;
	mainWindow->show();

	return core.exec();
}
