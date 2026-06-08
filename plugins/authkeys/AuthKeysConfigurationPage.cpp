/*
 * AuthKeysConfigurationPage.cpp - implementation of the authentication configuration page
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QAbstractButton>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

#include "AuthKeysConfigurationPage.h"
#include "AuthKeysManager.h"
#include "FileSystemBrowser.h"
#include "UserGroupsBackendManager.h"
#include "VeyonConfiguration.h"
#include "Configuration/UiMapping.h"

#include "ui_AuthKeysConfigurationPage.h"


namespace
{
static void clearDialogButtonIcons( QDialog* dialog )
{
	if( auto buttonBox = dialog->findChild<QDialogButtonBox *>() )
	{
		for( auto button : buttonBox->buttons() )
		{
			button->setIcon( QIcon() );
		}
	}
}


QString getTextWithoutButtonIcons( QWidget* parent, const QString& title, const QString& label )
{
	QInputDialog dialog( parent );
	dialog.setInputMode( QInputDialog::TextInput );
	dialog.setWindowTitle( title );
	dialog.setLabelText( label );
	dialog.setTextEchoMode( QLineEdit::Normal );

	clearDialogButtonIcons( &dialog );

	if( dialog.exec() == QDialog::Accepted )
	{
		return dialog.textValue();
	}

	return {};
}


QString getItemWithoutButtonIcons( QWidget* parent, const QString& title, const QString& label,
								   const QStringList& items, int current, bool editable, bool* ok )
{
	QDialog dialog( parent );
	dialog.setWindowTitle( title );

	auto* mainLayout = new QVBoxLayout( &dialog );
	auto* labelWidget = new QLabel( label, &dialog );
	auto* comboBox = new QComboBox( &dialog );
	comboBox->setEditable( editable );
	comboBox->addItems( items );
	comboBox->setCurrentIndex( current >= 0 ? current : 0 );

	auto* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog );
	for( auto button : buttonBox->buttons() )
	{
		button->setIcon( QIcon() );
	}

	mainLayout->addWidget( labelWidget );
	mainLayout->addWidget( comboBox );
	mainLayout->addWidget( buttonBox );

	QObject::connect( buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept );
	QObject::connect( buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject );

	if( dialog.exec() == QDialog::Accepted )
	{
		if( ok )
		{
			*ok = true;
		}
		return comboBox->currentText();
	}

	if( ok )
	{
		*ok = false;
	}
	return {};
}
}


AuthKeysConfigurationPage::AuthKeysConfigurationPage() :
	ConfigurationPage(),
	ui(new Ui::AuthKeysConfigurationPage),
	m_authKeyTableModel( this ),
	m_keyFilesFilter( tr( "Key files (*.pem)" ) )
{
	ui->setupUi(this);

	Configuration::UiMapping::setFlags( ui->keyFileDirectories, Configuration::Property::Flag::Advanced );

#define CONNECT_BUTTON_SLOT(name) \
			connect( ui->name, &QAbstractButton::clicked, this, &AuthKeysConfigurationPage::name );

	CONNECT_BUTTON_SLOT( openPublicKeyBaseDir );
	CONNECT_BUTTON_SLOT( openPrivateKeyBaseDir );
	CONNECT_BUTTON_SLOT( createKeyPair );
	CONNECT_BUTTON_SLOT( deleteKey );
	CONNECT_BUTTON_SLOT( importKey );
	CONNECT_BUTTON_SLOT( exportKey );
	CONNECT_BUTTON_SLOT( setAccessGroup );

	reloadKeyTable();

	ui->keyTable->setModel( &m_authKeyTableModel );
}


AuthKeysConfigurationPage::~AuthKeysConfigurationPage()
{
	delete ui;
}



void AuthKeysConfigurationPage::resetWidgets()
{
	FOREACH_VEYON_KEY_AUTHENTICATION_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);

	reloadKeyTable();
}



void AuthKeysConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_VEYON_KEY_AUTHENTICATION_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



void AuthKeysConfigurationPage::applyConfiguration()
{
}



void AuthKeysConfigurationPage::openPublicKeyBaseDir()
{
	FileSystemBrowser(FileSystemBrowser::ExistingDirectory, this).exec(ui->publicKeyBaseDir);
}



void AuthKeysConfigurationPage::openPrivateKeyBaseDir()
{
	FileSystemBrowser(FileSystemBrowser::ExistingDirectory, this).exec(ui->privateKeyBaseDir);
}



void AuthKeysConfigurationPage::createKeyPair()
{
	const auto keyName = getTextWithoutButtonIcons( this, tr( "Authentication key name" ),
											  tr( "Please enter the name of the user group or role for which to create an authentication key pair:") );
	if( keyName.isEmpty() == false )
	{
		AuthKeysManager authKeysManager;
		const auto success = authKeysManager.createKeyPair( keyName );

		showResultMessage( success, tr( "Create key pair" ), authKeysManager.resultMessage() );

		reloadKeyTable();
	}
}



void AuthKeysConfigurationPage::deleteKey()
{
	const auto title = ui->deleteKey->text();

	const auto nameAndType = selectedKey().split(QLatin1Char('/'));

	if( nameAndType.size() > 1 )
	{
		const auto name = nameAndType[0];
		const auto type = nameAndType[1];

		QMessageBox msgBox( this );
		msgBox.setWindowTitle( title );
		msgBox.setText( tr( "Do you really want to delete authentication key \"%1/%2\"?" ).arg( name, type ) );
		msgBox.setIcon( QMessageBox::NoIcon );
		msgBox.setStandardButtons( QMessageBox::Yes | QMessageBox::No );
		msgBox.setDefaultButton( QMessageBox::No );
		msgBox.button( QMessageBox::Yes )->setIcon( QIcon() );
		msgBox.button( QMessageBox::No )->setIcon( QIcon() );

		if( msgBox.exec() == QMessageBox::Yes )
		{
			AuthKeysManager authKeysManager;
			const auto success = authKeysManager.deleteKey( name, type );

			showResultMessage( success, title, authKeysManager.resultMessage() );

			reloadKeyTable();
		}
	}
	else
	{
		showResultMessage( false, title, tr( "Please select a key to delete!" ) );
	}
}



void AuthKeysConfigurationPage::importKey()
{
	const auto title = ui->importKey->text();

	QFileDialog fileDialog( this, title, {}, m_keyFilesFilter );
	fileDialog.setAcceptMode( QFileDialog::AcceptOpen );
	fileDialog.setOption( QFileDialog::DontUseNativeDialog );
	clearDialogButtonIcons( &fileDialog );
	const auto inputFile = fileDialog.exec() == QDialog::Accepted ? fileDialog.selectedFiles().value( 0 ) : QString();
	if( inputFile.isEmpty() )
	{
		return;
	}

	auto keyName = AuthKeysManager::keyNameFromExportedKeyFile(inputFile);
	if (keyName.isEmpty())
	{
		keyName = getTextWithoutButtonIcons( this, tr("Authentication key name"),
												  tr("Please enter the name of the user group or role for which to import the authentication key.\n\nMake sure that the names of the keys belonging to each other are identical on all computers.") );
	}

	if( keyName.isEmpty() )
	{
		return;
	}

	AuthKeysManager authKeysManager;
	const auto keyType = authKeysManager.detectKeyType( inputFile );
	const auto success = authKeysManager.importKey( keyName, keyType, inputFile );

	showResultMessage( success, title, authKeysManager.resultMessage() );

	reloadKeyTable();
}



void AuthKeysConfigurationPage::exportKey()
{
	const auto title = ui->exportKey->text();

	const auto nameAndType = selectedKey().split(QLatin1Char('/'));

	if( nameAndType.size() > 1 )
	{
		const auto name = nameAndType[0];
		const auto type = nameAndType[1];

		QFileDialog fileDialog( this, title, QDir::homePath() + QDir::separator() +
								 AuthKeysManager::exportedKeyFileName( name, type ),
								 m_keyFilesFilter );
		fileDialog.setAcceptMode( QFileDialog::AcceptSave );
		fileDialog.setOption( QFileDialog::DontUseNativeDialog );
		clearDialogButtonIcons( &fileDialog );
		const auto outputFile = fileDialog.exec() == QDialog::Accepted ? fileDialog.selectedFiles().value( 0 ) : QString();
		if( outputFile.isEmpty() == false )
		{
			AuthKeysManager authKeysManager;
			const auto success = authKeysManager.exportKey( name, type, outputFile, true );

			showResultMessage( success, title, authKeysManager.resultMessage() );
		}
	}
	else
	{
		showResultMessage( false, title, tr( "Please select a key to export!" ) );
	}
}



void AuthKeysConfigurationPage::setAccessGroup()
{
	const auto title = ui->setAccessGroup->text();

	const auto key = selectedKey();

	if( key.isEmpty() == false )
	{
		const auto userGroups = VeyonCore::userGroupsBackendManager().configuredBackend()->userGroups(VeyonCore::config().useDomainUserGroups());
		const auto currentGroup = AuthKeysManager().accessGroup( key );

		bool ok = false;
		const auto selectedGroup = getItemWithoutButtonIcons( this, title,
															  tr( "Please select a user group which to grant access to key \"%1\":" ).arg( key ),
															  userGroups, userGroups.indexOf( currentGroup ), true, &ok );

		if( ok && selectedGroup.isEmpty() == false )
		{
			AuthKeysManager manager;
			const auto success = manager.setAccessGroup( key, selectedGroup );

			showResultMessage( success, title, manager.resultMessage() );

			reloadKeyTable();
		}
	}
	else
	{
		showResultMessage( false, title, tr( "Please select a key which to set the access group for!" ) );
	}
}



void AuthKeysConfigurationPage::reloadKeyTable()
{
	m_authKeyTableModel.reload();
	ui->keyTable->resizeColumnsToContents();
}



QString AuthKeysConfigurationPage::selectedKey() const
{
	const auto row = ui->keyTable->currentIndex().row();
	if( row >= 0 && row < m_authKeyTableModel.rowCount() )
	{
		return m_authKeyTableModel.key( row );
	}

	return {};
}



void AuthKeysConfigurationPage::showResultMessage( bool success, const QString& title, const QString& message )
{
	if( message.isEmpty() )
	{
		return;
	}

	QMessageBox msgBox( this );
	msgBox.setWindowTitle( title );
	msgBox.setText( message );
	msgBox.setIcon( QMessageBox::NoIcon );
	msgBox.setStandardButtons( QMessageBox::Ok );
	msgBox.setDefaultButton( QMessageBox::Ok );
	msgBox.button( QMessageBox::Ok )->setIcon( QIcon() );
	msgBox.exec();
}
