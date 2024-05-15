#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

//cliente...

#define PIPE_NAME TEXT("\\\\.\\pipe\\teste")

DWORD WINAPI recebeMsg(LPVOID data) {

    HANDLE hPipe = (HANDLE)data;
    TCHAR buf[256];
    BOOL ret;
    DWORD n;

    OVERLAPPED ov;
    //EVENTO PARA AVISAR QUE JA LEU
    HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);

    do {
        //OVERLAPPED I/O
        ZeroMemory(&ov, sizeof(OVERLAPPED));
        ov.hEvent = hEv;

        ret = ReadFile(hPipe, buf, sizeof(buf), &n, NULL);
        if (ret == TRUE)
            _tprintf(_T("Li de imediato\n"));
        else
        {
            if (GetLastError() == ERROR_IO_PENDING) { 
                _tprintf(_T("Agendei uma leitura\n"));
                //...
                WaitForSingleObject(hEv, INFINITE);
                GetOverlappedResult(hPipe, &ov, &n, FALSE);
            }
            else {
                _tprintf(_T("[ERRO] leitura\n"));
                exit(-1);
            }
        }

        if (!ret || !n) {
            _tprintf(TEXT("[ERRO] %d %d... (ReadFile)\n"), ret, n);
            break;
        }
        buf[n / sizeof(TCHAR)] = '\0';
        _tprintf(TEXT("[CLIENTE] Recebi %d bytes: '%s'... (ReadFile)\n"), n, buf);
    } while (_tcscmp(buf, _T("SAIR")));

    return 0;
}

int _tmain(int argc, LPTSTR argv[]) {
    TCHAR buf[256];
    HANDLE hPipe, hThread;
    int i = 0;
    BOOL ret;
    DWORD n;

#ifdef UNICODE
    _setmode(_fileno(stdin), _O_WTEXT);
    _setmode(_fileno(stdout), _O_WTEXT);
    _setmode(_fileno(stderr), _O_WTEXT);
#endif

    _tprintf(TEXT("[CLIENTE] Esperar pelo pipe '%s' (WaitNamedPipe)\n"),
        PIPE_NAME);
    if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_NAME);
        exit(-1);
    }
    _tprintf(TEXT("[CLIENTE] Ligação ao pipe do escritor... (CreateFile)\n"));
    hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    if (hPipe == NULL) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_NAME);
        exit(-1);
    }
    _tprintf(TEXT("[CLIENTE] Liguei-me...\n"));


    //CRIAR THREAD
    hThread = CreateThread(NULL, 0, recebeMsg, (LPVOID)hPipe, 0, NULL);

    OVERLAPPED ov;
    //EVENTO PARA AVISAR QUE JA LEU
    HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);
    ZeroMemory(&ov, sizeof(OVERLAPPED));
    ov.hEvent = hEv;

    ret = WriteFile(hPipe, buf, sizeof(buf), &n, NULL);     //3º parametro imcompleto
    if (ret == TRUE)
        _tprintf(_T("Li de imediato\n"));
    else
    {
        if (GetLastError() == ERROR_IO_PENDING) {
            _tprintf(_T("Agendei uma leitura\n"));
            //...
            WaitForSingleObject(hEv, INFINITE);
            GetOverlappedResult(hPipe, &ov, &n, FALSE);
        }
        else {
            _tprintf(_T("[ERRO] leitura\n"));
            exit(-1);
        }
    }

    do {
        _tprintf(TEXT("[CLIENTE] Frase: "));
        _fgetts(buf, 256, stdin);
        buf[_tcslen(buf) - 1] = '\0';
        //OVERLAPPED I/O
        if (!WriteFile(hPipe, buf, (DWORD)_tcslen(buf) * sizeof(TCHAR), &n, NULL)) {
            _tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
            exit(-1);
        }
        _tprintf(TEXT("[CLIENTE] Enviei %d bytes ao leitor...' ' (WriteFile)\n"), n);

    } while (_tcscmp(buf, _T("sair")));

    //ELIMINAR THREAD
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    CloseHandle(hPipe);
    return 0;
}
