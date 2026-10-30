#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define _WIN32_WINNT 0x0601

#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <winhttp.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#define TRAY_MESSAGE (WM_APP + 1)
#define RESTORE_TRAY_MESSAGE (WM_APP + 2)
#define RESTORE_TRAY_RESTORED 1
#define RESTORE_TRAY_ALREADY_VISIBLE 2
#define TEMP_TRAY_ICON_TIMER 1
#define TEMP_TRAY_ICON_FALLBACK_MS 15000
#define APP_NAME L"SlimClash"
#define MENU_DASHBOARD 1001
#define MENU_CONFIG 1002
#define MENU_AUTOSTART 1003
#define MENU_EXIT 1005
#define MENU_CONSOLE 1006
#define MENU_HIDE_TRAY 1007
#define MENU_MODE_DIRECT 1008
#define MENU_MODE_GLOBAL 1009
#define MENU_MODE_RULE 1010
#define MENU_RELOAD_CONFIG 1011
#define MENU_GITHUB 1012
#define IDI_APP_ICON 101
#define AUTOSTART_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define AUTOSTART_VALUE L"SlimClash"
#define INTERNET_SETTINGS_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
#define USER_ENVIRONMENT_KEY L"Environment"
#define PROXY_OVERRIDE L"localhost;127.*;10.*;172.16.*;172.17.*;172.18.*;172.19.*;172.20.*;172.21.*;172.22.*;172.23.*;172.24.*;172.25.*;172.26.*;172.27.*;172.28.*;172.29.*;172.30.*;172.31.*;192.168.*"

static WCHAR g_module_path[MAX_PATH];
static WCHAR g_clash_dir[MAX_PATH];
static WCHAR g_proxy_path[MAX_PATH];
static WCHAR g_config_dir[MAX_PATH];
static WCHAR g_config_path[MAX_PATH];
static WCHAR g_hide_tray_path[MAX_PATH];
static WCHAR g_dashboard_url[512];
static WCHAR g_controller_url[512];
static char g_http_port[32];
static char g_socks_port[32];
static char g_mixed_port[32];
static char g_api_secret[512];
static char g_current_mode[16];
static PROCESS_INFORMATION g_proxy;
static HANDLE g_mutex;
static BOOL g_mutex_owned;
static BOOL g_console_visible;
static BOOL g_tray_hidden;
static BOOL g_hide_tray_marker;
static BOOL g_unhide_forwarded;
static BOOL g_temporary_tray_icon;
static BOOL g_proxy_settings_active;
static HICON g_icon;
static BOOL g_custom_icon;
static NOTIFYICONDATAW g_tray_icon;
static BOOL g_tray_added;
static UINT g_taskbar_created_message;

static void ShowSystemError(const WCHAR *operation, DWORD error)
{
    WCHAR system_message[512] = L"Unknown system error.";
    WCHAR message[1024];

    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, error, 0, system_message,
                   (DWORD)(sizeof(system_message) / sizeof(system_message[0])),
                   NULL);
    swprintf_s(message, sizeof(message) / sizeof(message[0]),
               L"%ls\n\n%ls (error code %lu)", operation, system_message, error);
    MessageBoxW(NULL, message, APP_NAME, MB_OK | MB_ICONERROR);
}

static BOOL AppendPath(WCHAR *buffer, size_t capacity, const WCHAR *part)
{
    return wcscat_s(buffer, capacity, part) == 0;
}

static BOOL ReadTrayHiddenMarker(BOOL *hidden)
{
    DWORD attributes = GetFileAttributesW(g_hide_tray_path);
    DWORD error;

    if (attributes != INVALID_FILE_ATTRIBUTES) {
        *hidden = (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
        if (!*hidden) {
            MessageBoxW(NULL, L"The tray visibility marker path is a directory.",
                        APP_NAME, MB_OK | MB_ICONERROR);
            return FALSE;
        }
        return TRUE;
    }

    error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
        *hidden = FALSE;
        return TRUE;
    }
    ShowSystemError(L"Could not read the tray visibility marker.", error);
    return FALSE;
}

static BOOL GetLauncherPaths(void)
{
    WCHAR executable_dir[MAX_PATH];
    WCHAR module_path[MAX_PATH];
    WCHAR *last_separator;
    DWORD length;

    length = GetModuleFileNameW(NULL, g_module_path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        ShowSystemError(L"Could not get the launcher path.", GetLastError());
        return FALSE;
    }
    length = GetFullPathNameW(g_module_path, MAX_PATH, module_path, NULL);
    if (length == 0 || length >= MAX_PATH) {
        ShowSystemError(L"Could not resolve the launcher path.", GetLastError());
        return FALSE;
    }
    wcscpy_s(g_module_path, MAX_PATH, module_path);

    wcscpy_s(executable_dir, MAX_PATH, g_module_path);
    last_separator = wcsrchr(executable_dir, L'\\');
    if (last_separator == NULL) {
        MessageBoxW(NULL, L"Could not determine the launcher directory.", APP_NAME,
                    MB_OK | MB_ICONERROR);
        return FALSE;
    }
    *last_separator = L'\0';
    wcscpy_s(g_hide_tray_path, MAX_PATH, executable_dir);
    if (!AppendPath(g_hide_tray_path, MAX_PATH, L"\\hide_tray_icon")) {
        MessageBoxW(NULL, L"The tray visibility marker path is too long.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    wcscpy_s(g_clash_dir, MAX_PATH, executable_dir);

    wcscpy_s(g_proxy_path, MAX_PATH, g_clash_dir);
    if (!AppendPath(g_proxy_path, MAX_PATH, L"\\clash.exe")) {
        MessageBoxW(NULL, L"The Clash executable path is too long.", APP_NAME,
                    MB_OK | MB_ICONERROR);
        return FALSE;
    }

    wcscpy_s(g_config_dir, MAX_PATH, g_clash_dir);
    if (!AppendPath(g_config_dir, MAX_PATH, L"\\.clash") ||
        wcscpy_s(g_config_path, MAX_PATH, g_config_dir) != 0 ||
        !AppendPath(g_config_path, MAX_PATH, L"\\config.yaml")) {
        MessageBoxW(NULL, L"The configuration directory path is too long.", APP_NAME,
                    MB_OK | MB_ICONERROR);
        return FALSE;
    }

    return TRUE;
}

static BOOL GetProcessImagePath(HANDLE process, WCHAR *path, DWORD capacity)
{
    DWORD length = capacity;
    WCHAR full_path[32768];
    DWORD full_length;

    if (!QueryFullProcessImageNameW(process, 0, path, &length))
        return FALSE;
    full_length = GetFullPathNameW(path, ARRAYSIZE(full_path), full_path, NULL);
    if (full_length == 0 || full_length >= ARRAYSIZE(full_path) ||
        full_length >= capacity)
        return FALSE;
    return wcscpy_s(path, capacity, full_path) == 0;
}

typedef struct LauncherWindowSearch {
    HWND window;
} LauncherWindowSearch;

static BOOL CALLBACK FindLauncherWindowCallback(HWND window, LPARAM parameter)
{
    LauncherWindowSearch *search = (LauncherWindowSearch *)parameter;
    WCHAR class_name[128];
    WCHAR image_path[32768];
    DWORD process_id;
    HANDLE process;

    if (GetClassNameW(window, class_name, ARRAYSIZE(class_name)) == 0 ||
        wcscmp(class_name, L"SlimClashLauncherWindow") != 0)
        return TRUE;

    GetWindowThreadProcessId(window, &process_id);
    if (process_id == 0 || process_id == GetCurrentProcessId())
        return TRUE;

    process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
    if (process == NULL)
        return TRUE;

    if (GetProcessImagePath(process, image_path, ARRAYSIZE(image_path)) &&
        _wcsicmp(image_path, g_module_path) == 0)
        search->window = window;
    CloseHandle(process);
    return search->window == NULL;
}

static HWND FindLauncherWindow(void)
{
    LauncherWindowSearch search;

    search.window = NULL;
    EnumWindows(FindLauncherWindowCallback, (LPARAM)&search);
    return search.window;
}

static BOOL TerminateMatchingProcesses(BOOL terminate_proxy, BOOL terminate_launcher)
{
    HANDLE snapshot;
    PROCESSENTRY32W entry;
    BOOL has_process;
    BOOL success = TRUE;
    DWORD current_pid = GetCurrentProcessId();

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        ShowSystemError(L"Could not inspect running processes.", GetLastError());
        return FALSE;
    }

    ZeroMemory(&entry, sizeof(entry));
    entry.dwSize = sizeof(entry);
    SetLastError(ERROR_SUCCESS);
    has_process = Process32FirstW(snapshot, &entry);
    if (!has_process && GetLastError() != ERROR_NO_MORE_FILES) {
        ShowSystemError(L"Could not read the running process list.", GetLastError());
        CloseHandle(snapshot);
        return FALSE;
    }

    while (has_process) {
        BOOL is_proxy = terminate_proxy &&
                        _wcsicmp(entry.szExeFile, L"clash.exe") == 0;
        BOOL is_launcher = terminate_launcher &&
                           _wcsicmp(entry.szExeFile, L"SlimClash.exe") == 0;

        if (entry.th32ProcessID != current_pid && (is_proxy || is_launcher)) {
            HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION |
                                         PROCESS_TERMINATE | SYNCHRONIZE,
                                         FALSE, entry.th32ProcessID);
            if (process != NULL) {
                WCHAR image_path[32768];
                const WCHAR *expected_path = is_proxy ? g_proxy_path : g_module_path;
                if (GetProcessImagePath(process, image_path, ARRAYSIZE(image_path)) &&
                    _wcsicmp(image_path, expected_path) == 0) {
                    if (!TerminateProcess(process, 0)) {
                        DWORD error = GetLastError();
                        if (WaitForSingleObject(process, 0) != WAIT_OBJECT_0) {
                            ShowSystemError(is_proxy ?
                                            L"Could not terminate the Clash process in this directory." :
                                            L"Could not terminate the previous SlimClash process.",
                                            error);
                            success = FALSE;
                        }
                    } else {
                        DWORD wait_result = WaitForSingleObject(process, 5000);
                        if (wait_result == WAIT_FAILED) {
                            ShowSystemError(L"An error occurred while waiting for the previous process to exit.",
                                            GetLastError());
                            success = FALSE;
                        } else if (wait_result == WAIT_TIMEOUT) {
                            MessageBoxW(NULL, L"The previous process did not exit in time.",
                                        APP_NAME, MB_OK | MB_ICONERROR);
                            success = FALSE;
                        }
                    }
                }
                CloseHandle(process);
            }
        }

        has_process = Process32NextW(snapshot, &entry);
        if (!has_process && GetLastError() != ERROR_NO_MORE_FILES) {
            ShowSystemError(L"An error occurred while reading the running process list.",
                            GetLastError());
            success = FALSE;
        }
    }

    CloseHandle(snapshot);
    return success;
}

static BOOL AcquireInstanceLock(void)
{
    WCHAR mutex_name[64];
    ULONGLONG path_hash = 14695981039346656037ULL;
    const WCHAR *character;
    DWORD error;
    DWORD wait_result;
    BOOL marker_exists;

    for (character = g_module_path; *character != L'\0'; ++character) {
        WCHAR normalized = *character == L'/' ? L'\\' : *character;
        normalized = (WCHAR)towlower(normalized);
        path_hash ^= (BYTE)(normalized & 0xff);
        path_hash *= 1099511628211ULL;
        path_hash ^= (BYTE)(normalized >> 8);
        path_hash *= 1099511628211ULL;
    }
    if (swprintf_s(mutex_name, ARRAYSIZE(mutex_name),
                   L"Local\\SlimClashLauncher.%016llX", path_hash) < 0) {
        MessageBoxW(NULL, L"Could not create the instance lock name.", APP_NAME,
                    MB_OK | MB_ICONERROR);
        return FALSE;
    }

    SetLastError(ERROR_SUCCESS);
    g_mutex = CreateMutexW(NULL, TRUE, mutex_name);
    if (g_mutex == NULL) {
        ShowSystemError(L"Could not create the instance lock.", GetLastError());
        return FALSE;
    }
    error = GetLastError();
    if (error != ERROR_ALREADY_EXISTS) {
        g_mutex_owned = TRUE;
        return TRUE;
    }

    wait_result = WaitForSingleObject(g_mutex, 0);
    if (wait_result == WAIT_OBJECT_0 || wait_result == WAIT_ABANDONED) {
        g_mutex_owned = TRUE;
        return TRUE;
    }
    if (wait_result == WAIT_FAILED) {
        ShowSystemError(L"Could not check the existing launcher lock.",
                        GetLastError());
        return FALSE;
    }

    if (!ReadTrayHiddenMarker(&marker_exists))
        return FALSE;
    if (marker_exists) {
        DWORD elapsed;
        BOOL existing_tray_visible = FALSE;

        for (elapsed = 0; elapsed < 5000; elapsed += 100) {
            HWND existing_window = FindLauncherWindow();
            if (existing_window != NULL) {
                DWORD_PTR response = 0;
                SetLastError(ERROR_SUCCESS);
                if (SendMessageTimeoutW(existing_window, RESTORE_TRAY_MESSAGE,
                                        0, 0, SMTO_ABORTIFHUNG, 1000,
                                        &response)) {
                    if (response == RESTORE_TRAY_RESTORED) {
                        g_unhide_forwarded = TRUE;
                        return TRUE;
                    }
                    if (response == RESTORE_TRAY_ALREADY_VISIBLE) {
                        existing_tray_visible = TRUE;
                        break;
                    }
                }
            }
            Sleep(100);
        }

        if (!existing_tray_visible) {
            MessageBoxW(NULL, L"A hidden launcher instance was found, but it could not be restored. The existing instance was left running.",
                        APP_NAME, MB_OK | MB_ICONERROR);
            return FALSE;
        }
    }

    if (!TerminateMatchingProcesses(FALSE, TRUE))
        return FALSE;

    wait_result = WaitForSingleObject(g_mutex, 15000);
    if (wait_result != WAIT_OBJECT_0 && wait_result != WAIT_ABANDONED) {
        if (wait_result == WAIT_FAILED)
            ShowSystemError(L"An error occurred while waiting for the previous launcher to exit.",
                            GetLastError());
        else
            MessageBoxW(NULL, L"Timed out waiting for the previous launcher. A new proxy instance was not started.",
                        APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    g_mutex_owned = TRUE;
    return TRUE;
}

static void TrimAscii(char *text)
{
    size_t length;
    char *start = text;

    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')
        ++start;
    if (start != text)
        memmove(text, start, strlen(start) + 1);

    length = strlen(text);
    while (length > 0 &&
           (text[length - 1] == ' ' || text[length - 1] == '\t' ||
            text[length - 1] == '\r' || text[length - 1] == '\n')) {
        text[--length] = '\0';
    }
}

static void RemoveYamlComment(char *value)
{
    BOOL single_quoted = FALSE;
    BOOL double_quoted = FALSE;
    size_t i;
    size_t length;

    for (i = 0; value[i] != '\0'; ++i) {
        if (value[i] == '\'' && !double_quoted)
            single_quoted = !single_quoted;
        else if (value[i] == '"' && !single_quoted)
            double_quoted = !double_quoted;
        else if (value[i] == '#' && !single_quoted && !double_quoted &&
                 (i == 0 || value[i - 1] == ' ' || value[i - 1] == '\t')) {
            value[i] = '\0';
            break;
        }
    }
    TrimAscii(value);
    length = strlen(value);
    if (length >= 2 &&
        ((value[0] == '\'' && value[length - 1] == '\'') ||
         (value[0] == '"' && value[length - 1] == '"'))) {
        memmove(value, value + 1, length - 2);
        value[length - 2] = '\0';
    }
}

static void CopyConfigValue(char *destination, size_t capacity, const char *value)
{
    if (capacity == 0)
        return;
    strncpy_s(destination, capacity, value, _TRUNCATE);
}

static BOOL ReadConfig(char *http_port, size_t http_capacity,
                       char *socks_port, size_t socks_capacity,
                       char *mixed_port, size_t mixed_capacity,
                       char *controller, size_t controller_capacity,
                       char *secret, size_t secret_capacity)
{
    FILE *file;
    char line[8192];
    BOOL first_line = TRUE;

    file = _wfopen(g_config_path, L"rb");
    if (file == NULL) {
        WCHAR message[MAX_PATH + 128];
        swprintf_s(message, sizeof(message) / sizeof(message[0]),
                   L"Could not open the proxy configuration file:\n%ls", g_config_path);
        MessageBoxW(NULL, message, APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char *colon;
        char *value;

        if (first_line && (unsigned char)line[0] == 0xef &&
            (unsigned char)line[1] == 0xbb && (unsigned char)line[2] == 0xbf)
            memmove(line, line + 3, strlen(line + 3) + 1);
        first_line = FALSE;

        if (line[0] == ' ' || line[0] == '\t' || line[0] == '#' ||
            line[0] == '\r' || line[0] == '\n')
            continue;
        colon = strchr(line, ':');
        if (colon == NULL)
            continue;
        *colon = '\0';
        value = colon + 1;
        TrimAscii(line);
        RemoveYamlComment(value);

        if (strcmp(line, "port") == 0)
            CopyConfigValue(http_port, http_capacity, value);
        else if (strcmp(line, "socks-port") == 0)
            CopyConfigValue(socks_port, socks_capacity, value);
        else if (strcmp(line, "mixed-port") == 0)
            CopyConfigValue(mixed_port, mixed_capacity, value);
        else if (strcmp(line, "external-controller") == 0)
            CopyConfigValue(controller, controller_capacity, value);
        else if (strcmp(line, "secret") == 0)
            CopyConfigValue(secret, secret_capacity, value);
    }

    if (ferror(file)) {
        fclose(file);
        ShowSystemError(L"Could not read the proxy configuration file.", ERROR_READ_FAULT);
        return FALSE;
    }
    fclose(file);
    return TRUE;
}

static BOOL BuildControllerUrls(const char *controller)
{
    char endpoint[256];
    WCHAR wide_endpoint[256];
    int converted;
    size_t length;
    BOOL secure = FALSE;

    if (controller[0] == '\0')
        controller = "127.0.0.1:9090";
    strncpy_s(endpoint, sizeof(endpoint), controller, _TRUNCATE);

    if (strncmp(endpoint, "http://", 7) == 0)
        memmove(endpoint, endpoint + 7, strlen(endpoint + 7) + 1);
    else if (strncmp(endpoint, "https://", 8) == 0) {
        secure = TRUE;
        memmove(endpoint, endpoint + 8, strlen(endpoint + 8) + 1);
    }

    if (strncmp(endpoint, "0.0.0.0:", 8) == 0) {
        memmove(endpoint + 9, endpoint + 8, strlen(endpoint + 8) + 1);
        memcpy(endpoint, "127.0.0.1", 9);
    } else if (endpoint[0] == ':') {
        memmove(endpoint + 9, endpoint, strlen(endpoint) + 1);
        memcpy(endpoint, "127.0.0.1", 9);
    }

    length = strlen(endpoint);
    while (length > 0 && endpoint[length - 1] == '/')
        --length;
    if (length >= sizeof(endpoint))
        return FALSE;
    endpoint[length] = '\0';

    converted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, endpoint,
                                    -1, wide_endpoint,
                                    (int)(sizeof(wide_endpoint) / sizeof(wide_endpoint[0])));
    if (converted == 0)
        return FALSE;

    if (swprintf_s(g_controller_url, ARRAYSIZE(g_controller_url),
                   secure ? L"https://%ls" : L"http://%ls",
                   wide_endpoint) < 0 ||
        swprintf_s(g_dashboard_url, ARRAYSIZE(g_dashboard_url),
                   secure ? L"https://%ls/ui" : L"http://%ls/ui",
                   wide_endpoint) < 0)
        return FALSE;
    return TRUE;
}

typedef struct JsonParser {
    const char *text;
    size_t length;
    size_t position;
} JsonParser;

static void JsonSkipWhitespace(JsonParser *parser)
{
    while (parser->position < parser->length &&
           (parser->text[parser->position] == ' ' ||
            parser->text[parser->position] == '\t' ||
            parser->text[parser->position] == '\r' ||
            parser->text[parser->position] == '\n'))
        ++parser->position;
}

static BOOL JsonReadString(JsonParser *parser, char *output, size_t capacity)
{
    size_t written = 0;

    if (parser->position >= parser->length ||
        parser->text[parser->position++] != '"')
        return FALSE;
    while (parser->position < parser->length) {
        unsigned char character = (unsigned char)parser->text[parser->position++];
        if (character == '"') {
            if (capacity != 0)
                output[written] = '\0';
            return TRUE;
        }
        if (character < 0x20)
            return FALSE;
        if (character == '\\') {
            unsigned int codepoint;
            unsigned int digit;
            int i;

            if (parser->position >= parser->length)
                return FALSE;
            character = (unsigned char)parser->text[parser->position++];
            switch (character) {
            case '"': case '\\': case '/': break;
            case 'b': character = '\b'; break;
            case 'f': character = '\f'; break;
            case 'n': character = '\n'; break;
            case 'r': character = '\r'; break;
            case 't': character = '\t'; break;
            case 'u':
                codepoint = 0;
                for (i = 0; i < 4; ++i) {
                    if (parser->position >= parser->length)
                        return FALSE;
                    character = (unsigned char)parser->text[parser->position++];
                    if (character >= '0' && character <= '9')
                        digit = character - '0';
                    else if (character >= 'a' && character <= 'f')
                        digit = character - 'a' + 10;
                    else if (character >= 'A' && character <= 'F')
                        digit = character - 'A' + 10;
                    else
                        return FALSE;
                    codepoint = (codepoint << 4) | digit;
                }
                character = codepoint <= 0x7f ? (unsigned char)codepoint : '?';
                break;
            default:
                return FALSE;
            }
        }
        if (capacity != 0) {
            if (written + 1 >= capacity)
                return FALSE;
            output[written++] = (char)character;
        }
    }
    return FALSE;
}

static BOOL JsonSkipValue(JsonParser *parser, unsigned int depth);

static BOOL JsonSkipNumber(JsonParser *parser)
{
    size_t start = parser->position;

    if (parser->position < parser->length &&
        parser->text[parser->position] == '-')
        ++parser->position;
    if (parser->position >= parser->length)
        return FALSE;
    if (parser->text[parser->position] == '0') {
        ++parser->position;
    } else {
        if (parser->text[parser->position] < '1' ||
            parser->text[parser->position] > '9')
            return FALSE;
        while (parser->position < parser->length &&
               parser->text[parser->position] >= '0' &&
               parser->text[parser->position] <= '9')
            ++parser->position;
    }
    if (parser->position < parser->length &&
        parser->text[parser->position] == '.') {
        ++parser->position;
        if (parser->position >= parser->length ||
            parser->text[parser->position] < '0' ||
            parser->text[parser->position] > '9')
            return FALSE;
        while (parser->position < parser->length &&
               parser->text[parser->position] >= '0' &&
               parser->text[parser->position] <= '9')
            ++parser->position;
    }
    if (parser->position < parser->length &&
        (parser->text[parser->position] == 'e' ||
         parser->text[parser->position] == 'E')) {
        ++parser->position;
        if (parser->position < parser->length &&
            (parser->text[parser->position] == '+' ||
             parser->text[parser->position] == '-'))
            ++parser->position;
        if (parser->position >= parser->length ||
            parser->text[parser->position] < '0' ||
            parser->text[parser->position] > '9')
            return FALSE;
        while (parser->position < parser->length &&
               parser->text[parser->position] >= '0' &&
               parser->text[parser->position] <= '9')
            ++parser->position;
    }
    return parser->position > start;
}

static BOOL JsonSkipValue(JsonParser *parser, unsigned int depth)
{
    char ignored[1];
    char character;

    JsonSkipWhitespace(parser);
    if (parser->position >= parser->length || depth > 32)
        return FALSE;
    character = parser->text[parser->position];
    if (character == '"')
        return JsonReadString(parser, ignored, 0);
    if (character == '{' || character == '[') {
        char closing = character == '{' ? '}' : ']';
        BOOL is_object = character == '{';

        ++parser->position;
        JsonSkipWhitespace(parser);
        if (parser->position < parser->length &&
            parser->text[parser->position] == closing) {
            ++parser->position;
            return TRUE;
        }
        for (;;) {
            if (is_object) {
                if (!JsonReadString(parser, ignored, 0))
                    return FALSE;
                JsonSkipWhitespace(parser);
                if (parser->position >= parser->length ||
                    parser->text[parser->position++] != ':')
                    return FALSE;
            }
            if (!JsonSkipValue(parser, depth + 1))
                return FALSE;
            JsonSkipWhitespace(parser);
            if (parser->position >= parser->length)
                return FALSE;
            character = parser->text[parser->position++];
            if (character == closing)
                return TRUE;
            if (character != ',')
                return FALSE;
            JsonSkipWhitespace(parser);
        }
    }
    if (character == '-' || (character >= '0' && character <= '9'))
        return JsonSkipNumber(parser);
    if (parser->length - parser->position >= 4 &&
        memcmp(parser->text + parser->position, "true", 4) == 0) {
        parser->position += 4;
        return TRUE;
    }
    if (parser->length - parser->position >= 5 &&
        memcmp(parser->text + parser->position, "false", 5) == 0) {
        parser->position += 5;
        return TRUE;
    }
    if (parser->length - parser->position >= 4 &&
        memcmp(parser->text + parser->position, "null", 4) == 0) {
        parser->position += 4;
        return TRUE;
    }
    return FALSE;
}

static BOOL ParseConfigMode(const char *json, size_t length,
                            char *mode, size_t mode_capacity)
{
    JsonParser parser;
    BOOL found_mode = FALSE;

    parser.text = json;
    parser.length = length;
    parser.position = 0;
    JsonSkipWhitespace(&parser);
    if (parser.position >= parser.length ||
        parser.text[parser.position++] != '{')
        return FALSE;
    JsonSkipWhitespace(&parser);
    while (parser.position < parser.length &&
           parser.text[parser.position] != '}') {
        char key[64];

        if (!JsonReadString(&parser, key, sizeof(key)))
            return FALSE;
        JsonSkipWhitespace(&parser);
        if (parser.position >= parser.length ||
            parser.text[parser.position++] != ':')
            return FALSE;
        JsonSkipWhitespace(&parser);
        if (strcmp(key, "mode") == 0) {
            if (found_mode || !JsonReadString(&parser, mode, mode_capacity))
                return FALSE;
            found_mode = TRUE;
        } else if (!JsonSkipValue(&parser, 0)) {
            return FALSE;
        }
        JsonSkipWhitespace(&parser);
        if (parser.position >= parser.length)
            return FALSE;
        if (parser.text[parser.position] == '}')
            break;
        if (parser.text[parser.position++] != ',')
            return FALSE;
        JsonSkipWhitespace(&parser);
        if (parser.position < parser.length &&
            parser.text[parser.position] == '}')
            return FALSE;
    }
    if (parser.position >= parser.length ||
        parser.text[parser.position++] != '}')
        return FALSE;
    JsonSkipWhitespace(&parser);
    if (parser.position != parser.length || !found_mode)
        return FALSE;
    return strcmp(mode, "direct") == 0 ||
           strcmp(mode, "global") == 0 ||
           strcmp(mode, "rule") == 0;
}

static BOOL IsControllerUnavailableError(DWORD error)
{
    return error == ERROR_WINHTTP_TIMEOUT ||
           error == ERROR_WINHTTP_NAME_NOT_RESOLVED ||
           error == ERROR_WINHTTP_CANNOT_CONNECT ||
           error == ERROR_WINHTTP_CONNECTION_ERROR;
}

static BOOL ClashHttpRequest(const WCHAR *method, const WCHAR *path,
                            char *body, char *response,
                            size_t response_capacity, DWORD *status_code,
                            BOOL quiet_unavailable)
{
    URL_COMPONENTSW components;
    WCHAR host[256];
    WCHAR wide_secret[512];
    WCHAR authorization[600];
    HINTERNET session = NULL;
    HINTERNET connection = NULL;
    HINTERNET request = NULL;
    DWORD error = ERROR_SUCCESS;
    DWORD status_size = sizeof(*status_code);
    DWORD bytes_read;
    DWORD available;
    size_t response_length = 0;
    BOOL success = FALSE;
    int converted;
    char discarded_response[256];

    if (response_capacity == 0) {
        response = discarded_response;
        response_capacity = sizeof(discarded_response);
    }

    ZeroMemory(&components, sizeof(components));
    components.dwStructSize = sizeof(components);
    components.lpszHostName = host;
    components.dwHostNameLength = ARRAYSIZE(host);
    if (!WinHttpCrackUrl(g_controller_url, 0, 0, &components)) {
        error = GetLastError();
        goto cleanup;
    }
    session = WinHttpOpen(L"SlimClash", WINHTTP_ACCESS_TYPE_NO_PROXY,
                          WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (session == NULL) {
        error = GetLastError();
        goto cleanup;
    }
    if (!WinHttpSetTimeouts(session, 2000, 2000, 2000, 2000)) {
        error = GetLastError();
        goto cleanup;
    }
    connection = WinHttpConnect(session, host, components.nPort, 0);
    if (connection == NULL) {
        error = GetLastError();
        goto cleanup;
    }
    request = WinHttpOpenRequest(connection, method, path, NULL,
                                 WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 components.nScheme == INTERNET_SCHEME_HTTPS ?
                                 WINHTTP_FLAG_SECURE : 0);
    if (request == NULL) {
        error = GetLastError();
        goto cleanup;
    }
    if (g_api_secret[0] != '\0') {
        converted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                        g_api_secret, -1,
                                        wide_secret, ARRAYSIZE(wide_secret));
        if (converted == 0) {
            error = GetLastError();
            goto cleanup;
        }
        if (swprintf_s(authorization, ARRAYSIZE(authorization),
                       L"Authorization: Bearer %ls\r\n", wide_secret) < 0) {
            error = ERROR_INSUFFICIENT_BUFFER;
            goto cleanup;
        }
        if (!WinHttpAddRequestHeaders(request, authorization, (DWORD)-1L,
                                      WINHTTP_ADDREQ_FLAG_ADD)) {
            error = GetLastError();
            goto cleanup;
        }
    }
    if (body != NULL &&
        !WinHttpAddRequestHeaders(request, L"Content-Type: application/json\r\n",
                                  (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD)) {
        error = GetLastError();
        goto cleanup;
    }
    if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            body, body == NULL ? 0 : (DWORD)strlen(body),
                            body == NULL ? 0 : (DWORD)strlen(body), 0)) {
        error = GetLastError();
        goto cleanup;
    }
    if (!WinHttpReceiveResponse(request, NULL)) {
        error = GetLastError();
        goto cleanup;
    }
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, status_code,
                             &status_size, WINHTTP_NO_HEADER_INDEX)) {
        error = GetLastError();
        goto cleanup;
    }
    if (response_capacity != 0)
        response[0] = '\0';
    for (;;) {
        if (!WinHttpQueryDataAvailable(request, &available)) {
            error = GetLastError();
            goto cleanup;
        }
        if (available == 0)
            break;
        if (available > response_capacity - response_length - 1) {
            error = ERROR_INSUFFICIENT_BUFFER;
            goto cleanup;
        }
        if (!WinHttpReadData(request, response + response_length, available,
                             &bytes_read)) {
            error = GetLastError();
            goto cleanup;
        }
        if (memchr(response + response_length, '\0', bytes_read) != NULL) {
            error = ERROR_INVALID_DATA;
            goto cleanup;
        }
        response_length += bytes_read;
    }
    if (response_capacity != 0)
        response[response_length] = '\0';
    success = TRUE;

cleanup:
    if (request != NULL)
        WinHttpCloseHandle(request);
    if (connection != NULL)
        WinHttpCloseHandle(connection);
    if (session != NULL)
        WinHttpCloseHandle(session);
    if (!success &&
        !(quiet_unavailable && IsControllerUnavailableError(error)))
        ShowSystemError(L"Could not communicate with the Clash controller.", error);
    return success;
}

static BOOL ReadProxyMode(char *mode, size_t capacity)
{
    char response[8192];
    DWORD status_code = 0;

    if (!ClashHttpRequest(L"GET", L"/configs", NULL, response,
                          sizeof(response), &status_code, TRUE))
        return FALSE;
    if (status_code >= 500)
        return FALSE;
    if (status_code != HTTP_STATUS_OK) {
        WCHAR message[128];
        swprintf_s(message, ARRAYSIZE(message),
                   L"Clash returned HTTP status %lu while reading the proxy mode.",
                   status_code);
        MessageBoxW(NULL, message, APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    if (!ParseConfigMode(response, strlen(response), mode, capacity)) {
        MessageBoxW(NULL, L"Could not read a valid proxy mode from the Clash response.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    return TRUE;
}

static BOOL SetProxyMode(const char *mode)
{
    char body[64];
    DWORD status_code = 0;

    if (sprintf_s(body, sizeof(body), "{\"mode\":\"%s\"}", mode) < 0) {
        MessageBoxW(NULL, L"Could not create the proxy mode request.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    if (!ClashHttpRequest(L"PATCH", L"/configs", body, NULL, 0,
                          &status_code, FALSE))
        return FALSE;
    if (status_code < 200 || status_code >= 300) {
        WCHAR message[128];
        swprintf_s(message, ARRAYSIZE(message),
                   L"Clash returned HTTP status %lu while changing the proxy mode.",
                   status_code);
        MessageBoxW(NULL, message, APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    strcpy_s(g_current_mode, sizeof(g_current_mode), mode);
    return TRUE;
}

static void ReloadProxyConfig(void)
{
    char body[] = "{\"path\":\"\",\"payload\":\"\"}";
    DWORD status_code = 0;

    if (!ClashHttpRequest(L"PUT", L"/configs?reload=true", body,
                          NULL, 0, &status_code, FALSE))
        return;
    if (status_code != HTTP_STATUS_NO_CONTENT) {
        WCHAR message[128];
        swprintf_s(message, ARRAYSIZE(message),
                   L"Clash returned HTTP status %lu while reloading the configuration.",
                   status_code);
        MessageBoxW(NULL, message, APP_NAME, MB_OK | MB_ICONERROR);
        return;
    }
    g_tray_icon.cbSize = sizeof(g_tray_icon);
    g_tray_icon.uFlags = NIF_INFO;
    wcscpy_s(g_tray_icon.szInfoTitle, ARRAYSIZE(g_tray_icon.szInfoTitle),
             APP_NAME);
    wcscpy_s(g_tray_icon.szInfo, ARRAYSIZE(g_tray_icon.szInfo),
             L"Configuration reloaded successfully.");
    g_tray_icon.dwInfoFlags = NIIF_INFO;
    g_tray_icon.hBalloonIcon = NULL;
    if (!Shell_NotifyIconW(NIM_MODIFY, &g_tray_icon))
        ShowSystemError(L"Could not notify that the configuration was reloaded.",
                        GetLastError());
}

static BOOL SetRegistryString(HKEY key, const WCHAR *name, const WCHAR *value,
                              const WCHAR *operation)
{
    LONG result = RegSetValueExW(key, name, 0, REG_SZ, (const BYTE *)value,
                                 (DWORD)((wcslen(value) + 1) * sizeof(WCHAR)));
    if (result != ERROR_SUCCESS) {
        ShowSystemError(operation, (DWORD)result);
        return FALSE;
    }
    return TRUE;
}

static BOOL SetRegistryDword(HKEY key, const WCHAR *name, DWORD value,
                             const WCHAR *operation)
{
    LONG result = RegSetValueExW(key, name, 0, REG_DWORD, (const BYTE *)&value,
                                 sizeof(value));
    if (result != ERROR_SUCCESS) {
        ShowSystemError(operation, (DWORD)result);
        return FALSE;
    }
    return TRUE;
}

static BOOL BroadcastSettingsChange(const WCHAR *setting)
{
    DWORD_PTR result_value;

    SetLastError(ERROR_SUCCESS);
    if (!SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                             (LPARAM)setting, SMTO_ABORTIFHUNG, 5000,
                             &result_value)) {
        DWORD error = GetLastError();
        if (error == ERROR_SUCCESS)
            error = ERROR_TIMEOUT;
        ShowSystemError(L"Could not notify applications that settings changed.",
                        error);
        return FALSE;
    }
    return TRUE;
}

static BOOL SetProxySettings(BOOL enable, const char *proxy_port)
{
    HKEY internet_key = NULL;
    HKEY environment_key = NULL;
    WCHAR proxy_server[64] = L"";
    WCHAR environment_proxy[80] = L"";
    DWORD disposition;
    LONG result;
    BOOL success = TRUE;
    BOOL have_proxy_port = FALSE;

    if (enable) {
        WCHAR wide_port[32];
        int converted;

        if (proxy_port == NULL || proxy_port[0] == '\0') {
            MessageBoxW(NULL, L"Neither mixed-port nor port is configured; system proxy settings were not changed.",
                        APP_NAME, MB_OK | MB_ICONERROR);
            return FALSE;
        }
        converted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, proxy_port,
                                        -1, wide_port, ARRAYSIZE(wide_port));
        if (converted == 0 ||
            swprintf_s(proxy_server, ARRAYSIZE(proxy_server),
                       L"127.0.0.1:%ls", wide_port) < 0 ||
            swprintf_s(environment_proxy, ARRAYSIZE(environment_proxy),
                       L"http://127.0.0.1:%ls", wide_port) < 0) {
            MessageBoxW(NULL, L"The mixed-port setting is invalid; system proxy settings were not changed.",
                        APP_NAME, MB_OK | MB_ICONERROR);
            return FALSE;
        }
        have_proxy_port = TRUE;
    }

    result = RegCreateKeyExW(HKEY_CURRENT_USER, INTERNET_SETTINGS_KEY, 0, NULL,
                             0, KEY_SET_VALUE, NULL, &internet_key, &disposition);
    if (result != ERROR_SUCCESS) {
        ShowSystemError(L"Could not open the current user's Internet Settings.",
                        (DWORD)result);
        success = FALSE;
    }
    result = RegCreateKeyExW(HKEY_CURRENT_USER, USER_ENVIRONMENT_KEY, 0, NULL,
                             0, KEY_SET_VALUE, NULL, &environment_key, &disposition);
    if (result != ERROR_SUCCESS) {
        ShowSystemError(L"Could not open the current user's environment settings.",
                        (DWORD)result);
        success = FALSE;
    }

    if (internet_key != NULL) {
        if (!SetRegistryDword(internet_key, L"ProxyEnable", enable ? 1 : 0,
                              enable ? L"Could not enable the system proxy." :
                                       L"Could not disable the system proxy."))
            success = FALSE;
        if (!SetRegistryString(internet_key, L"ProxyServer",
                               enable ? proxy_server : L"",
                               enable ? L"Could not set the system proxy server." :
                                        L"Could not clear the system proxy server."))
            success = FALSE;
        if (enable &&
            !SetRegistryString(internet_key, L"ProxyOverride", PROXY_OVERRIDE,
                               L"Could not set the system proxy bypass list."))
            success = FALSE;
        RegCloseKey(internet_key);
    }

    if (environment_key != NULL) {
        if (!SetRegistryString(environment_key, L"http_proxy",
                               have_proxy_port ? environment_proxy : L"",
                               L"Could not set the http_proxy environment variable."))
            success = FALSE;
        if (!SetRegistryString(environment_key, L"https_proxy",
                               have_proxy_port ? environment_proxy : L"",
                               L"Could not set the https_proxy environment variable."))
            success = FALSE;
        RegCloseKey(environment_key);
    }

    if (!BroadcastSettingsChange(L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"))
        success = FALSE;
    if (!BroadcastSettingsChange(L"Environment"))
        success = FALSE;
    return success;
}

static BOOL FlushDnsCache(void)
{
    WCHAR system_directory[MAX_PATH];
    WCHAR ipconfig_path[MAX_PATH];
    WCHAR command_line[MAX_PATH + 32];
    STARTUPINFOW startup;
    PROCESS_INFORMATION process;
    DWORD length;
    DWORD wait_result;
    DWORD exit_code;

    length = GetSystemDirectoryW(system_directory, ARRAYSIZE(system_directory));
    if (length == 0 || length >= ARRAYSIZE(system_directory)) {
        ShowSystemError(L"Could not locate ipconfig.exe to flush the DNS cache.",
                        GetLastError());
        return FALSE;
    }
    if (wcscpy_s(ipconfig_path, ARRAYSIZE(ipconfig_path), system_directory) != 0 ||
        !AppendPath(ipconfig_path, ARRAYSIZE(ipconfig_path), L"\\ipconfig.exe") ||
        swprintf_s(command_line, ARRAYSIZE(command_line),
                   L"\"%ls\" /flushdns", ipconfig_path) < 0) {
        MessageBoxW(NULL, L"The ipconfig.exe path is too long.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    ZeroMemory(&startup, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    ZeroMemory(&process, sizeof(process));
    if (!CreateProcessW(ipconfig_path, command_line, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &startup, &process)) {
        ShowSystemError(L"Could not start ipconfig.exe to flush the DNS cache.",
                        GetLastError());
        return FALSE;
    }
    CloseHandle(process.hThread);
    wait_result = WaitForSingleObject(process.hProcess, 15000);
    if (wait_result == WAIT_TIMEOUT) {
        TerminateProcess(process.hProcess, ERROR_TIMEOUT);
        WaitForSingleObject(process.hProcess, 5000);
        CloseHandle(process.hProcess);
        MessageBoxW(NULL, L"Timed out while flushing the DNS cache.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    if (wait_result == WAIT_FAILED) {
        DWORD error = GetLastError();
        CloseHandle(process.hProcess);
        ShowSystemError(L"Could not wait for the DNS cache flush to finish.", error);
        return FALSE;
    }
    if (!GetExitCodeProcess(process.hProcess, &exit_code)) {
        DWORD error = GetLastError();
        CloseHandle(process.hProcess);
        ShowSystemError(L"Could not check the DNS cache flush result.", error);
        return FALSE;
    }
    CloseHandle(process.hProcess);
    if (exit_code != 0) {
        WCHAR message[256];
        swprintf_s(message, ARRAYSIZE(message),
                   L"ipconfig.exe could not flush the DNS cache (exit code %lu).",
                   exit_code);
        MessageBoxW(NULL, message, APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    return TRUE;
}

static BOOL IsAutostartEnabled(BOOL *enabled)
{
    HKEY key;
    WCHAR value[MAX_PATH + 8];
    WCHAR expected[MAX_PATH + 8];
    DWORD type = 0;
    DWORD size = sizeof(value);
    LONG result;

    *enabled = FALSE;
    result = RegOpenKeyExW(HKEY_CURRENT_USER, AUTOSTART_KEY, 0, KEY_QUERY_VALUE, &key);
    if (result == ERROR_FILE_NOT_FOUND)
        return TRUE;
    if (result != ERROR_SUCCESS) {
        ShowSystemError(L"Could not read the startup setting.", (DWORD)result);
        return FALSE;
    }

    result = RegQueryValueExW(key, AUTOSTART_VALUE, NULL, &type,
                              (BYTE *)value, &size);
    RegCloseKey(key);
    if (result == ERROR_FILE_NOT_FOUND)
        return TRUE;
    if (result != ERROR_SUCCESS) {
        ShowSystemError(L"Could not read the startup setting.", (DWORD)result);
        return FALSE;
    }
    if (type != REG_SZ) {
        MessageBoxW(NULL, L"The startup registry value has an invalid format.", APP_NAME,
                    MB_OK | MB_ICONERROR);
        return FALSE;
    }

    value[(sizeof(value) / sizeof(value[0])) - 1] = L'\0';
    if (swprintf_s(expected, ARRAYSIZE(expected), L"\"%ls\"", g_module_path) < 0) {
        MessageBoxW(NULL, L"The launcher path is too long to read the startup setting.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    *enabled = _wcsicmp(value, expected) == 0;
    return TRUE;
}

static BOOL SetAutostart(BOOL enabled)
{
    HKEY key;
    LONG result;

    if (!enabled) {
        result = RegOpenKeyExW(HKEY_CURRENT_USER, AUTOSTART_KEY, 0, KEY_SET_VALUE, &key);
        if (result == ERROR_FILE_NOT_FOUND)
            return TRUE;
        if (result != ERROR_SUCCESS) {
            ShowSystemError(L"Could not update the startup setting.", (DWORD)result);
            return FALSE;
        }
        result = RegDeleteValueW(key, AUTOSTART_VALUE);
        RegCloseKey(key);
        if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
            ShowSystemError(L"Could not disable startup.", (DWORD)result);
            return FALSE;
        }
        return TRUE;
    }

    result = RegCreateKeyExW(HKEY_CURRENT_USER, AUTOSTART_KEY, 0, NULL, 0,
                             KEY_SET_VALUE, NULL, &key, NULL);
    if (result != ERROR_SUCCESS) {
        ShowSystemError(L"Could not update the startup setting.", (DWORD)result);
        return FALSE;
    }

    {
        WCHAR command_line[MAX_PATH + 8];
        if (swprintf_s(command_line,
                       sizeof(command_line) / sizeof(command_line[0]),
                       L"\"%ls\"", g_module_path) < 0) {
            RegCloseKey(key);
            MessageBoxW(NULL, L"The launcher path is too long to enable startup.",
                        APP_NAME, MB_OK | MB_ICONERROR);
            return FALSE;
        }
        result = RegSetValueExW(key, AUTOSTART_VALUE, 0, REG_SZ,
                                (const BYTE *)command_line,
                                (DWORD)((wcslen(command_line) + 1) * sizeof(WCHAR)));
    }
    RegCloseKey(key);
    if (result != ERROR_SUCCESS) {
        ShowSystemError(L"Could not enable startup.", (DWORD)result);
        return FALSE;
    }
    return TRUE;
}

static BOOL LaunchProxy(void)
{
    WCHAR command_line[MAX_PATH + 32];
    STARTUPINFOW startup;
    DWORD attributes;

    attributes = GetFileAttributesW(g_proxy_path);
    if (attributes == INVALID_FILE_ATTRIBUTES ||
        (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        WCHAR message[MAX_PATH + 128];
        swprintf_s(message, sizeof(message) / sizeof(message[0]),
                   L"Could not find the proxy executable:\n%ls", g_proxy_path);
        MessageBoxW(NULL, message, APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    if (swprintf_s(command_line, sizeof(command_line) / sizeof(command_line[0]),
                   L"\"%ls\" -d \".clash\"", g_proxy_path) < 0) {
        MessageBoxW(NULL, L"The Clash command line is too long.", APP_NAME,
                    MB_OK | MB_ICONERROR);
        return FALSE;
    }

    ZeroMemory(&startup, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    ZeroMemory(&g_proxy, sizeof(g_proxy));
    if (!CreateProcessW(g_proxy_path, command_line, NULL, NULL, FALSE,
                        CREATE_NEW_CONSOLE, NULL, g_clash_dir, &startup, &g_proxy)) {
        ShowSystemError(L"Could not start Clash.", GetLastError());
        return FALSE;
    }
    CloseHandle(g_proxy.hThread);
    g_proxy.hThread = NULL;
    return TRUE;
}

static BOOL AddTrayIcon(HWND window, BOOL show_notification);
static BOOL RemoveTrayIcon(void);
static BOOL SetTrayHidden(HWND window, BOOL hidden);

static BOOL SetProxyConsoleVisible(BOOL visible)
{
    HWND console;
    DWORD error;
    DWORD wait_result;

    if (g_proxy.hProcess == NULL) {
        MessageBoxW(NULL, L"The Clash process is not running.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    wait_result = WaitForSingleObject(g_proxy.hProcess, 0);
    if (wait_result == WAIT_OBJECT_0) {
        MessageBoxW(NULL, L"The Clash process is not running.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    if (wait_result == WAIT_FAILED) {
        ShowSystemError(L"Could not check the Clash process state.", GetLastError());
        return FALSE;
    }

    if (!AttachConsole(g_proxy.dwProcessId)) {
        ShowSystemError(L"Could not attach to the Clash console.", GetLastError());
        return FALSE;
    }

    SetLastError(ERROR_SUCCESS);
    console = GetConsoleWindow();
    if (console == NULL) {
        error = GetLastError();
        FreeConsole();
        if (error != ERROR_SUCCESS)
            ShowSystemError(L"Could not get the Clash console window.", error);
        else
            MessageBoxW(NULL, L"The Clash process has no console window.",
                        APP_NAME, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    ShowWindow(console, visible ? SW_SHOW : SW_HIDE);
    if (!FreeConsole()) {
        ShowSystemError(L"Could not detach from the Clash console.", GetLastError());
        return FALSE;
    }
    g_console_visible = visible;
    return TRUE;
}

static void ShowTrayMenu(HWND window)
{
    HMENU menu = CreatePopupMenu();
    HMENU mode_menu;
    BOOL autostart = FALSE;
    char mode[16] = "";
    UINT flags;
    POINT cursor;

    if (menu == NULL) {
        ShowSystemError(L"Could not create the tray menu.", GetLastError());
        return;
    }
    if (!IsAutostartEnabled(&autostart)) {
        DestroyMenu(menu);
        return;
    }

    mode_menu = CreatePopupMenu();
    if (mode_menu == NULL) {
        ShowSystemError(L"Could not create the proxy mode menu.", GetLastError());
        DestroyMenu(menu);
        return;
    }
    if (ReadProxyMode(mode, sizeof(mode))) {
        strcpy_s(g_current_mode, sizeof(g_current_mode), mode);
    } else {
        g_current_mode[0] = '\0';
    }
    AppendMenuW(mode_menu, MF_STRING |
                (strcmp(g_current_mode, "direct") == 0 ? MF_CHECKED : MF_UNCHECKED),
                MENU_MODE_DIRECT, L"Direct");
    AppendMenuW(mode_menu, MF_STRING |
                (strcmp(g_current_mode, "global") == 0 ? MF_CHECKED : MF_UNCHECKED),
                MENU_MODE_GLOBAL, L"Global");
    AppendMenuW(mode_menu, MF_STRING |
                (strcmp(g_current_mode, "rule") == 0 ? MF_CHECKED : MF_UNCHECKED),
                MENU_MODE_RULE, L"Rule");

    AppendMenuW(menu, MF_STRING, MENU_DASHBOARD, L"Open Dashboard");
    AppendMenuW(menu, MF_STRING, MENU_CONFIG, L"Open Config Folder");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)mode_menu, L"Proxy Mode");
    AppendMenuW(menu, MF_STRING, MENU_RELOAD_CONFIG, L"Reload Config");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    flags = MF_STRING | (autostart ? MF_CHECKED : MF_UNCHECKED);
    AppendMenuW(menu, flags, MENU_AUTOSTART, L"Start with Windows");
    AppendMenuW(menu, MF_STRING, MENU_CONSOLE,
                g_console_visible ? L"Hide Clash Console" : L"Show Console");
    flags = MF_STRING | (g_hide_tray_marker ? MF_CHECKED : MF_UNCHECKED);
    AppendMenuW(menu, flags, MENU_HIDE_TRAY, L"Hide Tray Icon");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, MENU_GITHUB, L"GitHub");
    AppendMenuW(menu, MF_STRING, MENU_EXIT, L"Quit");

    GetCursorPos(&cursor);
    SetForegroundWindow(window);
    {
        UINT command = TrackPopupMenu(menu,
                                      TPM_RIGHTBUTTON | TPM_BOTTOMALIGN |
                                      TPM_RETURNCMD,
                                      cursor.x, cursor.y, 0, window, NULL);
        if (command != 0)
            SendMessageW(window, WM_COMMAND, MAKEWPARAM(command, 0), 0);
    }
    PostMessageW(window, WM_NULL, 0, 0);
    DestroyMenu(menu);
}

static void OpenPath(const WCHAR *path, const WCHAR *description)
{
    HINSTANCE result = ShellExecuteW(NULL, L"open", path, NULL, g_clash_dir, SW_SHOWNORMAL);
    if ((INT_PTR)result <= 32) {
        WCHAR message[MAX_PATH + 160];
        swprintf_s(message, sizeof(message) / sizeof(message[0]),
                   L"Could not %ls:\n%ls", description, path);
        MessageBoxW(NULL, message, APP_NAME, MB_OK | MB_ICONERROR);
    }
}

static void StopProxy(void)
{
    DWORD wait_result;

    if (g_proxy.hProcess == NULL)
        return;
    wait_result = WaitForSingleObject(g_proxy.hProcess, 0);
    if (wait_result == WAIT_TIMEOUT) {
        if (!TerminateProcess(g_proxy.hProcess, 0)) {
            DWORD error = GetLastError();
            if (WaitForSingleObject(g_proxy.hProcess, 0) != WAIT_OBJECT_0)
                ShowSystemError(L"Could not terminate the Clash process.", error);
        } else {
            wait_result = WaitForSingleObject(g_proxy.hProcess, 5000);
            if (wait_result == WAIT_FAILED)
                ShowSystemError(L"An error occurred while waiting for Clash to exit.",
                                GetLastError());
            else if (wait_result == WAIT_TIMEOUT)
                MessageBoxW(NULL, L"Clash did not exit in time.", APP_NAME,
                            MB_OK | MB_ICONERROR);
        }
    } else if (wait_result == WAIT_FAILED) {
        ShowSystemError(L"An error occurred while checking the Clash process.",
                        GetLastError());
    }
    CloseHandle(g_proxy.hProcess);
    g_proxy.hProcess = NULL;
}

static LRESULT CALLBACK WindowProc(HWND window, UINT message,
                                   WPARAM wparam, LPARAM lparam)
{
    if (message == g_taskbar_created_message && g_tray_added) {
        g_tray_added = FALSE;
        if (g_icon != NULL && g_custom_icon)
            DestroyIcon(g_icon);
        g_icon = NULL;
        g_custom_icon = FALSE;
        if (!g_tray_hidden || g_temporary_tray_icon)
            AddTrayIcon(window, FALSE);
        return 0;
    }

    switch (message) {
    case TRAY_MESSAGE: {
        UINT event = (UINT)lparam;
        if (g_temporary_tray_icon &&
            (event == NIN_BALLOONHIDE || event == NIN_BALLOONTIMEOUT ||
             event == NIN_BALLOONUSERCLICK)) {
            KillTimer(window, TEMP_TRAY_ICON_TIMER);
            g_temporary_tray_icon = FALSE;
            RemoveTrayIcon();
            return 0;
        }
        if (event == WM_RBUTTONUP || event == WM_CONTEXTMENU ||
            event == WM_LBUTTONUP || event == WM_LBUTTONDBLCLK ||
            event == NIN_SELECT || event == NIN_KEYSELECT)
            ShowTrayMenu(window);
        return 0;
    }
    case RESTORE_TRAY_MESSAGE:
        if (g_tray_hidden) {
            if (g_temporary_tray_icon) {
                KillTimer(window, TEMP_TRAY_ICON_TIMER);
                g_temporary_tray_icon = FALSE;
            } else if (!AddTrayIcon(window, FALSE)) {
                return 0;
            }
            g_tray_hidden = FALSE;
            return RESTORE_TRAY_RESTORED;
        }
        return RESTORE_TRAY_ALREADY_VISIBLE;
    case WM_TIMER:
        if (wparam == TEMP_TRAY_ICON_TIMER && g_temporary_tray_icon) {
            KillTimer(window, TEMP_TRAY_ICON_TIMER);
            g_temporary_tray_icon = FALSE;
            RemoveTrayIcon();
            return 0;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case MENU_DASHBOARD:
            OpenPath(g_dashboard_url, L"open the Dashboard");
            return 0;
        case MENU_CONFIG:
            OpenPath(g_config_dir, L"open the configuration folder");
            return 0;
        case MENU_MODE_DIRECT:
            SetProxyMode("direct");
            return 0;
        case MENU_MODE_GLOBAL:
            SetProxyMode("global");
            return 0;
        case MENU_MODE_RULE:
            SetProxyMode("rule");
            return 0;
        case MENU_RELOAD_CONFIG:
            ReloadProxyConfig();
            return 0;
        case MENU_GITHUB:
            OpenPath(L"https://github.com/coo11/SlimClash",
                     L"open the GitHub repository");
            return 0;
        case MENU_AUTOSTART: {
            BOOL enabled = FALSE;
            if (IsAutostartEnabled(&enabled))
                SetAutostart(!enabled);
            return 0;
        }
        case MENU_CONSOLE:
            SetProxyConsoleVisible(!g_console_visible);
            return 0;
        case MENU_HIDE_TRAY:
            SetTrayHidden(window, !g_hide_tray_marker);
            return 0;
        case MENU_EXIT:
            StopProxy();
            DestroyWindow(window);
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_ENDSESSION:
        if (wparam)
            StopProxy();
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

static BOOL AddTrayIcon(HWND window, BOOL show_notification)
{
    WCHAR ports[256];

    g_icon = (HICON)LoadImageW(GetModuleHandleW(NULL),
                               MAKEINTRESOURCEW(IDI_APP_ICON),
                               IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (g_icon != NULL)
        g_custom_icon = TRUE;
    else {
        ShowSystemError(L"Could not load the embedded application icon.",
                        GetLastError());
        g_icon = LoadIconW(NULL, IDI_APPLICATION);
    }

    ZeroMemory(&g_tray_icon, sizeof(g_tray_icon));
    g_tray_icon.cbSize = NOTIFYICONDATA_V3_SIZE;
    g_tray_icon.hWnd = window;
    g_tray_icon.uID = 1;
    g_tray_icon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_tray_icon.uCallbackMessage = TRAY_MESSAGE;
    g_tray_icon.hIcon = g_icon;
    wcscpy_s(g_tray_icon.szTip, ARRAYSIZE(g_tray_icon.szTip), APP_NAME);

    if (show_notification) {
        if (swprintf_s(ports, ARRAYSIZE(ports),
                       L"HTTP: %hs  SOCKS: %hs  Mixed: %hs",
                       g_http_port[0] ? g_http_port : "-",
                       g_socks_port[0] ? g_socks_port : "-",
                       g_mixed_port[0] ? g_mixed_port : "-") < 0) {
            MessageBoxW(NULL, L"Could not create the port notification.",
                        APP_NAME, MB_OK | MB_ICONERROR);
            if (g_custom_icon)
                DestroyIcon(g_icon);
            g_icon = NULL;
            g_custom_icon = FALSE;
            return FALSE;
        }
    }

    if (!Shell_NotifyIconW(NIM_ADD, &g_tray_icon)) {
        ShowSystemError(L"Could not create the tray icon.", GetLastError());
        if (g_custom_icon)
            DestroyIcon(g_icon);
        g_icon = NULL;
        g_custom_icon = FALSE;
        return FALSE;
    }
    g_tray_added = TRUE;

    if (!show_notification)
        return TRUE;

    g_tray_icon.cbSize = sizeof(g_tray_icon);
    g_tray_icon.uFlags = NIF_INFO;
    wcscpy_s(g_tray_icon.szInfoTitle, ARRAYSIZE(g_tray_icon.szInfoTitle),
             APP_NAME);
    wcscpy_s(g_tray_icon.szInfo, ARRAYSIZE(g_tray_icon.szInfo), ports);
    g_tray_icon.dwInfoFlags = NIIF_NONE;
    g_tray_icon.hBalloonIcon = NULL;
    if (!Shell_NotifyIconW(NIM_MODIFY, &g_tray_icon)) {
        ShowSystemError(L"Could not display the startup port notification.",
                        GetLastError());
        if (g_tray_hidden) {
            RemoveTrayIcon();
            return FALSE;
        }
        return TRUE;
    }

    if (g_tray_hidden) {
        if (SetTimer(window, TEMP_TRAY_ICON_TIMER, TEMP_TRAY_ICON_FALLBACK_MS,
                     NULL) == 0) {
            ShowSystemError(L"Could not start the temporary tray icon timeout.",
                            GetLastError());
            RemoveTrayIcon();
            return FALSE;
        }
        g_temporary_tray_icon = TRUE;
    }
    return TRUE;
}

static BOOL SetTrayHidden(HWND window, BOOL hidden)
{
    HANDLE marker;

    if (hidden == g_hide_tray_marker)
        return TRUE;

    if (hidden) {
        marker = CreateFileW(g_hide_tray_path, GENERIC_WRITE, 0, NULL,
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (marker == INVALID_HANDLE_VALUE) {
            ShowSystemError(L"Could not create the tray visibility marker.",
                            GetLastError());
            return FALSE;
        }
        if (!CloseHandle(marker)) {
            DWORD error = GetLastError();
            DeleteFileW(g_hide_tray_path);
            ShowSystemError(L"Could not close the tray visibility marker.",
                            error);
            return FALSE;
        }

        g_hide_tray_marker = TRUE;
        if (g_tray_hidden)
            return TRUE;
        if (!RemoveTrayIcon())
            return FALSE;
        g_tray_hidden = TRUE;
        return TRUE;
    }

    if (g_tray_hidden) {
        if (!AddTrayIcon(window, FALSE))
            return FALSE;
        g_tray_hidden = FALSE;
    }
    if (!DeleteFileW(g_hide_tray_path)) {
        DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND) {
            ShowSystemError(L"Could not remove the tray visibility marker.", error);
            return FALSE;
        }
    }

    g_hide_tray_marker = FALSE;
    return TRUE;
}

static BOOL RemoveTrayIcon(void)
{
    if (g_tray_added) {
        KillTimer(g_tray_icon.hWnd, TEMP_TRAY_ICON_TIMER);
        g_temporary_tray_icon = FALSE;
        g_tray_icon.cbSize = NOTIFYICONDATA_V3_SIZE;
        if (!Shell_NotifyIconW(NIM_DELETE, &g_tray_icon)) {
            ShowSystemError(L"Could not remove the tray icon.", GetLastError());
            return FALSE;
        }
        g_tray_added = FALSE;
    }
    if (g_icon != NULL && g_custom_icon) {
        DestroyIcon(g_icon);
    }
    g_icon = NULL;
    g_custom_icon = FALSE;
    return TRUE;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous_instance,
                    PWSTR command_line, int show_command)
{
    const WCHAR class_name[] = L"SlimClashLauncherWindow";
    WNDCLASSEXW window_class;
    HWND window;
    MSG message;
    char controller[256] = "";
    const char *proxy_port;
    DWORD wait_result;
    int exit_code = 0;

    (void)previous_instance;
    (void)command_line;
    (void)show_command;

    if (!GetLauncherPaths())
        goto cleanup;
    if (!ReadTrayHiddenMarker(&g_hide_tray_marker))
        goto cleanup;
    if (!AcquireInstanceLock())
        goto cleanup;
    if (g_unhide_forwarded)
        goto cleanup;
    if (!TerminateMatchingProcesses(TRUE, TRUE))
        goto cleanup;
    if (!ReadConfig(g_http_port, sizeof(g_http_port),
                    g_socks_port, sizeof(g_socks_port),
                    g_mixed_port, sizeof(g_mixed_port),
                    controller, sizeof(controller),
                    g_api_secret, sizeof(g_api_secret)))
        goto cleanup;
    if (!BuildControllerUrls(controller)) {
        MessageBoxW(NULL, L"Could not parse the Dashboard controller address in the configuration.",
                    APP_NAME, MB_OK | MB_ICONERROR);
        goto cleanup;
    }
    proxy_port = g_mixed_port[0] != '\0' ? g_mixed_port : g_http_port;
    g_proxy_settings_active = TRUE;
    if (!SetProxySettings(TRUE, proxy_port))
        goto cleanup;

    g_taskbar_created_message = RegisterWindowMessageW(L"TaskbarCreated");
    if (g_taskbar_created_message == 0) {
        ShowSystemError(L"Could not register the taskbar restart message.",
                        GetLastError());
        goto cleanup;
    }

    ZeroMemory(&window_class, sizeof(window_class));
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = WindowProc;
    window_class.hInstance = instance;
    window_class.lpszClassName = class_name;
    if (RegisterClassExW(&window_class) == 0) {
        ShowSystemError(L"Could not register the tray window.", GetLastError());
        goto cleanup;
    }

    window = CreateWindowExW(0, class_name, APP_NAME, 0,
                             0, 0, 0, 0, NULL, NULL, instance, NULL);
    if (window == NULL) {
        ShowSystemError(L"Could not create the tray window.", GetLastError());
        UnregisterClassW(class_name, instance);
        goto cleanup;
    }

    if (!LaunchProxy()) {
        DestroyWindow(window);
        UnregisterClassW(class_name, instance);
        goto cleanup;
    }
    g_tray_hidden = g_hide_tray_marker;
    if (g_tray_hidden) {
        AddTrayIcon(window, TRUE);
    } else if (!AddTrayIcon(window, TRUE)) {
        StopProxy();
        DestroyWindow(window);
        UnregisterClassW(class_name, instance);
        goto cleanup;
    }

    for (;;) {
        wait_result = MsgWaitForMultipleObjectsEx(1, &g_proxy.hProcess, INFINITE,
                                                   QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (wait_result == WAIT_OBJECT_0)
            break;
        if (wait_result == WAIT_OBJECT_0 + 1) {
            while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT)
                    goto message_loop_done;
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            continue;
        }
        if (wait_result == WAIT_FAILED) {
            ShowSystemError(L"An error occurred while waiting for the Clash process.",
                            GetLastError());
            exit_code = 1;
            break;
        }
    }

message_loop_done:
    RemoveTrayIcon();
    StopProxy();
    if (IsWindow(window))
        DestroyWindow(window);
    UnregisterClassW(class_name, instance);

cleanup:
    if (g_proxy_settings_active) {
        SetProxySettings(FALSE, NULL);
        FlushDnsCache();
        g_proxy_settings_active = FALSE;
    }
    if (g_mutex != NULL && g_mutex_owned)
        ReleaseMutex(g_mutex);
    if (g_mutex != NULL)
        CloseHandle(g_mutex);
    return exit_code;
}
