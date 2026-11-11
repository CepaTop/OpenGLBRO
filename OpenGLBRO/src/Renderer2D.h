#ifndef RENDERER2D_H
#define RENDERER2D_H

// Renderizador 2D muy simple por lotes (batch): acumula rectángulos de color
// (en coordenadas de píxel, origen arriba-izquierda) y los dibuja con una sola
// llamada a glDrawArrays. Se usa para la grilla y las figuras; la interfaz la
// dibuja Dear ImGui.

#include <glad/glad.h>
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
