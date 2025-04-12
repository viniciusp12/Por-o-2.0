#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Evento global para parar as threads
HANDLE stopEvent;

// Arquivo de log
FILE* logFile;

// Variável global para sinalizar detecção de ransomware
BOOL ransomwareDetected = FALSE;

// Função para registrar eventos no log
void LogEvent(const char* message) {
    time_t now = time(NULL);
    char* dt = ctime(&now);
    dt[strlen(dt) - 1] = '\0'; // Remove o \n do ctime
    fprintf(logFile, "%s: %s\n", dt, message);
    fflush(logFile);
    printf("%s: %s\n", dt, message);
}

// Função para criar diretórios se não existirem
BOOL CreateDirectoryIfNotExists(const wchar_t* path) {
    if (!CreateDirectoryW(path, NULL)) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            return FALSE;
        }
    }
    return TRUE;
}

// Função para criar arquivos honeypot
void CreateHoneypotFiles(const wchar_t* directory) {
    wchar_t filePath[MAX_PATH];
    for (int i = 1; i <= 10; i++) {
        swprintf(filePath, MAX_PATH, L"%s\\.porao%d.txt", directory, i);
        HANDLE hFile = CreateFileW(filePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            const char* content = "Arquivo honeypot para detectar ransomware\n";
            DWORD bytesWritten;
            WriteFile(hFile, content, strlen(content), &bytesWritten, NULL);
            CloseHandle(hFile);
            char logMsg[512];
            sprintf(logMsg, "Honeypot criado: %S", filePath);
            LogEvent(logMsg);
        }
    }
}

// Função para criar backup de uma pasta
BOOL CreateBackup(const wchar_t* source, const wchar_t* destination) {
    wchar_t command[MAX_PATH * 2];
    swprintf(command, MAX_PATH * 2, L"xcopy \"%s\\*.*\" \"%s\" /Y /E /Q", source, destination);
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi;
    BOOL success = CreateProcessW(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (success) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (exitCode == 0) {
            char logMsg[512];
            sprintf(logMsg, "Backup criado: %S -> %S", source, destination);
            LogEvent(logMsg);
            return TRUE;
        } else {
            char logMsg[512];
            sprintf(logMsg, "Erro ao criar backup: %S -> %S (Código: %lu)", source, destination, exitCode);
            LogEvent(logMsg);
        }
    } else {
        char logMsg[512];
        sprintf(logMsg, "Falha ao executar xcopy para backup: %S -> %S", source, destination);
        LogEvent(logMsg);
    }
    return FALSE;
}

// Função para restaurar backup
BOOL RestoreBackup(const wchar_t* source, const wchar_t* destination) {
    wchar_t command[MAX_PATH * 2];
    swprintf(command, MAX_PATH * 2, L"xcopy \"%s\\*.*\" \"%s\" /Y /E /Q", source, destination);
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi;
    BOOL success = CreateProcessW(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (success) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (exitCode == 0) {
            char logMsg[512];
            sprintf(logMsg, "Backup restaurado: %S -> %S", source, destination);
            LogEvent(logMsg);
            return TRUE;
        } else {
            char logMsg[512];
            sprintf(logMsg, "Erro ao restaurar backup: %S -> %S (Código: %lu)", source, destination, exitCode);
            LogEvent(logMsg);
        }
    } else {
        char logMsg[512];
        sprintf(logMsg, "Falha ao executar xcopy para restauração: %S -> %S", source, destination);
        LogEvent(logMsg);
    }
    return FALSE;
}

// Função de controle de console para capturar Ctrl+C
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT) {
        SetEvent(stopEvent);
    }
    return TRUE;
}

// Thread para monitorar processos suspeitos
DWORD WINAPI MonitorProcessThread(LPVOID lpParam) {
    const wchar_t* suspiciousProcesses[] = {
        L"vssadmin.exe",
        L"wmic.exe",
        L"bcdedit.exe",
        L"taskkill.exe",
        NULL
    };
    while (WaitForSingleObject(stopEvent, 1000) == WAIT_TIMEOUT) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
            if (Process32First(hSnapshot, &pe)) {
                do {
                    for (int i = 0; suspiciousProcesses[i] != NULL; i++) {
                        if (_wcsicmp(pe.szExeFile, suspiciousProcesses[i]) == 0) {
                            char logMsg[512];
                            sprintf(logMsg, "Processo suspeito detectado: %S (PID: %lu)", pe.szExeFile, pe.th32ProcessID);
                            LogEvent(logMsg);
                            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                            if (hProcess != NULL) {
                                TerminateProcess(hProcess, 1);
                                CloseHandle(hProcess);
                                LogEvent("Processo suspeito encerrado.");
                                ransomwareDetected = TRUE; // Sinalizar detecção
                            }
                        }
                    }
                } while (Process32Next(hSnapshot, &pe));
            }
            CloseHandle(hSnapshot);
        }
    }
    return 0;
}

// Thread para monitorar mudanças no registro
DWORD WINAPI MonitorRegistryThread(LPVOID lpParam) {
    const wchar_t* regKeys[] = {
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
        NULL
    };
    HKEY hKey;
    HANDLE hRegEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hRegEvent == NULL) {
        LogEvent("Falha ao criar evento de registro");
        return 1;
    }
    for (int i = 0; regKeys[i] != NULL; i++) {
        if (RegOpenKeyExW(HKEY_CURRENT_USER, regKeys[i], 0, KEY_NOTIFY | KEY_READ, &hKey) == ERROR_SUCCESS) {
            while (TRUE) {
                if (RegNotifyChangeKeyValue(hKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, hRegEvent, TRUE) == ERROR_SUCCESS) {
                    HANDLE handles[2] = {hRegEvent, stopEvent};
                    DWORD waitResult = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
                    if (waitResult == WAIT_OBJECT_0) {
                        char logMsg[512];
                        sprintf(logMsg, "Chave de registro alterada: HKEY_CURRENT_USER\\%S, possível ransomware.", regKeys[i]);
                        LogEvent(logMsg);
                        ransomwareDetected = TRUE; // Sinalizar detecção
                    } else if (waitResult == WAIT_OBJECT_0 + 1) {
                        break;
                    }
                } else {
                    break;
                }
            }
            RegCloseKey(hKey);
        } else {
            char logMsg[512];
            sprintf(logMsg, "Falha ao abrir chave de registro: HKEY_CURRENT_USER\\%S", regKeys[i]);
            LogEvent(logMsg);
        }
    }
    CloseHandle(hRegEvent);
    return 0;
}

// Thread para monitorar mudanças em diretórios (honeypot)
DWORD WINAPI MonitorDirectoryThread(LPVOID lpParam) {
    const wchar_t** directories = (const wchar_t**)lpParam;
    HANDLE* hChanges = (HANDLE*)malloc(sizeof(HANDLE) * 3);
    if (hChanges == NULL) {
        LogEvent("Falha ao alocar memória para notificações de diretórios");
        return 1;
    }
    int dirCount = 0;
    for (int i = 0; i < 3; i++) {
        hChanges[i] = FindFirstChangeNotificationW(directories[i], FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
        if (hChanges[i] == INVALID_HANDLE_VALUE) {
            char logMsg[512];
            sprintf(logMsg, "Falha ao configurar notificação para %S", directories[i]);
            LogEvent(logMsg);
        } else {
            dirCount++;
        }
    }
    if (dirCount == 0) {
        free(hChanges);
        return 1;
    }
    while (TRUE) {
        HANDLE* handles = (HANDLE*)malloc(sizeof(HANDLE) * (dirCount + 1));
        for (int i = 0; i < dirCount; i++) {
            handles[i] = hChanges[i];
        }
        handles[dirCount] = stopEvent;
        DWORD waitResult = WaitForMultipleObjects(dirCount + 1, handles, FALSE, INFINITE);
        if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0 + dirCount) {
            int index = waitResult - WAIT_OBJECT_0;
            FindNextChangeNotification(hChanges[index]);
            char logMsg[512];
            sprintf(logMsg, "Mudança detectada em %S, possível ransomware.", directories[index]);
            LogEvent(logMsg);
            ransomwareDetected = TRUE; // Sinalizar detecção
        } else if (waitResult == WAIT_OBJECT_0 + dirCount) {
            free(handles);
            break;
        }
        free(handles);
    }
    for (int i = 0; i < dirCount; i++) {
        FindCloseChangeNotification(hChanges[i]);
    }
    free(hChanges);
    return 0;
}

// Thread para criar backups periodicamente
DWORD WINAPI BackupThread(LPVOID lpParam) {
    const wchar_t** directories = (const wchar_t**)lpParam;
    wchar_t userProfile[MAX_PATH];
    wchar_t backupRoot[MAX_PATH];

    // Obter o valor de %USERPROFILE%
    if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH) == 0) {
        LogEvent("Erro ao obter USERPROFILE para backup");
        return 1;
    }

    // Construir o caminho do backupRoot
    swprintf(backupRoot, MAX_PATH, L"%s\\ProtectedBackup", userProfile);
    CreateDirectoryIfNotExists(backupRoot);

    while (WaitForSingleObject(stopEvent, 5400000) == WAIT_TIMEOUT) { // 1h30
        for (int i = 0; i < 3; i++) {
            wchar_t backupPath[MAX_PATH];
            swprintf(backupPath, MAX_PATH, L"%s\\%s", backupRoot, wcsrchr(directories[i], L'\\') + 1);
            CreateDirectoryIfNotExists(backupPath);
            CreateBackup(directories[i], backupPath);
        }
    }
    return 0;
}

int main() {
    // Abrir arquivo de log
    logFile = fopen("antivirus_log.txt", "a");
    if (logFile == NULL) {
        printf("Falha ao abrir arquivo de log\n");
        return 1;
    }

    stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (stopEvent == NULL) {
        LogEvent("Falha ao criar evento de parada");
        fclose(logFile);
        return 1;
    }

    SetConsoleCtrlHandler(HandlerRoutine, TRUE);

    // Definir pastas críticas
    const wchar_t* criticalFolders[] = {
        NULL, // Downloads
        NULL, // Documents
        NULL  // Desktop
    };
    wchar_t userProfile[MAX_PATH];
    GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH);
    criticalFolders[0] = (wchar_t*)malloc(MAX_PATH * sizeof(wchar_t));
    criticalFolders[1] = (wchar_t*)malloc(MAX_PATH * sizeof(wchar_t));
    criticalFolders[2] = (wchar_t*)malloc(MAX_PATH * sizeof(wchar_t));
    swprintf((wchar_t*)criticalFolders[0], MAX_PATH, L"%s\\Downloads", userProfile);
    swprintf((wchar_t*)criticalFolders[1], MAX_PATH, L"%s\\Documents", userProfile);
    swprintf((wchar_t*)criticalFolders[2], MAX_PATH, L"%s\\Desktop", userProfile);

    // Criar pastas e honeypots
    for (int i = 0; i < 3; i++) {
        CreateDirectoryIfNotExists(criticalFolders[i]);
        CreateHoneypotFiles(criticalFolders[i]);
    }

    // Criar threads
    HANDLE hProcessThread = CreateThread(NULL, 0, MonitorProcessThread, NULL, 0, NULL);
    HANDLE hRegistryThread = CreateThread(NULL, 0, MonitorRegistryThread, NULL, 0, NULL);
    HANDLE hDirectoryThread = CreateThread(NULL, 0, MonitorDirectoryThread, (LPVOID)criticalFolders, 0, NULL);
    HANDLE hBackupThread = CreateThread(NULL, 0, BackupThread, (LPVOID)criticalFolders, 0, NULL);

    if (hProcessThread == NULL || hRegistryThread == NULL || hDirectoryThread == NULL || hBackupThread == NULL) {
        LogEvent("Falha ao criar threads");
        for (int i = 0; i < 3; i++) {
            free((void*)criticalFolders[i]);
        }
        CloseHandle(stopEvent);
        fclose(logFile);
        return 1;
    }

    LogEvent("Programa em execução. Pressione Ctrl+C para parar.");

    // Loop principal para restaurar backups quando necessário
    wchar_t backupRoot[MAX_PATH];
    swprintf(backupRoot, MAX_PATH, L"%s\\ProtectedBackup", userProfile);
    while (WaitForSingleObject(stopEvent, 1000) == WAIT_TIMEOUT) {
        if (ransomwareDetected) {
            LogEvent("Ransomware detectado! Restaurando backups...");
            for (int i = 0; i < 3; i++) {
                wchar_t backupPath[MAX_PATH];
                swprintf(backupPath, MAX_PATH, L"%s\\%s", backupRoot, wcsrchr(criticalFolders[i], L'\\') + 1);
                RestoreBackup(backupPath, criticalFolders[i]);
            }
            ransomwareDetected = FALSE; // Resetar a flag
        }
    }

    // Aguarda as threads terminarem
    WaitForSingleObject(hProcessThread, INFINITE);
    WaitForSingleObject(hRegistryThread, INFINITE);
    WaitForSingleObject(hDirectoryThread, INFINITE);
    WaitForSingleObject(hBackupThread, INFINITE);

    // Limpeza
    CloseHandle(hProcessThread);
    CloseHandle(hRegistryThread);
    CloseHandle(hDirectoryThread);
    CloseHandle(hBackupThread);
    CloseHandle(stopEvent);
    for (int i = 0; i < 3; i++) {
        free((void*)criticalFolders[i]);
    }
    LogEvent("Programa parado.");
    fclose(logFile);

    return 0;
}