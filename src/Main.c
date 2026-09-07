#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define IDC_DISPLAY 1001
#define BTN_BASE 2000

static HWND hDisplay;
static double firstValue = 0.0;
static char pendingOp = 0;
static int newNumber = 1;

static void set_display(const char *text) {
    SetWindowTextA(hDisplay, text);
}

static void get_display(char *buf, int size) {
    GetWindowTextA(hDisplay, buf, size);
}

static double display_value(void) {
    char buf[128];
    get_display(buf, sizeof(buf));
    return strtod(buf, NULL);
}

static void format_number(double value) {
    char buf[128];
    if (fabs(value) < 1e-12) value = 0.0;
    snprintf(buf, sizeof(buf), "%.12g", value);
    set_display(buf);
}

static void calculate(void) {
    double second = display_value();
    double result = 0.0;

    switch (pendingOp) {
        case '+': result = firstValue + second; break;
        case '-': result = firstValue - second; break;
        case '*': result = firstValue * second; break;
        case '/':
            if (fabs(second) < 1e-15) {
                set_display("Error");
                pendingOp = 0;
                newNumber = 1;
                return;
            }
            result = firstValue / second;
            break;
        default: return;
    }

    format_number(result);
    firstValue = result;
    pendingOp = 0;
    newNumber = 1;
}

static void button_action(const char *text) {
    if (strcmp(text, "C") == 0) {
        firstValue = 0.0;
        pendingOp = 0;
        newNumber = 1;
        set_display("0");
        return;
    }

    if (strcmp(text, "⌫") == 0) {
        char buf[128];
        get_display(buf, sizeof(buf));
        if (newNumber || strcmp(buf, "Error") == 0) {
            set_display("0");
            newNumber = 1;
            return;
        }
        size_t len = strlen(buf);
        if (len <= 1 || (len == 2 && buf[0] == '-'))
            set_display("0");
        else {
            buf[len - 1] = '\0';
            set_display(buf);
        }
        return;
    }

    if (strcmp(text, ".") == 0) {
        char buf[128];
        get_display(buf, sizeof(buf));
        if (newNumber) {
            set_display("0.");
            newNumber = 0;
        } else if (!strchr(buf, '.')) {
            strcat(buf, ".");
            set_display(buf);
        }
        return;
    }

    if (strlen(text) == 1 && text[0] >= '0' && text[0] <= '9') {
        char buf[128];
        get_display(buf, sizeof(buf));
        if (newNumber || strcmp(buf, "Error") == 0) {
            set_display(text);
            newNumber = 0;
        } else if (strlen(buf) < 30) {
            if (strcmp(buf, "0") == 0) {
                set_display(text);
            } else {
                strcat(buf, text);
                set_display(buf);
            }
        }
        return;
    }

    if (strcmp(text, "%") == 0) {
        double v = display_value();
        format_number(v / 100.0);
        newNumber = 1;
        return;
    }

    if (strcmp(text, "√") == 0) {
        double v = display_value();
        if (v < 0) {
            set_display("Error");
        } else {
            format_number(sqrt(v));
        }
        newNumber = 1;
        return;
    }

    if (strcmp(text, "x²") == 0) {
        double v = display_value();
        format_number(v * v);
        newNumber = 1;
        return;
    }

    if (strcmp(text, "=") == 0) {
        if (pendingOp) calculate();
        return;
    }

    if (strlen(text) == 1 &&
        (text[0] == '+' || text[0] == '-' || text[0] == '*' || text[0] == '/')) {
        if (pendingOp && !newNumber)
            calculate();

        firstValue = display_value();
        pendingOp = text[0];
        newNumber = 1;
    }
}

static void set_button_font(HWND button) {
    HFONT font = CreateFontA(
        22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    SendMessageA(button, WM_SETFONT, (WPARAM)font, TRUE);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            hDisplay = CreateWindowExA(
                WS_EX_CLIENTEDGE, "EDIT", "0",
                WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_AUTOHSCROLL,
                15, 15, 350, 55, hwnd, (HMENU)IDC_DISPLAY,
                GetModuleHandle(NULL), NULL);

            HFONT displayFont = CreateFontA(
                30, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            SendMessageA(hDisplay, WM_SETFONT, (WPARAM)displayFont, TRUE);

            const char *labels[20] = {
                "C", "⌫", "%", "/",
                "7", "8", "9", "*",
                "4", "5", "6", "-",
                "1", "2", "3", "+",
                "0", ".", "x²", "√"
            };

            int startX = 15, startY = 85;
            int w = 80, h = 55, gap = 10;

            for (int i = 0; i < 20; i++) {
                int row = i / 4;
                int col = i % 4;
                HWND b = CreateWindowExA(
                    0, "BUTTON", labels[i],
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    startX + col * (w + gap),
                    startY + row * (h + gap),
                    w, h,
                    hwnd, (HMENU)(BTN_BASE + i),
                    GetModuleHandle(NULL), NULL);
                set_button_font(b);
            }

            HWND equal = CreateWindowExA(
                0, "BUTTON", "=",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                startX, startY + 5 * (h + gap),
                350, h,
                hwnd, (HMENU)(BTN_BASE + 20),
                GetModuleHandle(NULL), NULL);
            set_button_font(equal);

            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id >= BTN_BASE && id <= BTN_BASE + 20 &&
                HIWORD(wParam) == BN_CLICKED) {
                const char *labels[21] = {
                    "C", "⌫", "%", "/",
                    "7", "8", "9", "*",
                    "4", "5", "6", "-",
                    "1", "2", "3", "+",
                    "0", ".", "x²", "√", "="
                };
                button_action(labels[id - BTN_BASE]);
            }
            return 0;
        }

        case WM_KEYDOWN: {
            char text[2] = {0, 0};
            if (wParam >= '0' && wParam <= '9') {
                text[0] = (char)wParam;
                button_action(text);
            } else if (wParam == VK_ADD) button_action("+");
            else if (wParam == VK_SUBTRACT) button_action("-");
            else if (wParam == VK_MULTIPLY) button_action("*");
            else if (wParam == VK_DIVIDE) button_action("/");
            else if (wParam == VK_RETURN) button_action("=");
            else if (wParam == VK_DECIMAL) button_action(".");
            else if (wParam == VK_ESCAPE) button_action("C");
            else if (wParam == VK_BACK) button_action("⌫");
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "CCalculatorWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassA(&wc))
        return 1;

    HWND hwnd = CreateWindowExA(
        0, "CCalculatorWindow", "C Calculator",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 390, 485,
        NULL, NULL, hInstance, NULL);

    if (!hwnd)
        return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    SetFocus(hDisplay);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
