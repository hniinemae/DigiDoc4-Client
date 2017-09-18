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

#include <QPainter>
#include <QPaintEvent>
#include <QWidget>

class Overlay : public QWidget
{
public:
    Overlay(QWidget *parent)
    : QWidget (parent)
    {
        setPalette(Qt::transparent);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setMinimumSize(parent->width(), parent->height());
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter( this );
        painter.setRenderHint( QPainter::Antialiasing );
        // #858585 80%
		//painter.setBrush( QBrush( QColor( QColor::from(0x85, 0x85, 0x85, 0xff * 0.8)) ) );
		painter.setBrush( QBrush( QColor( 0x85, 0x85, 0x85, (int)((double)0xff * 0.8) ) ) );
		painter.setPen( Qt::NoPen );
        painter.drawRect( rect() );
    }
};
