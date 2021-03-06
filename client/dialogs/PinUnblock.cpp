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

#include "PinUnblock.h"
#include "ui_PinUnblock.h"

#include "Styles.h"
#include "effects/Overlay.h"

#include <QLabel>
#include <QList>
#include <QtGui/QRegExpValidator>


PinUnblock::PinUnblock(WorkMode mode, QWidget *parent, QSmartCardData::PinType type, short leftAttempts,
	QDate birthDate, QString personalCode)
	: QDialog(parent)
	, ui(new Ui::PinUnblock)
	, birthDate(birthDate)
	, personalCode(std::move(personalCode))
{
	init( mode, type, leftAttempts );
	adjustSize();
	setFixedSize( size() );
}

PinUnblock::~PinUnblock()
{
	delete ui;
}

void PinUnblock::init( WorkMode mode, QSmartCardData::PinType type, short leftAttempts )
{
	ui->setupUi( this );
	setWindowFlags( Qt::Dialog | Qt::FramelessWindowHint );
	setWindowModality( Qt::ApplicationModal );

	ui->unblock->setEnabled( false );
	connect( ui->unblock, &QPushButton::clicked, this, &PinUnblock::accept );
	connect( ui->cancel, &QPushButton::clicked, this, &PinUnblock::reject );
	connect( this, &PinUnblock::finished, this, &PinUnblock::close );

	QFont condensed12 = Styles::font(Styles::Condensed, 12);
	ui->labelPuk->setFont(condensed12);
	ui->labelPin->setFont(condensed12);
	ui->labelRepeat->setFont(condensed12);

	if( mode == PinUnblock::ChangePinWithPuk )
	{
		ui->labelNameId->setText( tr("%1 code change").arg( QSmartCardData::typeString( type ) ) );
		regexpFirstCode.setPattern(QStringLiteral("\\d{8,12}"));
		regexpNewCode.setPattern((type == QSmartCardData::Pin1Type) ? QStringLiteral("\\d{4,12}") : QStringLiteral("\\d{5,12}"));
		ui->unblock->setText( tr("CHANGE") );
	}
	if( mode == PinUnblock::UnBlockPinWithPuk )
	{
		ui->labelNameId->setText( tr("%1 unblocking").arg( QSmartCardData::typeString( type ) ) );
		regexpFirstCode.setPattern(QStringLiteral("\\d{8,12}"));
		regexpNewCode.setPattern((type == QSmartCardData::Pin1Type) ? QStringLiteral("\\d{4,12}") : QStringLiteral("\\d{5,12}"));
	}
	else if( mode == PinUnblock::PinChange )
	{
		ui->labelNameId->setText( tr("%1 code change").arg( QSmartCardData::typeString( type ) ) );
		ui->labelPuk->setText( tr( "VALID %1 CODE").arg( QSmartCardData::typeString( type ) ) );
		regexpFirstCode.setPattern((type == QSmartCardData::Pin1Type) ? QStringLiteral("\\d{4,12}") :
			(type == QSmartCardData::Pin2Type) ? QStringLiteral("\\d{5,12}") : QStringLiteral("\\d{8,12}"));
		regexpNewCode.setPattern((type == QSmartCardData::Pin1Type) ? QStringLiteral("\\d{4,12}") :
			(type == QSmartCardData::Pin2Type) ? QStringLiteral("\\d{5,12}") : QStringLiteral("\\d{8,12}"));
		ui->unblock->setText( tr("CHANGE") );
	}
	setWindowTitle(ui->labelNameId->text());
	ui->labelPin->setText( tr( "NEW %1 CODE").arg( QSmartCardData::typeString( type ) ) );
	ui->labelRepeat->setText( tr( "NEW %1 CODE AGAIN").arg( QSmartCardData::typeString( type ) ) );
	ui->pin->setAccessibleName(ui->labelPin->text().toLower());
	ui->repeat->setAccessibleName(ui->labelRepeat->text().toLower());
	ui->puk->setAccessibleName(ui->labelPuk->text().toLower());
	ui->unblock->setAccessibleName(ui->unblock->text().toLower());

	ui->puk->setValidator( new QRegExpValidator( regexpFirstCode, ui->puk ) );
	ui->pin->setValidator( new QRegExpValidator( regexpFirstCode, ui->pin ) );
	ui->repeat->setValidator( new QRegExpValidator( regexpFirstCode, ui->repeat ) );

	QFont condensed14(Styles::font(Styles::Condensed, 14));
	QFont headerFont(Styles::font(Styles::Regular, 18));
	QFont regular13(Styles::font(Styles::Regular, 13));
	headerFont.setWeight( QFont::Bold );
	ui->labelNameId->setFont( headerFont );
	ui->cancel->setFont( condensed14 );
	ui->unblock->setFont( condensed14 );
	ui->labelAttemptsLeft->setFont(regular13);
	ui->labelPinValidation->setFont(regular13);
	if( mode == PinUnblock::ChangePinWithPuk || mode == PinUnblock::UnBlockPinWithPuk )
		ui->labelAttemptsLeft->setText( tr("PUK remaining attempts: %1").arg( leftAttempts ) );
	else
		ui->labelAttemptsLeft->setText( tr("Remaining attempts: %1").arg( leftAttempts ) );
	ui->labelAttemptsLeft->setVisible( leftAttempts < 3 );

	initIntro(mode, type);

	connect(ui->puk, &QLineEdit::textChanged, this,
				[this](const QString &text)
				{
					isFirstCodeOk = regexpFirstCode.exactMatch(text);
					setUnblockEnabled();
				}
			);
	connect(ui->pin, &QLineEdit::textChanged, this,
				[this, type, mode](const QString &text)
				{
					ui->labelPinValidation->setText(QString());
					isNewCodeOk = regexpNewCode.exactMatch(text) && validatePin(text, type, mode);
					isRepeatCodeOk = !text.isEmpty() && ui->pin->text() == ui->repeat->text();
					setUnblockEnabled();
				}
			);
	connect(ui->repeat, &QLineEdit::textChanged, this,
				[this]()
				{
					isRepeatCodeOk = !ui->pin->text().isEmpty() && ui->pin->text() == ui->repeat->text();
					setUnblockEnabled();
				}
			);
}

void PinUnblock::initIntro(WorkMode mode, QSmartCardData::PinType type)
{
	struct LineText
	{
		bool replacePin;
		bool showBullet;
		QString text;
	};
	QList<LineText> labels;
	int pin = type == QSmartCardData::Pin2Type ? 2 : 1;
	QFont font = Styles::font(Styles::Regular, 14);

	if(mode == PinUnblock::ChangePinWithPuk)
	{
		labels << LineText{true, true, tr("New PIN%1 must be different from current PIN%1.")}
			<< (type == QSmartCardData::Pin2Type
				? LineText{false, true, tr("PIN2 code is used to digitally sign documents.")}
				: LineText{false, true, tr("PIN1 code is used for confirming the identity of a person.")}
			)
			<< LineText{true, true, tr("If you have forgotten PIN%1, but know PUK, then")}
			<< LineText{true, false, tr("here you can enter new PIN%1.")}
			<< LineText{false, true, tr("PUK code is written in the envelope, that is given")}
			<< LineText{false, false, tr("with the ID-card.")};
	}
	else if(mode == PinUnblock::UnBlockPinWithPuk)
	{
		labels << LineText{false, true, tr("To unblock the certificate you have to enter the PUK")}
			<< LineText{false, false, tr("code.")}
			<< LineText{false, true, tr("You can find your PUK code inside the ID-card codes")}
			<< LineText{false, false, tr("envelope.")}
			<< LineText{false, false, tr("PUK_LINE3")}
			<< LineText{true, true, tr("New PIN%1 must be different from current PIN%1.")}
			<< LineText{false, true, tr("If you have forgotten the PUK code for your ID card, please")}
			<< LineText{false, false, tr("VISIT_SERVICE_CENTRE")}
			<< LineText{false, false, tr("obtain new PIN codes.")};
	}
	else if(mode == PinUnblock::PinChange)
	{
		if(type == QSmartCardData::Pin2Type ||  type == QSmartCardData::Pin1Type)
		{
			// ui->label->setText(tr("ConditionsChangePIN2"));
			labels << LineText{true, true, tr("New PIN%1 must be different from current PIN%1.")}
				<< (type == QSmartCardData::Pin2Type
					? LineText{false, true, tr("PIN2 code is used to digitally sign documents.")}
					: LineText{false, true, tr("PIN1 code is used for confirming the identity of a person.")}
				)
				<< (type == QSmartCardData::Pin2Type
					? LineText{false, true, tr("If PIN2 is inserted incorrectly 3 times the signing")}
					: LineText{false, true, tr("If PIN1 is inserted incorrectly 3 times the identification")}
				)
				<< (type == QSmartCardData::Pin2Type
					? LineText{false, false, tr("signing certificate will be blocked and it will be impossible")}
					: LineText{false, false, tr("auth certificate will be blocked and it will be impossible")}
				)
				<< (type == QSmartCardData::Pin2Type
					? LineText{false, false, tr("to use ID-card to digital signing, until it is")}
					: LineText{false, false, tr("to use ID-card to verify identification, until it is")}
				)
				<< LineText{false, false, tr("unblocked via the PUK code.")};
		}
		else if(type == QSmartCardData::PukType)
		{
			// ui->label->setText(tr("ConditionsChangePUK"));
			labels << LineText{false, true, tr("PUK code is used for unblocking the certificates, when")}
				<< LineText{false, false, tr("PIN1 or PIN2 has been entered 3 times incorrectly.")}
				<< LineText{false, false, tr("PUK_INFO_LINE3")}
				<< LineText{false, true, tr("If you forget the PUK code or the certificates remain")}
				<< LineText{false, false, tr("PUK_BLOCKED_LINE2")}
				<< LineText{false, false, tr("new codes.")}
				<< LineText{false, false, tr("PUK_BLOCKED_LINE4")};
		}
	}

	struct InfoLine
	{
		QWidget *line;
		QLabel *bullet;
		QLabel *text;
	} lines[] = {{ ui->line1, ui->line1_bullet, ui->line1_text },
				{ ui->line2, ui->line2_bullet, ui->line2_text },
				{ ui->line3, ui->line3_bullet, ui->line3_text },
				{ ui->line4, ui->line4_bullet, ui->line4_text },
				{ ui->line5, ui->line5_bullet, ui->line5_text },
				{ ui->line6, ui->line6_bullet, ui->line6_text },
				{ ui->line7, ui->line7_bullet, ui->line7_text },
				{ ui->line8, ui->line8_bullet, ui->line8_text }};

	int lineOffset = 0;
	for(const LineText &lbl: labels)
	{
		auto text = lbl.replacePin ? QString(lbl.text).arg(pin) : lbl.text;

		if(text.trimmed().isEmpty())
			continue;

		if(lbl.showBullet)
			lines[lineOffset].bullet->setText(QStringLiteral("&bull;"));
		else
			lines[lineOffset].bullet->clear();
		lines[lineOffset].bullet->setFont(font);
		lines[lineOffset].text->setText(text);
		lines[lineOffset].text->setFont(font);

		lineOffset++;
	}

	// hide empty rows
	for(int i = lineOffset; i < 8; i++)
		lines[i].line->hide();
}

void PinUnblock::setUnblockEnabled()
{
	ui->iconLabelPuk->load(isFirstCodeOk ? QStringLiteral(":/images/icon_check.svg") : QString());
	ui->iconLabelPin->load(isNewCodeOk ? QStringLiteral(":/images/icon_check.svg") : QString());
	ui->iconLabelRepeat->load(isRepeatCodeOk ? QStringLiteral(":/images/icon_check.svg") : QString());
	ui->unblock->setEnabled( isFirstCodeOk && isNewCodeOk && isRepeatCodeOk );
}

int PinUnblock::exec()
{
	Overlay overlay( parentWidget() );
	overlay.show();
	auto rc = QDialog::exec();
	overlay.close();

	return rc;
}

QString PinUnblock::firstCodeText() const { return ui->puk->text(); }

QString PinUnblock::newCodeText() const { return ui->pin->text(); }

bool PinUnblock::validatePin(const QString& pin, QSmartCardData::PinType type, WorkMode mode)
{
	const static QString SEQUENCE_ASCENDING = QStringLiteral("1234567890123456789012");
	const static QString SEQUENCE_DESCENDING = QStringLiteral("0987654321098765432109");
	QString pinType = type == QSmartCardData::Pin1Type ? QStringLiteral("PIN1") : (type == QSmartCardData::Pin2Type ? QStringLiteral("PIN2") : QStringLiteral("PUK"));

	if(mode == PinUnblock::PinChange && pin == ui->puk->text())
	{
		ui->labelPinValidation->setText(tr("Current %1 code and new %1 code must be different").arg(pinType));
		return false;
	}
	if(SEQUENCE_ASCENDING.contains(pin))
	{
		ui->labelPinValidation->setText(tr("New %1 code can't be increasing sequence").arg(pinType));
		return false;
	}
	if(SEQUENCE_DESCENDING.contains(pin))
	{
		ui->labelPinValidation->setText(tr("New %1 code can't be decreasing sequence").arg(pinType));
		return false;
	}
	if(pin.count(pin[1]) == pin.length())
	{
		ui->labelPinValidation->setText(tr("New %1 code can't be sequence of same numbers").arg(pinType));
		return false;
	}
	if(personalCode.contains(pin))
	{
		ui->labelPinValidation->setText(tr("New %1 code can't be part of your personal code").arg(pinType));
		return false;
	}
	if(pin == birthDate.toString(QStringLiteral("yyyy")) || pin == birthDate.toString(QStringLiteral("ddMM")) || pin == birthDate.toString(QStringLiteral("MMdd")) ||
		 pin == birthDate.toString(QStringLiteral("yyyyMMdd")) || pin == birthDate.toString(QStringLiteral("ddMMyyyy")))
	{
		ui->labelPinValidation->setText(tr("New %1 code can't be your date of birth").arg(pinType));
		return false;
	}

	return true;
}
