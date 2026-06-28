#define INITGUID

#include <initguid.h>
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#include <shellapi.h>
#include <stdio.h>

#pragma comment(lib, "ole32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")

/*=====================================================*/
/* PATHS */
/*=====================================================*/

#define NTVDBM_PATH "C:\\WINDOWS\\SYSTEM32\\NTVDBM.EXE"
#define WINBOX_PATH "C:\\WINDOWS\\SYSTEM32\\WINBOX.EXE"

#define ICON_DIR    "C:\\WINDOWS\\SYSTEM32\\gameicons\\"
#define CONF_DIR    "C:\\WINDOWS\\SYSTEM32\\CONF\\"

#define ICON_DOS   "C:\\WINDOWS\\SYSTEM32\\gameicons\\default_dos.ico"
#define ICON_WIN16 "C:\\WINDOWS\\SYSTEM32\\gameicons\\default_win16.ico"
#define ICON_WIN32 "C:\\WINDOWS\\SYSTEM32\\gameicons\\default_win32.ico"

#define DEFAULT_CONF "C:\\WINDOWS\\SYSTEM32\\NTVDBM.CONF"

#define WINBOX_DIR "C:\\WINDOWS\\SYSTEM32\\WINBOX"

/*=====================================================*/
/* TYPES */
/*=====================================================*/

#define TYPE_DOS     0
#define TYPE_WIN32   1
#define TYPE_WIN16   2

int find_local_icon(
    const char *target,
    char *icon
);
/*=====================================================*/
/* FIND LOCAL ICO */
/*=====================================================*/

int find_local_icon(
    const char *target,
    char *icon
)
{
    char dir[MAX_PATH];

    lstrcpyA(
        dir,
        target
    );

    PathRemoveFileSpecA(dir);

    /*=============================================*/
    /* prioridade:
       mesmo nome do exe
    /*=============================================*/

    char exename[MAX_PATH];

    lstrcpyA(
        exename,
        PathFindFileNameA(target)
    );

    PathRemoveExtensionA(exename);

    wsprintfA(
        icon,
        "%s\\%s.ico",
        dir,
        exename
    );

    if(
        GetFileAttributesA(icon)
        !=
        INVALID_FILE_ATTRIBUTES
    )
    {
        return 1;
    }

    /*=============================================*/
    /* fallback:
       qualquer .ico
    /*=============================================*/

    char search[MAX_PATH];

    wsprintfA(
        search,
        "%s\\*.ico",
        dir
    );

    WIN32_FIND_DATAA fd;

    HANDLE h=
        FindFirstFileA(
            search,
            &fd
        );

    if(h==INVALID_HANDLE_VALUE)
        return 0;

    wsprintfA(
        icon,
        "%s\\%s",
        dir,
        fd.cFileName
    );

    FindClose(h);

    return 1;
}

/*=====================================================*/
/* CRC16 FILE */
/*=====================================================*/

unsigned short crc16_file(const char *path)
{
    FILE *f=fopen(path,"rb");

    if(!f)
        return 0;

    unsigned short crc=0xFFFF;

    unsigned char buffer[4096];

    size_t rd;

    while((rd=fread(buffer,1,sizeof(buffer),f))>0)
    {
        size_t i;

        for(i=0;i<rd;i++)
        {
            crc^=buffer[i];

            int j;

            for(j=0;j<8;j++)
            {
                if(crc&1)
                    crc=(crc>>1)^0xA001;
                else
                    crc>>=1;
            }
        }
    }

    fclose(f);

    return crc;
}

/*=====================================================*/
/* .COM */
/*=====================================================*/

int is_com(const char *path)
{
    const char *e=strrchr(path,'.');

    if(!e)
        return 0;

    return lstrcmpiA(e,".com")==0;
}

/*=====================================================*/
/* DETECTOR */
/*=====================================================*/

int exe_type(const char *path)
{
    FILE *f=fopen(path,"rb");

    if(!f)
        return TYPE_DOS;

    unsigned char mz[2];

    if(fread(mz,1,2,f)!=2)
    {
        fclose(f);
        return TYPE_DOS;
    }

    if(mz[0]!='M'||mz[1]!='Z')
    {
        fclose(f);
        return TYPE_DOS;
    }

    fseek(f,0x3C,SEEK_SET);

    unsigned int ofs=0;

    fread(&ofs,4,1,f);

    fseek(f,ofs,SEEK_SET);

    unsigned char sig[2]={0};

    fread(sig,1,2,f);

    fclose(f);

    if(sig[0]=='P'&&sig[1]=='E')
        return TYPE_WIN32;

    if(sig[0]=='N'&&sig[1]=='E')
        return TYPE_WIN16;
	
	if(sig[0]=='L'&&sig[1]=='E')
		return TYPE_WIN16;

    return TYPE_DOS;
}

/*=====================================================*/
/* HAS ICON */
/*=====================================================*/

int has_icon(const char *path)
{
    HICON hLarge=NULL;
    HICON hSmall=NULL;

    UINT count=
        ExtractIconExA(
            path,
            0,
            &hLarge,
            &hSmall,
            1
        );

    if(hLarge)
        DestroyIcon(hLarge);

    if(hSmall)
        DestroyIcon(hSmall);

    return count>0;
}

/*=====================================================*/
/* BUILD ICON */
/*=====================================================*/

void build_icon_path(
    const char *target,
    unsigned short crc,
    char *iconpath
)
{
    char name[MAX_PATH];

    lstrcpyA(
        name,
        PathFindFileNameA(target)
    );

    PathRemoveExtensionA(name);

    wsprintfA(
        iconpath,
        "%s%s_%04X.ico",
        ICON_DIR,
        name,
        crc
    );

    if(
        GetFileAttributesA(iconpath)
        !=
        INVALID_FILE_ATTRIBUTES
    )
        return;

    char source[MAX_PATH];

    int type=
        exe_type(target);

    if(type==TYPE_WIN32)
    {
        lstrcpyA(
            source,
            ICON_WIN32
        );
    }
    else
    if(type==TYPE_WIN16)
    {
        lstrcpyA(
            source,
            ICON_WIN16
        );
    }
    else
    {
        lstrcpyA(
            source,
            ICON_DOS
        );
    }

    CopyFileA(
        source,
        iconpath,
        FALSE
    );
}

/*=====================================================*/
/* UNIQUE SHORTCUT */
/*=====================================================*/

void build_unique_shortcut(
const char *desktop,
const char *name,
char *out
)
{
    wsprintfA(
        out,
        "%s\\%s.lnk",
        desktop,
        name
    );

    if(
        GetFileAttributesA(out)
        ==
        INVALID_FILE_ATTRIBUTES
    )
        return;

    int i;

    for(i=2;i<1000;i++)
    {
        wsprintfA(
            out,
            "%s\\%s (%d).lnk",
            desktop,
            name,
            i
        );

        if(
            GetFileAttributesA(out)
            ==
            INVALID_FILE_ATTRIBUTES
        )
            return;
    }
}

/*=====================================================*/
/* MAIN */
/*=====================================================*/

int WINAPI WinMain(
HINSTANCE hInst,
HINSTANCE hPrev,
LPSTR lpCmdLine,
int nShow
)
{
    if(
        !lpCmdLine
        ||
        !lpCmdLine[0]
    )
        return 0;

    char target[MAX_PATH];

    lstrcpyA(
        target,
        lpCmdLine
    );

    /* remove quotes */

    if(target[0]=='"')
    {
        memmove(
            target,
            target+1,
            strlen(target)
        );

        char *q=
            strrchr(
                target,
                '"'
            );

        if(q)
            *q=0;
    }

    char full[MAX_PATH];

    GetFullPathNameA(
        target,
        MAX_PATH,
        full,
        NULL
    );

    /*=================================================*/
    /* desktop */
/*=================================================*/

    char desktop[MAX_PATH];

    SHGetFolderPathA(
        NULL,
        CSIDL_DESKTOPDIRECTORY,
        NULL,
        0,
        desktop
    );

    /*=================================================*/
    /* extrai nome base */
    /*=================================================*/

    char name[MAX_PATH];

    lstrcpyA(
        name,
        PathFindFileNameA(full)
    );

    PathRemoveExtensionA(name);

    /*=================================================*/
    /* workdir */
    /*=================================================*/

    char workdir[MAX_PATH];

    lstrcpyA(
        workdir,
        full
    );

    PathRemoveFileSpecA(workdir);

    /*=================================================*/
    /* CRC16 do CONTEUDO do arquivo */
    /*=================================================*/

    unsigned short crc = crc16_file(full);

    /*=================================================*/
    /* conf path */
    /*=================================================*/

    char conf[MAX_PATH];

    wsprintfA(
        conf,
        "%s%s_%04X.conf",
        CONF_DIR,
        name,
        crc
    );

    /*=================================================*/
    /* shortcut path */
    /*=================================================*/

    char shortcut[MAX_PATH];

    build_unique_shortcut(
        desktop,
        name,
        shortcut
    );

    /*=================================================*/
    /* COM */
/*=================================================*/

    /* garante que CONF dir e gameicons existem */
    CreateDirectoryA(CONF_DIR, NULL);
    CreateDirectoryA(ICON_DIR, NULL);

    /* copia config padrao se nao existir (NTVDBM/WINBOX precisam dela) */
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


    CoInitialize(NULL);

    IShellLinkA *psl;

    HRESULT hr=
        CoCreateInstance(
            &CLSID_ShellLink,
            NULL,
            CLSCTX_INPROC_SERVER,
            &IID_IShellLinkA,
            (LPVOID*)&psl
        );

    if(SUCCEEDED(hr))
    {
        int type = exe_type(full);

        /*=================================================*/
        /* DOS -> NTVDBM */
        /*=================================================*/

        if(type == TYPE_DOS || is_com(full))
        {
            psl->lpVtbl->SetPath(
                psl,
                NTVDBM_PATH
            );

            char args[4096];

            _snprintf(
                args,
                sizeof(args),
                "-conf \"%s\" "
                "-exit "
                "-c \"mount c '%s'\" "
                "-c \"c:\" "
                "-c \"%s\" "
                "-c \"exit\"",
                conf,
                workdir,
                PathFindFileNameA(full)
            );

            psl->lpVtbl->SetArguments(
                psl,
                args
            );

            psl->lpVtbl->SetWorkingDirectory(
                psl,
                workdir
            );

            /* usa default-dos.ico */
            {
                char iconpath[MAX_PATH];
                build_icon_path(full, crc, iconpath);
                psl->lpVtbl->SetIconLocation(
                    psl,
                    iconpath,
                    0
                );
            }
        }
        else

        /*=================================================*/
        /* WIN16 -> WINBOX */
        /*=================================================*/

        if(type == TYPE_WIN16)
        {
            psl->lpVtbl->SetPath(
                psl,
                WINBOX_PATH
            );

            char args[4096];

            _snprintf(
                args,
                sizeof(args),
                "-conf \"%s\" "
                "-exit "
                "-c \"mount c '%s'\" "
                "-c \"mount w '%s'\" "
                "-c \"w:\" "
                "-c \"win\"",
                conf,
                workdir,
                WINBOX_DIR
            );

            psl->lpVtbl->SetArguments(
                psl,
                args
            );

            psl->lpVtbl->SetWorkingDirectory(
                psl,
                workdir
            );

            /* usa default-win16.ico */
            {
                char iconpath[MAX_PATH];
                build_icon_path(full, crc, iconpath);
                psl->lpVtbl->SetIconLocation(
                    psl,
                    iconpath,
                    0
                );
            }
        }
        else

        /*=================================================*/
        /* WIN32 -> atalho direto para o executavel */
        /* (handler do registro ja redireciona via HDL) */
        /*=================================================*/

        {
            psl->lpVtbl->SetPath(
                psl,
                full
            );

            /* sem argumentos extras */

            psl->lpVtbl->SetArguments(
                psl,
                ""
            );

            psl->lpVtbl->SetWorkingDirectory(
                psl,
                workdir
            );

            /*=================================================*/
            /* ICON */
            /*=================================================*/

            if(has_icon(full))
            {
                psl->lpVtbl->SetIconLocation(
                    psl,
                    full,
                    0
                );
            }
            else
            {
                char iconpath[MAX_PATH];

                /* tenta .ico local */

                if(find_local_icon(full, iconpath))
                {
                    psl->lpVtbl->SetIconLocation(
                        psl,
                        iconpath,
                        0
                    );
                }
                else
                {
                    /* fallback CRC */

                    build_icon_path(
                        full,
                        crc,
                        iconpath
                    );

                    psl->lpVtbl->SetIconLocation(
                        psl,
                        iconpath,
                        0
                    );
                }
            }
        }

        /*=================================================*/
        /* save */
        /*=================================================*/

        IPersistFile *ppf;

        hr=
            psl->lpVtbl->QueryInterface(
                psl,
                &IID_IPersistFile,
                (LPVOID*)&ppf
            );

        if(SUCCEEDED(hr))
        {
            WCHAR wsz[MAX_PATH];

            MultiByteToWideChar(
                CP_ACP,
                0,
                shortcut,
                -1,
                wsz,
                MAX_PATH
            );

            ppf->lpVtbl->Save(
                ppf,
                wsz,
                TRUE
            );

            ppf->lpVtbl->Release(ppf);
        }

        psl->lpVtbl->Release(psl);
    }

    CoUninitialize();

    return 0;
}