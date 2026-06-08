/*
 * TextMessageFeaturePlugin.cpp - implementation of TextMessageFeaturePlugin class
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

#include <QIcon>
#include <QDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "TextMessageFeaturePlugin.h"
#include "TextMessageDialog.h"
#include "FeatureWorkerManager.h"
#include "ComputerControlInterface.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"


TextMessageFeaturePlugin::TextMessageFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_textMessageFeature( Feature( QStringLiteral( "TextMessage" ),
								   Feature::Flag::Action | Feature::Flag::AllComponents,
								   Feature::Uid( "e75ae9c8-ac17-4d00-8f0d-019348346208" ),
								   Feature::Uid(),
								   tr( "Text message" ), {},
								   tr( "Use this function to send a text message to all "
									   "users e.g. to assign them new tasks." ),
								   QStringLiteral(":/master/fa/message.svg") ) ),
	m_features( { m_textMessageFeature } )
{
}



const FeatureList &TextMessageFeaturePlugin::featureList() const
{
	return m_features;
}



bool TextMessageFeaturePlugin::controlFeature( Feature::Uid featureUid,
											  Operation operation,
											  const QVariantMap& arguments,
											  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( operation != Operation::Start )
	{
		return false;
	}

	if( featureUid == m_textMessageFeature.uid() )
	{
		const auto text = arguments.value( argToString(Argument::Text) ).toString();
		const auto icon = arguments.value( argToString(Argument::Icon) ).toInt();

		sendFeatureMessage(FeatureMessage{featureUid, FeatureCommand::ShowTextMessage}
						   .addArgument(Argument::Text, text)
						   .addArgument(Argument::Icon, icon), computerControlInterfaces);

		return true;
	}

	return false;
}



bool TextMessageFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
											 const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature.uid() != m_textMessageFeature.uid() )
	{
		return false;
	}

	QString textMessage;

	TextMessageDialog( textMessage, master.mainWindow() ).exec();

	if( textMessage.isEmpty() == false )
	{
		controlFeature( m_textMessageFeature.uid(), Operation::Start,
						{
							{ argToString(Argument::Text), textMessage },
							{ argToString(Argument::Icon), QMessageBox::Information }
						},
						computerControlInterfaces );
	}

	return true;
}




bool TextMessageFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
													 const MessageContext& messageContext,
													 const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( m_textMessageFeature.uid() == message.featureUid() )
	{
		// forward message to worker
		server.featureWorkerManager().sendMessageToUnmanagedSessionWorker( message );

		return true;
	}

	return false;
}



bool TextMessageFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker);

	if( message.featureUid() == m_textMessageFeature.uid() )
	{
		auto dialog = new QDialog();
		dialog->setAttribute( Qt::WA_DeleteOnClose );
		dialog->setWindowTitle( tr( "Message from teacher" ) );
		dialog->setWindowIcon( QIcon() );
		dialog->setWindowFlags( Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint );
		dialog->setMinimumWidth( 420 );
		dialog->setStyleSheet( QStringLiteral(
			"QDialog {"
			"    background-color: #ffffff;"
			"    color: #202124;"
			"}"
			"QLabel {"
			"    background-color: transparent;"
			"    color: #202124;"
			"    font-size: 13px;"
			"}"
			"QPushButton {"
			"    background-color: #ffffff;"
			"    border: 1px solid #dadce0;"
			"    border-radius: 7px;"
			"    color: #1a73e8;"
			"    font-weight: bold;"
			"    min-width: 96px;"
			"    padding: 7px 12px;"
			"}"
			"QPushButton:hover {"
			"    background-color: #f8f9fa;"
			"    border-color: #1a73e8;"
			"}"
			"QPushButton:pressed {"
			"    background-color: #f1f3f4;"
			"}"
		) );

		auto layout = new QVBoxLayout( dialog );
		layout->setContentsMargins( 24, 18, 24, 18 );
		layout->setSpacing( 18 );

		auto label = new QLabel( message.argument( Argument::Text ).toString(), dialog );
		label->setTextFormat( Qt::RichText );
		label->setTextInteractionFlags( Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard );
		label->setOpenExternalLinks( true );
		label->setWordWrap( true );
		layout->addWidget( label );

		auto okButton = new QPushButton( tr( "OK" ), dialog );
		okButton->setIcon( QIcon() );
		layout->addWidget( okButton, 0, Qt::AlignRight );
		connect( okButton, &QPushButton::clicked, dialog, &QDialog::accept );
		connect( dialog, &QDialog::accepted, dialog, &QDialog::deleteLater );

		dialog->show();

		return true;
	}

	return true;
}
