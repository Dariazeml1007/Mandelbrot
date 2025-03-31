#include <cmath>
#include "txlib.h"

const int WIDTH = 800;
const int HEIGHT = 600;
const int MAX_ITER = 256;
const int R_MAX = 4;

RGBQUAD getColor(int iter);
int mandelbrot(float x0, float y0);

int mandelbrot(float x0, float y0) {
    float x = 0, y = 0;
    int iter = 0;
    while (iter < MAX_ITER) {
        float x2 = x * x;
        float y2 = y * y;

        if (x2 + y2 >= R_MAX) break;

        float xy = x * y;
        x = x2 - y2 + x0;
        y = 2 * xy + y0;
        iter++;
    }
    return iter;
}

RGBQUAD getColor(int iter)
{
    if (iter == MAX_ITER) return RGBQUAD{0, 0, 0};

    int r = (iter * 55) & 0xFF ;
    int g = (iter * 256) & 0xFF ;
    int b = (iter * 125) & 0xFF ;

    return RGBQUAD{(BYTE)b, (BYTE)g, (BYTE)r};
}

int main()
{
    txCreateWindow(WIDTH, HEIGHT);
    txSetFillColor(TX_WHITE);
    txClear();

    float xMin = -2.5f, xMax = 1.5f;
    float yMin = -1.5f, yMax = 1.5f;

    for (int py = 0; py < HEIGHT; py++)
    {
        for (int px = 0; px < WIDTH; px++)
        {
            float x0 = xMin + (xMax - xMin) * px / WIDTH;
            float y0 = yMin + (yMax - yMin) * py / HEIGHT;

            int iter = mandelbrot(x0, y0);
            RGBQUAD quad = getColor(iter);
            COLORREF color = RGB(quad.rgbRed, quad.rgbGreen, quad.rgbBlue);
            txSetPixel(px, py, color);
        }

        txSleep(1);
    }

    txTextCursor(false);
    return 0;
}
