#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

//cliente...

#define PIPE_NAME TEXT("\\\\.\\pipe\\teste")

int _tmain(int argc, LPTSTR argv[]) {
    TCHAR buf[256];
    HANDLE hPipe;
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
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (hPipe == NULL) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_NAME);
        exit(-1);
    }
    _tprintf(TEXT("[CLIENTE] Liguei-me...\n"));

    do {

        //PEDIR TEXTO AO UTILIZADOR
         _tprintf(TEXT("[CLIENTE] Frase: "));
        _fgetts(buf, 256, stdin);
        buf[_tcslen(buf) - 1] = '\0';
        //ENVIAR PEDIDO AO SERVIDOR
        if (!WriteFile(hPipe, buf, (DWORD)_tcslen(buf) * sizeof(TCHAR), &n, NULL)) {
            _tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
            exit(-1);
        }
        _tprintf(TEXT("[CLIENTE] Enviei %d bytes ao leitor...' ' (WriteFile)\n"), n);

        ret = ReadFile(hPipe, buf, sizeof(buf), &n, NULL);
        buf[n / sizeof(TCHAR)] = '\0';
        if (!ret || !n) {
            _tprintf(TEXT("[ERRO] %d %d... (ReadFile)\n"), ret, n);
            break;
        }
        _tprintf(TEXT("[CLIENTE] Recebi %d bytes: '%s'... (ReadFile)\n"), n, buf);
    }while (_tcscmp(buf, _T("sair")));
    CloseHandle(hPipe);
    Sleep(200);
    return 0;
}
