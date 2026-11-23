Carlos Pacheco
6014324


Prototipo de editor de dibujo hecho en C++ con OpenGL 3.3. Todo lo que se dibuja se rasteriza sobre una
grilla de 60 × 60 celdas con algoritmos clásicos: línea y círculo de Bresenham, triángulo formado por tres
líneas y relleno por línea de barrido (*scanline*). El panel de herramientas está hecho con
[Dear ImGui](https://github.com/ocornut/imgui).

Las celdas se recalculan en cada cuadro, así que rotar muchas veces no deforma la figura. Para rotar, cada
punto gira alrededor del centroide, se redondea a una celda y se vuelve a aplicar Bresenham. El dibujo
libre y el borrador unen cada par de puntos consecutivos con una línea de Bresenham, así el trazo no queda
con huecos aunque el ratón se mueva rápido.

**Dibujo en pantalla** (`src/Renderer2D.cpp`, `default.vert`, `default.frag`). Cada celda es un
rectángulo: dos triángulos de seis vértices, cada uno con posición `x, y` en píxeles y color `r, g, b, a`.
Todos los rectángulos de un cuadro se envían juntos en un VBO dinámico y se dibujan con un solo
`glDrawArrays`. El vertex shader convierte los píxeles a coordenadas de OpenGL (de −1 a 1), y el *alpha
blending* hace que funcione la opacidad.

**Interfaz** (`src/main.cpp`, función `dibujarPanel`). Dear ImGui funciona en modo inmediato: el panel se
reconstruye en cada cuadro y cada botón ejecuta su acción cuando se le hace clic. Los clics sobre el panel
no llegan al lienzo.

## Estructura del proyecto

```
OpenGLBRO.slnx
Linking/              GLFW y glad: cabeceras y bibliotecas
OpenGLBRO/
├── OpenGLBRO.vcxproj
├── default.vert      vertex shader: de píxeles a coordenadas normalizadas
├── default.frag      fragment shader: color con opacidad
└── src/
    ├── main.cpp          figuras, acciones, panel de ImGui y bucle principal
    ├── Rasterizado.*     Bresenham (línea y círculo), triángulo y relleno scanline
    ├── Renderer2D.*      dibujo de rectángulos por lotes
    ├── shaderClass.*     lectura y compilación de los shaders
    ├── VAO.*  VBO.*  EBO.*
    ├── glad.c
    └── imgui/            Dear ImGui 1.91.9b con sus backends para GLFW y OpenGL 3
```

## Compilar y ejecutar

1. Abrir `OpenGLBRO.slnx` en Visual Studio 2022 o posterior.
2. Elegir la configuración **Debug | x64**: es la que tiene configuradas las rutas de `Linking/`.
3. Compilar y ejecutar con **F5**.

Los shaders se leen desde la carpeta de trabajo, que en Visual Studio es `OpenGLBRO/`. Para ejecutar el
`.exe` desde otra carpeta, hay que copiar `default.vert` y `default.frag` junto a él.

## Limitaciones conocidas

- Solo se pueden rellenar círculos y triángulos.
- El borrador pinta con el color del lienzo: si después se mueve o se rota una figura borrada, el borrón no la sigue.
- Los textos del panel van sin tildes para que se vean bien aunque el compilador no use la opción `/utf-8`.
- La grilla tiene un tamaño fijo de 60 × 60 celdas.

## Créditos

- [Dear ImGui](https://github.com/ocornut/imgui), licencia MIT (`OpenGLBRO/src/imgui/LICENSE.txt`).
- [GLFW](https://www.glfw.org/) para la ventana y la entrada, y [glad](https://github.com/Dav1dde/glad) para cargar OpenGL.