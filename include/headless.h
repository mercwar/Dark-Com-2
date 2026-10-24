#ifndef DARK_COM_HEADLESS_H
#define DARK_COM_HEADLESS_H

#ifdef __cplusplus
extern "C" {
#endif

// Master Export Signature for Fire-Gem to call dynamically
// Pass target URL and standard parameters. (Set postData to NULL for automatic GET)
__declspec(dllexport) const char* RunHeadlessRequest(const wchar_t* url, const wchar_t* postData);

#ifdef __cplusplus
}
#endif

#endif // DARK_COM_HEADLESS_H
