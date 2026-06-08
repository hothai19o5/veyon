/*
 * ComputerItemDelegate.cpp - implementation of ComputerItemDelegate

 * Copyright (c) 2025-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QPainter>
#include <QPainterPath>

#include "ComputerControlListModel.h"
#include "ComputerItemDelegate.h"
#include "FeatureManager.h"
#include "IconUtils.h"


ComputerItemDelegate::ComputerItemDelegate(QObject* parent) :
	QStyledItemDelegate(parent)
{
	initFeaturePixmaps();
}



void ComputerItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	drawComputerCard(painter, option, index);
}



QSize ComputerItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	Q_UNUSED(option);

	if( index.model() )
	{
		const auto image = index.model()->data(index, Qt::DecorationRole).value<QImage>();
		if( image.isNull() == false )
		{
			return image.size() + QSize( ( CardPadding + ShadowSize ) * 2,
									 LabelBarHeight + ( CardPadding + ShadowSize ) * 2 );
		}
	}

	return { 260, 190 };
}



void ComputerItemDelegate::drawComputerCard(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if( painter == nullptr || index.isValid() == false || index.model() == nullptr )
	{
		return;
	}

	const auto image = index.model()->data(index, Qt::DecorationRole).value<QImage>();
	const auto label = index.model()->data(index, Qt::DisplayRole).toString();
	const auto controlInterface = index.model()->data(index, ComputerControlListModel::ControlInterfaceRole).value<ComputerControlInterface::Pointer>();

	painter->save();
	painter->setRenderHint(QPainter::Antialiasing);

	const auto cardRect = option.rect.adjusted(CardPadding, CardPadding, -CardPadding, -CardPadding);
	const QRect thumbnailRect(cardRect.left(), cardRect.top(), cardRect.width(), cardRect.height() - LabelBarHeight);
	const QRect labelRect(cardRect.left(), thumbnailRect.bottom() + 1, cardRect.width(), LabelBarHeight);
	const auto selected = option.state.testFlag(QStyle::State_Selected);
	const auto hovered = option.state.testFlag(QStyle::State_MouseOver);

	for( int i = ShadowSize; i > 0; --i )
	{
		const auto alpha = 24 - i;
		painter->setPen(Qt::NoPen);
		painter->setBrush(QColor(0, 0, 0, qMax(0, alpha)));
		painter->drawRoundedRect(cardRect.adjusted(-i / 2, 2, i / 2, i + 2), CardRadius + i / 2, CardRadius + i / 2);
	}

	QPainterPath cardPath;
	cardPath.addRoundedRect(cardRect, CardRadius, CardRadius);
	painter->fillPath(cardPath, QColor(QStringLiteral("#ffffff")));
	painter->setClipPath(cardPath);

	QPainterPath thumbnailPath;
	thumbnailPath.moveTo(thumbnailRect.left() + CardRadius, thumbnailRect.top());
	thumbnailPath.lineTo(thumbnailRect.right() - CardRadius, thumbnailRect.top());
	thumbnailPath.arcTo(thumbnailRect.right() - CardRadius * 2, thumbnailRect.top(),
						CardRadius * 2, CardRadius * 2, 90, -90);
	thumbnailPath.lineTo(thumbnailRect.right(), thumbnailRect.bottom());
	thumbnailPath.lineTo(thumbnailRect.left(), thumbnailRect.bottom());
	thumbnailPath.lineTo(thumbnailRect.left(), thumbnailRect.top() + CardRadius);
	thumbnailPath.arcTo(thumbnailRect.left(), thumbnailRect.top(),
						CardRadius * 2, CardRadius * 2, 180, -90);
	thumbnailPath.closeSubpath();
	painter->fillPath(thumbnailPath, QColor(QStringLiteral("#202124")));

	if( image.isNull() == false )
	{
		const auto scaled = QPixmap::fromImage(image).scaled(thumbnailRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
		const QRect sourceRect((scaled.width() - thumbnailRect.width()) / 2,
						   (scaled.height() - thumbnailRect.height()) / 2,
						   thumbnailRect.width(), thumbnailRect.height());

		painter->save();
		painter->setClipPath(thumbnailPath);
		painter->drawPixmap(thumbnailRect.topLeft(), scaled, sourceRect);
		painter->restore();
	}

	painter->setPen(Qt::NoPen);
	painter->setBrush(QColor(QStringLiteral("#ffffff")));
	painter->drawRect(labelRect);

	painter->setClipping(false);
	painter->setPen(QColor(QStringLiteral("#dadce0")));
	painter->setBrush(Qt::NoBrush);
	painter->drawRoundedRect(QRectF(cardRect).adjusted(0.5, 0.5, -0.5, -0.5), CardRadius, CardRadius);

	if( hovered || selected )
	{
		const auto penWidth = selected ? 2 : 1;
		painter->setPen(QPen(QColor(selected ? QStringLiteral("#1a73e8") : QStringLiteral("#bdc1c6")), penWidth));
		const auto inset = penWidth / 2.0;
		painter->drawRoundedRect(QRectF(cardRect).adjusted(inset, inset, -inset, -inset), CardRadius, CardRadius);
	}

	QFont labelFont(option.font);
	labelFont.setBold(true);
	painter->setFont(labelFont);
	painter->setPen(QColor(QStringLiteral("#202124")));
	painter->drawText(labelRect.adjusted(12, 0, -12, 0), Qt::AlignCenter | Qt::TextSingleLine, label);

	drawFeatureIcons(painter, thumbnailRect.topLeft(), controlInterface);

	painter->restore();
}



void ComputerItemDelegate::initFeaturePixmaps()
{
	for (const auto& feature : VeyonCore::featureManager().features() )
	{
		if (feature.testFlag(Feature::Flag::Master) && !feature.iconUrl().isEmpty())
		{
			m_featurePixmaps[feature.uid()] = IconUtils::iconFromUrl( feature.iconUrl() ).pixmap( QSize( OverlayIconSize, OverlayIconSize ) );
		}
	}
}



void ComputerItemDelegate::drawFeatureIcons(QPainter* painter, const QPoint& pos, ComputerControlInterface::Pointer controlInterface) const
{
	if (painter &&
		controlInterface &&
		controlInterface->state() == ComputerControlInterface::State::Connected)
	{
		auto count = 0;
		for (const auto& feature : controlInterface->activeFeatures())
		{
			if (m_featurePixmaps.contains(feature))
			{
				count++;
			}
		}

		if (count == 0)
		{
			return;
		}

		int x = pos.x() + OverlayIconsPadding;
		const int y = pos.y() + OverlayIconsPadding;

		painter->setRenderHint(QPainter::Antialiasing);
		painter->setBrush(QColor(255, 255, 255, 220));
		painter->setPen(QColor(QStringLiteral("#dadce0")));
		painter->drawRoundedRect(QRect(x, y, count * OverlayIconSize + qMax(0, count - 1) * OverlayIconSpacing, OverlayIconSize),
								 OverlayIconsRadius, OverlayIconsRadius);

		for (const auto& feature : controlInterface->activeFeatures())
		{
			const auto it = m_featurePixmaps.find(feature);
			if (it != m_featurePixmaps.constEnd())
			{
				painter->drawPixmap(QPoint(x, y), *it);
				x += OverlayIconSize + OverlayIconSpacing;
			}
		}
	}
}
