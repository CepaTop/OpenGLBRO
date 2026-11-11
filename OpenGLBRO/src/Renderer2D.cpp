#include "Renderer2D.h"

Renderer2D::Renderer2D() : vao(), vbo(nullptr, 0)
{
    // Formato de vértice: posición (x, y) en píxeles + color RGBA
    vao.Bind();
    vao.LinkAttrib(vbo, 0, 2, GL_FLOAT, 6 * sizeof(float), (void*)0);
    vao.LinkAttrib(vbo, 1, 4, GL_FLOAT, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    vao.Unbind();
}

void Renderer2D::vertice(float x, float y, const Color& c)
{
    vertices.push_back(x);
    vertices.push_back(y);
    vertices.push_back(c.r);
    vertices.push_back(c.g);
    vertices.push_back(c.b);
    vertices.push_back(c.a);
}

void Renderer2D::rect(float x, float y, float w, float h, const Color& c)
{
    float x2 = x + w;
    float y2 = y + h;
    // Triángulo 1
    vertice(x, y, c);
    vertice(x2, y, c);
    vertice(x2, y2, c);
    // Triángulo 2
    vertice(x, y, c);
    vertice(x2, y2, c);
    vertice(x, y2, c);
}

void Renderer2D::rectBorde(float x, float y, float w, float h, float t, const Color& c)
{
    rect(x, y, w, t, c);                     // arriba
    rect(x, y + h - t, w, t, c);             // abajo
    rect(x, y + t, t, h - 2 * t, c);         // izquierda
    rect(x + w - t, y + t, t, h - 2 * t, c); // derecha
}

void Renderer2D::dibujar()
{
    if (vertices.empty()) return;
    vbo.Update(vertices.data(), vertices.size() * sizeof(GLfloat));
    vao.Bind();
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 6));
    vao.Unbind();
    vertices.clear();
}

void Renderer2D::eliminar()
{
    vao.Delete();
    vbo.Delete();
}