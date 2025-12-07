#ifndef VECTOR2D_H
#define VECTOR2D_H

#include <QtMath>
#include <QPointF>

class Vector2D
{
public:
    // --- Constructores ---
    Vector2D() : m_x(0.0), m_y(0.0) {}
    Vector2D(qreal x, qreal y) : m_x(x), m_y(y) {}
    Vector2D(const Vector2D &other)
        : m_x(other.m_x), m_y(other.m_y) {}
    explicit Vector2D(const QPointF &p)
        : m_x(p.x()), m_y(p.y()) {}

    // --- Getters / Setters ---
    inline qreal x() const { return m_x; }
    inline qreal y() const { return m_y; }
    inline void setX(qreal x) { m_x = x; }
    inline void setY(qreal y) { m_y = y; }
    inline void set(qreal x, qreal y) { m_x = x; m_y = y; }

    // --- Magnitud ---
    inline qreal magnitud() const {
        return qSqrt(m_x*m_x + m_y*m_y);
    }

    inline qreal magnitud2() const {
        return m_x*m_x + m_y*m_y;
    }

    // --- Normalizar ---
    inline Vector2D normalizado() const {
        qreal m = magnitud();
        return (m > 0.0) ? Vector2D(m_x/m, m_y/m) : Vector2D(0.0, 0.0);
    }

    inline void normalizarEnSitio() {
        qreal m = magnitud();
        if (m > 0.0) { m_x /= m; m_y /= m; }
    }

    // --- Ángulos / dirección ---
    inline qreal cosDir() const {
        qreal m = magnitud();
        return (m > 0.0) ? (m_x / m) : 0.0;
    }

    inline qreal sinDir() const {
        qreal m = magnitud();
        return (m > 0.0) ? (m_y / m) : 0.0;
    }

    // Ángulo en radianes
    inline qreal angulo() const {
        return qAtan2(m_y, m_x); // [-pi, pi]
    }

    // Crear vector desde magnitud + ángulo
    static inline Vector2D desdePolar(qreal magnitud, qreal anguloRad) {
        return Vector2D(magnitud * qCos(anguloRad),
                        magnitud * qSin(anguloRad));
    }

    // --- Distancias ---
    inline qreal distanciaA(const Vector2D &o) const {
        return (*this - o).magnitud();
    }

    inline qreal distancia2A(const Vector2D &o) const {
        return (*this - o).magnitud2();
    }

    // --- Producto punto ---
    inline qreal dot(const Vector2D &o) const {
        return m_x*o.m_x + m_y*o.m_y;
    }

    // --- Conversión a QPointF ---
    inline QPointF toPointF() const {
        return QPointF(m_x, m_y);
    }

    // --- Operadores aritméticos ---
    inline Vector2D operator+(const Vector2D &o) const {
        return Vector2D(m_x + o.m_x, m_y + o.m_y);
    }

    inline Vector2D operator-(const Vector2D &o) const {
        return Vector2D(m_x - o.m_x, m_y - o.m_y);
    }

    inline Vector2D operator*(qreal escalar) const {
        return Vector2D(m_x * escalar, m_y * escalar);
    }

    inline Vector2D operator/(qreal escalar) const {
        return Vector2D(m_x / escalar, m_y / escalar);
    }

    inline Vector2D& operator+=(const Vector2D &o) {
        m_x += o.m_x; m_y += o.m_y; return *this;
    }

    inline Vector2D& operator-=(const Vector2D &o) {
        m_x -= o.m_x; m_y -= o.m_y; return *this;
    }

    inline Vector2D& operator*=(qreal s) {
        m_x *= s; m_y *= s; return *this;
    }

    inline Vector2D& operator/=(qreal s) {
        m_x /= s; m_y /= s; return *this;
    }

    Vector2D& operator=(const Vector2D &other)
    {
        if (this != &other) {    // protección autoasignación (por costumbre)
            m_x = other.m_x;
            m_y = other.m_y;
        }
        return *this;
    }

    // --- Comparaciones ---
    // Igualdad componente a componente con tolerancia
    inline bool operator==(const Vector2D &o) const {
        const qreal eps = 1e-6;
        return qAbs(m_x - o.m_x) < eps &&
               qAbs(m_y - o.m_y) < eps;
    }

    inline bool operator!=(const Vector2D &o) const {
        return !(*this == o);
    }

    // Comparación por magnitud (útil para IA, distancias, prioridades)
    inline bool magnitudMenorQue(const Vector2D &o) const {
        return magnitud() < o.magnitud();
    }

    inline bool magnitudMayorQue(const Vector2D &o) const {
        return magnitud() > o.magnitud();
    }

    inline bool mismaMagnitudQue(const Vector2D &o, qreal eps = 1e-6) const {
        return qAbs(magnitud() - o.magnitud()) < eps;
    }

    // --- Vector nulo ---
    static inline Vector2D nulo() {
        return Vector2D(0.0, 0.0);
    }

    Vector2D rotCanonicoY(const Vector2D &o) const;
    Vector2D getNormal() const;
    Vector2D desdeEspacioLocal(const Vector2D &local) const;

private:
    qreal m_x;
    qreal m_y;
};

#endif // VECTOR2D_H
