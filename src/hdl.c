#define INITGUID

#include <initguid.h>
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#include <stdio.h>
#include <string.h>

/*=====================================================*/
/* DEBUG LOG (arquivo aberto uma vez, sem overhead) */
/*=====================================================*/

static FILE *debug_fp = NULL;

void debug_log(const char *msg)
{
    if(!debug_fp)
        return;

    fprintf(debug_fp, "%s\r\n", msg);
    fflush(debug_fp);
}

#define NTVDBM_PATH  "C:\\WINDOWS\\SYSTEM32\\NTVDBM.EXE"
#define WINBOX_PATH  "C:\\WINDOWS\\SYSTEM32\\WINBOX.EXE"

#define DEFAULT_CONF "C:\\WINDOWS\\SYSTEM32\\NTVDBM.CONF"
#define CONF_DIR     "C:\\WINDOWS\\SYSTEM32\\CONF\\"

#define WINBOX_DIR   "C:\\WINDOWS\\SYSTEM32\\WINBOX"
#define WIN_INI      "C:\\WINDOWS\\SYSTEM32\\WINBOX\\WIN.INI"

#define TYPE_DOS     0
#define TYPE_WIN32   1
#define TYPE_WIN16   2


/*=====================================================*/
/* CRC16 - file content */
/*=====================================================*/

unsigned short crc16_file(const char *path)
{
    FILE *f = fopen(path,"rb");

    if(!f)
        return 0;

    unsigned short crc = 0xFFFF;

    unsigned char buffer[4096];

    size_t rd;

    while((rd = fread(buffer,1,sizeof(buffer),f)) > 0)
    {
        size_t i;

        for(i = 0; i < rd; i++)
        {
            crc ^= buffer[i];

            int j;

            for(j = 0; j < 8; j++)
            {
                if(crc & 1)
                    crc = (crc >> 1) ^ 0xA001;
                else
                    crc >>= 1;
            }
        }
    }

    fclose(f);

    return crc;
}


/*=====================================================*/
/* .COM sempre DOS */
/*=====================================================*/

int is_com(const char *path)
{
    const char *e = strrchr(path,'.');

    if(!e)
        return 0;

    return lstrcmpiA(e,".com")==0;
}


/*=====================================================*/
/* detector refinado */
/*=====================================================*/

int exe_type(const char *path)
{
    FILE *f = fopen(path,"rb");

    if(!f)
        return TYPE_DOS;

    unsigned char mz[2];

    if(fread(mz,1,2,f)!=2)
    {
        fclose(f);
        return TYPE_DOS;
    }

    if(mz[0]!='M' || mz[1]!='Z')
    {
        fclose(f);
        return TYPE_DOS;
    }

    /* tamanho do arquivo para validacao */
    fseek(f,0,SEEK_END);
    long fsize = ftell(f);

    fseek(f,0x3C,SEEK_SET);

    unsigned int ofs = 0;

    /* valida: leitura OK, offset >= 0x40 (min PE/NE), cabe no arquivo */
    if(fread(&ofs,4,1,f) != 1 || ofs < 0x40 || (long)ofs + 2 > fsize)
    {
        fclose(f);
        return TYPE_DOS;
    }

    fseek(f,ofs,SEEK_SET);

    unsigned char sig[2] = {0};

    if(fread(sig,1,2,f) != 2)
    {
        fclose(f);
        return TYPE_DOS;
    }

    fclose(f);

    if(sig[0]=='P' && sig[1]=='E')
        return TYPE_WIN32;

    if(sig[0]=='N' && sig[1]=='E')
        return TYPE_WIN16;

    return TYPE_DOS;
}


/*=====================================================*/
/* pega exe real */
/*=====================================================*/

void get_real_exe(char *out)
{
    char *cmd = GetCommandLineA();

    if(*cmd=='"')
    {
        cmd++;

        char *q = strchr(cmd,'"');

        if(!q)
        {
            out[0]=0;
            return;
        }

        cmd = q + 1;
    }
    else
    {
        char *sp = strchr(cmd,' ');

        if(!sp)
        {
            out[0]=0;
            return;
        }

        cmd = sp + 1;
    }

    while(*cmd==' ')
        cmd++;

    lstrcpyA(out,cmd);

    if(out[0]=='"')
    {
        memmove(out,out+1,strlen(out));

        char *q = strchr(out,'"');

        if(q)
            *q=0;
    }
}


/*=====================================================*/
/* escreve shell= no WIN.INI */
/*=====================================================*/

void write_win_shell(const char *exe)
{
    char shell[1024];

    wsprintfA(
        shell,
        "%s",
        exe
    );

    WritePrivateProfileStringA(
        "boot",
        "shell",
        shell,
        WIN_INI
    );
}


/*=====================================================*/
/* pega linha de comando completa apos o HDL */
/*=====================================================*/

void get_full_cmdline(char *out)
{
    char *cmd = GetCommandLineA();

    /* pula o caminho do proprio HDL */
    if(*cmd == '"')
    {
        cmd++;

        char *q = strchr(cmd, '"');

        if(!q)
        {
            out[0] = 0;
            return;
        }

        cmd = q + 1;
    }
    else
    {
        char *sp = strchr(cmd, ' ');

        if(!sp)
        {
            out[0] = 0;
            return;
        }

        cmd = sp + 1;
    }

    while(*cmd == ' ')
        cmd++;

    lstrcpyA(out, cmd);
}


/*=====================================================*/
/* cria atalho .lnk para o arquivo .conf */
/*=====================================================*/

void create_conf_shortcut(
    const char *conf_path,
    const char *target_dir
)
{
    char name[MAX_PATH];

    lstrcpyA(name, PathFindFileNameA(conf_path));

    PathRemoveExtensionA(name);

    char shortcut_path[MAX_PATH];

    wsprintfA(
        shortcut_path,
        "%s\\%s.lnk",
        target_dir,
        name
    );

    CoInitialize(NULL);

    IShellLinkA *psl;

    HRESULT hr =
        CoCreateInstance(
            &CLSID_ShellLink,
            NULL,
            CLSCTX_INPROC_SERVER,
            &IID_IShellLinkA,
            (LPVOID *)&psl
        );

    if(SUCCEEDED(hr))
    {
        psl->lpVtbl->SetPath(psl, conf_path);

        IPersistFile *ppf;

        hr =
            psl->lpVtbl->QueryInterface(
                psl,
                &IID_IPersistFile,
                (LPVOID *)&ppf
            );

        if(SUCCEEDED(hr))
        {
            WCHAR wsz[MAX_PATH];

            MultiByteToWideChar(
                CP_ACP,
                0,
                shortcut_path,
                -1,
                wsz,
                MAX_PATH
            );

            ppf->lpVtbl->Save(ppf, wsz, TRUE);

            ppf->lpVtbl->Release(ppf);
        }

        psl->lpVtbl->Release(psl);
    }

    CoUninitialize();
}


/*=====================================================*/
/* MAIN */
/*=====================================================*/

int WINAPI WinMain(
    HINSTANCE a,
    HINSTANCE b,
    LPSTR c,
    int d
)
{
    /* debug: abre o log uma unica vez para todo o ciclo de vida */
    debug_fp = fopen(
        "C:\\WINDOWS\\SYSTEM32\\HDL_DEBUG.LOG",
        "w"
    );

    if(debug_fp)
    {
        fprintf(debug_fp, "=== HDL DEBUG ===\r\n");
    }

    char cmdline[4096];

    lstrcpyA(cmdline, GetCommandLineA());

    debug_log("--- HDL iniciado ---");
    debug_log(cmdline);


    char exe[MAX_PATH];

    get_real_exe(exe);

    debug_log("exe:");
    debug_log(exe);


    if(!exe[0])
    {
        debug_log("ERRO: exe vazio");
        return 0;
    }


    char self[MAX_PATH];

    GetModuleFileNameA(
        NULL,
        self,
        MAX_PATH
    );

    debug_log("self:");
    debug_log(self);


    if(lstrcmpiA(exe,self)==0)
    {
        debug_log("ERRO: exe == self (loop)");
        return 0;
    }


    /*========================================*/
    /* identifica tipo */
    /*========================================*/

    int type = exe_type(exe);


    /*========================================*/
    /* WIN32 NATIVO - preserva TODOS os args */
    /*========================================*/

    if(
        !is_com(exe) &&
        type == TYPE_WIN32
    )
    {
        char full_cmd[4096];

        get_full_cmdline(full_cmd);

        debug_log("WIN32 full_cmd:");
        debug_log(full_cmd);


        if(!full_cmd[0])
        {
            debug_log("ERRO: full_cmd vazio");
            return 0;
        }

        STARTUPINFOA si = {0};
        PROCESS_INFORMATION pi = {0};

        si.cb = sizeof(si);

        BOOL ok = CreateProcessA(
            NULL,
            full_cmd,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &si,
            &pi
        );

        debug_log(ok ? "CreateProcess OK" : "CreateProcess FALHOU");


        if(!ok)
        {
            char errbuf[64];

            wsprintfA(errbuf, "GetLastError=%lu", GetLastError());

            debug_log(errbuf);
        }

        return 0;
    }


    /*========================================*/
    /* WIN16 -> WINBOX */
    /*========================================*/

    if(
        !is_com(exe) &&
        type == TYPE_WIN16
    )
    {
        debug_log("Tipo: WIN16");


        char dir[MAX_PATH];

        lstrcpyA(dir,exe);

        char *slash = strrchr(dir,'\\');

        if(!slash)
        {
            debug_log("ERRO: sem barra em exe");
            return 0;
        }

        *slash = 0;


        /*=====================================*/
        /* shell=app */
        /*=====================================*/

        debug_log("escrevendo shell= no WIN.INI");
        write_win_shell(exe);


        /*=====================================*/
        /* inicia WINBOX */
        /*=====================================*/

        char cmd[4096];

        wsprintfA(
            cmd,

            "\"%s\" "
            "-exit "
            "-c \"mount c '%s'\" "
            "-c \"mount w '%s'\" "
            "-c \"w:\" "
            "-c \"win\"",

            WINBOX_PATH,
            dir,
            WINBOX_DIR
        );


        debug_log("cmd:");
        debug_log(cmd);


        STARTUPINFOA si = {0};
        PROCESS_INFORMATION pi = {0};

        si.cb = sizeof(si);

        BOOL ok = CreateProcessA(
            NULL,
            cmd,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            dir,   /* working dir = pasta do jogo */
            &si,
            &pi
        );

        debug_log(ok ? "CreateProcess OK" : "CreateProcess FALHOU");


        if(!ok)
        {
            char errbuf[64];

            wsprintfA(errbuf, "GetLastError=%lu", GetLastError());

            debug_log(errbuf);
        }

        return 0;
    }


    /*========================================*/
    /* DOS */
    /*========================================*/

    char dir[MAX_PATH];

    lstrcpyA(dir,exe);

    char *slash = strrchr(dir,'\\');

    if(!slash)
        return 0;


    char name[MAX_PATH];

    lstrcpyA(name,slash+1);

    *slash = 0;


    char exename[MAX_PATH];

    lstrcpyA(exename,name);

    char *dot = strrchr(exename,'.');

    if(dot)
        *dot = 0;


    /* CRC16 baseado no CONTEUDO do arquivo, nao no path */
    unsigned short crc =
        crc16_file(exe);


    char conf[MAX_PATH];

    wsprintfA(
        conf,
        "%s%s_%04X.conf",
        CONF_DIR,
        exename,
        crc
    );


    CreateDirectoryA(CONF_DIR, NULL);

    if(
        GetFileAttributesA(conf)
        ==
        INVALID_FILE_ATTRIBUTES
    )
    {
        CopyFileA(
            DEFAULT_CONF,
            conf,
            FALSE
        );
    }

    /* cria atalho para o .conf na pasta do exe */
    create_conf_shortcut(conf, dir);


    SetEnvironmentVariableA(
        "SDL_STDIO_REDIRECT",
        "0"
    );


    char cmd[4096];

    debug_log("Tipo: DOS");


    wsprintfA(
        cmd,

        "\"%s\" "
        "-conf \"%s\" "
        "-exit "
        "-c \"mount c '%s'\" "
        "-c \"c:\" "
        "-c \"%s\" "
        "-c \"exit\"",

        NTVDBM_PATH,
        conf,
        dir,
        name
    );


    debug_log("cmd:");
    debug_log(cmd);


    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};

    si.cb = sizeof(si);

    BOOL ok = CreateProcessA(
        NULL,
        cmd,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        dir,   /* working dir = pasta do jogo */
        &si,
        &pi
    );

    debug_log(ok ? "CreateProcess OK" : "CreateProcess FALHOU");

    if(!ok)
    {
        char errbuf[64];

        wsprintfA(errbuf, "GetLastError=%lu", GetLastError());

        debug_log(errbuf);
    }

    /* fecha o debug log */
    if(debug_fp)
        fclose(debug_fp);

    return 0;
}