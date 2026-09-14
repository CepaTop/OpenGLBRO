#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>

#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"

int main()
{
    //INICIALIZACIÓN DE GLFW Y LA VENTANA
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "OpenGLBRO - Grilla de Cuadrados", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    gladLoadGL();
    glViewport(0, 0, 800, 800);

    Shader shaderProgram("default.vert", "default.frag");

    // GENERACIÓN MATEMÁTICA DE LA MATRIZ DE CUADRADOS 
    std::vector<GLfloat> squareVertices;

    float tamano = 0.12f; // Tamaño de cada cuadrado
    float paso = 0.16f;   // Distancia entre el inicio de cada cuadrado

    // Doble bucle for: recorre eje X (columnas) y eje Y (filas)
    for (float x = -0.9f; x <= 0.8f; x += paso)
    {
        for (float y = -0.9f; y <= 0.8f; y += paso)
        {
            // Coordenadas de las 4 esquinas del cuadrado
            float x1 = x;
            float y1 = y;
            float x2 = x + tamano;
            float y2 = y + tamano;

            // Color para el cuadrado (R, G, B) 
            float r = 0.1f, g = 0.3f, b = 0.8f;

            // TRIÁNGULO 1 (Abajo-Izquierda, Abajo-Derecha, Arriba-Derecha) 
            squareVertices.push_back(x1); squareVertices.push_back(y1); squareVertices.push_back(0.0f);
            squareVertices.push_back(r);  squareVertices.push_back(g);  squareVertices.push_back(b);

            squareVertices.push_back(x2); squareVertices.push_back(y1); squareVertices.push_back(0.0f);
            squareVertices.push_back(r);  squareVertices.push_back(g);  squareVertices.push_back(b);

            squareVertices.push_back(x2); squareVertices.push_back(y2); squareVertices.push_back(0.0f);
            squareVertices.push_back(r);  squareVertices.push_back(g);  squareVertices.push_back(b);

            // TRIÁNGULO 2 (Abajo-Izquierda, Arriba-Derecha, Arriba-Izquierda) 
            squareVertices.push_back(x1); squareVertices.push_back(y1); squareVertices.push_back(0.0f);
            squareVertices.push_back(r);  squareVertices.push_back(g);  squareVertices.push_back(b);

            squareVertices.push_back(x2); squareVertices.push_back(y2); squareVertices.push_back(0.0f);
            squareVertices.push_back(r);  squareVertices.push_back(g);  squareVertices.push_back(b);

            squareVertices.push_back(x1); squareVertices.push_back(y2); squareVertices.push_back(0.0f);
            squareVertices.push_back(r);  squareVertices.push_back(g);  squareVertices.push_back(b);
        }
    }

    // Calculamos la cantidad total de vértices
    GLsizei numVertices = static_cast<GLsizei>(squareVertices.size() / 6);

    //CONFIGURACIÓN DE VAO Y VBO 
    VAO VAO1;
    VAO1.Bind();

    VBO VBO1(squareVertices.data(), squareVertices.size() * sizeof(GLfloat));

    // Posición (Attribute 0)
    VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
    // Color (Attribute 1)
    VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    VAO1.Unbind();
    VBO1.Unbind();

    // OPCIONAL: Descomenta la siguiente línea si quieres ver SOLO los bordes/líneas de los cuadrados (Wireframe)
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // BUCLE PRINCIPAL 
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shaderProgram.Activate();
        VAO1.Bind();

        // Cambiamos GL_LINES por GL_TRIANGLES
        glDrawArrays(GL_TRIANGLES, 0, numVertices);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // LIMPIEZA
    VAO1.Delete();
    VBO1.Delete();
    shaderProgram.Delete();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}