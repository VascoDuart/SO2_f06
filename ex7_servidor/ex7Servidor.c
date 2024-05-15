#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#define PIPE_NAME TEXT("\\\\.\\pipe\\teste")
#define MAX 5

//servidor...
typedef struct {
    HANDLE hTrinco;
    HANDLE hLeitor[MAX];
    BOOL continua;
}TDATA;

typedef struct {
    TDATA* ptd;
    DWORD id;
}TDATA_EXTRA;




DWORD WINAPI atendCli(LPVOID data) {
    //thread para tratar de um clienteº
    TDATA_EXTRA* td_extra = (TDATA_EXTRA*)data;
    HANDLE hPipe;
    DWORD i, n, id;
    TCHAR buf[256];

    id = td_extra->id;
    WaitForSingleObject(td_extra->ptd->hTrinco, INFINITE);
    hPipe = td_extra->ptd->hLeitor[id];
    ReleaseMutex(td_extra->ptd->hTrinco);

    do {
        //ler pedido do cliente
        BOOL ret = ReadFile(hPipe, buf, sizeof(buf), &n, NULL);
        if (!ret || !n) {
            _tprintf(TEXT("[ERRO] %d %d... (ReadFile)\n"), ret, n);
            break;
        }
        buf[n / sizeof(TCHAR)] = '\0';
        _tprintf(TEXT("[SERVIDOR] Recebi %d bytes no pipe %d: '%s'... (ReadFile)\n"), n, id, buf);

        CharUpperBuff(buf, (DWORD)_tcslen(buf)); //processar...
        if (!WriteFile(hPipe, buf, (DWORD)_tcslen(buf) * sizeof(TCHAR), &n, NULL)) {    //ENVIAR RESPOSTA
            _tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
            exit(-1);
        }
        _tprintf(TEXT("[SERVIDOR] Enviei %d bytes ao cliente %d...' ' (WriteFile)\n"), n, id);

    } while (_tcscmp(buf, _T("SAIR")));

    FlushFileBuffers(hPipe);
    WaitForSingleObject(td_extra->ptd->hTrinco, INFINITE);
    td_extra->ptd->hLeitor[id] = NULL;

    _tprintf(_T("[SERVIDOR] Desligar o pipe %d (DisconnectedNamedPipe)\n"), id);
    if (!DisconnectNamedPipe(hPipe)) {
        _tprintf(TEXT("[ERRO] Desligar o pipe! (DisconnectNamedPipe)"));
        exit(-1);
    }
    CloseHandle(hPipe);


    return 0;
}

int _tmain(int argc, LPTSTR argv[]) {
    DWORD nCLientes = 0, i;
    DWORD n;
    HANDLE hPipe, hThread;
    HANDLE hThreads[MAX];
    TDATA td;
    TCHAR buf[256];
    TDATA_EXTRA td_extra[MAX];

#ifdef UNICODE
    _setmode(_fileno(stdin), _O_WTEXT);
    _setmode(_fileno(stdout), _O_WTEXT);
    _setmode(_fileno(stderr), _O_WTEXT);
#endif

    for (i = 0; i < MAX; i++) {
        td.hLeitor[i] = NULL;
        td.hTrinco = CreateMutex(NULL, FALSE, NULL);
        td.continua = TRUE;
    }

    _tprintf(TEXT("[SERVIDOR] Criar uma cópia do pipe '%s' ... (CreateNamedPipe)\n"), PIPE_NAME);
    hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_WAIT |
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1,
        sizeof(buf), sizeof(buf), 1000, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) {
        _tprintf(TEXT("[ERRO] Criar Named Pipe! (CreateNamedPipe)"));
        exit(-1);
    }

    BOOL res = ConnectNamedPipe(hPipe, NULL);
    if (res) {
        _tprintf(TEXT("[SERVIDOR] Ligação ao cliente! (ConnectNamedPipe\n"));
    }
    if (nCLientes < MAX) {
        WaitForSingleObject(td.hTrinco, INFINITE);
        td.hLeitor[nCLientes] = hPipe;
        ReleaseMutex(td.hTrinco);

        td_extra[nCLientes].id = nCLientes;
        td_extra[nCLientes].ptd = &td;
        hThreads[nCLientes] = CreateThread(NULL, 0, atendCli, (LPVOID)&td_extra, 0, NULL);

        nCLientes++;
    }

    for (i = 0; i < MAX; i++) {
        if (td.hLeitor[i] != NULL) {
            WaitForSingleObject(hThreads[i], INFINITE);
            CloseHandle(hThreads[i]);
        }
    }

    CloseHandle(td.hTrinco);

    do {
        _tprintf(TEXT("[SERVIDOR] Frase: "));
        _fgetts(buf, 256, stdin);
        buf[_tcslen(buf) - 1] = '\0';
        if (!WriteFile(hPipe, buf, (DWORD)_tcslen(buf) * sizeof(TCHAR), &n, NULL)) {
            _tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
            exit(-1);
        }
        _tprintf(TEXT("[SERVIDOR] Enviei %d bytes ao leitor...' ' (WriteFile)\n"), n);
    } while (_tcscmp(buf, TEXT("fim")));

    _tprintf(TEXT("[SERVIDOR] Desligar o pipe (DisconnectNamedPipe)\n"));

    FlushFileBuffers(hPipe);

    if (!DisconnectNamedPipe(hPipe)) {
        _tprintf(TEXT("[ERRO] Desligar o pipe! (DisconnectNamedPipe)"));
        exit(-1);
    }

    Sleep(2000);
    CloseHandle(hPipe);
    return 0;
}
