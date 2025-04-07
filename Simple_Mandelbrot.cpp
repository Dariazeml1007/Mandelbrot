#include <cmath>
#include <cstring>
#include "txlib.h"

const int WIDTH = 800;
const int HEIGHT = 600;
const int MAX_ITER = 256;
const int R_MAX = 4;

struct ViewParams
{
    float xMin = -2.5f;
    float xMax = 1.5f;
    float yMin = -1.5f;
    float yMax = 1.5f;
    bool needRedraw = true;

    void reset()
    {
        xMin = -2.5f;
        xMax = 1.5f;
        yMin = -1.5f;
        yMax = 1.5f;
    }

    void zoom(float factor)
    {
        float centerX = (xMin + xMax) / 2;
        float centerY = (yMin + yMax) / 2;

        float width = (xMax - xMin) / factor;
        float height = (yMax - yMin) / factor;

        xMin = centerX - width / 2;
        xMax = centerX + width / 2;
        yMin = centerY - height / 2;
        yMax = centerY + height / 2;
    }

    void move(float dx, float dy)
    {
        float width = xMax - xMin;
        float height = yMax - yMin;

        xMin += dx * width;
        xMax += dx * width;
        yMin += dy * height;
        yMax += dy * height;
    }
};

RGBQUAD get_color(int iter);
int mandelbrot(float x0, float y0);
void run_time(ViewParams *viewParams);
void draw_mandelbrot(ViewParams *viewParams);
void count_mandelbrot(COLORREF* colorBuffer, ViewParams *viewParams);
void display(COLORREF* colorBuffer, ViewParams *viewParams);
void handleInput(ViewParams* viewParams);

int main(const int argc, const char* argv[])
{
    ViewParams viewParams;

    if (argc > 1 && strcmp(argv[1], "--graphics") == 0)
    {
        viewParams.needRedraw = true;
        txCreateWindow(WIDTH, HEIGHT);
        txTextCursor(false);
        while (!GetAsyncKeyState(VK_ESCAPE))
        {
            handleInput(&viewParams);

            if (viewParams.needRedraw )
            {
                draw_mandelbrot(&viewParams);
                viewParams.needRedraw  = false;
            }

            txSleep(10);
        }
    }
    else
    {
        run_time(&viewParams);
    }
    return 0;
}

void handleInput(ViewParams *viewParams)
{
    assert(viewParams);

    const float moveStep = 0.2f;
    const float zoomFactor = 1.5f;

    if (GetAsyncKeyState(VK_LEFT))
    {
        viewParams->move(-moveStep, 0);
        viewParams->needRedraw =  true;
    }
    if (GetAsyncKeyState(VK_RIGHT))
    {
        viewParams->move(moveStep, 0);
        viewParams->needRedraw =  true;
    }
    if (GetAsyncKeyState(VK_UP))
    {
        viewParams->move(0, -moveStep);
        viewParams->needRedraw =  true;
    }
    if (GetAsyncKeyState(VK_DOWN)) {
        viewParams->move(0, moveStep);
        viewParams->needRedraw =  true;
    }

    if (GetAsyncKeyState(VK_ADD) )
    {
        viewParams->zoom(zoomFactor);
        viewParams->needRedraw =  true;
    }
    if (GetAsyncKeyState(VK_SUBTRACT) )
    {
        viewParams->zoom(1/zoomFactor);
        viewParams->needRedraw =  true;
    }

    if (GetAsyncKeyState(VK_SPACE))
    {
        viewParams->reset();
        viewParams->needRedraw = true;
    }

    return;
}

RGBQUAD get_color(int iter)
{
    if (iter == MAX_ITER) return RGBQUAD{0, 0, 0};

    int r = (iter * 197) & 0xFF;
    int g = (iter * 237) & 0xFF;
    int b = (iter * 255) & 0xFF;

    return RGBQUAD{(BYTE)b, (BYTE)g, (BYTE)r};
}

void display(COLORREF* colorBuffer, ViewParams *viewParams)
{
    assert(colorBuffer);
    assert(viewParams);

    for (int py = 0; py < HEIGHT; py++)
    {
        for (int px = 0; px < WIDTH; px++)
        {
             txSetPixel(px, py, colorBuffer[py * WIDTH + px]);
        }
    }
    char info[256];
    sprintf(info, "X: [%.5f, %.5f] Y: [%.5f, %.5f]",
            viewParams->xMin, viewParams->xMax, viewParams->yMin, viewParams->yMax);
    txSetColor(TX_WHITE);
    txTextOut(10, 10, info);
}

void draw_mandelbrot(ViewParams *viewParams)
{
    assert(viewParams);

    COLORREF* colorBuffer = (COLORREF*)calloc(HEIGHT * WIDTH, sizeof(COLORREF));
    if (!colorBuffer) return;

    count_mandelbrot(colorBuffer, viewParams);
    display(colorBuffer, viewParams);

    free(colorBuffer);
}

void run_time(ViewParams *viewParams)
{
    assert(viewParams);

    COLORREF* colorBuffer = (COLORREF*)calloc(HEIGHT * WIDTH, sizeof(COLORREF));
    if (!colorBuffer) return;

    clock_t start = clock();
    count_mandelbrot(colorBuffer, viewParams);
    double time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Render time: %.3f seconds\n", time);

    free(colorBuffer);
}




void count_mandelbrot(COLORREF* colorBuffer, ViewParams *viewParams)
{
    assert(colorBuffer);
    assert(viewParams);

    for (int py = 0; py < HEIGHT; py++)
    {
        for (int px = 0; px < WIDTH; px++)
        {
            float x0 =  viewParams->xMin + ( viewParams->xMax -  viewParams->xMin) * (float)px / (float)WIDTH;
            float y0 =  viewParams->yMin + ( viewParams->yMax -  viewParams->yMin) * (float)py / (float)HEIGHT;

            int iter = mandelbrot(x0, y0);
            RGBQUAD quad = get_color(iter);
            colorBuffer[py * WIDTH + px] = RGB(quad.rgbRed, quad.rgbGreen, quad.rgbBlue);
        }
    }
}


int mandelbrot(float x0, float y0)
{
    float x = 0, y = 0;
    int iter = 0;
    while (iter < MAX_ITER)
    {
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
