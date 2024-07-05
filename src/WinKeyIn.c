/**
 * \file WinKeyIn.c
 *
 * Programme de "frappe" automatique de touches du clavier sous Windows NT.
 */

#define _WIN32_WINNT  0x0500
        /* programme conçu pour Windows 2000 ou ultérieur */

#include <assert.h>
#include <stdio.h>

#include <windows.h>
#include <shellapi.h>

#include "resource.h"


/*========================================================================*/
/*                               CONSTANTES                               */
/*========================================================================*/

/**
 * \def VERSION_PRG
 *
 * Version courante du programme WinKeyIn.
 */
#define VERSION_PRG  L"0.7.7 du 03/07/2024"

/* == Messages affichés == */

static const WCHAR* TITRE_PRG =
		L"Frappes clavier automatiques Windows NT" ;
static const WCHAR* FMT_MSG_A_PROPOS =
		L"WinKeyIn version %ls." ;
static const WCHAR* FMT_MSG_ECHEC_FNCT =
		L"Échec de %ls : erreur no. %lu !" ;
static const WCHAR* ENTETE_MSG_ERR_SYS =
		L"\n Message d'erreur système : " ;


/* == Autres constantes == */

/* Nom de classe de fenêtre (ne pas traduire !) */
static const WCHAR* CLASSE_FENETRE = L"WinKeyIn" ;

/* taille maximale d'un message affiché par ce programme */
static const unsigned TAILLE_MAX_MSG = 4096U ;

/* taille de la fenêtre */
static const int LARGEUR_FENETRE = 320 ;
static const int HAUTEUR_FENETRE = 200 ;

/* identifiant du "timer" de déplacement de la souris */
static const UINT_PTR ID_EVNT_TIMER_CLAVIER = 0x5059454B ;
                                              /* chaîne 'KEYP' en hexa */
/* délai entre deux séquences de frappes de touches (en millisecondes) */
static const UINT DELAI_EVNTS_CLAVIER = 1000U ;


/* liste des touches à "déclencher" ('virtual key codes') */
static const WORD VIRTKEYS[] = {
		VK_ATTN   /* la touche 'Pause' est pressée puis immédiatement
		             relâchée, ce qui ne devrait avoir aucun effet visible */
} ;


/*========================================================================*/
/*                           VARIABLES GLOBALES                           */
/*========================================================================*/

/* "handle" de la fenêtre principale de ce programme */
static HWND main_window = NULL ;

/* "flag" indiquant qu'un traitement d'erreur est en cours */
static volatile BOOL in_error = FALSE;


/*========================================================================*/
/*                               FONCTIONS                                */
/*========================================================================*/

/* === FONCTIONS UTILITAIRES "PRIVEES" === */

static DWORD
MsgErreurSys (const WCHAR* nomFnct)
{
	DWORD codeErr = GetLastError () ;
	if (codeErr == 0) return 0 ;
	WCHAR msgErr[TAILLE_MAX_MSG] ;
	swprintf (msgErr, TAILLE_MAX_MSG,
	          FMT_MSG_ECHEC_FNCT, nomFnct, codeErr) ;
	/* essaie d'obtenir une description de l'erreur en question */
	LPWSTR ptrMsgSys;
	DWORD res = FormatMessageW (FORMAT_MESSAGE_ALLOCATE_BUFFER
	                            | FORMAT_MESSAGE_FROM_SYSTEM,
	                            NULL,
	                            codeErr,
	                            MAKELANGID(LANG_SYSTEM_DEFAULT,
	                                       SUBLANG_SYS_DEFAULT),
	                            (LPWSTR)&ptrMsgSys,
	                            0,
	                            NULL) ;
	if (res != 0) {
		wcscat (msgErr, ENTETE_MSG_ERR_SYS) ;
		wcsncat (msgErr, ptrMsgSys,
		         TAILLE_MAX_MSG - wcslen(msgErr)) ;
		LocalFree (ptrMsgSys) ;
	}
	/* ne pas multiplier les boîtes de message d'erreurs */
	if (!in_error) {
		in_error = TRUE ;
		MessageBoxW (main_window,
		             msgErr,
		             TITRE_PRG,
		             MB_ICONERROR | MB_SETFOREGROUND) ;
		in_error = FALSE ;
	}
	/* renvoie le code d'erreur système utilisé */
	return codeErr ;
}


/* === FONCTIONS "PUBLIQUES" === */

/* fonction appelée par le "timer" des "frappes" clavier */
VOID CALLBACK
KeyTimerProc (HWND hwnd,
              UINT uMsg,
              UINT_PTR idEvent,
              DWORD dwTime)
{
	/* paramètres inutilisés */
	(void) hwnd;   (void) uMsg;   (void) dwTime;

	/* vérifie que l'on est bien le destinataire du message */
	if (idEvent != ID_EVNT_TIMER_CLAVIER) return ;

	/* évènements clavier à générer */
	unsigned nbEvnts = 2 * ARRAYSIZE(VIRTKEYS);
	INPUT inpMsgs[nbEvnts];
	ZeroMemory (inpMsgs, sizeof(inpMsgs)) ;

	/* préparation de l'entrée pour le système */
	for (unsigned n = 0; n < ARRAYSIZE(VIRTKEYS); n++) {
		/* pression de la touche */
		inpMsgs[n].type = INPUT_KEYBOARD ;
		inpMsgs[n].ki.wVk = VIRTKEYS[n] ;
		inpMsgs[n].ki.dwFlags = 0 ;
		/* relâchement de la touche */
		unsigned rel = nbEvnts - 1U - n ;
		inpMsgs[rel].type = INPUT_KEYBOARD ;
		inpMsgs[rel].ki.wVk = VIRTKEYS[n] ;
		inpMsgs[rel].ki.dwFlags = KEYEVENTF_KEYUP ;
	}

	/* génère les évènements clavier voulus */
	UINT inpSent = SendInput (nbEvnts, inpMsgs, sizeof(INPUT)) ;
	if (inpSent < nbEvnts) {
		MsgErreurSys (L"SendInput()") ;
		return ;
	}
}

/* fonction de gestion des messages de la fenêtre principale du programme */
LRESULT CALLBACK
MainWndProc (HWND hwnd,
             UINT message,
             WPARAM wParam,
             LPARAM lParam)
{
	static HFONT hFont ;
	static HPEN hPen ;
	PAINTSTRUCT ps ;
	HDC hDC ;
	RECT clientRect ;
	WCHAR txt[MAX_PATH] ;

	switch (message) {
	/* création de la fenêtre => objets de dessin utiles */
	case WM_CREATE:
		hFont = GetStockObject (SYSTEM_FONT) ;
		hPen = GetStockObject (BLACK_PEN) ;

		return 0;

	/* dessine le contenu de la fenêtre */
	case WM_PAINT:
		hDC = BeginPaint (hwnd, &ps) ;
		SelectObject (hDC, hFont) ;
		SelectObject (hDC, hPen) ;
		GetClientRect (hwnd, &clientRect) ;

		swprintf (txt, MAX_PATH, FMT_MSG_A_PROPOS, VERSION_PRG) ;
		DrawTextW (hDC,
		           txt,
		           -1,
		           &clientRect,
		           DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX) ;

		EndPaint (hwnd, &ps) ;

		return 0 ;

	/* ferme la fenêtre => quitte le programme */
	case WM_DESTROY:
		PostQuitMessage (0) ;
		return 0 ;
	}

	/* autres messages => traitement par défaut */
	return DefWindowProcW (hwnd, message, wParam, lParam) ;
}


/* === POINT D'ENTREE === */

int WINAPI
WinMain (HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPSTR lpCmdLine,
         int nShowCmd)
{
	/* paramètres inutiles */
	(void) hPrevInstance ;   (void) lpCmdLine;

	/* définition de la classe de notre fenêtre */
	WNDCLASSEXW wndClass ;
	wndClass.cbSize        = sizeof(WNDCLASSEXW) ;
	wndClass.style         = CS_HREDRAW | CS_VREDRAW ;
	wndClass.lpfnWndProc   = MainWndProc ;
	wndClass.cbClsExtra    = 0 ;
	wndClass.cbWndExtra    = 0 ;
	wndClass.hInstance     = hInstance ;
	wndClass.hIcon         = LoadIconW (hInstance,
	                                    MAKEINTRESOURCEW (ID_ICO_WIN_KEY_IN)) ;
	wndClass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wndClass.lpszMenuName  = NULL ;
	wndClass.lpszClassName = CLASSE_FENETRE ;
	wndClass.hIconSm       = LoadIconW (hInstance,
	                                    MAKEINTRESOURCEW (ID_ICO_WIN_KEY_IN)) ;

	if (!RegisterClassExW (&wndClass)) {
		MsgErreurSys (L"RegisterClass()") ;
		return -1 ;
	}

	/* créé la fenêtre principale (et unique) de notre application */
	main_window = CreateWindowExW (WS_EX_APPWINDOW,
	                               CLASSE_FENETRE,
	                               TITRE_PRG,
	                               WS_OVERLAPPEDWINDOW,
	                               CW_USEDEFAULT,
	                               CW_USEDEFAULT,
	                               LARGEUR_FENETRE,
	                               HAUTEUR_FENETRE,
	                               NULL,
	                               NULL,
	                               hInstance,
	                               NULL) ;
	if (main_window == NULL) {
		MsgErreurSys (L"CreateWindowEx()") ;
		return -1 ;
	}

	/* affiche la fenêtre nouvellement créée */
	ShowWindow (main_window, nShowCmd) ;
	BOOL ok = UpdateWindow (main_window) ;
	if (!ok) {
		MsgErreurSys (L"UpdateWindow()") ;
		return -1 ;
	}

	/* créé une icône pour la zone de notification */
	NOTIFYICONDATAW nid;
	ZeroMemory (&nid, sizeof(NOTIFYICONDATAW)) ;
	nid.cbSize = NOTIFYICONDATAW_V2_SIZE ;   /* valeur pour Windows 2000 */
	nid.hWnd   = main_window ;
	nid.uID    = ID_ICO_WIN_KEY_IN ;
	nid.uFlags = NIF_ICON | NIF_TIP ;
	nid.hIcon  = wndClass.hIconSm ;
	wcsncpy (nid.szTip, TITRE_PRG, 128) ;

	/* ajoute l'icône à la zone de notification */
	Shell_NotifyIconW (NIM_ADD, &nid) ;   /* inutile de réagir en cas d'échec */

	/* création du "timer" générant les évènements de frappe au clavier */
	UINT_PTR idTmr = SetTimer (main_window,
	                           ID_EVNT_TIMER_CLAVIER,
	                           DELAI_EVNTS_CLAVIER,
	                           KeyTimerProc) ;
	if (idTmr == 0U) {
		MsgErreurSys (L"SetTimer()") ;
		return -1 ;
	}

	/* boucle de réception des messages destinés à notre programme */
	MSG msg;
	int res = GetMessageW (&msg, NULL, 0, 0) ;
	while (res > 0) {
		/* traite le message courant */
		TranslateMessage (&msg) ;
		DispatchMessageW (&msg) ;

		/* attend le message suivant */
		res = GetMessageW (&msg, NULL, 0, 0) ;
	}

	/* arrête et supprime le "timer" créé */
	KillTimer (main_window, ID_EVNT_TIMER_CLAVIER) ;

	/* teste si une erreur est survenue */
	if (res == -1) {
		MsgErreurSys (L"GetMessage()") ;
	}

	Shell_NotifyIconW (NIM_DELETE, &nid) ;

	/* programme terminé */
	return (int) msg.wParam ;
}


