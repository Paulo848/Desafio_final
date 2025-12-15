#ifndef PROYECCIONISO_H
#define PROYECCIONISO_H

#include <QPointF>

class ProyeccionIso
{
public:
    // Convierte coordenadas del mundo 2D lógico a la pantalla (2.5D)
    static QPointF toScreen(const QPointF &worldPoint);
};

#endif // PROYECCIONISO_H
