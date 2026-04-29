# SecurityRedact – Notepad++ Plugin

Schwärzt API-Keys, Tokens, Passwörter und sicherheitsrelevante Daten
direkt in Notepad++ – auf Knopfdruck, mit Undo-Support.

---

## Projektstruktur

    SecurityRedact/
    ├── src/
    │   ├── SecurityRedact.cpp       ← DLL-Einstiegspunkt (DllMain + NPP-Exports)
    │   ├── PluginDefinition.h       ← Plugin-Name, Befehlsanzahl, Deklarationen
    │   ├── PluginDefinition.cpp     ← Gesamte Logik: Patterns, Redaction, UI
    │   ├── PluginInterface.h        ← Offizielle NPP-Strukturen (FuncItem etc.)
    │   ├── Notepad_plus_msgs.h      ← Offizielle NPP-Messages
    │   ├── Scintilla.h              ← Offizielle Scintilla-API
    │   ├── Sci_Position.h           ← Scintilla-Position-Typen
    │   └── menuCmdID.h              ← NPP-Menü-Kommando-IDs
    └── vs.proj/
        └── SecurityRedact.vcxproj  ← Visual Studio 2022 Projektdatei


---

## Build-Anleitung

### Voraussetzungen
- **Visual Studio 2022** (Community-Edition reicht)
  https://visualstudio.microsoft.com/de/downloads/
  → Workload: „Desktopentwicklung mit C++" auswählen

### Schritte

1. `vs.proj\SecurityRedact.vcxproj` in Visual Studio öffnen

2. Konfiguration wählen:
   - **Release | x64**  → für normale 64-bit Notepad++-Installation  ← empfohlen
   - **Release | Win32** → nur für ältere 32-bit Notepad++-Version

3. **Build → Build Solution** (F7)

4. Der PostBuild-Schritt kopiert die DLL automatisch nach:
       C:\Program Files\Notepad++\plugins\SecurityRedact\SecurityRedact.dll
   
   Falls der Schritt wegen fehlender Rechte scheitert:
   Visual Studio als Administrator starten (Rechtsklick → Als Administrator ausführen)

5. Notepad++ neu starten → Menü **Plugins → SecurityRedact** erscheint


---

## Installation (manuell, ohne Build)

Falls du eine fertig kompilierte DLL hast:

    C:\Program Files\Notepad++\plugins\SecurityRedact\SecurityRedact.dll

Wichtig: Der Unterordner muss exakt so heißen wie die DLL (ohne .dll).

Danach:
- Rechtsklick auf die DLL → Eigenschaften → „Zulassen" (Unblock) aktivieren
- Notepad++ neu starten


---

## Benutzung

| Aktion | Weg |
|---|---|
| Gesamtes Dokument schwärzen | Plugins → SecurityRedact → Dokument schwärzen |
| | oder **Ctrl+Shift+R** |
| Nur Auswahl schwärzen | Plugins → SecurityRedact → Auswahl schwärzen |
| | oder **Ctrl+Shift+E** |

Der Plugin zeigt vor dem Ändern eine Vorschau der Funde und fragt zur Bestätigung.
Alle Änderungen sind per **Strg+Z** rückgängig machbar.


---

## Erkannte Muster

| Kategorie | Beispiele |
|---|---|
| AWS | Access Key ID (AKIA…), Secret Access Key |
| Azure | Storage Account Key, SAS Token |
| Google | API Key (AIza…) |
| GitHub / GitLab | ghp_…, glpat-… |
| Slack | xoxb-… |
| Stripe | sk_live_…, pk_test_… |
| Twilio | Account SID, Auth Token |
| SendGrid | SG.… |
| Mailgun | key-… |
| Heroku | UUID-Format API Key |
| JWT | eyJ…eyJ…signature |
| PEM | -----BEGIN PRIVATE KEY----- … |
| Datenbanken | mysql://user:PASS@host |
| Generisch | password=, api_key=, token=, secret=, … |


---

## Quellen / Referenzen

- Notepad++ Plugin Template: https://github.com/npp-plugins/plugintemplate
- Offizielle Plugin-Dokumentation: https://npp-user-manual.org/docs/plugins/
- Scintilla API: https://www.scintilla.org/ScintillaDoc.html
- Plugin Communication: https://npp-user-manual.org/docs/plugin-communication/
