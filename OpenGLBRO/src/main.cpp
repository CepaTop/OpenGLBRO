#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <set>
#include <cmath>

#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"

// Tamaño de la grilla 
const int GRID_SIZE = 40;

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

        int e2 = 2 * err;
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
std::set<std::pair<int, int>> calcularCirculoBresenham(int xc, int yc, int r)
{
    std::set<std::pair<int, int>> celdasCirculo;
    int x = 0;
    int y = r;
    int d = 3 - 2 * r; // Parámetro de decisión inicial

    // Función lambda para aprovechar la simetría y pintar los 8 lados a la vez
    auto agregarSimetria = [&](int cx, int cy, int px, int py) {
        celdasCirculo.insert({ cx + px, cy + py });
        celdasCirculo.insert({ cx - px, cy + py });
        celdasCirculo.insert({ cx + px, cy - py });
        celdasCirculo.insert({ cx - px, cy - py });
        celdasCirculo.insert({ cx + py, cy + px });
        celdasCirculo.insert({ cx - py, cy + px });
        celdasCirculo.insert({ cx + py, cy - px });
        celdasCirculo.insert({ cx - py, cy - px });
        };

    while (y >= x)
    {
        agregarSimetria(xc, yc, x, y);
        x++;
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
        {
            d = d + 4 * x + 6;
        }
    }
    return celdasCirculo;
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // Actualiza el área de dibujo de OpenGL para que coincida con la ventana
    glViewport(0, 0, width, height);
}

int main()
{
    // --- Configuración Inicial de GLFW y OpenGL ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1080, 1900, "Linea Rasterizada en Grilla", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Error al crear la ventana" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    gladLoadGL();


    Shader shaderProgram("default.vert", "default.frag");

    // --- Definir Inicio y Fin de la Línea en la Grilla ---
    int xInicial = 1, yInicial = 1;  // Esquina inferior izquierda
    int xFinal = 48, yFinal = 48; // Esquina superior derecha

	int xCentro = 10, yCentro = 28; // Centro del círculo
	int radio = 8
        ; // Radio del círculo

    // Calculamos qué cuadrados formarán la línea
    std::set<std::pair<int, int>> lineaPintada = calcularLineaBresenham(xInicial, yInicial, xFinal, yFinal);

    std::set<std::pair<int, int>> circuloPintado = calcularCirculoBresenham(xCentro, yCentro, radio);

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
            if (lineaPintada.count({ c, r }) > 0 && circuloPintado.count({ c, r }) > 0)
            {
                // Si se cruzan: Color Amarillo
                red = 1.0f; green = 0.8f; blue = 0.2f;
            }
            else if (lineaPintada.count({ c, r }) > 0)
            {
                // Solo línea: Color Rojo
                red = 1.0f; green = 0.2f; blue = 0.2f;
            }
            else if (circuloPintado.count({ c, r }) > 0)
            {
                // Solo círculo: Color Verde brillante
                red = 0.2f; green = 1.0f; blue = 0.4f;
            }
            else
            {
                // Fondo normal: Gris oscuro
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