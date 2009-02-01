/*   PictureView wrapper  for ConEmu   */
/*  Copyright (c) Maxim Moysyuk, 2009  */

#include <windows.h>
#include <TCHAR.H>
#include "PicViewWrapper.h"

/* FAR */
PluginStartupInfo psi;
FarStandardFunctions fsf;
FWrapperAdvControl fWrapperAdvControl=NULL;

/* PictureView */
HMODULE hPicViewDll=NULL;
FClosePlugin fClosePlugin=NULL;
FConfigure fConfigure=NULL;
FExitFAR fExitFAR=NULL;
FGetMinFarVersion fGetMinFarVersion=NULL;
FGetPluginInfo fGetPluginInfo=NULL;
FOpenFilePlugin fOpenFilePlugin=NULL;
FOpenPlugin fOpenPlugin=NULL;
FProcessEditorEvent fProcessEditorEvent=NULL;
FProcessViewerEvent fProcessViewerEvent=NULL;
FSetStartupInfo fSetStartupInfo=NULL;

/* Local */
HINSTANCE ghInstance = NULL;
HWND ghConEmu = NULL;
TCHAR gsTermMsg[255];

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		{
			// Init variables
			ghInstance = (HINSTANCE)hModule;
			gsTermMsg[0] = 0;

			#ifdef _DEBUG
			if (!IsDebuggerPresent())
				MessageBox(0,_T("Process Attach"), _T("PictureView wrapper"), MB_SETFOREGROUND);
			#endif

			// Check ConEmu instance
			TCHAR szVarValue[MAX_PATH];
			if (GetEnvironmentVariable(_T("ConEmuHWND"), szVarValue, MAX_PATH)) {
				if (szVarValue[0]==_T('0') && szVarValue[1]==_T('x')) {
					ghConEmu = AtoH(szVarValue+2, 8);
					if (ghConEmu) {
						TCHAR className[100];
						GetClassName(ghConEmu, (LPTSTR)className, 100);
						if (lstrcmpi(className, _T("VirtualConsoleClass")) != 0)
						{
							ghConEmu = NULL;
						}
					}
				}
			}

			#ifdef _DEBUG
			lstrcpy(szVarValue, _T("VTNT"));
			#endif
			szVarValue[0] = 0;
			if (GetEnvironmentVariable(_T("TERM"), szVarValue, 63)) {
				lstrcpy(gsTermMsg, _T("PictureView wrapper\nPicture viewing is not available\nin terminal mode ("));
				lstrcat(gsTermMsg, szVarValue);
				lstrcat(gsTermMsg, _T(")"));
			}


			// Init PictureView plugin
			if (!hPicViewDll) {
				TCHAR szModulePath[MAX_PATH+1];
				int nLen = 0;
				if ((nLen=GetModuleFileName(ghInstance, szModulePath, MAX_PATH))>0) {
					if (szModulePath[nLen-1]!=_T('L') && szModulePath[nLen-1]!=_T('l'))
						return FALSE;
					szModulePath[nLen-1] = '_';
					hPicViewDll = LoadLibrary(szModulePath);
					if (!hPicViewDll)
						return FALSE;
				}
			}
			fClosePlugin=(FClosePlugin)GetProcAddress(hPicViewDll,"ClosePlugin");
			fConfigure=(FConfigure)GetProcAddress(hPicViewDll,"Configure");
			fExitFAR=(FExitFAR)GetProcAddress(hPicViewDll,"ExitFAR");
			fGetMinFarVersion=(FGetMinFarVersion)GetProcAddress(hPicViewDll,"GetMinFarVersion");
			fGetPluginInfo=(FGetPluginInfo)GetProcAddress(hPicViewDll,"GetPluginInfo");
			fOpenFilePlugin=(FOpenFilePlugin)GetProcAddress(hPicViewDll,"OpenFilePlugin");
			fOpenPlugin=(FOpenPlugin)GetProcAddress(hPicViewDll,"OpenPlugin");
			fProcessEditorEvent=(FProcessEditorEvent)GetProcAddress(hPicViewDll,"ProcessEditorEvent");
			fProcessViewerEvent=(FProcessViewerEvent)GetProcAddress(hPicViewDll,"ProcessViewerEvent");
			fSetStartupInfo=(FSetStartupInfo)GetProcAddress(hPicViewDll,"SetStartupInfo");
		} break;
	case DLL_PROCESS_DETACH:
		{
			if (hPicViewDll) {
				FreeLibrary(hPicViewDll);
				hPicViewDll = NULL;
			}
		} break;
	}
    return TRUE;
}


void WINAPI ClosePlugin(
  HANDLE hPlugin
)
{
	if (hPicViewDll && fClosePlugin)
		fClosePlugin(hPlugin);
}

int WINAPI Configure(
  int ItemNumber
)
{
	if (hPicViewDll && fConfigure)
		return fConfigure(ItemNumber);
	return FALSE;
}

void WINAPI ExitFAR( void )
{
	if (hPicViewDll && fExitFAR)
		fExitFAR();
}

int WINAPI GetMinFarVersion(void)
{
	if (hPicViewDll && fGetMinFarVersion)
		return fGetMinFarVersion();
	return MAKEFARVERSION(1,71,2470);
}

void WINAPI GetPluginInfo(
  struct PluginInfo *Info
)
{
	if (hPicViewDll && fGetPluginInfo)
		fGetPluginInfo(Info);
}

HANDLE WINAPI OpenFilePlugin(
  char *Name,
  const unsigned char *Data,
  int DataSize
)
{
	HANDLE hRc = INVALID_HANDLE_VALUE;
	if (hPicViewDll && fOpenFilePlugin) {
		if (gsTermMsg[0]) {
			TCHAR *pszExt = NULL;
			// PicView ����� �������������� � �� Enter/CtrlPgDn, ����� �������� ������ �� "���������" �����, ��������� ������ ������������
			if (Name && *Name) {
				for (pszExt=Name+lstrlen(Name)-1; pszExt>=Name && *pszExt!=_T('\\'); pszExt--) {
					if (*pszExt==_T('.')) {
						pszExt++;
						if (lstrcmpi(pszExt, _T("jpg")) && lstrcmpi(pszExt, _T("jpeg")) && lstrcmpi(pszExt, _T("jfif")) &&
							lstrcmpi(pszExt, _T("tif")) && lstrcmpi(pszExt, _T("tiff")) &&
							lstrcmpi(pszExt, _T("bmp")) && lstrcmpi(pszExt, _T("dib")) &&
							lstrcmpi(pszExt, _T("wmf")) && lstrcmpi(pszExt, _T("emf")) &&
							lstrcmpi(pszExt, _T("pcx")) &&
							lstrcmpi(pszExt, _T("gif")) &&
							1)
							pszExt=NULL;
						break;
					}
				}

			}
			if (pszExt) {
				psi.Message(psi.ModuleNumber, FMSG_MB_OK|FMSG_ALLINONE, NULL, 
					(const TCHAR *const *)gsTermMsg, 0, 0);
			}
		} else {
			hRc = fOpenFilePlugin(Name, Data, DataSize);
		}
	}
	return hRc;
}

HANDLE WINAPI OpenPlugin(
  int OpenFrom,
  INT_PTR Item
)
{
	HANDLE hRc = INVALID_HANDLE_VALUE;
	if (hPicViewDll && fOpenPlugin) {
		if (gsTermMsg[0])
			psi.Message(psi.ModuleNumber, FMSG_MB_OK|FMSG_ALLINONE, NULL, 
				(const TCHAR *const *)gsTermMsg, 0, 0);
		else
			hRc = fOpenPlugin(OpenFrom, Item);
	}
	return hRc;
}

int WINAPI ProcessEditorEvent(
  int Event,
  void *Param
)
{
	if (hPicViewDll && fProcessEditorEvent)
		return fProcessEditorEvent(Event, Param);
	return FALSE;
}

int WINAPI ProcessViewerEvent(
  int Event,
  void *Param
)
{
	if (hPicViewDll && fProcessViewerEvent)
		return fProcessViewerEvent(Event, Param);
	return FALSE;
}

void WINAPI SetStartupInfo(
  const struct PluginStartupInfo *Info
)
{
	::psi = *Info;
	::fsf = *(Info->FSF);

	fWrapperAdvControl = psi.AdvControl;
	psi.AdvControl = WrapperAdvControl;

	psi.FSF = &fsf;

	if (hPicViewDll && fSetStartupInfo)
		fSetStartupInfo(&psi);
}

INT_PTR WINAPI WrapperAdvControl(int ModuleNumber,int Command,void *Param)
{
	if (!fWrapperAdvControl)
		return 0;

	if (Command==ACTL_GETFARHWND && ghConEmu) {
	    HWND hFarWnd = (HWND)fWrapperAdvControl(ModuleNumber, ACTL_GETFARHWND, NULL);
	    if (IsWindowVisible(hFarWnd))
		    return (INT_PTR)hFarWnd;
		    
		if (IsWindow(ghConEmu)) {
			return (INT_PTR)ghConEmu;
		} else {
			return (INT_PTR)hFarWnd;
		}
	}

	return fWrapperAdvControl(ModuleNumber, Command, Param);
}

HWND AtoH(TCHAR *Str, int Len)
{
	DWORD Ret=0;
	for (; Len && *Str; Len--, Str++)
	{
		if (*Str>=_T('0') && *Str<=_T('9'))
			(Ret*=16)+=*Str-_T('0');
		else if (*Str>=_T('a') && *Str<=_T('f'))
			(Ret*=16)+=(*Str-_T('a')+10);
		else if (*Str>=_T('A') && *Str<=_T('F'))
			(Ret*=16)+=(*Str-_T('A')+10);
	}
	return (HWND)Ret;
}
