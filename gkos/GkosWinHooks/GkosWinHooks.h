#if defined(GKOSWINHOOKS_EXPORTS)
#   define GKOSWINHOOKS_API __declspec(dllexport)
#else
#   define GKOSWINHOOKS_API __declspec(dllimport)
#endif

namespace GkosWinHooks {

GKOSWINHOOKS_API LRESULT CALLBACK KeyboardHookProc (
    int    code,
    WPARAM wParam,
    LPARAM lParam
);

} // namespace GkosWinHooks
