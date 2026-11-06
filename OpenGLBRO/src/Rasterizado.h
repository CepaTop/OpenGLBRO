#ifndef RASTERIZADO_H
#define RASTERIZADO_H

#include <set>
#include <utility>

using Celda = std::pair<int, int>;          // (x = columna, y = fila)
using ConjuntoCeldas = std::set<Celda>;

// Línea de Bresenham entre (x0, y0) y (x1, y1)
ConjuntoCeldas calcularLineaBresenham(int x0, int y0, int x1, int y1);

// Círculo de Bresenham (punto medio) con centro (xc, yc) y radio r
ConjuntoCeldas calcularCirculoBresenham(int xc, int yc, int r);

// Contorno de un triángulo: tres líneas de Bresenham entre sus vértices
ConjuntoCeldas calcularTrianguloBresenham(int x0, int y0, int x1, int y1, int x2, int y2);

// Relleno por línea de barrido (scanline): para cada fila del contorno se pintan
// todas las celdas entre la x mínima y la x máxima. Válido para figuras convexas
// (círculo y triángulo).
ConjuntoCeldas rellenarScanline(const ConjuntoCeldas& contorno);

#endif