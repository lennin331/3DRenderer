#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cmath>
#include <vector>

#define PI 3.14159265358979323846


struct Point3D {
    float x, y, z;
    Point3D(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};

struct Point2D {
    int x, y;
    Point2D(int x = 0, int y = 0) : x(x), y(y) {}
};

std::vector<Point3D> cubeVertices = {

        Point3D(-0.25f, -0.25f, -0.25f), 

        Point3D(0.25f, -0.25f, -0.25f), 
        Point3D(0.25f, -0.25f,  0.25f),
        Point3D(-0.25f, -0.25f,  0.25f),
        Point3D(-0.25f,  0.25f, -0.25f), 
        Point3D(0.25f,  0.25f, -0.25f),
        Point3D(0.25f,  0.25f,  0.25f), 
        Point3D(-0.25f,  0.25f,  0.25f)
};

std::vector<std::pair<int, int>> cubeEdges = {
    {0,1}, {1,2}, {2,3}, {3,0}, 
    {4,5}, {5,6}, {6,7}, {7,4},
    {0,4}, {1,5}, {2,6}, {3,7}  
};

class Renderer3D {
private:
    HWND hwnd;
    int width, height;
    float rotationX, rotationY, rotationZ;
    float distance;
    int frameCount;
    float fps;
    DWORD lastTime;
    Point2D project(const Point3D& point) {
        Point3D rotated = point;

        float cosX = cos(rotationX);
        float sinX = sin(rotationX);
        float y1 = rotated.y * cosX - rotated.z * sinX;
        float z1 = rotated.y * sinX + rotated.z * cosX;
        rotated.y = y1;
        rotated.z = z1;

        float cosY = cos(rotationY);
        float sinY = sin(rotationY);
        float x1 = rotated.x * cosY + rotated.z * sinY;
        float z2 = -rotated.x * sinY + rotated.z * cosY;
        rotated.x = x1;
        rotated.z = z2;

        float cosZ = cos(rotationZ);
        float sinZ = sin(rotationZ);
        float x2 = rotated.x * cosZ - rotated.y * sinZ;
        float y2 = rotated.x * sinZ + rotated.y * cosZ;
        rotated.x = x2;
        rotated.y = y2;

        float perspective = distance / (distance + rotated.z);
        int screenX = (int)(rotated.x * perspective * width / 2 + width / 2);
        int screenY = (int)(-rotated.y * perspective * height / 2 + height / 2);

        return Point2D(screenX, screenY);
    }

public:
    Renderer3D(HWND window, int w, int h)
        : hwnd(window), width(w), height(h),
        rotationX(0), rotationY(0), rotationZ(0), distance(20),
        frameCount(0), fps(0), lastTime(GetTickCount()){
    }

    void updateRotation(float dx, float dy, float dz) {
        rotationX += dx;
        rotationY += dy;
        rotationZ += dz;
    }
    
    void updateFPS() {
        frameCount++;
        DWORD currentTime = GetTickCount();
        if (currentTime - lastTime >= 1000) {
            fps = frameCount * 1000.0f / (currentTime - lastTime);
            frameCount = 0;
            lastTime = currentTime;
        }
	}

    void render(HDC hdc) {

        RECT rect;
        GetClientRect(hwnd, &rect);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(memDC, &rect, blackBrush);
        DeleteObject(blackBrush);
		updateFPS();
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
        HPEN oldPen = (HPEN)SelectObject(memDC, pen);

        std::vector<Point2D> projectedVertices;
        for (const auto& vertex : cubeVertices) {
            projectedVertices.push_back(project(vertex));
        }

        for (const auto& edge : cubeEdges) {
            const Point2D& p1 = projectedVertices[edge.first];
            const Point2D& p2 = projectedVertices[edge.second];
            MoveToEx(memDC, p1.x, p1.y, NULL);
            LineTo(memDC, p2.x, p2.y);
        }

        HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
        HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, redBrush);

        for (const auto& vertex : projectedVertices) {
            Ellipse(memDC, vertex.x - 3, vertex.y - 3, vertex.x + 3, vertex.y + 3);
        }

        SelectObject(memDC, oldBrush);
        DeleteObject(redBrush);
        SelectObject(memDC, oldPen);
        DeleteObject(pen);

        char text[256];
        sprintf_s(text, "Rotation X: %.2f  Y: %.2f  Z: %.2f", rotationX, rotationY, rotationZ);
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, RGB(0, 255, 0));

        wchar_t wtext[256];
        MultiByteToWideChar(CP_ACP, 0, text, -1, wtext, 256);
        TextOutW(memDC, 10, 10, wtext, (int)wcslen(wtext));

        wchar_t instructions[] = L"Controls: WASD to rotate | SPACE: auto-rotate | ESC: exit";
        TextOutW(memDC, 10, 40, instructions, (int)wcslen(instructions));

		wchar_t fpsText[256];
		swprintf_s(fpsText, L"FPS: %.2f", fps);
		SetTextColor(memDC, RGB(255, 255, 0));
		TextOutW(memDC, 10, 70, fpsText, (int)wcslen(fpsText));

        BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
    }
};

Renderer3D* g_renderer = nullptr;
bool g_animating = true;
bool g_leftRotating = false;
bool g_rightRotating = false;
bool g_upRotating = false;
bool g_downRotating = false;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_ESCAPE:
            PostQuitMessage(0);
            return 0;
        case 'A':
            g_leftRotating = true;
            break;
        case 'D':
            g_rightRotating = true;
            break;
        case 'W':
            g_upRotating = true;
            break;
        case 'S':
            g_downRotating = true;
            break;
        case VK_SPACE:
            g_animating = !g_animating;
            break;
        }
        return 0;

    case WM_KEYUP:
        switch (wParam) {
        case 'A':
            g_leftRotating = false;
            break;
        case 'D':
            g_rightRotating = false;
            break;
        case 'W':
            g_upRotating = false;
            break;
        case 'S':
            g_downRotating = false;
            break;
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (g_renderer) {
            g_renderer->render(hdc);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR eventId, DWORD dwTime) {
    if (g_renderer) {
        if (g_leftRotating) g_renderer->updateRotation(0, -0.05f, 0);
        if (g_rightRotating) g_renderer->updateRotation(0, 0.05f, 0);
        if (g_upRotating) g_renderer->updateRotation(-0.05f, 0, 0);
        if (g_downRotating) g_renderer->updateRotation(0.05f, 0, 0);

        if (g_animating && !g_leftRotating && !g_rightRotating && !g_upRotating && !g_downRotating) {
            g_renderer->updateRotation(0.02f, 0.03f, 0.01f);
        }

        InvalidateRect(hwnd, NULL, TRUE);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"3DRendererClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    wc.hbrBackground = NULL;  
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        L"3DRendererClass",
        L"Simple 3D Cube Renderer",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);


    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    g_renderer = new Renderer3D(hwnd, width, height);

    SetTimer(hwnd, 1, 33, TimerProc);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    delete g_renderer;
    return 0;
}