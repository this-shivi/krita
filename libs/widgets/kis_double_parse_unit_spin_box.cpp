/*
 *  Copyright (c) 2016 Laurent Valentin Jospin <laurent.valentin@famillejospin.ch>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_double_parse_unit_spin_box.h"
#include "kis_spin_box_unit_manager.h"

#include <QLineEdit>

class Q_DECL_HIDDEN KisDoubleParseUnitSpinBox::Private
{
public:
    Private(double low, double up, double step)
        : lowerInPoints(low),
        upperInPoints(up),
        stepInPoints(step),
        unit(KoUnit(KoUnit::Point)),
        unitManager(new KisSpinBoxUnitManager()),
        defaultUnitManager(unitManager),
        isDeleting(false)
    {
    }

    double lowerInPoints; ///< lowest value in points
    double upperInPoints; ///< highest value in points
    double stepInPoints;  ///< step in points
    KoUnit unit;

    KisSpinBoxUnitManager* unitManager; //manage more units than permitted by KoUnit.
    KisSpinBoxUnitManager* defaultUnitManager; //the default unit manager is the one the spinbox rely on and go back to if a connected unit manager is destroyed before the spinbox.

    bool isDeleting;
};

KisDoubleParseUnitSpinBox::KisDoubleParseUnitSpinBox(QWidget *parent) :
    KisDoubleParseSpinBox(parent),
    d(new Private(-9999, 9999, 1))
{
    setUnit( KoUnit(KoUnit::Point) );
    setAlignment( Qt::AlignRight );

    connect(this, SIGNAL(valueChanged( double )), SLOT(privateValueChanged()));
    connect(lineEdit(), SIGNAL(textChanged(QString)),
            this, SLOT(detectUnitChanges()) );


}

KisDoubleParseUnitSpinBox::~KisDoubleParseUnitSpinBox()
{
    d->isDeleting = true;
    delete d->defaultUnitManager;
    delete d;
}

void KisDoubleParseUnitSpinBox::setUnitManager(KisSpinBoxUnitManager* unitManager)
{
    qreal oldVal = d->unitManager->getReferenceValue(KisDoubleParseSpinBox::value());
    QString oldSymbol = d->unitManager->getApparentUnitSymbol();

    if (oldSymbol == unitManager->getApparentUnitSymbol() &&
            d->unitManager->getUnitDimensionType() == unitManager->getUnitDimensionType())
    {
        d->unitManager = unitManager; //set the new unitmanager anyway, since it may be a subclass, so change the behavior anyway.
        return;
    }

    qreal newVal;

    if (d->unitManager->getUnitDimensionType() != unitManager->getUnitDimensionType()) {
        //dimension is the same, calculate the new value
        newVal = unitManager->getApparentValue(oldVal);
    } else {
        newVal = unitManager->getApparentValue(d->lowerInPoints);
    }

    double newMin = unitManager->getApparentValue(d->lowerInPoints);
    double newMax = unitManager->getApparentValue(d->upperInPoints);
    double newStep = unitManager->getApparentValue(d->stepInPoints);

    if (unitManager->getApparentUnitSymbol() == KoUnit(KoUnit::Pixel).symbol()) {
        // limit the pixel step by 1.0
        newStep = qMax(qreal(1.0), newStep);
    }

    KisDoubleParseSpinBox::setMinimum(newMin);
    KisDoubleParseSpinBox::setMaximum(newMax);
    KisDoubleParseSpinBox::setSingleStep(newStep);

    if (d->unitManager != d->defaultUnitManager) {
        disconnect(d->unitManager, &QObject::destroyed,
                   this, &KisDoubleParseUnitSpinBox::disconnectExternalUnitManager); //there's no dependance anymore.
    }

    d->unitManager = unitManager;

    connect(d->unitManager, &QObject::destroyed,
            this, &KisDoubleParseUnitSpinBox::disconnectExternalUnitManager);

    KisDoubleParseSpinBox::setValue(newVal);
}

void KisDoubleParseUnitSpinBox::changeValue( double newValue )
{
    if (d->unitManager->getApparentValue(newValue) == KisDoubleParseSpinBox::value()) {
        return;
    }

    KisDoubleParseSpinBox::setValue( d->unitManager->getApparentValue(newValue) );
}

void KisDoubleParseUnitSpinBox::setUnit( const KoUnit & unit)
{
    if (d->unitManager->getUnitDimensionType() != KisSpinBoxUnitManager::LENGTH) {
        d->unitManager->setUnitDim(KisSpinBoxUnitManager::LENGTH); //setting the unit using a KoUnit mean you want to use a length.
    }

    setUnit(unit.symbol());
    d->unit = unit;
}
void KisDoubleParseUnitSpinBox::setUnit(const QString &symbol)
{

    double oldValue = d->unitManager->getReferenceValue(KisDoubleParseSpinBox::value());
    QString oldSymbol = d->unitManager->getApparentUnitSymbol();

    if (symbol == oldSymbol) {
        return;
    }

    d->unitManager->setApparentUnitFromSymbol(symbol);

    if (d->unitManager->getApparentUnitSymbol() == oldSymbol) { //the setApparentUnitFromSymbol is a bit clever, for example in regard of Casesensitivity. So better check like this.
        return;
    }

    KisDoubleParseSpinBox::setMinimum( d->unitManager->getApparentValue( d->lowerInPoints ) );
    KisDoubleParseSpinBox::setMaximum( d->unitManager->getApparentValue( d->upperInPoints ) );

    qreal step = d->unitManager->getApparentValue( d->stepInPoints );

    if (symbol == KoUnit(KoUnit::Pixel).symbol()) {
        // limit the pixel step by 1.0
        step = qMax(qreal(1.0), step);
    }

    KisDoubleParseSpinBox::setSingleStep( step );
    KisDoubleParseSpinBox::setValue( d->unitManager->getApparentValue( oldValue ) );

}


void KisDoubleParseUnitSpinBox::setDimensionType(int dim)
{
    if (!KisSpinBoxUnitManager::isUnitId(dim)) {
        return;
    }

    d->unitManager->setUnitDim((KisSpinBoxUnitManager::UnitDimension) dim);
}

double KisDoubleParseUnitSpinBox::value( ) const
{
    return d->unitManager->getReferenceValue( KisDoubleParseSpinBox::value() );
}

void KisDoubleParseUnitSpinBox::setMinimum(double min)
{
    d->lowerInPoints = min;
    KisDoubleParseSpinBox::setMinimum( d->unitManager->getApparentValue( min ) );
}

void KisDoubleParseUnitSpinBox::setMaximum(double max)
{
    d->upperInPoints = max;
    KisDoubleParseSpinBox::setMaximum( d->unitManager->getApparentValue( max ) );
}

void KisDoubleParseUnitSpinBox::setLineStep(double step)
{
    d->stepInPoints = d->unitManager->getReferenceValue(step);
    KisDoubleParseSpinBox::setSingleStep( step );
}

void KisDoubleParseUnitSpinBox::setLineStepPt(double step)
{
    d->stepInPoints = step;
    KisDoubleParseSpinBox::setSingleStep( d->unitManager->getApparentValue( step ) );
}


void KisDoubleParseUnitSpinBox::setMinMaxStep( double min, double max, double step )
{
    setMinimum( min );
    setMaximum( max );
    setLineStepPt( step );
}


QValidator::State KisDoubleParseUnitSpinBox::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos);

    QRegExp regexp ("([ a-zA-Z]+)$"); // Letters or spaces at end
    const int res = input.indexOf( regexp );

    if ( res == -1 ) {
        // Nothing like an unit? The user is probably editing the unit
        return QValidator::Intermediate;
    }

    QString expr ( input.left( res ) );
    const QString unitName ( regexp.cap( 1 ).trimmed().toLower() );

    bool ok = true;
    bool interm = false;

    QValidator::State exprState = KisDoubleParseSpinBox::validate(expr, pos);

    if (exprState == QValidator::Invalid) {
        return exprState;
    } else if (exprState == QValidator::Intermediate) {
        interm = true;
    }

    //check if we can parse the unit.
    QStringList listOfSymbol = d->unitManager->getsUnitSymbolList();
    ok = listOfSymbol.contains(unitName);

    if (!ok || interm) {
        return QValidator::Intermediate;
    }

    return QValidator::Acceptable;
}

QString KisDoubleParseUnitSpinBox::textFromValue( double value ) const
{
    QString txt = KisDoubleParseSpinBox::textFromValue(value);
    if (!txt.endsWith(d->unitManager->getApparentUnitSymbol())) {
        txt += " " + d->unitManager->getApparentUnitSymbol();
    }
    return txt;
}

QString KisDoubleParseUnitSpinBox::veryCleanText() const
{

    return makeTextClean(cleanText());

}

double KisDoubleParseUnitSpinBox::valueFromText( const QString& str ) const
{

    QString txt = makeTextClean(str);

    return KisDoubleParseSpinBox::valueFromText(txt); //this function will take care of prefix (and don't mind if suffix has been removed.
}

void KisDoubleParseUnitSpinBox::privateValueChanged()
{
    emit valueChangedPt( value() );
}

QString KisDoubleParseUnitSpinBox::detectUnit()
{
    QString str = veryCleanText().trimmed(); //text with the new unit but not the old one.

    QRegExp regexp ("([ ]*[a-zA-Z]+[ ]*)$"); // Letters or spaces at end
    int res = str.indexOf( regexp );

    if (res > -1) {
        QString expr ( str.right( str.size() - res ) );
        expr = expr.trimmed();
        return expr;
    }

    return "";
}

void KisDoubleParseUnitSpinBox::detectUnitChanges()
{
    QString unitSymb = detectUnit();

    if (unitSymb.isEmpty()) {
        return;
    }

    setUnit(unitSymb);
    setValue(valueFromText(cleanText())); //change value keep the old value, but converted to new unit... which is different from the value the user entered in the new unit. So we need to set the new value.
}

QString KisDoubleParseUnitSpinBox::makeTextClean(QString const& txt) const
{
    QString expr = txt;
    QString symbol = d->unitManager->getApparentUnitSymbol();

    if ( expr.endsWith(suffix()) ) {
        expr.remove(expr.size()-suffix().size(), suffix().size());
    }

    expr = expr.trimmed();

    if ( expr.endsWith(symbol) ) {
        expr.remove(expr.size()-symbol.size(), symbol.size());
    }

    return expr;
}

void KisDoubleParseUnitSpinBox::disconnectExternalUnitManager()
{
    if (!d->isDeleting)
    {
        setUnitManager(d->defaultUnitManager); //go back to default unit manager.
    }
}
