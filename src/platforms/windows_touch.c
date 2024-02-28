#define WINVER 0x0601
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_NATIVE_INCLUDE_NONE // To avoid some symbols re-definition in windows.h
#include "GLFW/glfw3native.h"

#include <stdio.h>
#include <stdlib.h>

#include "windows_touch.h"

TouchCallback touchCallback = 0;
TouchPoint touchPoints[MAX_TOUCH_POINTS] = {0};
int touchPointCount = 0;

GLFWwindow *glfwWindow;
HWND hWnd;
WNDPROC glfwWndProc = NULL;
TOUCHINPUT touchInputs[MAX_TOUCH_POINTS] = { 0 };

CRITICAL_SECTION mutex;

void SetWindowsTouchCallback(TouchCallback cb)
{
    touchCallback = cb;
}

void GetWindowsTouchPoints(TouchPoint *out, int *outCount)
{
    EnterCriticalSection(&mutex);
    *outCount = touchPointCount;
    for (int i=0; i<MAX_TOUCH_POINTS; i++)
        out[i] = touchPoints[i];
    LeaveCriticalSection(&mutex);
}

int FindWindowsTouchPointIndex(int id)
{
    for (int i=0; i<MAX_TOUCH_POINTS; i++)
    {
        if (touchPoints[i].id == id)
            return i;
    }
    return -1;
}

void SetWindowsTouchPoints(PTOUCHINPUT inputs, int numInputs) {
    int screenWidth, screenHeight;
    glfwGetFramebufferSize(glfwWindow, &screenWidth, &screenHeight);

    EnterCriticalSection(&mutex);

    for (int i = 0; i < numInputs; i++)
    {
        TOUCHINPUT ti = inputs[i];
        int pointIndex;

        if (ti.dwFlags & TOUCHEVENTF_DOWN)
        {
            if (touchPointCount == MAX_TOUCH_POINTS)
                continue;

            pointIndex = touchPointCount++;
            touchPoints[pointIndex].state = 1;
        }
        else
        {
            pointIndex = FindWindowsTouchPointIndex(ti.dwID);
            if (pointIndex == -1)
                continue;
        }

        POINT point = { .x = TOUCH_COORD_TO_PIXEL(ti.x), .y = TOUCH_COORD_TO_PIXEL(ti.y) };
        ScreenToClient(hWnd, &point);

        touchPoints[pointIndex].id = ti.dwID;
        touchPoints[pointIndex].x = point.x;
        touchPoints[pointIndex].y = point.y;
        touchPoints[pointIndex].state = 1;

        if (ti.dwFlags & TOUCHEVENTF_UP)
        {
            // Was 4. Now 3.
            touchPointCount--;

            for (int j=pointIndex; i<touchPointCount; i++)
                touchPoints[j] = touchPoints[j+1];

            touchPoints[touchPointCount].state = 0;
            continue;
        }
    }

    LeaveCriticalSection(&mutex);
}

void ProcessWindowsTouch(WPARAM wParam, LPARAM lParam)
{
    UINT numInputs = LOWORD(wParam);
    if (GetTouchInputInfo((HTOUCHINPUT)lParam,
                          numInputs,
                          touchInputs,
                          sizeof(TOUCHINPUT)))
    {
        SetWindowsTouchPoints(touchInputs, numInputs);
        if (touchCallback)
            touchCallback(touchPoints, numInputs);
    }
}

LRESULT CALLBACK RaylibWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_TOUCH)
    {
        ProcessWindowsTouch(wParam, lParam);
        return 0;
    }
    else
        return CallWindowProc(glfwWndProc, hWnd, msg, wParam, lParam);
}

void InitWindowsTouch(GLFWwindow *window)
{
    InitializeCriticalSectionAndSpinCount(&mutex,0x00000400);

    glfwWindow = window;
    hWnd = glfwGetWin32Window(window);

    // Ask Windows to send WM_TOUCH events.
    RegisterTouchWindow(hWnd, 0);

    // Store a pointer to GLFW's WNDPROC, so RaylibWinProc can call it above.
    glfwWndProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);

    // Set the WNDPROC of the window to RaylibWinProc, to handle touch events.
    SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)RaylibWinProc);
}
