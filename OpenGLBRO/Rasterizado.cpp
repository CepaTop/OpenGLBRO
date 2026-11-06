#include "Rasterizado.h"

#include <cmath>
#include <cstdlib>
#include <map>

// 1. ALGORITMO DE BRESENHAM PARA LÍNEAS
// Calcula qué coordenadas enteras (x, y) de la grilla deben encenderse
ConjuntoCeldas calcularLineaBresenham(int x0, int y0, int x1, int y1)
{
    ConjuntoCeldas celdasLinea;

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

// 2. ALGORITMO DE BRESENHAM PARA CÍRCULOS
ConjuntoCeldas calcularCirculoBresenham(int xc, int yc, int r)
{
    ConjuntoCeldas celdasCirculo;
    int x = 0;
    int y = r;
    int d = 3 - 2 * r; // Parámetro de decisión inicial

    // Función lambda para aprovechar la simetría y pintar los 8 octantes a la vez
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

// 3. TRIÁNGULO: unión de las tres aristas trazadas con Bresenham
ConjuntoCeldas calcularTrianguloBresenham(int x0, int y0, int x1, int y1, int x2, int y2)
{
    ConjuntoCeldas triangulo = calcularLineaBresenham(x0, y0, x1, y1);
    ConjuntoCeldas arista2 = calcularLineaBresenham(x1, y1, x2, y2);
    ConjuntoCeldas arista3 = calcularLineaBresenham(x2, y2, x0, y0);
    triangulo.insert(arista2.begin(), arista2.end());
    triangulo.insert(arista3.begin(), arista3.end());
    return triangulo;
}

// 4. RELLENO POR LÍNEA DE BARRIDO (SCANLINE)
ConjuntoCeldas rellenarScanline(const ConjuntoCeldas& contorno)
{
    // Para cada fila (y) guardamos la x mínima y máxima del contorno
    std::map<int, std::pair<int, int>> tramos;
    for (const Celda& celda : contorno)
    {
        int x = celda.first;
        int y = celda.second;
        auto it = tramos.find(y);
        if (it == tramos.end())
        {
            tramos[y] = { x, x };
        }
        else
        {
            if (x < it->second.first)  it->second.first = x;
            if (x > it->second.second) it->second.second = x;
        }
    }

    // Pintamos cada tramo horizontal completo
    ConjuntoCeldas relleno;
    for (const auto& tramo : tramos)
    {
        int y = tramo.first;
        for (int x = tramo.second.first; x <= tramo.second.second; ++x)
            relleno.insert({ x, y });
    }
    return relleno;
}