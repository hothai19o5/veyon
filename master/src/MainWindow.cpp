/*
 * MainWindow.cpp - implementation of MainWindow class
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

#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QIcon>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QHostAddress>
#include <QMenu>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>

#include "AboutDialog.h"
#include "AccessControlProvider.h"
#include "MainWindow.h"
#include "BuiltinFeatures.h"
#include "AuthenticationCredentials.h"
#include "ComputerControlListModel.h"
#include "ComputerManager.h"
#include "ComputerSelectPanel.h"
#include "ScreenshotManagementPanel.h"
#include "FeatureManager.h"
#include "MonitoringMode.h"
#include "NetworkObjectDirectory.h"
#include "NetworkObjectDirectoryManager.h"
#include "PlatformUserFunctions.h"
#include "ToolButton.h"
#include "VeyonConfiguration.h"
#include "VeyonMaster.h"
#include "UserConfig.h"

#include "ui_MainWindow.h"


MainWindow::MainWindow( VeyonMaster &masterCore, QWidget* parent ) :
	QMainWindow( parent ),
	ui( new Ui::MainWindow ),
	m_master( masterCore ),
	m_modeGroup( new QButtonGroup( this ) )
{
	ui->setupUi( this );
	ui->statusBar->setObjectName( QStringLiteral("bottomBar") );
	ui->computerMonitoringWidget->setObjectName( QStringLiteral("computerMonitoringWidget") );
	ui->computerSelectPanelButton->setIcon( QIcon() );
	ui->computerSelectPanelButton->setToolButtonStyle( Qt::ToolButtonTextOnly );
	ui->screenshotManagementPanelButton->setIcon( QIcon() );
	ui->screenshotManagementPanelButton->setToolButtonStyle( Qt::ToolButtonTextOnly );

	restoreState( QByteArray::fromBase64( m_master.userConfig().windowState().toUtf8() ) );
	restoreGeometry( QByteArray::fromBase64( m_master.userConfig().windowGeometry().toUtf8() ) );

	// add widgets to status bar
	ui->statusBar->addWidget( ui->panelButtons );
	ui->panelButtons->layout()->setContentsMargins( 12, 0, 0, 0 );
	ui->statusBar->addWidget( ui->spacerLabel1 );
	ui->statusBar->addWidget( ui->filterLineEdit, 2 );
	ui->statusBar->addWidget( ui->filterPoweredOnComputersButton );
	ui->statusBar->addWidget( ui->filterComputersWithLoggedOnUsersButton );
	ui->statusBar->addWidget( ui->spacerLabel2, 1 );
	ui->statusBar->addWidget( ui->gridSizeSlider, 2 );
	ui->statusBar->addWidget( ui->autoAdjustComputerIconSizeButton );
	ui->statusBar->addWidget( ui->spacerLabel3 );
	ui->slideshowPanelButton->hide();
	ui->spotlightPanelButton->hide();
	ui->aboutButton->hide();

	// create all views
	auto mainSplitter = new QSplitter( Qt::Horizontal, ui->centralWidget );
	mainSplitter->setChildrenCollapsible( false );
	mainSplitter->setObjectName( QStringLiteral("MainSplitter") );

	auto computerSelectPanel = new ComputerSelectPanel( m_master.computerManager() );
	auto screenshotManagementPanel = new ScreenshotManagementPanel();
	auto sidebarIslandContainer = new QWidget( mainSplitter );
	sidebarIslandContainer->setObjectName( QStringLiteral( "sidebarIslandContainer" ) );
	sidebarIslandContainer->setMinimumWidth( 176 );
	sidebarIslandContainer->setMaximumWidth( 300 );
	auto sidebarIslandLayout = new QVBoxLayout( sidebarIslandContainer );
	sidebarIslandLayout->setContentsMargins( 10, 10, 10, 10 );
	sidebarIslandLayout->setSpacing( 0 );
	sidebarIslandLayout->addWidget( computerSelectPanel );
	sidebarIslandLayout->addWidget( screenshotManagementPanel );
	computerSelectPanel->setMinimumWidth( 148 );
	computerSelectPanel->setMaximumWidth( 280 );
	screenshotManagementPanel->setMinimumWidth( 148 );
	screenshotManagementPanel->setMaximumWidth( 280 );

	mainSplitter->addWidget( sidebarIslandContainer );
	mainSplitter->addWidget( ui->computerMonitoringWidget );

	mainSplitter->setStretchFactor( mainSplitter->indexOf(ui->computerMonitoringWidget), 1 );


	auto panelButtonGroup = new QButtonGroup( this );
	panelButtonGroup->addButton( ui->computerSelectPanelButton );
	panelButtonGroup->addButton( ui->screenshotManagementPanelButton );
	panelButtonGroup->setExclusive( true );
	connect( ui->computerSelectPanelButton, &QAbstractButton::toggled,
			 computerSelectPanel, &QWidget::setVisible );
	connect( ui->screenshotManagementPanelButton, &QAbstractButton::toggled,
			 screenshotManagementPanel, &QWidget::setVisible );
	connect( ui->computerSelectPanelButton, &QAbstractButton::toggled,
			 screenshotManagementPanel, [screenshotManagementPanel]( bool checked ) { screenshotManagementPanel->setHidden( checked ); } );
	connect( ui->screenshotManagementPanelButton, &QAbstractButton::toggled,
			 computerSelectPanel, [computerSelectPanel]( bool checked ) { computerSelectPanel->setHidden( checked ); } );
	screenshotManagementPanel->hide();
	ui->computerSelectPanelButton->setChecked( true );

	static const QHash<QWidget *, QAbstractButton *> panelButtons;

	QList<int> splitterSizes;
	for( auto* splitter : { mainSplitter } )
	{
		splitter->setHandleWidth( 0 );

		splitter->installEventFilter( this );

		int index = 0;

		const auto splitterStates = m_master.userConfig().splitterStates()[splitter->objectName()].toArray();
		splitterSizes.clear();
		splitterSizes.reserve( splitterStates.size() );

		for( const auto& sizeObject : splitterStates )
		{
			if( index >= splitter->count() )
			{
				break;
			}

			auto size = sizeObject.toInt();
			const auto widget = splitter->widget( index );
			const auto button = panelButtons.value( widget );

			if( widget )
			{
				if( button )
				{
					widget->setVisible( size > 0 );
					button->setChecked( size > 0 );
				}
				size = qAbs( size );
				if( splitter->orientation() == Qt::Horizontal )
				{
					widget->resize( size, widget->height() );
				}
				else
				{
					widget->resize( widget->width(), size );
				}

				widget->setProperty( originalSizePropertyName(), widget->size() );
			}
			splitterSizes.append( size );
			++index;
		}
		splitter->setSizes( splitterSizes );
	}

	ui->centralLayout->addWidget( mainSplitter );

	if( VeyonCore::config().autoOpenComputerSelectPanel() )
	{
		ui->computerSelectPanelButton->setChecked( true );
	}

	// initialize filter controls
	connect( ui->filterLineEdit, &QLineEdit::textChanged,
			 this, [this]( const QString& filter ) { ui->computerMonitoringWidget->setSearchFilter( filter ); } );
	connect( ui->filterPoweredOnComputersButton, &QToolButton::toggled,
			 this, [this]( bool enabled ) { ui->computerMonitoringWidget->setFilterPoweredOnComputers( enabled ); } );
	connect( ui->filterComputersWithLoggedOnUsersButton, &QToolButton::toggled,
			 this, [this]( bool enabled ) { ui->computerMonitoringWidget->setFilterComputersWithLoggedOnUsers( enabled ); } );
	ui->filterPoweredOnComputersButton->setChecked(m_master.userConfig().filterPoweredOnComputers());
	ui->filterComputersWithLoggedOnUsersButton->setChecked(m_master.userConfig().filterComputersWithLoggedOnUsers());

	// initialize monitoring screen size slider
	ui->gridSizeSlider->setMinimum( ComputerMonitoringWidget::MinimumComputerScreenSize );
	ui->gridSizeSlider->setMaximum( ComputerMonitoringWidget::MaximumComputerScreenSize );

	ui->autoAdjustComputerIconSizeButton->setChecked( ui->computerMonitoringWidget->autoAdjustIconSize() );

	connect( ui->gridSizeSlider, &QSlider::valueChanged,
			 this, [this]( int size ) { ui->computerMonitoringWidget->setComputerScreenSize( size ); } );
	connect( ui->computerMonitoringWidget, &ComputerMonitoringWidget::computerScreenSizeAdjusted,
			 ui->gridSizeSlider, &QSlider::setValue );
	connect( ui->autoAdjustComputerIconSizeButton, &QToolButton::toggled,
			 this, [this]( bool enabled ) {
				 ui->computerMonitoringWidget->setAutoAdjustIconSize( enabled );
				 m_master.userConfig().setAutoAdjustMonitoringIconSize( enabled );
			 } );

	int size = ComputerMonitoringWidget::DefaultComputerScreenSize;
	if( m_master.userConfig().monitoringScreenSize() >= ComputerMonitoringWidget::MinimumComputerScreenSize )
	{
		size = m_master.userConfig().monitoringScreenSize();
	}

	ui->gridSizeSlider->setValue( size );
	ui->computerMonitoringWidget->setComputerScreenSize( size );

	const auto toolButtons = findChildren<QToolButton*>();
	for(auto* btn : toolButtons)
	{
		btn->setIconSize(QSize(20, 20));
	}

	if (VeyonCore::useDarkMode())
	{
		ui->aboutButton->setIcon(QIcon(QStringLiteral(":/master/fa/circle-info.svg")));
		ui->filterComputersWithLoggedOnUsersButton->setIcon(QIcon(QStringLiteral(":/master/fa/users.svg")));
		ui->autoAdjustComputerIconSizeButton->setIcon(QIcon(QStringLiteral(":/master/fa/expand-arrows.svg")));
		ui->filterPoweredOnComputersButton->setIcon(QIcon(QStringLiteral(":/master/fa/power-off.svg")));
	}

	// Keep toolbar actions available internally, but use the spacious context-driven master layout.
	ui->toolBar->layout()->setSpacing( 2 );
	ui->toolBar->toggleViewAction()->setEnabled( false );
	ui->toolBar->hide();

	addFeaturesToToolBar();
	reloadSubFeatures();

	m_modeGroup->button(int(qHash(VeyonCore::builtinFeatures().monitoringMode().feature().uid())))->setChecked(true); // clazy:exclude=qt6-qhash-signature
}



MainWindow::~MainWindow()
{
	ui->computerMonitoringWidget->saveConfiguration();

	delete ui;
}



bool MainWindow::initAuthentication()
{
	if( VeyonCore::instance()->initAuthentication() )
	{
		return true;
	}

	if( VeyonCore::config().authenticationMethod() == VeyonCore::AuthenticationMethod::KeyFileAuthentication )
	{
		QMessageBox msgBox( QMessageBox::Information, tr("Authentication impossible"),
							tr("No authentication key files were found or your current ones "
							   "are outdated. Please create new key files using EduMonitor "
							   "Configurator. Alternatively set up logon authentication "
							   "using EduMonitor Configurator. Otherwise you won't be "
							   "able to access computers using EduMonitor."),
							QMessageBox::Ok, nullptr );
		msgBox.setIcon( QMessageBox::NoIcon );
		msgBox.button( QMessageBox::Ok )->setIcon( QIcon() );
		msgBox.exec();
	}

	return false;
}



bool MainWindow::initAccessControl()
{
	if (VeyonCore::config().accessControlForMasterEnabled())
	{
		const auto accessingUser = VeyonCore::authenticationCredentials().hasCredentials(AuthenticationCredentials::Type::UserLogon) ?
									   VeyonCore::authenticationCredentials().logonUsername() :
									   VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName);
		const auto accessControlResult = VeyonCore::builtinFeatures().accessControlProvider()
										 .checkAccess(accessingUser,
													  QHostAddress(QHostAddress::LocalHost).toString(),
													  {});
		if( accessControlResult.access == AccessControlProvider::Access::Deny )
		{
			vWarning() << "user" << VeyonCore::authenticationCredentials().logonUsername()
					   << "is not allowed to access computers";
			QMessageBox msgBox( QMessageBox::Critical, tr( "Access denied" ),
								tr( "According to the local configuration you're not allowed "
									"to access computers in the network. Please log in with a different "
									"account or let your system administrator check the local configuration." ),
								QMessageBox::Ok, nullptr );
			msgBox.setIcon( QMessageBox::NoIcon );
			msgBox.button( QMessageBox::Ok )->setIcon( QIcon() );
			msgBox.exec();
			return false;
		}
	}

	return true;
}



void MainWindow::reloadSubFeatures()
{
	for( const auto& feature : m_master.features() )
	{
		auto button = ui->toolBar->findChild<QToolButton *>( feature.name() );
		if( button )
		{
			addSubFeaturesToToolButton( button, feature );
		}
	}
}



ComputerControlInterfaceList MainWindow::selectedComputerControlInterfaces() const
{
	return ui->computerMonitoringWidget->selectedComputerControlInterfaces();
}



void MainWindow::closeEvent( QCloseEvent* event )
{
	if( m_master.currentMode() != VeyonCore::builtinFeatures().monitoringMode().feature().uid() )
	{
		const Feature& activeFeature = VeyonCore::featureManager().feature( m_master.currentMode() );

		QMessageBox msgBox( QMessageBox::Information, tr("Feature active"),
							tr("The feature \"%1\" is still active. Please stop it before closing EduMonitor.")
							.arg(activeFeature.displayName()),
							QMessageBox::Ok, this );
		msgBox.setIcon( QMessageBox::NoIcon );
		msgBox.button( QMessageBox::Ok )->setIcon( QIcon() );
		msgBox.exec();
		event->ignore();
		return;
	}

	QJsonObject splitterStates;
	const auto splitters = findChildren<QSplitter *>();
	for( const auto* splitter : splitters )
	{
		const auto sizes = splitter->sizes();
		QJsonArray splitterSizes;
		int i = 0;
		int hiddenSize = 0;
		for( auto size : sizes )
		{
			auto widget = splitter->widget(i);
			const auto originalSize = widget->property( originalSizePropertyName() ).toSize();
			if( widget->size().isEmpty() && originalSize.isEmpty() == false )
			{
				size = splitter->orientation() == Qt::Horizontal ? -originalSize.width() : -originalSize.height();
				hiddenSize += qAbs(size);
			}
			else
			{
				size -= hiddenSize;
			}

			splitterSizes.append( size );
			++i;
		}
		splitterStates[splitter->objectName()] = splitterSizes;
	}

	m_master.userConfig().setSplitterStates( splitterStates );

	m_master.userConfig().setWindowState( QString::fromLatin1( saveState().toBase64() ) );
	m_master.userConfig().setWindowGeometry( QString::fromLatin1( saveGeometry().toBase64() ) );

	QMainWindow::closeEvent( event );
}



bool MainWindow::eventFilter( QObject* object, QEvent* event )
{
	if( event->type() == QEvent::Resize )
	{
		const auto widget = qobject_cast<QWidget *>( object );
		const auto resizeEvent = static_cast<QResizeEvent *>( event );

		if( resizeEvent->oldSize().isEmpty() == false )
		{
			widget->setProperty( originalSizePropertyName(), resizeEvent->oldSize() );
		}
	}

	return QMainWindow::eventFilter( object, event );
}



void MainWindow::keyPressEvent( QKeyEvent* event )
{
	switch( event->key() )
	{
	case Qt::Key_F5:
		VeyonCore::networkObjectDirectoryManager().configuredDirectory()->update();
		m_master.computerControlListModel().reload();
		event->accept();
		break;
	case Qt::Key_F11:
		QWidget::setWindowState( QWidget::windowState() ^ Qt::WindowFullScreen );
		event->accept();
		break;
	default:
		QMainWindow::keyPressEvent( event );
		break;
	}
}



void MainWindow::showAboutDialog()
{
	AboutDialog( this ).exec();
}



void MainWindow::addFeaturesToToolBar()
{
	for( const auto& feature : m_master.features() )
	{
		if( feature.testFlag( Feature::Flag::Meta ) )
		{
			continue;
		}

		auto btn = new ToolButton( QIcon( feature.iconUrl() ),
										  feature.displayName(),
										  feature.displayNameActive(),
										  feature.description(),
										  feature.shortcut() );
		connect(btn, &QToolButton::clicked, this, [=, this] () {
			m_master.runFeature( feature );
			updateModeButtonGroup();
			if( feature.testFlag( Feature::Flag::Mode ) )
			{
				reloadSubFeatures();
			}
		});
		btn->setObjectName( feature.name() );
		btn->addTo( ui->toolBar );

		if( feature.testFlag( Feature::Flag::Mode ) )
		{
			btn->setCheckable( true );
			m_modeGroup->addButton( btn, buttonId( feature ) );
		}
	}
}



void MainWindow::addSubFeaturesToToolButton( QToolButton* button, const Feature& parentFeature )
{
	if( button->menu() )
	{
		button->menu()->close();
		button->menu()->deleteLater();
		button->setMenu( nullptr );
	}

	const auto parentFeatureIsMode = parentFeature.testFlag( Feature::Flag::Mode );
	const auto subFeatures = m_master.subFeatures( parentFeature.uid() );

	if( subFeatures.isEmpty() ||
		( parentFeatureIsMode && button->isChecked() ) )
	{
		return;
	}

	auto menu = new QMenu( button );
	menu->setObjectName( parentFeature.name() );
	menu->setToolTipsVisible( true );

	for( const auto& subFeature : subFeatures )
	{
		auto action = menu->addAction(QIcon(subFeature.iconUrl()), subFeature.displayName(),
							   #if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
									  subFeature.shortcut(),
							   #endif
									  this, [=, this]()
		{
			m_master.runFeature( subFeature );
			if( parentFeatureIsMode )
			{
				if( subFeature.testFlag( Feature::Flag::Option ) == false )
				{
					button->setChecked( true );
				}
				reloadSubFeatures();
			}
		}
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
		,subFeature.shortcut()
#endif
		);
		action->setToolTip( subFeature.description() );
		action->setObjectName( subFeature.uid().toString() );

		if( subFeature.testFlag( Feature::Flag::Option ) )
		{
			action->setCheckable( true );
			action->setChecked( subFeature.testFlag( Feature::Flag::Checked ) );
		}
	}

	button->setMenu( menu );
	button->setPopupMode( ToolButton::InstantPopup );
}



void MainWindow::updateModeButtonGroup()
{
	const auto& monitoringMode = VeyonCore::builtinFeatures().monitoringMode().feature();

	if( m_master.currentMode() == monitoringMode.uid() )
	{
		m_modeGroup->button( buttonId( monitoringMode ) )->setChecked( true );
	}
}



void MainWindow::loadComputerPositions()
{
	QFileDialog dialog( this, tr("Load computer positions"), QDir::homePath(), tr("JSON files (*.json)") );
	dialog.setFileMode( QFileDialog::ExistingFile );
	if( auto buttonBox = dialog.findChild<QDialogButtonBox *>() )
	{
		for( auto button : buttonBox->buttons() )
		{
			button->setIcon( QIcon() );
		}
	}
	if( dialog.exec() == QDialog::Accepted )
	{
		const auto fileNames = dialog.selectedFiles();
		if( fileNames.isEmpty() == false )
		{
			const auto fileName = fileNames.first();
			if (QFile file(fileName); file.open(QFile::ReadOnly))
			{
				const auto& computerPositionsProperty = m_master.userConfig().computerPositionsProperty();
				const auto config = QJsonDocument::fromJson(file.readAll()).object();
				const auto uiConfig = config[computerPositionsProperty.parentKey()].toObject();
				const auto computerPositions = uiConfig[computerPositionsProperty.key()].toObject()[QStringLiteral("JsonStoreArray")].toArray();

				ui->computerMonitoringWidget->loadPositions(computerPositions);
				ui->computerMonitoringWidget->setUseCustomComputerPositions(true);
				ui->computerMonitoringWidget->doItemsLayout();
			}
		}
	}
}



void MainWindow::saveComputerPositions()
{
	QFileDialog dialog( this, tr("Save computer positions"), QDir::homePath(), tr("JSON files (*.json)") );
	dialog.setFileMode( QFileDialog::AnyFile );
	dialog.setAcceptMode( QFileDialog::AcceptSave );
	if( auto buttonBox = dialog.findChild<QDialogButtonBox *>() )
	{
		for( auto button : buttonBox->buttons() )
		{
			button->setIcon( QIcon() );
		}
	}
	if( dialog.exec() == QDialog::Accepted )
	{
		const auto fileNames = dialog.selectedFiles();
		if( fileNames.isEmpty() )
		{
			return;
		}
		const auto fileName = fileNames.first();
		if (QFile file(fileName); file.open(QFile::WriteOnly | QFile::Truncate))
		{
			const auto& computerPositionsProperty = m_master.userConfig().computerPositionsProperty();

			// create structure identical to UserConfig so file with positions can be used as template for UserConfig
			QJsonObject computerPositions;
			computerPositions[QStringLiteral("JsonStoreArray")] = ui->computerMonitoringWidget->savePositions();
			QJsonObject uiConfig;
			uiConfig[computerPositionsProperty.key()] = computerPositions;
			QJsonObject config;
			config[computerPositionsProperty.parentKey()] = uiConfig;
			file.write(QJsonDocument(config).toJson());
		}
	}
}
