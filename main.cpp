#include <cmath>
#include "txlib.h"

const int WIDTH = 800;
const int HEIGHT = 600;
const int MAX_ITER = 256;
const int R_MAX = 4;
static COLORREF colorBuffer[HEIGHT][WIDTH];

RGBQUAD get_color(int iter);
int mandelbrot(float x0, float y0);
void run_time(void);
void draw_mandelbrot (void);
void count_mandelbrot(void);
void display(void);

int main(const int argc, const char* argv[])
{
    if (argc > 1 && strcmp(argv[1], "--graphics" ) == 0)
    {
        draw_mandelbrot ();
    }
    else
    {
        run_time();
    }
    return 0;
}


int mandelbrot(float x0, float y0)
{
    float x = 0, y = 0;
    int iter = 0;
    while (iter < MAX_ITER)
    {
        float x2 = x * x;  // Re(z_n^2)
        float y2 = y * y;  // Im(z_n^2)
        if (x2 + y2 >= R_MAX) break;  // |z_n| ≥ 2

        float xy = x * y;
        x = x2 - y2 + x0;  // Re(z_(n+1)) = Re(z_n^2) + Re(c)
        y = 2 * xy + y0;   // Im(z_(n+1)) = Im(z_n^2) + Im(c)
        iter++;
    }
    return iter;
}

RGBQUAD get_color(int iter)
{
    if (iter == MAX_ITER) return RGBQUAD{0, 0, 0};

    int r = (iter * 197) & 0xFF;
    int g = (iter * 237) & 0xFF;
    int b = (iter * 255) & 0xFF;

    return RGBQUAD{(BYTE)b, (BYTE)g, (BYTE)r};
}

void draw_mandelbrot (void)
{
    count_mandelbrot();
    display();
}

void count_mandelbrot(void)
{
    float xMin = -2.5f, xMax = 1.5f;
    float yMin = -1.5f, yMax = 1.5f ;

    for (int py = 0; py < HEIGHT; py++)
    {
        for (int px = 0; px < WIDTH; px++)
        {
            float x0 = xMin + (xMax - xMin) * (float)px / (float)WIDTH;
            float y0 = yMin + (yMax - yMin) * (float)py / (float)HEIGHT;

            int iter = mandelbrot(x0, y0);
            RGBQUAD quad = get_color(iter);
            colorBuffer[py][px] = RGB(quad.rgbRed, quad.rgbGreen, quad.rgbBlue);
        }
    }
}


void display(void)
{
    txCreateWindow(WIDTH, HEIGHT);

    for (int py = 0; py < HEIGHT; py++)
    {
        for (int px = 0; px < WIDTH; px++)
        {
            txSetPixel(px, py, colorBuffer[py][px]);
        }
    }

    txTextCursor(false);
}


void run_time(void)
{
    clock_t start = clock();

    count_mandelbrot();

    clock_t end = clock();
    double render_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Render time: %.3f seconds\n", render_time);
}
