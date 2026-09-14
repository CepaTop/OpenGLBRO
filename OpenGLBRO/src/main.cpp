#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"


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
	// Inicializa GLFW
	glfwInit();

	// Le dice a GLFW qué versión de OpenGL estamos usando
	// En este caso estamos usando OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Le dice a GLFW que estamos usando el CORE perfil
	// Lo que significa que solo tenemos las funciones modernas
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Crea un objeto GLFWwindow de 800 por 800 pixeles, llamandolo "OPENGLBRO"
	GLFWwindow* window = glfwCreateWindow(800, 800, "OPENGLBRO", NULL, NULL);
	// Error check if the window fails to create
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	// Introduce la ventana en el contexto actual
	glfwMakeContextCurrent(window);

	//Carga GLAD para que configure OpenGL
	gladLoadGL();
	// Especifica el viewport de OpenGL en la ventana
	// En este caso el viewport va desde x = 0, y = 0, hasta x = 800, y = 800
	glViewport(0, 0, 800, 800);



	// Genera el objeto Shader usando los shaders default.vert y default.frag
	Shader shaderProgram("default.vert", "default.frag");



	// Genera un Vertex Array Object (VAO) y lo vincula
	VAO VAO1;
	VAO1.Bind();

	// Genera un Vertex Buffer Object (VBO) y lo vincula a los vértices
	VBO VBO1(vertices, sizeof(vertices));
	// Genera un Element Buffer Object (EBO) y lo vincula a los índices
	EBO EBO1(indices, sizeof(indices));

	// Vincula los atributos del VBO, como coordenadas y colores, al VAO
	VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
	VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	// Desvincula todo para evitar modificarlos accidentalmente
	VAO1.Unbind();
	VBO1.Unbind();
	EBO1.Unbind();

	// Obtiene el ID del uniform llamado "scale"
	GLuint uniID = glGetUniformLocation(shaderProgram.ID, "scale");


	// Main while principal
	while (!glfwWindowShouldClose(window))
	{
		// Especifica el color del fondo
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Limpia el back buffer y le asigna el nuevo color
		glClear(GL_COLOR_BUFFER_BIT);
		// Le dice a OpenGL qué Shader Program queremos usar
		shaderProgram.Activate();
		// Asigna un valor al uniform; NOTA: Siempre debe hacerse después de activar el Shader Program
		glUniform1f(uniID, 0.5f);
		// Vincula el VAO para que OpenGL sepa que debe usarlo
		VAO1.Bind();
		// DIBUJAR LA LÍNEA: 
		// GL_LINES le dice a OpenGL que vincule los puntos de 2 en 2
		// 0 es el índice de inicio
		// 2 es la cantidad total de vértices
		glDrawArrays(GL_LINES, 0, 2);
		// Intercambia el back buffer con el front buffer
		glfwSwapBuffers(window);
		// Se encarga de todos los eventos de GLFW
		glfwPollEvents();
	}



	// Elimina todos los objetos que hemos creado
	VAO1.Delete();
	VBO1.Delete();
	EBO1.Delete();
	shaderProgram.Delete();
	// Elimina la ventana antes de finalizar el programa
	glfwDestroyWindow(window);
	// Termina GLFW antes de finalizar el programa
	glfwTerminate();
	return 0;
}