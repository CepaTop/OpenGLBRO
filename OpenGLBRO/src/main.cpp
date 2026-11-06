#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "shaderClass.h"
#include "Rasterizado.h"
#include "Renderer2D.h"

// Tamaño de la grilla 
const int GRID_SIZE = 60;

// Ancho del panel de herramientas en píxeles
const float PANEL_ANCHO = 330.0f;

const float PI = 3.14159265358979f;

const Color COL_FONDO = { 0.07f, 0.09f, 0.12f, 1.0f };
const Color COL_PANEL = { 0.13f, 0.15f, 0.19f, 1.0f };
const Color COL_BOTON = { 0.22f, 0.25f, 0.31f, 1.0f };
const Color COL_BOTON_HOVER = { 0.29f, 0.33f, 0.41f, 1.0f };
const Color COL_BOTON_ACTIVO = { 0.20f, 0.45f, 0.85f, 1.0f };
const Color COL_TEXTO = { 0.93f, 0.95f, 0.98f, 1.0f };
const Color COL_TEXTO_SUAVE = { 0.55f, 0.65f, 0.80f, 1.0f };
const Color COL_AVISO = { 1.00f, 0.80f, 0.25f, 1.0f };
const Color COL_LINEAS_GRID = { 0.80f, 0.82f, 0.86f, 1.0f };
const Color COL_CELDA = { 0.97f, 0.97f, 0.98f, 1.0f };
const Color COL_SELECCION = { 0.15f, 0.55f, 1.00f, 1.0f };

const Color PALETA[] = {
    {0.10f, 0.10f, 0.10f, 1}, {0.50f, 0.50f, 0.50f, 1}, {1.00f, 1.00f, 1.00f, 1}, {0.55f, 0.33f, 0.15f, 1},
    {0.90f, 0.20f, 0.20f, 1}, {1.00f, 0.55f, 0.10f, 1}, {1.00f, 0.85f, 0.10f, 1}, {0.95f, 0.85f, 0.65f, 1},
    {0.20f, 0.75f, 0.30f, 1}, {0.60f, 0.90f, 0.30f, 1}, {0.00f, 0.55f, 0.50f, 1}, {0.10f, 0.80f, 0.85f, 1},
    {0.15f, 0.40f, 0.90f, 1}, {0.10f, 0.15f, 0.50f, 1}, {0.55f, 0.25f, 0.80f, 1}, {1.00f, 0.45f, 0.70f, 1},
};
const int NUM_PALETA = sizeof(PALETA) / sizeof(PALETA[0]);

struct Punto
{
    float x, y; // coordenadas en celdas de la grilla
};

enum class Herramienta { Linea, Circulo, Triangulo, Seleccion };
enum class TipoFigura { Linea, Circulo, Triangulo };

struct Figura
{
    TipoFigura tipo = TipoFigura::Linea;
    std::vector<Punto> puntos; // Línea: 2, Círculo: 1 (centro), Triángulo: 3
    int radio = 0;             // Solo para el círculo
    float angulo = 0.0f;       // Rotación en grados (antihorario) alrededor del centro
    Color color = { 0, 0, 0, 1 }; // RGB + opacidad
    bool relleno = false;      // Solo círculo y triángulo
};

struct App
{
    Herramienta herramienta = Herramienta::Linea;
    Color colorActual = { 0.90f, 0.20f, 0.20f, 1.0f };
    bool rellenoActual = false;

    std::vector<Figura> figuras;
    int seleccion = -1;                         // índice de la figura seleccionada
    std::vector<Celda> clicsPendientes;         // vértices de la figura en construcción
    std::vector<std::vector<Figura>> historial; // pila para deshacer

    // Arrastre de figuras
    bool arrastrando = false;
    bool arrastreGuardado = false;
    Celda celdaArrastre = { 0, 0 };

    // Entrada del ratón (estado del frame actual)
    double mouseX = 0, mouseY = 0;
    bool clicPresionado = false; // flanco de bajada en este frame
    bool clicSoltado = false;    // flanco de subida en este frame
    bool clicAbajo = false;
    int sliderActivo = -1;

    // Mensajes temporales en el panel
    std::string mensaje;
    double tiempoMensaje = 0.0;
};

App app;

void mostrarMensaje(const std::string& texto)
{
    app.mensaje = texto;
    app.tiempoMensaje = glfwGetTime();
}

void guardarHistorial()
{
    app.historial.push_back(app.figuras);
    if (app.historial.size() > 100) app.historial.erase(app.historial.begin());
}

Figura* figuraSeleccionada()
{
    if (app.seleccion >= 0 && app.seleccion < (int)app.figuras.size())
        return &app.figuras[app.seleccion];
    return nullptr;
}

std::string nombreFigura(TipoFigura t)
{
    switch (t)
    {
    case TipoFigura::Linea:   return "LINEA";
    case TipoFigura::Circulo: return "CIRCULO";
    default:                  return "TRIANGULO";
    }
}

float normalizarAngulo(float a)
{
    while (a > 180.0f) a -= 360.0f;
    while (a <= -180.0f) a += 360.0f;
    return a;
}

Punto pivote(const Figura& f)
{
    Punto c = { 0.0f, 0.0f };
    for (const Punto& p : f.puntos) { c.x += p.x; c.y += p.y; }
    c.x /= f.puntos.size();
    c.y /= f.puntos.size();
    return c;
}

// Rotación 2D de un punto alrededor de un pivote:
//   x' = cx + (x - cx) cos(a) - (y - cy) sin(a)
//   y' = cy + (x - cx) sin(a) + (y - cy) cos(a)
Punto rotarPunto(Punto p, Punto c, float grados)
{
    float rad = grados * PI / 180.0f;
    float cs = std::cos(rad), sn = std::sin(rad);
    float dx = p.x - c.x, dy = p.y - c.y;
    return { c.x + dx * cs - dy * sn, c.y + dx * sn + dy * cs };
}

// Vértices finales (rotados y redondeados a celdas enteras)
std::vector<Celda> verticesTransformados(const Figura& f)
{
    Punto c = pivote(f);
    std::vector<Celda> res;
    for (const Punto& p : f.puntos)
    {
        Punto r = rotarPunto(p, c, f.angulo);
        res.push_back({ (int)std::lround(r.x), (int)std::lround(r.y) });
    }
    return res;
}

// Convierte una figura en el conjunto de celdas que ocupa en la grilla
ConjuntoCeldas rasterizarFigura(const Figura& f, bool forzarRelleno = false)
{
    std::vector<Celda> v = verticesTransformados(f);
    ConjuntoCeldas celdas;

    switch (f.tipo)
    {
    case TipoFigura::Linea:
        celdas = calcularLineaBresenham(v[0].first, v[0].second, v[1].first, v[1].second);
        break;
    case TipoFigura::Circulo:
        celdas = calcularCirculoBresenham(v[0].first, v[0].second, f.radio);
        break;
    case TipoFigura::Triangulo:
        celdas = calcularTrianguloBresenham(v[0].first, v[0].second, v[1].first, v[1].second,
            v[2].first, v[2].second);
        break;
    }

    if ((f.relleno || forzarRelleno) && f.tipo != TipoFigura::Linea)
    {
        ConjuntoCeldas interior = rellenarScanline(celdas);
        celdas.insert(interior.begin(), interior.end());
    }

    // Recortar a los límites de la grilla
    ConjuntoCeldas recortadas;
    for (const Celda& c : celdas)
        if (c.first >= 0 && c.first < GRID_SIZE && c.second >= 0 && c.second < GRID_SIZE)
            recortadas.insert(c);
    return recortadas;
}

// Devuelve el índice de la figura más arriba que contiene la celda (o -1)
int buscarFiguraEn(Celda celda)
{
    for (int i = (int)app.figuras.size() - 1; i >= 0; --i)
    {
        const Figura& f = app.figuras[i];
        ConjuntoCeldas celdas = rasterizarFigura(f, true); // clic dentro también selecciona
        if (f.tipo == TipoFigura::Linea)
        {
            // Tolerancia de una celda alrededor de la línea
            for (int dx = -1; dx <= 1; ++dx)
                for (int dy = -1; dy <= 1; ++dy)
                    if (celdas.count({ celda.first + dx, celda.second + dy })) return i;
        }
        else if (celdas.count(celda))
        {
            return i;
        }
    }
    return -1;
}
void cambiarHerramienta(Herramienta h)
{
    app.herramienta = h;
    app.clicsPendientes.clear();
    // Al elegir una herramienta de dibujo, los cambios de color/relleno son para la nueva figura
    if (h != Herramienta::Seleccion) app.seleccion = -1;
}

void seleccionar(int indice)
{
    app.seleccion = indice;
    if (Figura* f = figuraSeleccionada())
    {
        // La interfaz muestra el color/relleno de la figura seleccionada
        app.colorActual = f->color;
        app.rellenoActual = f->relleno;
    }
}

void rotarSeleccion(float grados)
{
    Figura* f = figuraSeleccionada();
    if (!f) { mostrarMensaje("SELECCIONA UNA FIGURA"); return; }
    guardarHistorial();
    f->angulo = normalizarAngulo(f->angulo + grados);
}

void alternarRelleno()
{
    app.rellenoActual = !app.rellenoActual;
    if (Figura* f = figuraSeleccionada())
    {
        if (f->tipo == TipoFigura::Linea)
        {
            mostrarMensaje("UNA LINEA NO SE RELLENA");
            return;
        }
        guardarHistorial();
        f->relleno = app.rellenoActual;
    }
}

// Aplica el color actual (incluida la opacidad) a la figura seleccionada
void aplicarColorASeleccion()
{
    if (Figura* f = figuraSeleccionada()) f->color = app.colorActual;
}

void borrarSeleccion()
{
    if (!figuraSeleccionada()) { mostrarMensaje("NADA SELECCIONADO"); return; }
    guardarHistorial();
    app.figuras.erase(app.figuras.begin() + app.seleccion);
    app.seleccion = -1;
}

void deshacer()
{
    if (app.historial.empty()) { mostrarMensaje("NADA QUE DESHACER"); return; }
    app.figuras = app.historial.back();
    app.historial.pop_back();
    app.clicsPendientes.clear();
    if (app.seleccion >= (int)app.figuras.size()) app.seleccion = -1;
    seleccionar(app.seleccion); // el panel vuelve a mostrar el color/relleno restaurado
}

void limpiarTodo()
{
    if (app.figuras.empty()) return;
    guardarHistorial();
    app.figuras.clear();
    app.seleccion = -1;
    app.clicsPendientes.clear();
}

void moverSeleccion(int dx, int dy)
{
    if (Figura* f = figuraSeleccionada())
        for (Punto& p : f->puntos) { p.x += dx; p.y += dy; }
}

// Construye una figura a partir de los clics (sirve también para la vista previa)
Figura construirFigura(Herramienta h, const std::vector<Celda>& clics)
{
    Figura f;
    f.color = app.colorActual;
    f.relleno = app.rellenoActual;
    if (h == Herramienta::Linea || (h == Herramienta::Triangulo && clics.size() == 2))
    {
        f.tipo = TipoFigura::Linea;
        f.relleno = false;
        f.puntos = { {(float)clics[0].first, (float)clics[0].second},
                     {(float)clics[1].first, (float)clics[1].second} };
    }
    else if (h == Herramienta::Circulo)
    {
        f.tipo = TipoFigura::Circulo;
        f.puntos = { {(float)clics[0].first, (float)clics[0].second} };
        float dx = (float)(clics[1].first - clics[0].first);
        float dy = (float)(clics[1].second - clics[0].second);
        f.radio = (int)std::lround(std::sqrt(dx * dx + dy * dy));
    }
    else
    {
        f.tipo = TipoFigura::Triangulo;
        for (const Celda& c : clics) f.puntos.push_back({ (float)c.first, (float)c.second });
    }
    return f;
}

// Número de clics necesarios para completar la figura de cada herramienta
size_t clicsNecesarios(Herramienta h)
{
    return h == Herramienta::Triangulo ? 3 : 2;
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // Actualiza el área de dibujo de OpenGL para que coincida con la ventana
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int boton, int accion, int mods)
{
    if (boton != GLFW_MOUSE_BUTTON_LEFT) return;
    if (accion == GLFW_PRESS) { app.clicPresionado = true; app.clicAbajo = true; }
    if (accion == GLFW_RELEASE) { app.clicSoltado = true;    app.clicAbajo = false; }
}

void key_callback(GLFWwindow* window, int tecla, int scancode, int accion, int mods)
{
    if (accion != GLFW_PRESS && accion != GLFW_REPEAT) return;

    switch (tecla)
    {
    case GLFW_KEY_L: cambiarHerramienta(Herramienta::Linea); break;
    case GLFW_KEY_C: cambiarHerramienta(Herramienta::Circulo); break;
    case GLFW_KEY_T: cambiarHerramienta(Herramienta::Triangulo); break;
    case GLFW_KEY_S: cambiarHerramienta(Herramienta::Seleccion); break;
    case GLFW_KEY_F: alternarRelleno(); break;
    case GLFW_KEY_Q: rotarSeleccion(15.0f); break;   // antihorario
    case GLFW_KEY_E: rotarSeleccion(-15.0f); break;  // horario
    case GLFW_KEY_Z: deshacer(); break;
    case GLFW_KEY_DELETE:
    case GLFW_KEY_BACKSPACE: borrarSeleccion(); break;
    case GLFW_KEY_ESCAPE:
        if (!app.clicsPendientes.empty()) app.clicsPendientes.clear();
        else app.seleccion = -1;
        break;
    case GLFW_KEY_LEFT:
    case GLFW_KEY_RIGHT:
    case GLFW_KEY_UP:
    case GLFW_KEY_DOWN:
        if (figuraSeleccionada())
        {
            if (accion == GLFW_PRESS) guardarHistorial();
            int dx = (tecla == GLFW_KEY_RIGHT) - (tecla == GLFW_KEY_LEFT);
            int dy = (tecla == GLFW_KEY_UP) - (tecla == GLFW_KEY_DOWN);
            moverSeleccion(dx, dy);
        }
        break;
    default: break;
    }
}
bool mouseDentro(float x, float y, float w, float h)
{
    return app.mouseX >= x && app.mouseX < x + w && app.mouseY >= y && app.mouseY < y + h;
}

bool boton(Renderer2D& r, float x, float y, float w, float h, const std::string& etiqueta, bool activo = false)
{
    bool hover = mouseDentro(x, y, w, h) && app.sliderActivo < 0;
    Color fondo = activo ? COL_BOTON_ACTIVO : (hover ? COL_BOTON_HOVER : COL_BOTON);
    r.rect(x, y, w, h, fondo);
    if (hover) r.rectBorde(x, y, w, h, 1.0f, { 1, 1, 1, 0.25f });

    float esc = 2.0f;
    float tw = r.anchoTexto(etiqueta, esc);
    r.texto(std::floor(x + (w - tw) / 2), std::floor(y + (h - r.altoTexto(esc)) / 2), etiqueta, esc, COL_TEXTO);
    return hover && app.clicPresionado;
}

// Slider horizontal. Devuelve true si el valor cambió en este frame.
bool slider(Renderer2D& r, int id, float x, float y, float w, float h, float& valor,
    float minimo, float maximo, const Color& colorBarra)
{
    if (app.clicPresionado && mouseDentro(x - 6, y - 4, w + 12, h + 8) && app.sliderActivo < 0)
    {
        app.sliderActivo = id;
        if (figuraSeleccionada()) guardarHistorial(); // un solo paso de deshacer por arrastre
    }

    bool cambio = false;
    if (app.sliderActivo == id && (app.clicAbajo || app.clicPresionado))
    {
        float t = std::clamp((float)(app.mouseX - x) / w, 0.0f, 1.0f);
        float nuevo = minimo + t * (maximo - minimo);
        if (nuevo != valor) { valor = nuevo; cambio = true; }
    }

    float t = std::clamp((valor - minimo) / (maximo - minimo), 0.0f, 1.0f);
    r.rect(x, y + h / 2 - 3, w, 6, COL_BOTON);
    r.rect(x, y + h / 2 - 3, w * t, 6, colorBarra);
    float kx = x + w * t;
    r.rect(kx - 6, y - 1, 12, h + 2, COL_TEXTO);
    r.rect(kx - 4, y + 1, 8, h - 2, colorBarra);
    return cambio;
}

// Fondo de tablero de ajedrez (para apreciar la opacidad)
void tablero(Renderer2D& r, float x, float y, float w, float h, float lado)
{
    for (float yy = 0; yy < h; yy += lado)
        for (float xx = 0; xx < w; xx += lado)
        {
            bool par = ((int)(xx / lado) + (int)(yy / lado)) % 2 == 0;
            float g = par ? 0.95f : 0.70f;
            r.rect(x + xx, y + yy, std::min(lado, w - xx), std::min(lado, h - yy), { g, g, g, 1 });
        }
}
void dibujarPanel(Renderer2D& r, float altoVentana, bool mouseEnLienzo, Celda celdaMouse)
{
    const float M = 16.0f;                 // margen
    const float W = PANEL_ANCHO - 2 * M;   // ancho útil
    const float x = M;
    float y = 16.0f;

    r.rect(0, 0, PANEL_ANCHO, altoVentana, COL_PANEL);
    r.rect(PANEL_ANCHO - 2, 0, 2, altoVentana, COL_FONDO);

    r.texto(x, y, "PAINT BRESENHAM", 3.0f, COL_TEXTO);
    y += 36;

    // --- Herramientas ---
    r.texto(x, y, "HERRAMIENTAS", 2.0f, COL_TEXTO_SUAVE);
    y += 22;
    float bw = (W - 8) / 2, bh = 34;
    if (boton(r, x, y, bw, bh, "LINEA", app.herramienta == Herramienta::Linea))
        cambiarHerramienta(Herramienta::Linea);
    if (boton(r, x + bw + 8, y, bw, bh, "CIRCULO", app.herramienta == Herramienta::Circulo))
        cambiarHerramienta(Herramienta::Circulo);
    y += bh + 6;
    if (boton(r, x, y, bw, bh, "TRIANGULO", app.herramienta == Herramienta::Triangulo))
        cambiarHerramienta(Herramienta::Triangulo);
    if (boton(r, x + bw + 8, y, bw, bh, "SELECCION", app.herramienta == Herramienta::Seleccion))
        cambiarHerramienta(Herramienta::Seleccion);
    y += bh + 6;
    if (boton(r, x, y, W, bh, app.rellenoActual ? "RELLENO: SI" : "RELLENO: NO", app.rellenoActual))
        alternarRelleno();
    y += bh + 16;

    // --- Color ---
    r.texto(x, y, "COLOR", 2.0f, COL_TEXTO_SUAVE);
    y += 22;
    const int porFila = 8;
    float sw = (W - (porFila - 1) * 6) / porFila, sh = 28;
    for (int i = 0; i < NUM_PALETA; ++i)
    {
        float sx = x + (i % porFila) * (sw + 6);
        float sy = y + (i / porFila) * (sh + 6);
        const Color& c = PALETA[i];
        bool igual = std::fabs(c.r - app.colorActual.r) < 0.01f && std::fabs(c.g - app.colorActual.g) < 0.01f &&
            std::fabs(c.b - app.colorActual.b) < 0.01f;
        if (igual) r.rectBorde(sx - 3, sy - 3, sw + 6, sh + 6, 2, COL_TEXTO);
        r.rect(sx, sy, sw, sh, c);
        if (mouseDentro(sx, sy, sw, sh) && app.clicPresionado && app.sliderActivo < 0)
        {
            if (figuraSeleccionada()) guardarHistorial();
            app.colorActual = { c.r, c.g, c.b, app.colorActual.a }; // conserva la opacidad
            aplicarColorASeleccion();
        }
    }
    y += 2 * (sh + 6) + 8;

    // Sliders RGB
    const char* nombres[3] = { "R", "G", "B" };
    float* canales[3] = { &app.colorActual.r, &app.colorActual.g, &app.colorActual.b };
    Color barras[3] = { {0.9f, 0.25f, 0.25f, 1}, {0.25f, 0.8f, 0.35f, 1}, {0.3f, 0.5f, 0.95f, 1} };
    for (int i = 0; i < 3; ++i)
    {
        r.texto(x, y + 1, nombres[i], 2.0f, COL_TEXTO);
        if (slider(r, i, x + 26, y, W - 26 - 50, 16, *canales[i], 0.0f, 1.0f, barras[i]))
            aplicarColorASeleccion();
        std::string v = std::to_string((int)std::lround(*canales[i] * 255));
        r.texto(x + W - r.anchoTexto(v, 2.0f), y + 1, v, 2.0f, COL_TEXTO);
        y += 26;
    }
    y += 8;

    // --- Opacidad ---
    int porcentaje = (int)std::lround(app.colorActual.a * 100);
    r.texto(x, y, "OPACIDAD: " + std::to_string(porcentaje) + "%", 2.0f, COL_TEXTO_SUAVE);
    y += 22;
    if (slider(r, 3, x + 6, y, W - 12, 16, app.colorActual.a, 0.0f, 1.0f, COL_BOTON_ACTIVO))
        aplicarColorASeleccion();
    y += 26;
    // Muestra del color actual sobre un tablero para ver la transparencia
    tablero(r, x, y, W, 26, 13);
    r.rect(x, y, W, 26, app.colorActual);
    r.rectBorde(x, y, W, 26, 1, { 0, 0, 0, 0.6f });
    y += 26 + 16;

    // --- Rotación ---
    Figura* sel = figuraSeleccionada();
    float anguloUI = sel ? sel->angulo : 0.0f;
    r.texto(x, y, "ROTACION: " + std::to_string((int)std::lround(anguloUI)) + " GRADOS", 2.0f, COL_TEXTO_SUAVE);
    y += 22;
    float rw = (W - 3 * 6) / 4;
    const char* etiquetasRot[4] = { "-15", "-1", "+1", "+15" };
    const float pasos[4] = { -15.0f, -1.0f, 1.0f, 15.0f };
    for (int i = 0; i < 4; ++i)
        if (boton(r, x + i * (rw + 6), y, rw, 32, etiquetasRot[i]))
            rotarSeleccion(pasos[i]);
    y += 32 + 12;
    if (slider(r, 4, x + 6, y, W - 12, 16, anguloUI, -180.0f, 180.0f, COL_BOTON_ACTIVO))
    {
        if (sel) sel->angulo = std::round(anguloUI);
        else mostrarMensaje("SELECCIONA UNA FIGURA");
    }
    y += 16 + 18;

    // --- Acciones ---
    r.texto(x, y, "ACCIONES", 2.0f, COL_TEXTO_SUAVE);
    y += 22;
    if (boton(r, x, y, bw, 32, "DESHACER")) deshacer();
    if (boton(r, x + bw + 8, y, bw, 32, "BORRAR")) borrarSeleccion();
    y += 32 + 6;
    if (boton(r, x, y, W, 32, "LIMPIAR TODO")) limpiarTodo();
    y += 32 + 16;

    // --- Ayuda / estado ---
    std::string ayuda;
    switch (app.herramienta)
    {
    case Herramienta::Linea:     ayuda = "CLIC INICIO Y CLIC FIN"; break;
    case Herramienta::Circulo:   ayuda = "CLIC CENTRO Y CLIC RADIO"; break;
    case Herramienta::Triangulo: ayuda = "CLIC EN LOS 3 VERTICES"; break;
    case Herramienta::Seleccion: ayuda = "CLIC Y ARRASTRA FIGURA"; break;
    }
    r.texto(x, y, ayuda, 2.0f, COL_TEXTO);
    y += 20;
    if (app.herramienta != Herramienta::Seleccion)
    {
        std::string progreso = "PUNTOS: " + std::to_string(app.clicsPendientes.size()) + "/" +
            std::to_string(clicsNecesarios(app.herramienta));
        r.texto(x, y, progreso, 2.0f, COL_TEXTO_SUAVE);
        y += 20;
    }
    // Qué figura reciben los cambios de color, opacidad, relleno y rotación
    if (sel)
        r.texto(x, y, "EDITANDO: " + nombreFigura(sel->tipo) + " #" + std::to_string(app.seleccion + 1),
            2.0f, COL_SELECCION);
    else
        r.texto(x, y, "SIN SELECCION", 2.0f, COL_TEXTO_SUAVE);
    y += 20;
    std::string info = "FIGURAS: " + std::to_string(app.figuras.size());
    if (mouseEnLienzo)
        info += "  (" + std::to_string(celdaMouse.first) + "," + std::to_string(celdaMouse.second) + ")";
    r.texto(x, y, info, 2.0f, COL_TEXTO_SUAVE);
    y += 20;

    if (!app.mensaje.empty() && glfwGetTime() - app.tiempoMensaje < 2.5)
        r.texto(x, y, app.mensaje, 2.0f, COL_AVISO);
    y += 26;

    // Atajos de teclado (solo si caben en la ventana)
    if (y + 4 * 18 > altoVentana) return;
    r.texto(x, y, "L C T S = HERRAMIENTAS", 2.0f, COL_TEXTO_SUAVE);
    y += 18;
    r.texto(x, y, "F=RELLENO  Z=DESHACER", 2.0f, COL_TEXTO_SUAVE);
    y += 18;
    r.texto(x, y, "Q/E=ROTAR  SUPR=BORRAR", 2.0f, COL_TEXTO_SUAVE);
    y += 18;
    r.texto(x, y, "FLECHAS=MOVER ESC=ANULAR", 2.0f, COL_TEXTO_SUAVE);
}

int main()
{
        glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Tamaño de ventana adaptado al monitor (máximo 1300 x 900)
    int anchoVentana = 1300, altoVentana = 900;
    if (const GLFWvidmode* modo = glfwGetVideoMode(glfwGetPrimaryMonitor()))
    {
        anchoVentana = std::min(anchoVentana, modo->width - 40);
        altoVentana = std::min(altoVentana, modo->height - 80);
    }
    GLFWwindow* window = glfwCreateWindow(anchoVentana, altoVentana, "Paint Bresenham - OpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Error al crear la ventana" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowSizeLimits(window, 900, 600, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);

    gladLoadGL();

    // Transparencia: mezcla alpha estándar
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader shaderProgram("default.vert", "default.frag");
    GLint uResolucion = glGetUniformLocation(shaderProgram.ID, "uResolucion");

    Renderer2D r;

    // Figuras de ejemplo
    {
        Figura linea;
        linea.tipo = TipoFigura::Linea;
        linea.puntos = { {3, 3}, {56, 20} };
        linea.color = { 0.90f, 0.20f, 0.20f, 1.0f };
        app.figuras.push_back(linea);

        Figura circulo;
        circulo.tipo = TipoFigura::Circulo;
        circulo.puntos = { {16, 42} };
        circulo.radio = 10;
        circulo.color = { 0.20f, 0.75f, 0.30f, 0.8f };
        circulo.relleno = true;
        app.figuras.push_back(circulo);

        Figura tri;
        tri.tipo = TipoFigura::Triangulo;
        tri.puntos = { {32, 30}, {54, 34}, {40, 54} };
        tri.color = { 0.15f, 0.40f, 0.90f, 0.6f };
        tri.relleno = true;
        tri.angulo = 15.0f;
        app.figuras.push_back(tri);
    }

    // --- Bucle de Renderizado ---
    while (!glfwWindowShouldClose(window))
    {
        int fbW, fbH, winW, winH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glfwGetWindowSize(window, &winW, &winH);
        if (fbW == 0 || fbH == 0) { glfwWaitEvents(); continue; } // ventana minimizada

        // Posición del ratón en píxeles del framebuffer (soporta pantallas HiDPI)
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        app.mouseX = mx * fbW / std::max(winW, 1);
        app.mouseY = my * fbH / std::max(winH, 1);

        // --- Disposición del lienzo (grilla cuadrada centrada a la derecha del panel) ---
        const float margen = 20.0f;
        float areaW = fbW - PANEL_ANCHO;
        float lado = std::max(1.0f, std::min(areaW, (float)fbH) - 2 * margen);
        float cs = std::max(2.0f, std::floor(lado / GRID_SIZE)); // tamaño de celda
        float lienzo = cs * GRID_SIZE;
        float lx = std::floor(PANEL_ANCHO + (areaW - lienzo) / 2);
        float ly = std::floor((fbH - lienzo) / 2);

        // Celda bajo el ratón (fila 0 abajo, como en el plano cartesiano)
        int colMouse = (int)std::floor((app.mouseX - lx) / cs);
        int filaMouse = GRID_SIZE - 1 - (int)std::floor((app.mouseY - ly) / cs);
        bool mouseEnLienzo = app.mouseX >= lx && app.mouseX < lx + lienzo &&
            app.mouseY >= ly && app.mouseY < ly + lienzo;
        Celda celdaMouse = { colMouse, filaMouse };

        // --- Interacción con el lienzo ---
        if (mouseEnLienzo && app.clicPresionado && app.sliderActivo < 0)
        {
            if (app.herramienta == Herramienta::Seleccion)
            {
                int indice = buscarFiguraEn(celdaMouse);
                seleccionar(indice);
                app.arrastrando = indice >= 0;
                app.arrastreGuardado = false;
                app.celdaArrastre = celdaMouse;
            }
            else
            {
                app.clicsPendientes.push_back(celdaMouse);
                if (app.clicsPendientes.size() == clicsNecesarios(app.herramienta))
                {
                    Figura nueva = construirFigura(app.herramienta, app.clicsPendientes);
                    app.clicsPendientes.clear();
                    if (nueva.tipo == TipoFigura::Circulo && nueva.radio == 0)
                    {
                        mostrarMensaje("RADIO DEMASIADO PEQUENO");
                    }
                    else
                    {
                        guardarHistorial();
                        app.figuras.push_back(nueva);
                        app.seleccion = (int)app.figuras.size() - 1; // queda lista para rotar/colorear
                    }
                }
            }
        }

        // Arrastrar la figura seleccionada
        if (app.arrastrando && app.clicAbajo && mouseEnLienzo && celdaMouse != app.celdaArrastre)
        {
            if (!app.arrastreGuardado) { guardarHistorial(); app.arrastreGuardado = true; }
            moverSeleccion(celdaMouse.first - app.celdaArrastre.first,
                celdaMouse.second - app.celdaArrastre.second);
            app.celdaArrastre = celdaMouse;
        }
        if (app.clicSoltado) app.arrastrando = false;

        // --- Dibujo ---
        glClearColor(COL_FONDO.r, COL_FONDO.g, COL_FONDO.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        shaderProgram.Activate();
        glUniform2f(uResolucion, (float)fbW, (float)fbH);

        // Posición en pantalla de una celda (c, f)
        auto xCelda = [&](int c) { return lx + c * cs; };
        auto yCelda = [&](int f) { return ly + (GRID_SIZE - 1 - f) * cs; };
        // Margen del 90% para dejar una separación visible entre cuadrados (como la grilla original)
        float tamCelda = cs > 4 ? cs * 0.9f : cs;
        auto pintarCeldas = [&](const ConjuntoCeldas& celdas, const Color& c) {
            for (const Celda& celda : celdas)
                r.rect(xCelda(celda.first), yCelda(celda.second), tamCelda, tamCelda, c);
            };

        // Grilla de fondo
        r.rect(lx, ly, lienzo, lienzo, COL_LINEAS_GRID);
        for (int f = 0; f < GRID_SIZE; ++f)
            for (int c = 0; c < GRID_SIZE; ++c)
                r.rect(xCelda(c), yCelda(f), tamCelda, tamCelda, COL_CELDA);

        // Figuras (en orden: las últimas quedan encima y se mezclan según su opacidad)
        for (const Figura& f : app.figuras)
            pintarCeldas(rasterizarFigura(f), f.color);

        // Marco de la figura seleccionada y su centro de rotación
        if (Figura* sel = figuraSeleccionada())
        {
            ConjuntoCeldas celdas = rasterizarFigura(*sel);
            if (!celdas.empty())
            {
                int minC = GRID_SIZE, maxC = -1, minF = GRID_SIZE, maxF = -1;
                for (const Celda& c : celdas)
                {
                    minC = std::min(minC, c.first);  maxC = std::max(maxC, c.first);
                    minF = std::min(minF, c.second); maxF = std::max(maxF, c.second);
                }
                float bx = xCelda(minC) - 3, by = yCelda(maxF) - 3;
                float bw = (maxC - minC + 1) * cs + 5, bh = (maxF - minF + 1) * cs + 5;
                r.rectBorde(bx, by, bw, bh, 2, COL_SELECCION);

                Punto c = pivote(*sel);
                float px = lx + (c.x + 0.45f) * cs, py = ly + (GRID_SIZE - 1 - c.y + 0.45f) * cs;
                r.rect(px - 5, py - 1, 10, 2, COL_SELECCION);
                r.rect(px - 1, py - 5, 2, 10, COL_SELECCION);
            }
        }

        // Vista previa de la figura en construcción
        if (!app.clicsPendientes.empty() && app.herramienta != Herramienta::Seleccion)
        {
            if (mouseEnLienzo)
            {
                std::vector<Celda> clics = app.clicsPendientes;
                clics.push_back(celdaMouse);
                Figura previa = construirFigura(app.herramienta, clics);
                Color cp = app.colorActual;
                cp.a *= 0.5f;
                pintarCeldas(rasterizarFigura(previa), cp);
            }
            for (const Celda& c : app.clicsPendientes)
                r.rectBorde(xCelda(c.first) - 2, yCelda(c.second) - 2, cs + 3, cs + 3, 2, { 0.1f, 0.1f, 0.1f, 1 });
        }

        // Celda bajo el cursor
        if (mouseEnLienzo)
            r.rectBorde(xCelda(colMouse), yCelda(filaMouse), tamCelda, tamCelda, 1, COL_SELECCION);

        // Panel de herramientas (procesa sus propios clics)
        dibujarPanel(r, (float)fbH, mouseEnLienzo, celdaMouse);

        r.dibujar();

        glfwSwapBuffers(window);

        // Reiniciar los eventos "de un solo frame" y leer los nuevos
        if (app.clicSoltado) app.sliderActivo = -1;
        app.clicPresionado = false;
        app.clicSoltado = false;
        glfwPollEvents();
    }

    // --- Limpieza ---
    r.eliminar();
    shaderProgram.Delete();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
