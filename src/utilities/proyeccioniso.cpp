#include "proyeccioniso.h"

QPointF ProyeccionIso::toScreen(const QPointF &worldPoint)
{
    // Coordenadas en el plano lógico 2D
    const qreal x = worldPoint.x();
    const qreal y = worldPoint.y();

    // Escalas (ajusta a gusto)
    const qreal sx = 1.0;   // escala horizontal
    const qreal sy = 0.5;   // aplasta un poco en vertical

    // Proyección isométrica clásica
    //const qreal isoX = x;
    const qreal isoX = (y + x) *sx;
    //const qreal isoY = y;
    const qreal isoY = (y - x) *sy;

    return QPointF(isoX, isoY);
}
