#pragma once

#include "tool.h"

class PencilTool : public Tool
{
public:
    QString name() const override { return QStringLiteral("Pencil"); }

protected:
    QPen pen(const QColor &color) const override
    {
        return QPen(color, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    }
};
