#ifndef SISTEMA_H
#define SISTEMA_H

class Nivel;

class Sistema
{
public:
    Sistema();
    ~Sistema();

    void Interfaz();
    bool DesplegarNivel();
    Nivel* getNivel();
    void setNivel(Nivel* nivel);
    void RepetirNivel();

private:
    Nivel* nivel;
};

#endif // SISTEMA_H
