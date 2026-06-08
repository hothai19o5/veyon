/*
 * ComputerMonitoringWidget.cpp - provides a view with computer monitor thumbnails
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

#include <QApplication>
#include <QGuiApplication>
#include <QHelpEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QScrollBar>
#include <QShowEvent>
#include <QToolTip>
#include <QVBoxLayout>

#include <functional>

#include "ComputerControlListModel.h"
#include "ComputerItemDelegate.h"
#include "ComputerMonitoringModel.h"
#include "ComputerMonitoringWidget.h"
#include "VeyonMaster.h"
#include "FeatureManager.h"
#include "VeyonConfiguration.h"


namespace
{
class RoundedPopup : public QWidget
{
public:
	explicit RoundedPopup( QWidget* parent = nullptr,
						   Qt::WindowFlags flags = Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint ) :
		QWidget( parent, flags )
	{
		setAttribute( Qt::WA_TranslucentBackground );
		setAttribute( Qt::WA_DeleteOnClose );
	}

	void popup( const QPoint& globalPos )
	{
		adjustSize();
		move( adjustedPopupPosition( globalPos, size() ) );
		show();
	}

	void popupSubMenu( const QPoint& globalPos, int parentItemWidth )
	{
		adjustSize();

		auto pos = adjustedPopupPosition( globalPos, size() );
		if( pos.x() < globalPos.x() && parentItemWidth > 0 )
		{
			pos.setX( globalPos.x() - parentItemWidth - width() );
			pos = adjustedPopupPosition( pos, size() );
		}

		move( pos );
		show();
	}

protected:
	void paintEvent( QPaintEvent* event ) override
	{
		Q_UNUSED( event );

		QPainter painter( this );
		painter.setRenderHint( QPainter::Antialiasing );

		const auto rect = QRectF( this->rect() ).adjusted( 0.5, 0.5, -0.5, -0.5 );
		QPainterPath path;
		path.addRoundedRect( rect, Radius, Radius );

		painter.fillPath( path, QColor( QStringLiteral( "#ffffff" ) ) );
		painter.setPen( QColor( QStringLiteral( "#dadce0" ) ) );
		painter.drawPath( path );
	}

protected:
	static QPoint adjustedPopupPosition( const QPoint& globalPos, const QSize& popupSize )
	{
		auto screen = QGuiApplication::screenAt( globalPos );
		if( screen == nullptr )
		{
			screen = QGuiApplication::primaryScreen();
		}

		if( screen == nullptr )
		{
			return globalPos;
		}

		const auto screenGeometry = screen->availableGeometry();
		auto x = globalPos.x();
		auto y = globalPos.y();

		if( x + popupSize.width() > screenGeometry.right() )
		{
			x = screenGeometry.right() - popupSize.width();
		}
		if( y + popupSize.height() > screenGeometry.bottom() )
		{
			y = screenGeometry.bottom() - popupSize.height();
		}

		x = qMax( screenGeometry.left(), x );
		y = qMax( screenGeometry.top(), y );

		return { x, y };
	}

	static constexpr auto Radius = 10;
};



class RoundedPanel : public QWidget
{
public:
	explicit RoundedPanel( QWidget* parent = nullptr ) :
		QWidget( parent )
	{
		setAttribute( Qt::WA_TranslucentBackground );
	}

protected:
	void paintEvent( QPaintEvent* event ) override
	{
		Q_UNUSED( event );

		QPainter painter( this );
		painter.setRenderHint( QPainter::Antialiasing );

		const auto rect = QRectF( this->rect() ).adjusted( 0.5, 0.5, -0.5, -0.5 );
		QPainterPath path;
		path.addRoundedRect( rect, Radius, Radius );

		painter.fillPath( path, QColor( QStringLiteral( "#ffffff" ) ) );
		painter.setPen( QColor( QStringLiteral( "#dadce0" ) ) );
		painter.drawPath( path );
	}

private:
	static constexpr auto Radius = 10;
};



class FeatureMenuItem : public QWidget
{
public:
	FeatureMenuItem( const QString& text,
					 std::function<void()> callback,
					 std::function<void( FeatureMenuItem* )> hoverCallback = {},
					 bool hasSubMenu = false,
					 QWidget* parent = nullptr ) :
		QWidget( parent ),
		m_text( text ),
		m_callback( std::move( callback ) ),
		m_hoverCallback( std::move( hoverCallback ) ),
		m_hasSubMenu( hasSubMenu )
	{
		setMouseTracking( true );
		setMinimumHeight( 30 );
		setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	}

	QSize sizeHint() const override
	{
		return { qMax( 180, fontMetrics().horizontalAdvance( m_text ) + 40 ), 30 };
	}

protected:
	void enterEvent(
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		QEnterEvent* event
#else
		QEvent* event
#endif
	) override
	{
		Q_UNUSED( event );
		m_hovered = true;
		update();
		if( m_hoverCallback )
		{
			m_hoverCallback( this );
		}
	}

	void leaveEvent( QEvent* event ) override
	{
		Q_UNUSED( event );
		m_hovered = false;
		update();
	}

	void mouseReleaseEvent( QMouseEvent* event ) override
	{
		if( event->button() == Qt::LeftButton && rect().contains( event->pos() ) && m_callback && m_hasSubMenu == false )
		{
			m_callback();
		}
	}

	void paintEvent( QPaintEvent* event ) override
	{
		Q_UNUSED( event );

		QPainter painter( this );
		painter.setRenderHint( QPainter::Antialiasing );

		if( m_hovered )
		{
			QPainterPath hoverPath;
			hoverPath.addRoundedRect( QRectF( rect() ).adjusted( 4, 1, -4, -1 ), 6, 6 );
			painter.fillPath( hoverPath, QColor( QStringLiteral( "#e8f0fe" ) ) );
		}

		painter.setPen( m_hovered ? QColor( QStringLiteral( "#1a73e8" ) ) : QColor( QStringLiteral( "#202124" ) ) );
		painter.drawText( rect().adjusted( 14, 0, m_hasSubMenu ? -28 : -14, 0 ), Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine, m_text );

		if( m_hasSubMenu )
		{
			painter.drawText( rect().adjusted( 0, 0, -12, 0 ), Qt::AlignVCenter | Qt::AlignRight, QStringLiteral( ">" ) );
		}
	}

private:
	QString m_text;
	std::function<void()> m_callback;
	std::function<void( FeatureMenuItem* )> m_hoverCallback;
	bool m_hasSubMenu{false};
	bool m_hovered{false};
};



class FeatureMenuPopup : public QWidget
{
public:
	explicit FeatureMenuPopup( QWidget* parent = nullptr,
						   Qt::WindowFlags flags = Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint ) :
		QWidget( parent, flags ),
		m_mainPanel( new RoundedPanel( this ) ),
		m_layout( new QVBoxLayout( m_mainPanel ) )
	{
		setAttribute( Qt::WA_TranslucentBackground );
		setAttribute( Qt::WA_DeleteOnClose );
		setMouseTracking( true );
		m_layout->setContentsMargins( 6, 6, 6, 6 );
		m_layout->setSpacing( 0 );
	}

	void addItem( const QString& label, std::function<void()> callback )
	{
		m_layout->addWidget( new FeatureMenuItem( label, [this, callback = std::move( callback )]() {
			closeRootMenu();
			callback();
		}, [this]( FeatureMenuItem* ) {
			closeSubMenu();
		}, false, this ) );
	}

	void addSubMenu( const QString& label, const QVector<QPair<QString, std::function<void()>>>& items )
	{
		m_layout->addWidget( new FeatureMenuItem( label, {}, [this, items]( FeatureMenuItem* item ) {
			showSubMenu( item, items );
		}, true, this ) );
	}

	void closeRootMenu()
	{
		closeSubMenu();
		close();
	}

	void popup( const QPoint& globalPos )
	{
		m_mainPanel->adjustSize();
		const auto pos = adjustedPopupPosition( globalPos, m_mainPanel->size() );
		setGeometry( QRect( pos, m_mainPanel->size() ) );
		m_mainPanel->move( 0, 0 );
		m_mainPanel->show();
		show();
	}

protected:
	void closeEvent( QCloseEvent* event ) override
	{
		closeSubMenu();
		QWidget::closeEvent( event );
	}

private:
	static QPoint adjustedPopupPosition( const QPoint& globalPos, const QSize& popupSize )
	{
		auto screen = QGuiApplication::screenAt( globalPos );
		if( screen == nullptr )
		{
			screen = QGuiApplication::primaryScreen();
		}

		if( screen == nullptr )
		{
			return globalPos;
		}

		const auto screenGeometry = screen->availableGeometry();
		auto x = globalPos.x();
		auto y = globalPos.y();

		if( x + popupSize.width() > screenGeometry.right() )
		{
			x = screenGeometry.right() - popupSize.width();
		}
		if( y + popupSize.height() > screenGeometry.bottom() )
		{
			y = screenGeometry.bottom() - popupSize.height();
		}

		x = qMax( screenGeometry.left(), x );
		y = qMax( screenGeometry.top(), y );

		return { x, y };
	}

	void showSubMenu( FeatureMenuItem* item, const QVector<QPair<QString, std::function<void()>>>& items )
	{
		if( m_subMenuAnchor == item && m_subMenuPanel )
		{
			return;
		}

		closeSubMenu();

		m_subMenuPanel = new RoundedPanel( this );
		m_subMenuAnchor = item;
		auto subMenuLayout = new QVBoxLayout( m_subMenuPanel );
		subMenuLayout->setContentsMargins( 6, 6, 6, 6 );
		subMenuLayout->setSpacing( 0 );

		for( const auto& entry : items )
		{
			subMenuLayout->addWidget( new FeatureMenuItem( entry.first, [this, callback = entry.second]() {
				closeRootMenu();
				callback();
			}, {}, false, m_subMenuPanel ) );
		}

		m_mainPanel->adjustSize();
		m_subMenuPanel->adjustSize();

		const auto mainGlobalPos = m_mainPanel->mapToGlobal( QPoint( 0, 0 ) );
		const auto itemPos = item->mapTo( m_mainPanel, QPoint( 0, 0 ) );
		auto subMenuGlobalPos = mainGlobalPos + QPoint( m_mainPanel->width(), itemPos.y() );

		auto screen = QGuiApplication::screenAt( subMenuGlobalPos );
		if( screen == nullptr )
		{
			screen = QGuiApplication::primaryScreen();
		}

		if( screen )
		{
			const auto screenGeometry = screen->availableGeometry();
			if( subMenuGlobalPos.x() + m_subMenuPanel->width() > screenGeometry.right() )
			{
				subMenuGlobalPos.setX( mainGlobalPos.x() - m_subMenuPanel->width() );
			}
			if( subMenuGlobalPos.y() + m_subMenuPanel->height() > screenGeometry.bottom() )
			{
				subMenuGlobalPos.setY( screenGeometry.bottom() - m_subMenuPanel->height() );
			}
			subMenuGlobalPos.setX( qMax( screenGeometry.left(), subMenuGlobalPos.x() ) );
			subMenuGlobalPos.setY( qMax( screenGeometry.top(), subMenuGlobalPos.y() ) );
		}

		const QRect mainRect( mainGlobalPos, m_mainPanel->size() );
		const QRect subMenuRect( subMenuGlobalPos, m_subMenuPanel->size() );
		const auto overlayRect = mainRect.united( subMenuRect );
		setGeometry( overlayRect );
		m_mainPanel->move( mainGlobalPos - overlayRect.topLeft() );
		m_subMenuPanel->move( subMenuGlobalPos - overlayRect.topLeft() );
		m_subMenuPanel->show();
	}

	void closeSubMenu()
	{
		if( m_subMenuPanel )
		{
			delete m_subMenuPanel;
		}
		m_subMenuPanel = nullptr;
		m_subMenuAnchor = nullptr;

		if( isVisible() )
		{
			const auto mainGlobalPos = m_mainPanel->mapToGlobal( QPoint( 0, 0 ) );
			setGeometry( QRect( mainGlobalPos, m_mainPanel->size() ) );
			m_mainPanel->move( 0, 0 );
		}
	}

	RoundedPanel* m_mainPanel{};
	QVBoxLayout* m_layout{};
	RoundedPanel* m_subMenuPanel{};
	FeatureMenuItem* m_subMenuAnchor{};
};



class ComputerToolTipPopup : public RoundedPopup
{
public:
	explicit ComputerToolTipPopup( const QString& text, QWidget* parent = nullptr ) :
		RoundedPopup( parent, Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint )
	{
		setAttribute( Qt::WA_TransparentForMouseEvents );

		auto layout = new QVBoxLayout( this );
		layout->setContentsMargins( 10, 7, 10, 7 );

		auto label = new QLabel( text, this );
		label->setTextFormat( Qt::RichText );
		label->setStyleSheet( QStringLiteral( "background: transparent; color: #202124;" ) );
		layout->addWidget( label );
	}

};
}


ComputerMonitoringWidget::ComputerMonitoringWidget( QWidget *parent ) :
	FlexibleListView( parent )
{
	const auto computerMonitoringThumbnailSpacing = VeyonCore::config().computerMonitoringThumbnailSpacing();

	setContextMenuPolicy( Qt::CustomContextMenu );
	setAcceptDrops( true );
	setDragEnabled( true );
	setDragDropMode( QAbstractItemView::DropOnly );
	setDefaultDropAction( Qt::MoveAction );
	setSelectionMode( QAbstractItemView::ExtendedSelection );
	setFlow( QListView::LeftToRight );
	setMouseTracking( true );
	viewport()->setMouseTracking( true );
	setWrapping( true );
	setResizeMode( QListView::Adjust );
	setSpacing( computerMonitoringThumbnailSpacing  );
	setViewMode( QListView::IconMode );
	setUniformItemSizes( true );
	setSelectionRectVisible( true );
	setItemDelegate(new ComputerItemDelegate(this));

	setUidRole( ComputerControlListModel::UidRole );

	connect( this, &QListView::doubleClicked, this, &ComputerMonitoringWidget::runDoubleClickFeature );
	connect( this, &QListView::customContextMenuRequested,
			 this, [this]( QPoint pos ) { showContextMenu( mapToGlobal( pos ) ); } );

	connect(dataModel(), &ComputerMonitoringModel::dataChanged,
			this, &ComputerMonitoringWidget::handleSizeHintChanges);

	initializeView( this );

	setModel( dataModel() );
}



ComputerControlInterfaceList ComputerMonitoringWidget::selectedComputerControlInterfaces() const
{
	ComputerControlInterfaceList computerControlInterfaces;

	const auto selectedIndices = selectionModel()->selectedIndexes(); // clazy:exclude=inefficient-qlist
	computerControlInterfaces.reserve( selectedIndices.size() );

	for( const auto& index : selectedIndices )
	{
		computerControlInterfaces.append( model()->data( index, ComputerControlListModel::ControlInterfaceRole )
											  .value<ComputerControlInterface::Pointer>() );
	}

	return computerControlInterfaces;
}



bool ComputerMonitoringWidget::performIconSizeAutoAdjust()
{
	if (isVisible() == false)
	{
		initiateIconSizeAutoAdjust();
		return false;
	}

	if( ComputerMonitoringView::performIconSizeAutoAdjust() == false)
	{
		return false;
	}

	m_ignoreResizeEvent = true;

	auto size = iconSize().width();

	setComputerScreenSize( size );
	QApplication::processEvents();

	while( verticalScrollBar()->isVisible() == false &&
		   horizontalScrollBar()->isVisible() == false &&
		   size < MaximumComputerScreenSize )
	{
		size += IncreaseIconSizeStepSize;
		setComputerScreenSize( size );
		QApplication::processEvents();
	}

	while( ( verticalScrollBar()->isVisible() ||
			 horizontalScrollBar()->isVisible() ) &&
		   size > MinimumComputerScreenSize )
	{
		const auto scrollMax = std::max(verticalScrollBar()->maximum(), horizontalScrollBar()->maximum());
		size -= std::max(DecreaseIconSizeMinimumStepSize, std::min(size / 10, scrollMax / 5));
		setComputerScreenSize( size );
		QApplication::processEvents();
	}

	Q_EMIT computerScreenSizeAdjusted( size );

	m_ignoreResizeEvent = false;

	return true;
}



void ComputerMonitoringWidget::handleSizeHintChanges(const QModelIndex& topLeft, const QModelIndex &bottomRight, const QVector<int>& roles)
{
	Q_UNUSED(topLeft);
	Q_UNUSED(bottomRight);

	if (roles.contains(Qt::SizeHintRole))
	{
		doItemsLayout();
	}
}



void ComputerMonitoringWidget::setUseCustomComputerPositions( bool enabled )
{
	setFlexible( enabled );
	initiateIconSizeAutoAdjust();
}



void ComputerMonitoringWidget::alignComputers()
{
	alignToGrid();
}



void ComputerMonitoringWidget::showContextMenu( QPoint globalPos )
{
	closeToolTipPopup();

	if( m_featurePopup )
	{
		m_featurePopup->close();
	}

	auto popup = new FeatureMenuPopup( this );
	m_featurePopup = popup;
	connect( popup, &QObject::destroyed, this, [this, popup]() {
		if( m_featurePopup == popup )
		{
			m_featurePopup = nullptr;
		}
	} );

	populateFeatureMenu( popup, selectedComputerControlInterfaces() );
	popup->popup( globalPos );
}



void ComputerMonitoringWidget::setIconSize( const QSize& size )
{
	QAbstractItemView::setIconSize( size );
}



void ComputerMonitoringWidget::setColors( const QColor& backgroundColor, const QColor& textColor )
{
	auto pal = palette();
	if (VeyonCore::useDarkMode())
	{
		pal.setColor(QPalette::Base, textColor);
		pal.setColor(QPalette::Text, backgroundColor);
	}
	else
	{
		pal.setColor(QPalette::Base, backgroundColor);
		pal.setColor(QPalette::Text, textColor);
	}
	setPalette( pal );
}



QJsonArray ComputerMonitoringWidget::saveComputerPositions()
{
	return savePositions();
}



bool ComputerMonitoringWidget::useCustomComputerPositions()
{
	return flexible();
}



void ComputerMonitoringWidget::loadComputerPositions( const QJsonArray& positions )
{
	loadPositions( positions );
}



void ComputerMonitoringWidget::populateFeatureMenu( QWidget* popupWidget, const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( popupWidget == nullptr )
	{
		return;
	}
	auto popup = static_cast<FeatureMenuPopup *>( popupWidget );

	for( const auto& feature : master()->features() )
	{
		if( feature.testFlag( Feature::Flag::Meta ) )
		{
			continue;
		}

		if( feature.displayNameActive().isEmpty() == false &&
			isFeatureOrRelatedFeatureActive( computerControlInterfaces, feature.uid() ) )
		{
			popup->addItem( feature.displayNameActive(), [=, this]() {
				runFeature( feature );
			} );
		}
		else
		{
			const auto subFeatures = master()->subFeatures( feature.uid() );
			if( subFeatures.isEmpty() )
			{
				popup->addItem( feature.displayName(), [=, this]() {
					runFeature( feature );
				} );
			}
			else
			{
				QVector<QPair<QString, std::function<void()>>> subMenuItems;
				subMenuItems.reserve( subFeatures.size() );
				for( const auto& subFeature : subFeatures )
				{
					subMenuItems.append( { subFeature.displayName(), [=, this]() {
						runFeature( subFeature );
					} } );
				}
				popup->addSubMenu( feature.displayName(), subMenuItems );
			}
		}
	}
}



void ComputerMonitoringWidget::closeToolTipPopup()
{
	if( m_toolTipPopup )
	{
		m_toolTipPopup->close();
	}
	m_toolTipPopup = nullptr;
	m_toolTipIndex = QModelIndex();
}



void ComputerMonitoringWidget::runDoubleClickFeature( const QModelIndex& index )
{
	const Feature& feature = VeyonCore::featureManager().feature( VeyonCore::config().computerDoubleClickFeature() );

	if( index.isValid() && feature.isValid() )
	{
		selectionModel()->select( index, QItemSelectionModel::SelectCurrent );
		runFeature( feature );
	}
}



void ComputerMonitoringWidget::resetIgnoreNumberOfMouseEvents( )
{
	m_ignoreNumberOfMouseEvents = IgnoredNumberOfMouseEventsWhileHold;
}



void ComputerMonitoringWidget::runMousePressAndHoldFeature( )
{
	m_mousePressAndHold.stop();
	const auto selectedInterfaces = selectedComputerControlInterfaces();
	if( !m_ignoreMousePressAndHoldEvent &&
		selectedInterfaces.count() > 0 &&
		selectedInterfaces.count() < 2 &&
		selectedInterfaces.first()->state() == ComputerControlInterface::State::Connected &&
		selectedInterfaces.first()->hasValidFramebuffer() )
	{
		m_ignoreMousePressAndHoldEvent = true;
		resetIgnoreNumberOfMouseEvents();
		delete m_computerZoomWidget;
		m_computerZoomWidget = new ComputerZoomWidget( selectedInterfaces.first()  );
		connect( m_computerZoomWidget, &ComputerZoomWidget::keypressInComputerZoomWidget, this, &ComputerMonitoringWidget::resetIgnoreNumberOfMouseEvents );
	}
}



void ComputerMonitoringWidget::stopMousePressAndHoldFeature( )
{
	disconnect( m_computerZoomWidget, &ComputerZoomWidget::keypressInComputerZoomWidget, this, &ComputerMonitoringWidget::resetIgnoreNumberOfMouseEvents );
	m_ignoreMousePressAndHoldEvent = false;
	m_ignoreNumberOfMouseEvents = 0;
	m_computerZoomWidget->close();
	delete m_computerZoomWidget;
	m_computerZoomWidget = nullptr;
}



void ComputerMonitoringWidget::mousePressEvent( QMouseEvent* event )
{
	if( event->buttons() == Qt::LeftButton && indexAt(event->pos()).isValid() )
	{
		if( !m_ignoreMousePressAndHoldEvent )
		{
			m_mousePressAndHold.setInterval( 500 );
			m_mousePressAndHold.start();
			connect(&m_mousePressAndHold, &QTimer::timeout, this, &ComputerMonitoringWidget::runMousePressAndHoldFeature );
		}
	}
	QListView::mousePressEvent( event );
}



void ComputerMonitoringWidget::mouseReleaseEvent( QMouseEvent* event )
{
	m_mousePressAndHold.stop();
	if ( m_ignoreMousePressAndHoldEvent )
	{
		stopMousePressAndHoldFeature();
	}
	QListView::mouseReleaseEvent( event );
}



void ComputerMonitoringWidget::mouseMoveEvent( QMouseEvent* event )
{
	m_mousePressAndHold.stop();
	if( m_toolTipPopup && indexAt( event->pos() ) != m_toolTipIndex )
	{
		closeToolTipPopup();
	}
	if ( m_ignoreNumberOfMouseEvents <= 0 )
	{
		if ( m_ignoreMousePressAndHoldEvent )
		{
			stopMousePressAndHoldFeature();
		}

		QListView::mouseMoveEvent( event );
	} else
	{
		m_ignoreNumberOfMouseEvents--;
		event->accept();
	}
}



bool ComputerMonitoringWidget::viewportEvent( QEvent* event )
{
	if( event->type() == QEvent::Leave )
	{
		closeToolTipPopup();
	}
	else if( event->type() == QEvent::MouseMove )
	{
		auto mouseEvent = static_cast<QMouseEvent *>( event );
		if( m_toolTipPopup && indexAt( mouseEvent->pos() ) != m_toolTipIndex )
		{
			closeToolTipPopup();
		}
	}

	if( event->type() == QEvent::ToolTip )
	{
		auto helpEvent = static_cast<QHelpEvent *>( event );
		const auto index = indexAt( helpEvent->pos() );
		const auto text = index.data( Qt::ToolTipRole ).toString();

		if( text.isEmpty() )
		{
			closeToolTipPopup();
			QToolTip::hideText();
			event->ignore();
			return true;
		}

		closeToolTipPopup();

		auto popup = new ComputerToolTipPopup( text, this );
		m_toolTipPopup = popup;
		m_toolTipIndex = index;
		connect( popup, &QObject::destroyed, this, [this, popup]() {
			if( m_toolTipPopup == popup )
			{
				m_toolTipPopup = nullptr;
				m_toolTipIndex = QModelIndex();
			}
		} );
		popup->popup( helpEvent->globalPos() + QPoint( 12, 18 ) );
		QToolTip::hideText();
		return true;
	}

	return FlexibleListView::viewportEvent( event );
}



void ComputerMonitoringWidget::resizeEvent( QResizeEvent* event )
{
	FlexibleListView::resizeEvent( event );

	if( m_ignoreResizeEvent == false )
	{
		initiateIconSizeAutoAdjust();
	}
}



void ComputerMonitoringWidget::showEvent( QShowEvent* event )
{
	if( event->spontaneous() == false )
	{
		initiateIconSizeAutoAdjust();
	}

	FlexibleListView::showEvent( event );
}



void ComputerMonitoringWidget::wheelEvent( QWheelEvent* event )
{
	if( m_ignoreWheelEvent == false &&
		event->modifiers().testFlag( Qt::ControlModifier ) )
	{
		setComputerScreenSize( iconSize().width() + event->angleDelta().y() / 8 );

		Q_EMIT computerScreenSizeAdjusted( computerScreenSize() );

		event->accept();
	}
	else
	{
		QListView::wheelEvent( event );
	}
}
