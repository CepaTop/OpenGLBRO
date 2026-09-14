#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

#include <vector>
#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"

// 2 Vértices: Posición (X, Y, Z) + Color (R, G, B)
GLfloat vertices[] =
{
	// Coordenadas (X, Y, Z)    // Colores (R, G, B)
	-0.8f,  0.0f, 0.0f,        1.0f, 1.0f, 1.0f, // Punto 1: Izquierda (Blanco)
	 0.8f,  0.0f, 0.0f,        1.0f, 1.0f, 1.0f  // Punto 2: Derecha (Blanco)
};

// Índices para el orden de los vértices
GLuint indices[] =
{
	0, 3, 5, // Triangulo inferior izquierdo
	3, 2, 4, // Triangulo inferior derecho
	5, 4, 1 // Triangulo superior
};

int main()
{
    // --- 1. INICIALIZACIÓN DE GLFW Y LA VENTANA (Lo que ya tenías) ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "OpenGLBRO - Grilla", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    gladLoadGL();
    glViewport(0, 0, 800, 800);

    // Genera el objeto Shader
    Shader shaderProgram("default.vert", "default.frag");

    // --- 2. GENERACIÓN MATEMÁTICA DE LA GRILLA ---
    // En lugar del arreglo fijo, usamos un vector que va creciendo
    std::vector<GLfloat> gridVertices;
    float paso = 0.1f;   // Qué tan juntas están las líneas (puedes cambiarlo a 0.2f, etc.)
    float limite = 1.0f; // Los bordes de la pantalla (-1.0 a 1.0)

    for (float i = -limite; i <= limite; i += paso)
    {
        // LINEAS VERTICALES
        // Punto inferior (X, Y, Z) y Color (R, G, B) - Color Gris claro
        gridVertices.push_back(i); gridVertices.push_back(-limite); gridVertices.push_back(0.0f);
        gridVertices.push_back(0.7f); gridVertices.push_back(0.7f); gridVertices.push_back(0.7f);
        // Punto superior (X, Y, Z) y Color (R, G, B)
        gridVertices.push_back(i); gridVertices.push_back(limite); gridVertices.push_back(0.0f);
        gridVertices.push_back(0.7f); gridVertices.push_back(0.7f); gridVertices.push_back(0.7f);

        // LINEAS HORIZONTALES
        // Punto izquierdo (X, Y, Z) y Color (R, G, B)
        gridVertices.push_back(-limite); gridVertices.push_back(i); gridVertices.push_back(0.0f);
        gridVertices.push_back(0.7f); gridVertices.push_back(0.7f); gridVertices.push_back(0.7f);
        // Punto derecho (X, Y, Z) y Color (R, G, B)
        gridVertices.push_back(limite); gridVertices.push_back(i); gridVertices.push_back(0.0f);
        gridVertices.push_back(0.7f); gridVertices.push_back(0.7f); gridVertices.push_back(0.7f);
    }

    // Calculamos cuántos vértices se generaron en total (cada vértice tiene 6 datos: x,y,z, r,g,b)
    GLsizei numVertices = static_cast<GLsizei>(gridVertices.size() / 6);


    // --- 3. CONFIGURACIÓN DE VAO Y VBO ---
    VAO VAO1;
    VAO1.Bind();

    // Aquí le pasamos el vector al VBO usando .data() y .size()
    VBO VBO1(gridVertices.data(), gridVertices.size() * sizeof(GLfloat));

    // Le decimos a OpenGL cómo leer los datos (exactamente igual que como lo tenías)
    // Posición (Layout 0)
    VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
    // Color (Layout 1)
    VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    // Desvinculamos para evitar modificaciones accidentales
    VAO1.Unbind();
    VBO1.Unbind();


    // --- 4. BUCLE DE DIBUJO (MAIN LOOP) ---
    while (!glfwWindowShouldClose(window))
    {
        // Fondo negro para que resalte la grilla gris
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Activamos los shaders y el VAO
        shaderProgram.Activate();
        VAO1.Bind();

        // ¡DIBUJAMOS LA GRILLA! usando GL_LINES en lugar de triángulos
        glDrawArrays(GL_LINES, 0, numVertices);

        // Refresca la ventana
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- 5. LIMPIEZA FINAL ---
    VAO1.Delete();
    VBO1.Delete();
    shaderProgram.Delete();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}