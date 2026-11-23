#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "shaderClass.h"
#include "Rasterizado.h"
#include "Renderer2D.h"

// Tamaño de la grilla (celdas por lado)
const int GRID_SIZE = 60;
// Ancho del panel de ImGui (en coordenadas de ventana)
const float PANEL_ANCHO = 310.0f;

const float PI = 3.14159265358979f;

// ---------------------------------------------------------------------------
// Colores del lienzo
// ---------------------------------------------------------------------------
const Color COL_FONDO = { 0.07f, 0.09f, 0.12f, 1.0f };
const Color COL_LINEAS_GRID = { 0.80f, 0.82f, 0.86f, 1.0f };
const Color COL_CELDA = { 0.97f, 0.97f, 0.98f, 1.0f };
const Color COL_SELECCION = { 0.15f, 0.55f, 1.00f, 1.0f };

// Paleta de colores rápida
const Color PALETA[] = {
    {0.10f, 0.10f, 0.10f, 1}, {0.50f, 0.50f, 0.50f, 1}, {1.00f, 1.00f, 1.00f, 1}, {0.55f, 0.33f, 0.15f, 1},
    {0.90f, 0.20f, 0.20f, 1}, {1.00f, 0.55f, 0.10f, 1}, {1.00f, 0.85f, 0.10f, 1}, {0.95f, 0.85f, 0.65f, 1},
    {0.20f, 0.75f, 0.30f, 1}, {0.60f, 0.90f, 0.30f, 1}, {0.00f, 0.55f, 0.50f, 1}, {0.10f, 0.80f, 0.85f, 1},
    {0.15f, 0.40f, 0.90f, 1}, {0.10f, 0.15f, 0.50f, 1}, {0.55f, 0.25f, 0.80f, 1}, {1.00f, 0.45f, 0.70f, 1},
};
const int NUM_PALETA = sizeof(PALETA) / sizeof(PALETA[0]);

// ---------------------------------------------------------------------------
// Modelo de datos
// ---------------------------------------------------------------------------
struct Punto
{
    float x, y; // coordenadas en celdas de la grilla
};

enum class Herramienta { Linea, Circulo, Triangulo, Seleccion, Dibujo, Balde, Borrador };
enum class TipoFigura { Linea, Circulo, Triangulo, Dibujo, Borrador };
// Qué color se está editando en el panel
enum class ObjetivoColor { Borde, Relleno };

struct Figura
{
    TipoFigura tipo = TipoFigura::Linea;
    std::vector<Punto> puntos;           // Línea: 2, Círculo: 1 (centro), Triángulo: 3
    int radio = 0;                       // Solo para el círculo
    float angulo = 0.0f;                 // Rotación en grados (antihorario) alrededor del centro
    Color color = { 0, 0, 0, 1 };        // Color del borde (RGB + opacidad)
    bool relleno = false;                // Solo círculo y triángulo
    Color colorRelleno = { 0, 0, 0, 1 }; // Color del interior cuando está rellena
    int grosor = 1;                      // Ancho del trazo en celdas (el borrador usa mas de 1)
};

struct App
{
    Herramienta herramienta = Herramienta::Linea;
    Color colorBorde = { 0.90f, 0.20f, 0.20f, 1.0f };   // color para bordes
    Color colorRelleno = { 1.00f, 0.85f, 0.10f, 1.0f }; // color para rellenar
    ObjetivoColor objetivoColor = ObjetivoColor::Borde;
    bool trazando = false;                      // dibujando o borrando con el boton presionado
    int trazoActual = -1;                       // indice del trazo que se esta dibujando
    int grosorBorrador = 3;                     // tamano del borrador en celdas

    std::vector<Figura> figuras;
    int seleccion = -1;                         // índice de la figura seleccionada
    std::vector<Celda> clicsPendientes;         // vértices de la figura en construcción
    std::vector<std::vector<Figura>> historial; // pila para deshacer
    bool edicionEnCurso = false;
    bool mostrarGrilla = false;
    bool mostrarMarcoSeleccion = false;

    // Arrastre de figuras
    bool arrastrando = false;
    bool arrastreGuardado = false;
    Celda celdaArrastre = { 0, 0 };

    // Entrada del ratón sobre el lienzo (estado del frame actual)
    double mouseX = 0, mouseY = 0;
    bool clicPresionado = false; // flanco de bajada en este frame
    bool clicSoltado = false;    // flanco de subida en este frame
    bool clicAbajo = false;
    bool clicDerecho = false;    // clic derecho en este frame (selecciona con cualquier herramienta)

    // Mensajes temporales en el panel
    std::string mensaje;
    double tiempoMensaje = 0.0;
};

App app;

// ---------------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------------
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

const char* nombreFigura(TipoFigura t)
{
    switch (t)
    {
    case TipoFigura::Linea:   return "Linea";
    case TipoFigura::Dibujo:   return "Trazo libre";
    case TipoFigura::Borrador: return "Borrador";
    case TipoFigura::Circulo: return "Circulo";
    default:                  return "Triangulo";
    }
}
bool esRellenable(TipoFigura t)
{
    return t == TipoFigura::Circulo || t == TipoFigura::Triangulo;
}

// Puntos que necesita cada tipo de figura para poder dibujarse
size_t puntosMinimos(TipoFigura t)
{
    switch (t)
    {
    case TipoFigura::Linea:     return 2;
    case TipoFigura::Triangulo: return 3;
    default:                    return 1; // circulo (centro) y dibujo libre
    }
}

float normalizarAngulo(float a)
{
    while (a > 180.0f) a -= 360.0f;
    while (a <= -180.0f) a += 360.0f;
    return a;
}

// ---------------------------------------------------------------------------
// Geometría: rotación y rasterizado de las figuras
// ---------------------------------------------------------------------------

// Centro de rotación de la figura (centroide de sus puntos)
Punto pivote(const Figura& f)
{
    Punto c = { 0.0f, 0.0f };
    if (f.puntos.empty()) return c;
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

bool dentroDeGrilla(const Celda& c)
{
    return c.first >= 0 && c.first < GRID_SIZE && c.second >= 0 && c.second < GRID_SIZE;
}

// Rasteriza una figura separando su contorno (Bresenham) de su interior (scanline),
// para poder pintar cada parte con su propio color.
// Si calcularInterior es false, el interior queda vacío.
void celdasFigura(const Figura& f, bool calcularInterior, ConjuntoCeldas& contorno, ConjuntoCeldas& interior)
{
    std::vector<Celda> v = verticesTransformados(f);
    ConjuntoCeldas borde;

    // Proteccion: una figura con menos puntos de los que su tipo necesita no se dibuja.
    // Sin esto, leer v[1] o v[2] fuera del vector cierra el programa con
    // "vector subscript out of range".
    if (v.size() < puntosMinimos(f.tipo))
    {
        contorno.clear();
        interior.clear();
        return;
    }

    switch (f.tipo)
    {
    case TipoFigura::Linea:
        borde = calcularLineaBresenham(v[0].first, v[0].second, v[1].first, v[1].second);
        break;
    case TipoFigura::Circulo:
        borde = calcularCirculoBresenham(v[0].first, v[0].second, f.radio);
        break;
    case TipoFigura::Triangulo:
        borde = calcularTrianguloBresenham(v[0].first, v[0].second, v[1].first, v[1].second,
            v[2].first, v[2].second);
        break;
    case TipoFigura::Dibujo:
    case TipoFigura::Borrador:
        // Trazo libre: una linea de Bresenham entre cada par de puntos consecutivos,
        // asi no quedan huecos aunque el raton se mueva rapido
        borde.insert(v[0]);
        for (size_t i = 1; i < v.size(); ++i)
        {
            ConjuntoCeldas tramo = calcularLineaBresenham(v[i - 1].first, v[i - 1].second, v[i].first, v[i].second);
            borde.insert(tramo.begin(), tramo.end());
        }
        break;
    }

    // Trazo grueso (borrador): cada celda se convierte en un cuadrado de grosor x grosor
    if (f.grosor > 1)
    {
        ConjuntoCeldas grueso;
        int desde = -(f.grosor - 1) / 2, hasta = f.grosor / 2;
        for (const Celda& c : borde)
            for (int dx = desde; dx <= hasta; ++dx)
                for (int dy = desde; dy <= hasta; ++dy)
                {
                    Celda g = { c.first + dx, c.second + dy };
                    if (dentroDeGrilla(g)) grueso.insert(g);
                }
        borde = grueso;
    }

    contorno.clear();
    interior.clear();
    for (const Celda& c : borde)
        if (dentroDeGrilla(c)) contorno.insert(c);

    if (calcularInterior && esRellenable(f.tipo))
    {
        // El interior son las celdas del scanline que no pertenecen al borde
        for (const Celda& c : rellenarScanline(borde))
            if (dentroDeGrilla(c) && !borde.count(c)) interior.insert(c);
    }
}

// Todas las celdas que ocupa la figura (borde + relleno). Con forzarRelleno se incluye
// el interior aunque la figura no esté rellena (sirve para seleccionar con un clic dentro).
ConjuntoCeldas rasterizarFigura(const Figura& f, bool forzarRelleno = false)
{
    ConjuntoCeldas contorno, interior;
    celdasFigura(f, f.relleno || forzarRelleno, contorno, interior);
    contorno.insert(interior.begin(), interior.end());
    return contorno;
}

// Devuelve el índice de la figura más arriba que contiene la celda (o -1)
int buscarFiguraEn(Celda celda)
{
    for (int i = (int)app.figuras.size() - 1; i >= 0; --i)
    {
        const Figura& f = app.figuras[i];
        if (f.tipo == TipoFigura::Borrador) continue;      // el borrador no se selecciona
        ConjuntoCeldas celdas = rasterizarFigura(f, true); // clic dentro también selecciona
        if (!esRellenable(f.tipo))
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

// ---------------------------------------------------------------------------
// Acciones del editor
// ---------------------------------------------------------------------------
void cambiarHerramienta(Herramienta h)
{
    app.herramienta = h;
    app.clicsPendientes.clear();
    // Al elegir una herramienta de dibujo, los cambios de color son para la nueva figura
    if (h != Herramienta::Seleccion && h != Herramienta::Balde) app.seleccion = -1;
}

void seleccionar(int indice)
{
    app.seleccion = indice;
    if (Figura* f = figuraSeleccionada())
    {
        // El panel muestra los colores de la figura seleccionada
        app.colorBorde = f->color;
        if (esRellenable(f->tipo)) app.colorRelleno = f->colorRelleno;
    }
}

// Selecciona la figura siguiente (paso = +1) o la anterior (paso = -1)
void seleccionarSiguiente(int paso)
{
    // Recorre la lista en forma circular saltando los trazos del borrador
    int n = (int)app.figuras.size();
    int i = app.seleccion;
    for (int intentos = 0; intentos < n; ++intentos)
    {
        if (i < 0) i = (paso > 0) ? 0 : n - 1;
        else i = ((i + paso) % n + n) % n;
        if (app.figuras[i].tipo != TipoFigura::Borrador)
        {
            seleccionar(i);
            app.clicsPendientes.clear();
            return;
        }
    }
    mostrarMensaje("No hay figuras");
}

// Color que se está editando en el panel (el del borde o el del relleno)
Color& colorEditado()
{
    return app.objetivoColor == ObjetivoColor::Borde ? app.colorBorde : app.colorRelleno;
}

void rotarSeleccion(float grados)
{
    Figura* f = figuraSeleccionada();
    if (!f) { mostrarMensaje("Selecciona una figura"); return; }
    guardarHistorial();
    f->angulo = normalizarAngulo(f->angulo + grados);
}

// Devuelve la figura seleccionada si se puede rellenar; si no, muestra el motivo
Figura* figuraRellenable()
{
    Figura* f = figuraSeleccionada();
    if (!f) { mostrarMensaje("Selecciona una figura"); return nullptr; }
    if (!esRellenable(f->tipo)) { mostrarMensaje("Solo se rellenan circulos y triangulos"); return nullptr; }
    return f;
}

// Rellena la figura seleccionada con el color de relleno actual
void rellenarSeleccion()
{
    if (Figura* f = figuraRellenable())
    {
        guardarHistorial();
        f->relleno = true;
        f->colorRelleno = app.colorRelleno;
    }
}

void quitarRelleno()
{
    if (Figura* f = figuraRellenable())
    {
        guardarHistorial();
        f->relleno = false;
    }
}

// Tecla F: rellena o quita el relleno de la figura seleccionada
// Herramienta BALDE: clic sobre una figura para seleccionarla y rellenarla
void rellenarConBalde(Celda celda)
{
    int indice = buscarFiguraEn(celda);
    if (indice < 0) { mostrarMensaje("Haz clic sobre una figura"); return; }
    app.seleccion = indice;
    rellenarSeleccion();
    seleccionar(indice);
}

// Empieza un trazo libre (dibujo o borrador); los puntos se agregan mientras se arrastra
void empezarTrazo(TipoFigura tipo, Celda celda)
{
    guardarHistorial();                       // un trazo completo = un paso de deshacer
    Figura trazo;
    trazo.tipo = tipo;
    trazo.puntos = { {(float)celda.first, (float)celda.second} };
    if (tipo == TipoFigura::Borrador)
    {
        trazo.color = COL_CELDA;              // pinta con el color del lienzo
        trazo.grosor = app.grosorBorrador;
    }
    else
    {
        trazo.color = app.colorBorde;
        trazo.colorRelleno = app.colorRelleno;
    }
    app.figuras.push_back(trazo);
    app.trazoActual = (int)app.figuras.size() - 1;
    // El dibujo queda seleccionado (para cambiarle color o rotarlo); el borrador no
    app.seleccion = (tipo == TipoFigura::Dibujo) ? app.trazoActual : -1;
    app.trazando = true;
}

// Corta el trazo en curso (al soltar el boton o si la lista de figuras cambia)
void terminarTrazo()
{
    app.trazando = false;
    app.trazoActual = -1;
}

void alternarRelleno()
{
    Figura* f = figuraSeleccionada();
    if (f && f->relleno) quitarRelleno();
    else rellenarSeleccion();
}
// Aplica el color que se está editando (incluida la opacidad) a la figura seleccionada
void aplicarColorASeleccion()
{
    Figura* f = figuraSeleccionada();
    if (!f) return;
    if (app.objetivoColor == ObjetivoColor::Borde)
    {
        f->color = app.colorBorde;
    }
    else if (esRellenable(f->tipo))
    {
        // Editar el color de relleno de una figura la deja rellena
        f->colorRelleno = app.colorRelleno;
        f->relleno = true;
    }
}

// Se llama cuando un control continuo (color, opacidad, ángulo) cambia la figura
// seleccionada: guarda el historial solo al comenzar a arrastrar, no en cada frame.
void iniciarEdicion()
{
    if (!app.edicionEnCurso && figuraSeleccionada())
    {
        guardarHistorial();
        app.edicionEnCurso = true;
    }
}

void borrarSeleccion()
{
    if (!figuraSeleccionada()) { mostrarMensaje("No hay nada seleccionado"); return; }
    guardarHistorial();
    app.figuras.erase(app.figuras.begin() + app.seleccion);
    app.seleccion = -1;
    terminarTrazo();
}

void deshacer()
{
    if (app.historial.empty()) { mostrarMensaje("No hay nada que deshacer"); return; }
    app.figuras = app.historial.back();
    app.historial.pop_back();
    app.clicsPendientes.clear();
    terminarTrazo();
    if (app.seleccion >= (int)app.figuras.size()) app.seleccion = -1;
    seleccionar(app.seleccion); // el panel vuelve a mostrar los colores restaurados
}

void limpiarTodo()
{
    if (app.figuras.empty()) return;
    guardarHistorial();
    app.figuras.clear();
    app.seleccion = -1;
    app.clicsPendientes.clear();
    terminarTrazo();
}

void moverSeleccion(int dx, int dy)
{
    if (Figura* f = figuraSeleccionada())
        for (Punto& p : f->puntos) { p.x += dx; p.y += dy; }
}

// Construye una figura a partir de los clics (sirve también para la vista previa)
Figura construirFigura(Herramienta h, const std::vector<Celda>& clics)
{
    // Las figuras nuevas se dibujan solo con su borde; se rellenan después
    Figura f;
    f.color = app.colorBorde;
    f.colorRelleno = app.colorRelleno;
    if (h == Herramienta::Linea || (h == Herramienta::Triangulo && clics.size() == 2))
    {
        f.tipo = TipoFigura::Linea;
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

// ---------------------------------------------------------------------------
// Callbacks de GLFW
// (ImGui los encadena: primero procesa el evento ImGui y luego se llama a estos)
// ---------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // Actualiza el área de dibujo de OpenGL para que coincida con la ventana
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int boton, int accion, int mods)
{
    // Soltar el botón siempre se registra, para no dejar un arrastre "pegado"
    if (boton == GLFW_MOUSE_BUTTON_LEFT && accion == GLFW_RELEASE)
    {
        app.clicSoltado = true;
        app.clicAbajo = false;
        return;
    }
    // Si el ratón está sobre el panel de ImGui, el clic es para la interfaz
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (boton == GLFW_MOUSE_BUTTON_RIGHT && accion == GLFW_PRESS) app.clicDerecho = true;
    if (boton == GLFW_MOUSE_BUTTON_LEFT && accion == GLFW_PRESS) { app.clicPresionado = true; app.clicAbajo = true; }
}

void key_callback(GLFWwindow* window, int tecla, int scancode, int accion, int mods)
{
    if (accion != GLFW_PRESS && accion != GLFW_REPEAT) return;
    // Si se está escribiendo en un campo de ImGui, las teclas no son atajos
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    switch (tecla)
    {
    case GLFW_KEY_L: cambiarHerramienta(Herramienta::Linea); break;
    case GLFW_KEY_C: cambiarHerramienta(Herramienta::Circulo); break;
    case GLFW_KEY_T: cambiarHerramienta(Herramienta::Triangulo); break;
    case GLFW_KEY_S: cambiarHerramienta(Herramienta::Seleccion); break;
    case GLFW_KEY_B: cambiarHerramienta(Herramienta::Balde); break;
    case GLFW_KEY_G: cambiarHerramienta(Herramienta::Borrador); break;
    case GLFW_KEY_F: alternarRelleno(); break;
    case GLFW_KEY_TAB: seleccionarSiguiente((mods & GLFW_MOD_SHIFT) ? -1 : 1); break;
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

// ---------------------------------------------------------------------------
// Panel de herramientas con Dear ImGui
// ---------------------------------------------------------------------------
ImVec4 aImVec4(const Color& c) { return ImVec4(c.r, c.g, c.b, c.a); }

// Botón que se resalta cuando está activo (para la herramienta actual)
bool botonActivo(const char* etiqueta, bool activo, const ImVec2& tam)
{
    if (activo) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    bool clic = ImGui::Button(etiqueta, tam);
    if (activo) ImGui::PopStyleColor();
    return clic;
}

// Muestra del color de borde alrededor y el de relleno adentro, sobre un tablero
// de ajedrez para apreciar la transparencia

void dibujarPanel(float altoVentana, bool mouseEnLienzo, Celda celdaMouse)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(PANEL_ANCHO, altoVentana));
    // NoNavInputs: ImGui no usa TAB para saltar entre campos, así TAB queda libre para
    // cambiar de figura seleccionada
    ImGui::Begin("Paint Bresenham", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoNavInputs);

    const float esp = ImGui::GetStyle().ItemSpacing.x;
    const float ancho = ImGui::GetContentRegionAvail().x;
    const ImVec2 medio((ancho - esp) / 2, 0);
    const ImVec2 completo(ancho, 0);

    // --- Herramientas ---
    ImGui::SeparatorText("Herramientas");
    // Dos botones por fila: un tercer SameLine() deja el boton fuera del panel
    if (botonActivo("Dibujo Libre", app.herramienta == Herramienta::Dibujo, medio))
        cambiarHerramienta(Herramienta::Dibujo);
    ImGui::SameLine();
    if (botonActivo("Borrador", app.herramienta == Herramienta::Borrador, medio))
        cambiarHerramienta(Herramienta::Borrador);
    if (botonActivo("Linea", app.herramienta == Herramienta::Linea, medio))
        cambiarHerramienta(Herramienta::Linea);
    ImGui::SameLine();
    if (botonActivo("Circulo", app.herramienta == Herramienta::Circulo, medio))
        cambiarHerramienta(Herramienta::Circulo);
    if (botonActivo("Triangulo", app.herramienta == Herramienta::Triangulo, medio))
        cambiarHerramienta(Herramienta::Triangulo);
    ImGui::SameLine();
    if (botonActivo("Balde", app.herramienta == Herramienta::Balde, medio))
        cambiarHerramienta(Herramienta::Balde);
    if (botonActivo("Seleccion", app.herramienta == Herramienta::Seleccion, completo))
        cambiarHerramienta(Herramienta::Seleccion);
    // El tamano del borrador solo se muestra cuando el borrador esta activo
    if (app.herramienta == Herramienta::Borrador)
    {
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::SliderInt("##grosor", &app.grosorBorrador, 1, 6, "Grosor del borrador: %d");
    }

    // --- Objeto seleccionado ---
    ImGui::SeparatorText("Objeto seleccionado");
    Figura* sel = figuraSeleccionada();
    if (sel)
        ImGui::TextColored(aImVec4(COL_SELECCION), "%s #%d%s", nombreFigura(sel->tipo), app.seleccion + 1,
            sel->relleno ? "  (relleno)" : "");
    else
        ImGui::TextDisabled("Ninguno (clic derecho o TAB)");

    // Para rellenar se usa el balde (o la tecla F); aqui solo se quita el relleno
    ImGui::BeginDisabled(!sel || !esRellenable(sel->tipo) || !sel->relleno);
    if (ImGui::Button("Sin relleno", completo)) quitarRelleno();
    ImGui::EndDisabled();

    const ImVec2 tercio((ancho - 2 * esp) / 3, 0);
    if (ImGui::Button("< Anterior", tercio)) seleccionarSiguiente(-1);
    ImGui::SameLine();
    ImGui::BeginDisabled(!sel);
    if (ImGui::Button("Borrar", tercio)) borrarSeleccion();
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Siguiente >", tercio)) seleccionarSiguiente(1);

    // --- Color y opacidad ---
    ImGui::SeparatorText("Color");
    int objetivo = (int)app.objetivoColor;
    ImGui::RadioButton("Borde", &objetivo, (int)ObjetivoColor::Borde);
    ImGui::SameLine();
    ImGui::RadioButton("Relleno", &objetivo, (int)ObjetivoColor::Relleno);
    app.objetivoColor = (ObjetivoColor)objetivo;

    Color& color = colorEditado();
    float rgba[4] = { color.r, color.g, color.b, color.a };
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::ColorEdit4("##color", rgba, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf))
    {
        iniciarEdicion();
        color = { rgba[0], rgba[1], rgba[2], rgba[3] };
        aplicarColorASeleccion();
    }

    // Paleta rápida (conserva la opacidad actual)
    const int porFila = 8;
    const float lado = (ancho - (porFila - 1) * esp) / porFila;
    for (int i = 0; i < NUM_PALETA; ++i)
    {
        ImGui::PushID(i);
        if (ImGui::ColorButton("##paleta", aImVec4(PALETA[i]), ImGuiColorEditFlags_NoTooltip, ImVec2(lado, lado)))
        {
            if (figuraSeleccionada()) guardarHistorial();
            color = { PALETA[i].r, PALETA[i].g, PALETA[i].b, color.a };
            aplicarColorASeleccion();
        }
        // Marca el color de la paleta que coincide con el color actual
        if (std::fabs(PALETA[i].r - color.r) < 0.01f && std::fabs(PALETA[i].g - color.g) < 0.01f &&
            std::fabs(PALETA[i].b - color.b) < 0.01f)
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);
        ImGui::PopID();
        if ((i + 1) % porFila != 0) ImGui::SameLine();
    }

    float opacidad = color.a * 100.0f;
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::SliderFloat("##opacidad", &opacidad, 0.0f, 100.0f, "Opacidad: %.0f %%"))
    {
        iniciarEdicion();
        color.a = opacidad / 100.0f;
        aplicarColorASeleccion();
    }

    // --- Rotación ---
    ImGui::SeparatorText("Rotacion");
    ImGui::BeginDisabled(!sel);
    float angulo = sel ? sel->angulo : 0.0f;
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::SliderFloat("##angulo", &angulo, -180.0f, 180.0f, "%.0f grados") && sel)
    {
        iniciarEdicion();
        sel->angulo = std::round(angulo);
    }
    ImGui::EndDisabled();

    // --- Acciones ---
    ImGui::SeparatorText("Vista");
    ImGui::Checkbox("Mostrar cuadricula", &app.mostrarGrilla);
    ImGui::Checkbox("Mostrar marco de seleccion", &app.mostrarMarcoSeleccion);

    ImGui::SeparatorText("Acciones");
    ImGui::BeginDisabled(app.historial.empty());
    if (ImGui::Button("Deshacer (Z)", medio)) deshacer();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(app.figuras.empty());
    if (ImGui::Button("Limpiar todo", medio)) limpiarTodo();
    ImGui::EndDisabled();

    // --- Estado ---
    ImGui::SeparatorText("Estado");
    switch (app.herramienta)
    {
    case Herramienta::Linea:     ImGui::TextWrapped("Clic en el inicio y clic en el final."); break;
    case Herramienta::Circulo:   ImGui::TextWrapped("Clic en el centro y clic para el radio."); break;
    case Herramienta::Triangulo: ImGui::TextWrapped("Clic en los 3 vertices."); break;
    case Herramienta::Seleccion: ImGui::TextWrapped("Clic sobre una figura y arrastra para moverla."); break;
    case Herramienta::Dibujo:    ImGui::TextWrapped("Manten presionado el clic y arrastra para dibujar."); break;
    case Herramienta::Balde:     ImGui::TextWrapped("Clic sobre la figura que quieres rellenar."); break;
    case Herramienta::Borrador:  ImGui::TextWrapped("Manten presionado el clic y arrastra para borrar."); break;
    }
    bool herramientaPorClics = app.herramienta == Herramienta::Linea || app.herramienta == Herramienta::Circulo ||
        app.herramienta == Herramienta::Triangulo;
    if (herramientaPorClics)
        ImGui::Text("Puntos: %d / %d", (int)app.clicsPendientes.size(), (int)clicsNecesarios(app.herramienta));
    // Los trazos del borrador no cuentan como figuras
    int figurasVisibles = 0;
    for (const Figura& f : app.figuras)
        if (f.tipo != TipoFigura::Borrador) ++figurasVisibles;
    ImGui::Text("Figuras: %d", figurasVisibles);
    if (mouseEnLienzo)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("   Celda (%d, %d)", celdaMouse.first, celdaMouse.second);
    }
    if (!app.mensaje.empty() && glfwGetTime() - app.tiempoMensaje < 2.5)
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.25f, 1.0f), "%s", app.mensaje.c_str());

    if (ImGui::CollapsingHeader("Atajos de teclado"))
    {
        ImGui::BulletText("L / C / T / S / B / G: herramientas");
        ImGui::BulletText("Clic derecho o TAB: seleccionar");
        ImGui::BulletText("F: rellenar / quitar relleno");
        ImGui::BulletText("Q / E: rotar 15 grados");
        ImGui::BulletText("Flechas: mover la seleccion");
        ImGui::BulletText("Z: deshacer   Supr: borrar");
        ImGui::BulletText("Esc: cancelar / deseleccionar");
    }

    // Al soltar un slider o el selector de color termina la edición (siguiente paso de deshacer)
    if (!ImGui::IsAnyItemActive()) app.edicionEnCurso = false;

    ImGui::End();
}

// Carga una fuente del sistema que tenga tildes y eñes; si no hay, usa la de ImGui
void cargarFuente(ImGuiIO& io)
{
    const char* candidatas[] = {
        "C:/Windows/Fonts/segoeui.ttf",                     // Windows
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",  // Linux
        "/System/Library/Fonts/Supplemental/Arial.ttf",     // macOS
    };
    for (const char* ruta : candidatas)
    {
        if (std::ifstream(ruta).good() && io.Fonts->AddFontFromFileTTF(ruta, 17.0f))
            return;
    }
    io.Fonts->AddFontDefault();
}

// ---------------------------------------------------------------------------
// Programa principal
// ---------------------------------------------------------------------------
int main()
{
    // --- Configuración Inicial de GLFW y OpenGL ---
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
    GLFWwindow* window = glfwCreateWindow(anchoVentana, altoVentana, "Paint Bresenham - OpenGL + Dear ImGui", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Error al crear la ventana" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowSizeLimits(window, 900, 600, GLFW_DONT_CARE, GLFW_DONT_CARE);

    // Nuestros callbacks se registran ANTES de iniciar ImGui para que ImGui los encadene
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);

    gladLoadGL();

    // --- Dear ImGui ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // no guardar imgui.ini
    ImGui::StyleColorsDark();
    ImGui::GetStyle().FrameRounding = 4.0f;
    ImGui::GetStyle().WindowRounding = 0.0f;
    cargarFuente(io);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Transparencia: mezcla alpha estándar
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader shaderProgram("default.vert", "default.frag");
    GLint uResolucion = glGetUniformLocation(shaderProgram.ID, "uResolucion");

    Renderer2D r;
    // --- Bucle de Renderizado ---
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        int fbW, fbH, winW, winH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glfwGetWindowSize(window, &winW, &winH);
        if (fbW == 0 || fbH == 0) { glfwWaitEvents(); continue; } // ventana minimizada

        // Escala entre coordenadas de ventana y píxeles del framebuffer (pantallas HiDPI)
        float escala = (float)fbW / std::max(winW, 1);

        // Posición del ratón en píxeles del framebuffer
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        app.mouseX = mx * escala;
        app.mouseY = my * escala;

        // --- Disposición del lienzo (grilla cuadrada centrada a la derecha del panel) ---
        const float margen = 20.0f * escala;
        float panel = PANEL_ANCHO * escala;
        float areaW = fbW - panel;
        float lado = std::max(1.0f, std::min(areaW, (float)fbH) - 2 * margen);
        float cs = std::max(2.0f, std::floor(lado / GRID_SIZE)); // tamaño de celda
        float lienzo = cs * GRID_SIZE;
        float lx = std::floor(panel + (areaW - lienzo) / 2);
        float ly = std::floor((fbH - lienzo) / 2);

        // Celda bajo el ratón (fila 0 abajo, como en el plano cartesiano)
        int colMouse = (int)std::floor((app.mouseX - lx) / cs);
        int filaMouse = GRID_SIZE - 1 - (int)std::floor((app.mouseY - ly) / cs);
        bool mouseEnLienzo = app.mouseX >= lx && app.mouseX < lx + lienzo &&
            app.mouseY >= ly && app.mouseY < ly + lienzo &&
            !ImGui::GetIO().WantCaptureMouse;
        Celda celdaMouse = { colMouse, filaMouse };

        // --- Panel de ImGui (sus botones ejecutan las acciones directamente) ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        dibujarPanel((float)winH, mouseEnLienzo, celdaMouse);

        // --- Interacción con el lienzo ---
        // Clic derecho: seleccionar la figura bajo el cursor con cualquier herramienta
        if (mouseEnLienzo && app.clicDerecho)
        {
            app.clicsPendientes.clear();
            int indice = buscarFiguraEn(celdaMouse);
            seleccionar(indice);
            if (indice < 0) mostrarMensaje("No hay ninguna figura ahi");
        }

        if (mouseEnLienzo && app.clicPresionado)
        {
            if (app.herramienta == Herramienta::Seleccion)
            {
                int indice = buscarFiguraEn(celdaMouse);
                seleccionar(indice);
                app.arrastrando = indice >= 0;
                app.arrastreGuardado = false;
                app.celdaArrastre = celdaMouse;
            }
            else if (app.herramienta == Herramienta::Dibujo)
            {
                empezarTrazo(TipoFigura::Dibujo, celdaMouse);
            }
            else if (app.herramienta == Herramienta::Borrador)
            {
                empezarTrazo(TipoFigura::Borrador, celdaMouse);
            }
            else if (app.herramienta == Herramienta::Balde)
            {
                rellenarConBalde(celdaMouse);
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
                        mostrarMensaje("El radio es demasiado chico");
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
        // Trazo libre o borrador: mientras el boton siga presionado se agrega la celda del raton
        if (app.trazando && app.clicAbajo && mouseEnLienzo &&
            app.trazoActual >= 0 && app.trazoActual < (int)app.figuras.size())
        {
            Figura& trazo = app.figuras[app.trazoActual];
            bool celdaNueva = trazo.puntos.empty() ||
                (int)trazo.puntos.back().x != celdaMouse.first ||
                (int)trazo.puntos.back().y != celdaMouse.second;
            if (celdaNueva)
                trazo.puntos.push_back({ (float)celdaMouse.first, (float)celdaMouse.second });
        }
        if (app.clicSoltado)
        {
            app.arrastrando = false;
            terminarTrazo();
        }

        // --- Dibujo del lienzo ---
        glViewport(0, 0, fbW, fbH);
        glClearColor(COL_FONDO.r, COL_FONDO.g, COL_FONDO.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        shaderProgram.Activate();
        glUniform2f(uResolucion, (float)fbW, (float)fbH);

        // Posición en pantalla de una celda (c, f)
        auto xCelda = [&](int c) { return lx + c * cs; };
        auto yCelda = [&](int f) { return ly + (GRID_SIZE - 1 - f) * cs; };
        // Margen del 90% para dejar una separación visible entre cuadrados (como la grilla original)
        float tamCelda = (app.mostrarGrilla && cs > 4) ? cs * 0.9f : cs;
        auto pintarCeldas = [&](const ConjuntoCeldas& celdas, const Color& c) {
            for (const Celda& celda : celdas)
                r.rect(xCelda(celda.first), yCelda(celda.second), tamCelda, tamCelda, c);
            };

        // Grilla de fondo
        if (app.mostrarGrilla)
        {
            r.rect(lx, ly, lienzo, lienzo, COL_LINEAS_GRID);
            for (int f = 0; f < GRID_SIZE; ++f)
                for (int c = 0; c < GRID_SIZE; ++c)
                    r.rect(xCelda(c), yCelda(f), tamCelda, tamCelda, COL_CELDA);
        }
        else
        {
            r.rect(lx, ly, lienzo, lienzo, COL_CELDA);
        }

        // Figuras (en orden: las últimas quedan encima y se mezclan según su opacidad).
        // Primero el interior con el color de relleno y luego el borde con su color.
        for (const Figura& f : app.figuras)
        {
            ConjuntoCeldas contorno, interior;
            celdasFigura(f, f.relleno, contorno, interior);
            pintarCeldas(interior, f.colorRelleno);
            pintarCeldas(contorno, f.color);
        }

        // Marco de la figura seleccionada y su centro de rotación
        Figura* selMarco = app.mostrarMarcoSeleccion ? figuraSeleccionada() : nullptr;
        if (Figura* sel = selMarco)
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
        if (!app.clicsPendientes.empty() && (app.herramienta == Herramienta::Linea ||
            app.herramienta == Herramienta::Circulo || app.herramienta == Herramienta::Triangulo))
        {
            if (mouseEnLienzo)
            {
                std::vector<Celda> clics = app.clicsPendientes;
                clics.push_back(celdaMouse);
                Figura previa = construirFigura(app.herramienta, clics);
                Color cp = app.colorBorde;
                cp.a *= 0.5f;
                pintarCeldas(rasterizarFigura(previa), cp);
            }
            for (const Celda& c : app.clicsPendientes)
                r.rectBorde(xCelda(c.first) - 2, yCelda(c.second) - 2, cs + 3, cs + 3, 2, { 0.1f, 0.1f, 0.1f, 1 });
        }

        // Tamano del borrador bajo el cursor
        if (mouseEnLienzo && app.herramienta == Herramienta::Borrador)
        {
            int desde = -(app.grosorBorrador - 1) / 2, hasta = app.grosorBorrador / 2;
            float lado = (hasta - desde + 1) * cs;
            r.rectBorde(xCelda(colMouse + desde), yCelda(filaMouse + hasta), lado, lado, 1,
                { 0.35f, 0.35f, 0.40f, 1.0f });
        }

        // Celda bajo el cursor
        if (mouseEnLienzo && app.mostrarGrilla)
            r.rectBorde(xCelda(colMouse), yCelda(filaMouse), tamCelda, tamCelda, 1, COL_SELECCION);

        r.dibujar();

        // --- ImGui se dibuja encima del lienzo ---
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        // Reiniciar los eventos "de un solo frame"
        app.clicPresionado = false;
        app.clicSoltado = false;
        app.clicDerecho = false;
    }

    // --- Limpieza ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    r.eliminar();
    shaderProgram.Delete();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}