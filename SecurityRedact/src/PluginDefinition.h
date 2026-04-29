//
// SecurityRedact – Notepad++ Plugin
// Schwärzt Keys, Tokens, Passwörter und sicherheitsrelevante Daten
//
#pragma once
#include "PluginInterface.h"

// ── STEP 1: Plugin-Name ───────────────────────────────────────────────────────
const TCHAR NPP_PLUGIN_NAME[] = TEXT("SecurityRedact");

// ── STEP 2: Anzahl der Menü-Befehle ──────────────────────────────────────────
const int nbFunc = 4;

// ── Lifecycle ─────────────────────────────────────────────────────────────────
void pluginInit(HANDLE hModule);
void pluginCleanUp();
void commandMenuInit();
void commandMenuCleanUp();

// ── Hilfsfunktion (aus Template) ──────────────────────────────────────────────
bool setCommand(size_t index, TCHAR* cmdName, PFUNCPLUGINCMD pFunc,
                ShortcutKey* sk = nullptr, bool check0nInit = false);

// ── STEP 4: Plugin-Funktionen (Menü-Callbacks) ───────────────────────────────
void cmd_RedactDocument();    // Strg+Shift+R  – gesamtes Dokument
void cmd_RedactSelection();   // Strg+Shift+E  – nur Auswahl
void cmd_MenuSeparator();     // Trennlinie
void cmd_About();             // Über-Dialog
