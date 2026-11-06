#ifndef RENDERER2D_H
#define RENDERER2D_H

/ Renderizador 2D muy simple por lotes(batch) : acumula rectángulos de color
// (en coordenadas de píxel, origen arriba-izquierda) y los dibuja con una sola
// llamada a glDrawArrays. También dibuja texto con una fuente bitmap de 5x7.

#include <glad/glad.h>
#include <string>
#include <vector>

#include "VAO.h"
#include "VBO.h"

struct Color
{
    float r, g, b, a;
};

class Renderer2D
{
public:
    Renderer2D();

    // Rectángulo relleno
    void rect(float x, float y, float w, float h, const Color& c);
    // Contorno de un rectángulo con grosor t
    void rectBorde(float x, float y, float w, float h, float t, const Color& c);
    // Texto en mayúsculas con fuente bitmap 5x7 (cada "píxel" mide escala x escala)
    void texto(float x, float y, const std::string& s, float escala, const Color& c);
    float anchoTexto(const std::string& s, float escala) const;
    float altoTexto(float escala) const { return 7.0f * escala; }

    // Sube los vértices acumulados a la GPU y los dibuja
    void dibujar();
    void eliminar();

private:
    std::vector<GLfloat> vertices;
    VAO vao;
    VBO vbo;

    void vertice(float x, float y, const Color& c);
};

#endif
