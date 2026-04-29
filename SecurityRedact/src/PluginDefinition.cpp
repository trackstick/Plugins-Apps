//
// SecurityRedact – PluginDefinition.cpp
// Gesamte Plugin-Logik: Muster, Erkennung, Schwärzung, UI
//
#include "PluginDefinition.h"
#include "menuCmdID.h"
#include <string>
#include <vector>
#include <regex>
#include <algorithm>

// ── Plugin-Daten (extern, werden in SecurityRedact.cpp deklariert) ─────────────
FuncItem funcItem[nbFunc];
NppData  nppData;

// ── Konstanten ────────────────────────────────────────────────────────────────
static const char* REDACTED_TAG = "[REDACTED]";

// ── Shortcut-Definitionen ─────────────────────────────────────────────────────
static ShortcutKey sk_all = { true, false, true, 'R' };  // Ctrl+Shift+R
static ShortcutKey sk_sel = { true, false, true, 'E' };  // Ctrl+Shift+E

// ============================================================================
// Regex-Pattern-Definitionen
// Jedes Pattern: { Anzeigename, Regex-String, Capture-Group (0=ganzer Match) }
// ============================================================================
struct PatternDef {
    const char* name;
    const char* pattern;
    int         valueGroup; // welche Gruppe enthält den Geheimwert
};

static const PatternDef PATTERN_DEFS[] = {

    // ── Cloud-Provider ────────────────────────────────────────────────────────
    { "AWS Access Key ID",
      R"((AKIA[0-9A-Z]{16}))", 0 },

    { "AWS Secret Access Key",
      R"((?:aws[_\-]?secret[_\-]?(?:access[_\-]?)?key\s*[=:]\s*["']?)([A-Za-z0-9/+=]{40}))", 1 },

    { "Azure Storage Account Key",
      R"((?:AccountKey|storageaccountkey)\s*[=:]\s*([A-Za-z0-9+/=]{60,}))", 1 },

    { "Azure SAS Token",
      R"((?:sig=)([A-Za-z0-9%+/=]{20,}))", 1 },

    { "Google API Key",
      R"((AIza[0-9A-Za-z\-_]{35}))", 0 },

    // ── Versionskontrolle & CI ────────────────────────────────────────────────
    { "GitHub Token",
      R"((gh[pousr]_[A-Za-z0-9]{36,}))", 0 },

    { "GitLab Token (glpat)",
      R"((glpat-[A-Za-z0-9\-_]{20,}))", 0 },

    // ── Kommunikations-Dienste ────────────────────────────────────────────────
    { "Slack Token",
      R"((xox[baprs]-[0-9A-Za-z\-]{10,}))", 0 },

    { "Twilio Account SID",
      R"((AC[a-f0-9]{32}))", 0 },

    { "Twilio Auth Token",
      R"((?:twilio[_\-\s]?(?:auth[_\-\s]?)?token\s*[=:]\s*["']?)([a-f0-9]{32}))", 1 },

    // ── Payment & E-Mail ──────────────────────────────────────────────────────
    { "Stripe Secret Key",
      R"((sk_(?:live|test)_[0-9A-Za-z]{24,}))", 0 },

    { "Stripe Publishable Key",
      R"((pk_(?:live|test)_[0-9A-Za-z]{24,}))", 0 },

    { "SendGrid API Key",
      R"((SG\.[A-Za-z0-9\-_]{22}\.[A-Za-z0-9\-_]{43}))", 0 },

    { "Mailgun API Key",
      R"((key-[0-9a-f]{32}))", 0 },

    // ── Plattform-Services ────────────────────────────────────────────────────
    { "Heroku API Key",
      R"((?:heroku[_\-\s]?(?:api[_\-\s]?)?key\s*[=:]\s*["']?)([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}))", 1 },

    // ── Kryptografie ─────────────────────────────────────────────────────────
    { "JWT Token",
      R"((eyJ[A-Za-z0-9\-_]+\.eyJ[A-Za-z0-9\-_]+\.[A-Za-z0-9\-_.+/=]*))", 0 },

    { "PEM Private Key",
      R"((-----BEGIN (?:RSA |EC |DSA |OPENSSH )?PRIVATE KEY-----[\s\S]+?-----END (?:RSA |EC |DSA |OPENSSH )?PRIVATE KEY-----))", 0 },

    // ── Datenbank-Verbindungsstrings ──────────────────────────────────────────
    { "DB Connection String Password",
      R"((?:jdbc:|mysql:|postgresql:|mongodb(?:\+srv)?:|redis://)(?:[^:@\s]+):([^@\s]+)@)", 1 },

    // ── Generische Key=Value-Muster ───────────────────────────────────────────
    { "Password",
      R"((?:password|passwd|pwd|pass)\s*[=:]\s*["']?([^\s"',;\r\n]{4,}))", 1 },

    { "API Key",
      R"((?:api[_\-]?key|apikey|api[_\-]?secret|app[_\-]?(?:key|secret))\s*[=:]\s*["']?([A-Za-z0-9\-_.+/=]{12,}))", 1 },

    { "Secret Key",
      R"((?:secret[_\-]?key|secretkey|client[_\-]?secret)\s*[=:]\s*["']?([A-Za-z0-9\-_.+/=]{8,}))", 1 },

    { "Token",
      R"((?:token|access[_\-]?token|auth[_\-]?token|bearer[_\-]?token|refresh[_\-]?token|id[_\-]?token)\s*[=:]\s*["']?([A-Za-z0-9\-_.+/=]{8,}))", 1 },

    { "Private Key",
      R"((?:private[_\-]?key|priv[_\-]?key)\s*[=:]\s*["']?([A-Za-z0-9\-_.+/=]{8,}))", 1 },

    { "Generic Secret (high-entropy)",
      R"((?:secret|credential|auth)\s*[=:"']\s*["']?([A-Za-z0-9+/=]{32,}))", 1 },
};

// ── Kompilierte Patterns (einmalig beim ersten Aufruf erstellt) ───────────────
struct CompiledPattern {
    std::string name;
    std::regex  rx;
    int         valueGroup;
};

static std::vector<CompiledPattern> g_patterns;

static void ensurePatternsCompiled()
{
    if (!g_patterns.empty()) return;
    for (auto& pd : PATTERN_DEFS) {
        try {
            g_patterns.push_back({
                pd.name,
                std::regex(pd.pattern, std::regex::ECMAScript | std::regex::icase),
                pd.valueGroup
            });
        }
        catch (...) { /* ungültiges Pattern überspringen */ }
    }
}

// ============================================================================
// Redaction-Engine
// ============================================================================
struct Finding {
    std::string typeName;
    std::string preview;   // maskierter Vorschau-Text
};

static std::string redactText(const std::string& input, std::vector<Finding>& findings)
{
    ensurePatternsCompiled();
    std::string text = input;

    for (auto& cp : g_patterns) {
        std::string result;
        result.reserve(text.size());
        size_t lastEnd = 0;

        auto begin = std::sregex_iterator(text.cbegin(), text.cend(), cp.rx);
        auto end   = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            auto& m = *it;

            // Text vor dem Match unverändert übernehmen
            result.append(text, lastEnd, (size_t)m.position() - lastEnd);

            if (cp.valueGroup == 0) {
                // Gesamter Match → REDACTED
                std::string raw = m.str();
                size_t plen = std::min(raw.size(), (size_t)6);
                findings.push_back({ cp.name,
                    raw.substr(0, plen) + std::string(std::min(raw.size() - plen, (size_t)6), '*') });
                result += REDACTED_TAG;
            }
            else {
                // Nur Capture-Group g ersetzen, Rest behalten
                int g = cp.valueGroup;
                std::string full  = m.str();
                std::string val   = m.str(g);
                size_t gStart = (size_t)(m.position(g) - m.position());
                size_t gLen   = val.size();

                size_t plen = std::min(val.size(), (size_t)6);
                findings.push_back({ cp.name,
                    val.substr(0, plen) + std::string(std::min(val.size() - plen, (size_t)6), '*') });

                result += full.substr(0, gStart);
                result += REDACTED_TAG;
                result += full.substr(gStart + gLen);
            }
            lastEnd = (size_t)(m.position() + m.length());
        }
        result.append(text, lastEnd, text.size() - lastEnd);
        text = std::move(result);
    }
    return text;
}

// ── Helper: aktuelles Scintilla-Handle ───────────────────────────────────────
static HWND currentScintilla()
{
    int which = 0;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    return which == 0 ? nppData._scintillaMainHandle
                      : nppData._scintillaSecondHandle;
}

// ── Bestätigungs-Dialog ───────────────────────────────────────────────────────
static bool askConfirmation(const std::vector<Finding>& findings, bool selOnly)
{
    std::wstring msg = selOnly
        ? L"Gefundene Einträge in der Auswahl:\n\n"
        : L"Gefundene Einträge im Dokument:\n\n";

    size_t show = std::min(findings.size(), (size_t)18);
    for (size_t i = 0; i < show; ++i) {
        std::wstring wName(findings[i].typeName.begin(), findings[i].typeName.end());
        std::wstring wPrev(findings[i].preview.begin(), findings[i].preview.end());
        msg += L"  \u25ba [" + wName + L"]   " + wPrev + L"\n";
    }
    if (findings.size() > show)
        msg += L"\n  \u2026 und " + std::to_wstring(findings.size() - show) + L" weitere Eintr\u00e4ge\n";

    msg += L"\n\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";
    msg += L"Gesamt: " + std::to_wstring(findings.size()) + L" Fund/Funde\n\n";
    msg += L"Alle Werte durch [REDACTED] ersetzen?";

    int res = ::MessageBoxW(nppData._nppHandle, msg.c_str(),
                            L"SecurityRedact \u2013 Best\u00e4tigung",
                            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    return res == IDYES;
}

// ── Fertig-Meldung ────────────────────────────────────────────────────────────
static void showDone(size_t count)
{
    std::wstring msg = L"Fertig! " + std::to_wstring(count)
        + L" Eintrag/Eintr\u00e4ge wurden durch [REDACTED] ersetzt.\n\n"
          L"R\u00fckg\u00e4ngig: Strg+Z";
    ::MessageBoxW(nppData._nppHandle, msg.c_str(),
                  L"SecurityRedact \u2013 Abgeschlossen",
                  MB_OK | MB_ICONINFORMATION);
}

// ============================================================================
// STEP 4: Menü-Funktionen
// ============================================================================

// ── Gesamtes Dokument schwärzen ───────────────────────────────────────────────
void cmd_RedactDocument()
{
    HWND sci = currentScintilla();

    LRESULT docLen = ::SendMessage(sci, SCI_GETLENGTH, 0, 0);
    if (docLen == 0) {
        ::MessageBoxW(nppData._nppHandle, L"Das Dokument ist leer.",
                      L"SecurityRedact", MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::string buf(docLen + 1, '\0');
    ::SendMessage(sci, SCI_GETTEXT, docLen + 1, (LPARAM)buf.data());
    buf.resize((size_t)docLen);

    std::vector<Finding> findings;
    std::string redacted = redactText(buf, findings);

    if (findings.empty()) {
        ::MessageBoxW(nppData._nppHandle,
                      L"Keine sicherheitsrelevanten Eintr\u00e4ge gefunden.\n\n"
                      L"Das Dokument wurde nicht ver\u00e4ndert.",
                      L"SecurityRedact \u2013 Kein Fund",
                      MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (!askConfirmation(findings, false)) return;

    ::SendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
    ::SendMessage(sci, SCI_SETTEXT, 0, (LPARAM)redacted.c_str());
    ::SendMessage(sci, SCI_ENDUNDOACTION, 0, 0);

    showDone(findings.size());
}

// ── Nur Auswahl schwärzen ─────────────────────────────────────────────────────
void cmd_RedactSelection()
{
    HWND sci = currentScintilla();

    LRESULT selLen = ::SendMessage(sci, SCI_GETSELTEXT, 0, 0);
    if (selLen <= 1) {
        ::MessageBoxW(nppData._nppHandle,
                      L"Keine Auswahl vorhanden.\nBitte zuerst Text markieren.",
                      L"SecurityRedact", MB_OK | MB_ICONWARNING);
        return;
    }

    std::string sel(selLen, '\0');
    ::SendMessage(sci, SCI_GETSELTEXT, 0, (LPARAM)sel.data());
    sel.resize((size_t)(selLen - 1));   // Null-Terminator entfernen

    std::vector<Finding> findings;
    std::string redacted = redactText(sel, findings);

    if (findings.empty()) {
        ::MessageBoxW(nppData._nppHandle,
                      L"Keine sicherheitsrelevanten Eintr\u00e4ge in der Auswahl.",
                      L"SecurityRedact \u2013 Kein Fund",
                      MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (!askConfirmation(findings, true)) return;

    ::SendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
    ::SendMessage(sci, SCI_REPLACESEL, 0, (LPARAM)redacted.c_str());
    ::SendMessage(sci, SCI_ENDUNDOACTION, 0, 0);

    showDone(findings.size());
}

// ── Trennlinie (leere Funktion) ───────────────────────────────────────────────
void cmd_MenuSeparator() {}

// ── Über-Dialog ───────────────────────────────────────────────────────────────
void cmd_About()
{
    ::MessageBoxW(nppData._nppHandle,
        L"SecurityRedact v1.0\n"
        L"\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n"
        L"Schwärzt Keys, Tokens, Passwörter und\n"
        L"sicherheitsrelevante Daten in Notepad++.\n\n"
        L"Erkannte Typen:\n"
        L"  \u2022 AWS, Azure, Google, GitHub, GitLab\n"
        L"  \u2022 Stripe, Twilio, SendGrid, Mailgun\n"
        L"  \u2022 Slack, Heroku\n"
        L"  \u2022 JWT-Tokens, PEM Private Keys\n"
        L"  \u2022 DB-Connection-Strings mit Passwort\n"
        L"  \u2022 password=, api_key=, token=, secret=, ...\n\n"
        L"Shortcuts:\n"
        L"  Ctrl+Shift+R  \u2192  Dokument schwärzen\n"
        L"  Ctrl+Shift+E  \u2192  Auswahl schwärzen\n\n"
        L"Alle \u00c4nderungen sind per Strg+Z r\u00fckg\u00e4ngig.\n\n"
        L"\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n"
        L"Autor:   Markus Weigl\n"
        L"GitHub:  github.com/trackstick/Plugins-Apps",
        L"\u00dcber SecurityRedact",
        MB_OK | MB_ICONINFORMATION);
}

// ============================================================================
// Lifecycle-Funktionen (werden von SecurityRedact.cpp aufgerufen)
// ============================================================================

void pluginInit(HANDLE /*hModule*/) {}

void pluginCleanUp()
{
    // Shortcut-Objekte wurden als statische Variablen angelegt, kein free() nötig
}

void commandMenuInit()
{
    //-- STEP 3: Menü-Einträge registrieren --
    setCommand(0, TEXT("Dokument schw\u00e4rzen  [Ctrl+Shift+R]"), cmd_RedactDocument,  &sk_all);
    setCommand(1, TEXT("Auswahl schw\u00e4rzen   [Ctrl+Shift+E]"), cmd_RedactSelection, &sk_sel);
    setCommand(2, TEXT("---"),                                    cmd_MenuSeparator,   nullptr);
    setCommand(3, TEXT("\u00dcber SecurityRedact"),               cmd_About,           nullptr);
}

void commandMenuCleanUp() {}

// ── Hilfsfunktion setCommand ──────────────────────────────────────────────────
bool setCommand(size_t index, TCHAR* cmdName, PFUNCPLUGINCMD pFunc,
                ShortcutKey* sk, bool check0nInit)
{
    if (index >= (size_t)nbFunc || !pFunc) return false;
    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc       = pFunc;
    funcItem[index]._init2Check  = check0nInit;
    funcItem[index]._pShKey      = sk;
    return true;
}
