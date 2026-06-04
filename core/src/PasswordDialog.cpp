/*
 * PasswordDialog.cpp - dialog for querying logon credentials
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

#include <QIcon>
#include <QMessageBox>
#include <QPushButton>

#include "PasswordDialog.h"
#include "PlatformUserFunctions.h"

#include "ui_PasswordDialog.h"


PasswordDialog::PasswordDialog( QWidget *parent ) :
	QDialog( parent ),
	ui( new Ui::PasswordDialog )
{
	ui->setupUi( this );
	ui->buttonBox->button( QDialogButtonBox::Ok )->setIcon( QIcon() );
	ui->buttonBox->button( QDialogButtonBox::Cancel )->setIcon( QIcon() );

	ui->username->setText(VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName));

	if( ui->username->text().isEmpty() == false )
	{
		ui->password->setFocus();
	}

	updateOkButton();
}



PasswordDialog::~PasswordDialog()
{
	delete ui;
}



QString PasswordDialog::username() const
{
	return ui->username->text();
}



CryptoCore::PlaintextPassword PasswordDialog::password() const
{
	return ui->password->text().toUtf8();
}




AuthenticationCredentials PasswordDialog::credentials() const
{
	AuthenticationCredentials cred;
	cred.setLogonUsername( username() );
	cred.setLogonPassword( password() );

	return cred;
}



void PasswordDialog::accept()
{
	if( VeyonCore::platform().userFunctions().authenticate( username(), password() ) == false )
	{
		QMessageBox messageBox( QMessageBox::Critical,
							 tr( "Authentication error" ),
							 tr( "Logon failed with given username and password. Please try again!" ),
							 QMessageBox::Ok,
							 window() );
		messageBox.setIcon( QMessageBox::NoIcon );
		messageBox.button( QMessageBox::Ok )->setIcon( QIcon() );
		messageBox.exec();
	}
	else
	{
		QDialog::accept();
	}
}



void PasswordDialog::updateOkButton()
{
	ui->buttonBox->button( QDialogButtonBox::Ok )->
					setEnabled( !username().isEmpty() && !password().isEmpty() );
}
