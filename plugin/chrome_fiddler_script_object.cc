#include "chrome_fiddler_script_object.h"

#include <stdlib.h>
#include <string.h>
#ifdef _WINDOWS
#include <atlenc.h>
#include <GdiPlus.h>
#include <io.h>
#include <ShlObj.h>
#elif defined GTK
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>
#elif defined __APPLE__
#include <libgen.h>
#include <resolv.h>
#endif

#include <string>

#include "log.h"
#include "chrome_fiddler_plugin.h"
#include "utils.h"

#ifdef _WINDOWS
using namespace Gdiplus;
#define snprintf sprintf_s
struct BrowserParam {
    TCHAR initial_path[MAX_PATH];
    TCHAR title[MAX_PATH];
};
#else
#define MAX_PATH 260
#endif

extern Log g_logger;

NPObject* ChromeFiddlerScriptObject::Allocate(NPP npp, NPClass *aClass) {
    g_logger.WriteLog("msg", "ChromeFiddlerScriptObject Allocate");
    ChromeFiddlerScriptObject* script_object = new ChromeFiddlerScriptObject;
    if (script_object != NULL)
        script_object->set_plugin((PluginBase*)npp->pdata);
    return script_object;
}

void ChromeFiddlerScriptObject::Deallocate() {
    g_logger.WriteLog("msg", "ChromeFiddlerScriptObject Deallocate");
    delete this;
}

void ChromeFiddlerScriptObject::InitHandler() {
    FunctionItem item;
    item.function_name = "OpenFileDialog";
    item.function_pointer = ON_INVOKEHELPER(&ChromeFiddlerScriptObject::OpenFileDialog);
    AddFunction(item);
}

#if defined __APPLE__
std::string GetFilePath(const char* path, const char* dialog_title, std::string);
#endif

bool ChromeFiddlerScriptObject::OpenFileDialog(const NPVariant* args, uint32_t argCount, NPVariant* result) {
    if (argCount < 2 || !NPVARIANT_IS_STRING(args[0]) || !NPVARIANT_IS_STRING(args[1]))
        return false;

    std::string path(NPVARIANT_TO_STRING(args[0]).UTF8Characters, NPVARIANT_TO_STRING(args[0]).UTF8Length);
    std::string option(NPVARIANT_TO_STRING(args[1]).UTF8Characters, NPVARIANT_TO_STRING(args[1]).UTF8Length);
    std::string dialog_title = "select something, please";

#if _WINDOWS
    TCHAR display_name[MAX_PATH] = {0};
    BrowserParam param = {0};
    BOOL bRet;

    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, param.initial_path, MAX_PATH);
    MultiByteToWideChar(CP_UTF8, 0, dialog_title.c_str(), -1, param.title, MAX_PATH);

    if (option == "file") {
        OPENFILENAME ofn = {0};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = get_plugin()->get_native_window();
        ofn.lpstrFilter = _T("All Files(*.*)\0*.*\0");
        ofn.lpstrInitialDir = param.initial_path;
        ofn.lpstrFile = display_name;
        ofn.nMaxFile = sizeof(display_name) / sizeof(*display_name);
        ofn.nFilterIndex = 0;
        ofn.lpstrTitle = param.title;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

        bRet = GetOpenFileName(&ofn);
    } else {
        BROWSEINFO info={0};
        info.hwndOwner = get_plugin()->get_native_window();
        info.lpszTitle = param.title;
        info.pszDisplayName = display_name;
        info.lpfn = NULL;
        info.ulFlags = BIF_RETURNONLYFSDIRS;
        info.lParam = (LPARAM)&param;

        bRet = SHGetPathFromIDList(SHBrowseForFolder(&info), display_name);
    }

    char utf8[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, bRet ? display_name : param.initial_path, -1, utf8, MAX_PATH, 0, 0);
    size_t length = strlen(utf8);

#elif defined __APPLE__
    std::string pathStr = GetFilePath(path.c_str(), dialog_title.c_str(), option);
    const char* utf8 = pathStr.c_str();
    size_t length = pathStr.length();

#endif

    char* copy = (char *)NPN_MemAlloc(length + 1);
    memcpy(copy, utf8, length);
    copy[length] = 0;
    STRINGN_TO_NPVARIANT(copy, length, *result);

    return true;
}
