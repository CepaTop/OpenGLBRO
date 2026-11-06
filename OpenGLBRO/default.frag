#version 330 core

// Color de salida en RGBA
out vec4 FragColor;

// Color recibido del Vertex Shader (incluye la opacidad en .a)
in vec4 color;

void main()
{
	FragColor = color;
}