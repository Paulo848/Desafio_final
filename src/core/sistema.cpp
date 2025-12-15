#include "sistema.h"
#include "../levels/nivel.h"

Sistema::Sistema() : nivel(nullptr)
{
}

Sistema::~Sistema()
{
    if (nivel != nullptr) {
        delete nivel;
    }
}

void Sistema::Interfaz()
{

}

bool Sistema::DesplegarNivel()
{
    // Por implementar: lógica de despliegue
    return nivel != nullptr;
}

Nivel* Sistema::getNivel()
{
    return nivel;
}

void Sistema::setNivel(Nivel* nuevoNivel)
{
    if (nivel != nullptr) {
        delete nivel;
    }
    nivel = nuevoNivel;
}
