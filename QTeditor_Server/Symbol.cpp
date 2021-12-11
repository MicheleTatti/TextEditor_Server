#include "Symbol.h"

Symbol::Symbol(QVector<int>& position, int counter, int siteId, QChar value, 
    bool bold, bool italic, bool underlined, int alignment,int textSize, QColor color,
    QString font): position(position), counter(counter), siteId(siteId), value(value),
    bold(bold), italic(italic), underlined(underlined), alignment(alignment), textSize(textSize),
    color(color), font(font)
{
}

Symbol::~Symbol()
{
}

QVector<int>& Symbol::getPosition()
{
    return position;
}

void Symbol::setPosition(QVector<int> position)
{
    this->position = position;
}

int Symbol::getCounter()
{
    return counter;
}

int Symbol::getSiteId()
{
    return siteId;
}

bool Symbol::isBold()
{
    return bold;
}

bool Symbol::isItalic()
{
    return italic;
}

bool Symbol::isUnderlined()
{
    return underlined;
}

int Symbol::getAlignment()
{
    return alignment;
}

int Symbol::getTextSize()
{
    return textSize;
}

QColor& Symbol::getColor()
{
    return color;
}

QString& Symbol::getFont()
{
    return font;
}

QChar Symbol::getValue()
{
    return value;
}


bool Symbol::equals(Symbol* s)
{
    bool tmp = ((s->siteId == siteId) && (s->counter == counter));
    if (!tmp)
        return false;
    bool equal = true;
    tmp = ((s->value == value) && (s->bold == bold) && (s->color == color) && (s->italic == italic) && (s->underlined == underlined)
        && (s->alignment == alignment) && (s->textSize == textSize) && (s->font == font));
    if (!tmp)
        return false;
    for (int i = 0; i < s->getPosition().size(); i++) {
        if (!s->getPosition()[i] == position[i]) {
            equal = false;
        }
    }
    return true;
}
