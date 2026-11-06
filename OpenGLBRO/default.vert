#version 330 core

// Posicion del vertice en pixeles (origen en la esquina superior izquierda)
layout (location = 0) in vec2 aPos;
// Color RGBA (la componente A es la opacidad)
layout (location = 1) in vec4 aColor;

// Color que se envia al Fragment Shader
out vec4 color;

// Tamano del framebuffer en pixeles, para convertir a coordenadas normalizadas (-1 a 1)
uniform vec2 uResolucion;

void main()
{
	vec2 ndc = vec2(aPos.x / uResolucion.x * 2.0 - 1.0,
	                1.0 - aPos.y / uResolucion.y * 2.0);
	gl_Position = vec4(ndc, 0.0, 1.0);
	color = aColor;
}