#include <config.h>
#include <GLFW/glfw3.h>

typedef struct TouchPoint {
    int id, x, y, state;
} TouchPoint;

typedef void (*TouchCallback)(TouchPoint[MAX_TOUCH_POINTS], int);

void InitWindowsTouch(GLFWwindow *window);
void GetWindowsTouchPoints(TouchPoint *outArray, int *outCount);
void SetWindowsTouchCallback(TouchCallback cb);
