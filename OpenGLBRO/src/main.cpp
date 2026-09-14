#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <set>
#include <cmath>

#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"

// Tamaño de la grilla (20x20 celdas)
const int GRID_SIZE = 20;

// 1. ALGORITMO DE BRESENHAM
// Calcula qué coordenadas enteras (x, y) de la grilla deben encenderse
std::set<std::pair<int, int>> calcularLineaBresenham(int x0, int y0, int x1, int y1)
{
    std::set<std::pair<int, int>> celdasLinea;

    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        celdasLinea.insert({ x0, y0 }); // Guardar la casilla actual

        if (x0 == x1 && y0 == y1) break;

        int e2 = 1.5 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
    return celdasLinea;
}

int main()
{
    // --- Configuración Inicial de GLFW y OpenGL ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Linea Rasterizada en Grilla", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Error al crear la ventana" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    gladLoadGL();
    glViewport(0, 0, 800, 800);

    Shader shaderProgram("default.vert", "default.frag");

    // --- Definir Inicio y Fin de la Línea en la Grilla ---
    int xInicial = 1, yInicial = 2;  // Esquina inferior izquierda
    int xFinal = 18, yFinal = 15; // Esquina superior derecha

    // Calculamos qué cuadrados formarán la línea
    std::set<std::pair<int, int>> lineaPintada = calcularLineaBresenham(xInicial, yInicial, xFinal, yFinal);

    // --- Generación de los Cuadrados de la Grilla ---
    std::vector<GLfloat> squareVertices;
    float cellSize = 2.0f / GRID_SIZE; // Ancho/alto de cada celda en pantalla (-1.0 a 1.0)

    for (int r = 0; r < GRID_SIZE; ++r)      // Eje Y (filas)
    {
        for (int c = 0; c < GRID_SIZE; ++c)  // Eje X (columnas)
        {
            // Coordenadas de la celda en pantalla
            float x1 = -1.0f + c * cellSize;
            float y1 = -1.0f + r * cellSize;
            // Margen del 90% (0.90f) para dejar una separación visible entre cuadrados
            float x2 = x1 + cellSize * 0.90f;
            float y2 = y1 + cellSize * 0.90f;

            // Determinar color: ¿Esta celda está dentro del camino de la línea?
            float red, green, blue;
            if (lineaPintada.count({ c, r }) > 0)
            {
                // Cuadrado de la línea: Rojo brillante
                red = 1.0f; green = 0.2f; blue = 0.2f;
            }
            else
            {
                // Fondo de la grilla: Gris tenue
                red = 0.18f; green = 0.22f; blue = 0.28f;
            }

            // Triángulo 1
            squareVertices.push_back(x1); squareVertices.push_back(y1); squareVertices.push_back(0.0f);
            squareVertices.push_back(red); squareVertices.push_back(green); squareVertices.push_back(blue);

            squareVertices.push_back(x2); squareVertices.push_back(y1); squareVertices.push_back(0.0f);
            squareVertices.push_back(red); squareVertices.push_back(green); squareVertices.push_back(blue);

            squareVertices.push_back(x2); squareVertices.push_back(y2); squareVertices.push_back(0.0f);
            squareVertices.push_back(red); squareVertices.push_back(green); squareVertices.push_back(blue);

            // Triángulo 2
            squareVertices.push_back(x1); squareVertices.push_back(y1); squareVertices.push_back(0.0f);
            squareVertices.push_back(red); squareVertices.push_back(green); squareVertices.push_back(blue);

            squareVertices.push_back(x2); squareVertices.push_back(y2); squareVertices.push_back(0.0f);
            squareVertices.push_back(red); squareVertices.push_back(green); squareVertices.push_back(blue);

            squareVertices.push_back(x1); squareVertices.push_back(y2); squareVertices.push_back(0.0f);
            squareVertices.push_back(red); squareVertices.push_back(green); squareVertices.push_back(blue);
        }
    }

    GLsizei numVertices = static_cast<GLsizei>(squareVertices.size() / 6);

    // --- Cargar Buffers en GPU ---
    VAO VAO1;
    VAO1.Bind();

    VBO VBO1(squareVertices.data(), squareVertices.size() * sizeof(GLfloat));

    VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
    VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    VAO1.Unbind();
    VBO1.Unbind();

    // --- Bucle de Renderizado ---
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.07f, 0.09f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shaderProgram.Activate();
        VAO1.Bind();

        glDrawArrays(GL_TRIANGLES, 0, numVertices);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- Limpieza ---
    VAO1.Delete();
    VBO1.Delete();
    shaderProgram.Delete();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}