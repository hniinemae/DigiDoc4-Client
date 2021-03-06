/*
 * QDigiDoc4
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once

#include <memory>

#include <QtCore/QtGlobal>
#include <QFont>
#include <QPixmap>
#include <QStyle>

class PictureInterface
{
public:
	virtual ~PictureInterface() {};

	virtual void clearPicture() = 0;
	virtual void showPicture( const QPixmap &pixmap) = 0;
};

class StylesPrivate;
class Styles
{
public:
	enum Font {
		Bold,
		Condensed,
		CondensedBold,
		Regular
	};

	static QFont font( Font font, int size );
	static QFont font( Font font, int size, QFont::Weight weight );

private:
	explicit Styles();

	Q_DISABLE_COPY(Styles);
};
