#define NOMINMAX
#include "ncbind.hpp"
#include "diff_match_patch.h"
#include <windows.h>
#include <process.h>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>

using namespace std;

class krDiff
{

public:
    // getDiffAsync(oldText, newText, callbackFunction, callbackData = void)
    // callbackFunction(patchText, callbackData, errorText) 会在 Kirikiri 主线程中执行。
    // 后台计算可以并行，回调按照任务实际完成顺序触发。
    static tjs_error TJS_INTF_METHOD getDiffPatchAsync(tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
    {
        if (numparams < 3 || !param[0] || !param[1] || !param[2])
            return TJS_E_BADPARAMCOUNT;
        if (!ensureMessageWindow())
            return TJS_E_FAIL;

        tTJSString oldText(*param[0]);
        tTJSString newText(*param[1]);
        if (param[2]->Type() != tvtObject || !param[2]->AsObjectClosureNoAddRef().Object)
            return TJS_E_BADPARAMCOUNT;

        // tTJSVariant 会保留函数对象及其原始 this 对象，以及用户传入的回传数据。
        // 该对象只在 Kirikiri 主线程中创建和销毁。
        tTJSVariant callbackData;
        if (numparams >= 4 && param[3])
            callbackData = *param[3];
        DiffTask *task = new DiffTask(oldText.c_str(), newText.c_str(), *param[2], callbackData);
        uintptr_t thread = _beginthreadex(NULL, 0, &diffThreadProc, task, 0, NULL);
        if (!thread)
        {
            delete task;
            return TJS_E_FAIL;
        }
        ::CloseHandle((HANDLE)thread);
        if (result)
            *result = 0;
        return TJS_S_OK;
    }
    // 生成差异化 patch 字符串
    // 参数 1: 原始文本 (oldText)
    // 参数 2: 新文本 (newText)
    // 返回：patch 字符串（统一差异格式）
    static tjs_error TJS_INTF_METHOD getDiffPatch(tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
    {

        if (numparams < 2 || !param[0] || !param[1])
        {
            return TJS_E_BADPARAMCOUNT; // 参数不足
        }

        // 获取两个字符串
        tTJSString oldText = *param[0];
        tTJSString newText = *param[1];

        // 创建 diff_match_patch 实例（用于 wchar_t 字符串）
        diff_match_patch<wstring> dmp;

        // 生成 patch
        wstring strPatch = dmp.patch_toText(dmp.patch_make(oldText.c_str(), newText.c_str()));

        // 返回结果
        *result = tTJSString(strPatch.c_str());
        return TJS_S_OK;
    }

    // 应用 patch 到文本
    // 参数 1: 原始文本 (sourceText)
    // 参数 2: patch 字符串
    // 返回：应用 patch 后的新文本
    static tjs_error TJS_INTF_METHOD loadDiffPatch(tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
    {

        if (numparams < 2 || !param[0] || !param[1])
        {
            return TJS_E_BADPARAMCOUNT; // 参数不足
        }

        // 获取参数
        tTJSString sourceText = *param[0];
        tTJSString patchText = *param[1];

        // 创建 diff_match_patch 实例
        diff_match_patch<wstring> dmp;

        // 解析 patch
        diff_match_patch<wstring>::Patches patches = dmp.patch_fromText(patchText.c_str());

        // 应用 patch
        pair<wstring, vector<bool>> applyResult = dmp.patch_apply(patches, sourceText.c_str());

        // 返回应用后的文本
        *result = tTJSString(applyResult.first.c_str());
        return TJS_S_OK;
    }

#if 0 // unfinished former async stub
		static tjs_error TJS_INTF_METHOD saveAsync(tTJSVariant* result, tjs_int numparams, tTJSVariant** param, iTJSDispatch2* objthis) {
			if (numparams < 2 || !param[0] || !param[1]) {
				return TJS_E_BADPARAMCOUNT; // 参数不足
			}
			ttstr name(*param[0]);
			ttstr mode(*param[1]);
		}
#endif

private:
    enum
    {
        WM_KRDIFF_COMPLETE = WM_APP + 9
    };

    struct DiffTask
    {
        wstring oldText, newText, patchText, errorText;
        tTJSVariant callback;
        tTJSVariant callbackData;
        DiffTask(const wchar_t *oldValue, const wchar_t *newValue,
                 const tTJSVariant &callbackValue, const tTJSVariant &callbackDataValue)
            : oldText(oldValue), newText(newValue), callback(callbackValue),
              callbackData(callbackDataValue) {}
    };

    static HWND messageWindow;
    static ATOM messageWindowClass;
    static bool ensureMessageWindow()
    {
        if (messageWindow)
            return true;
        HINSTANCE instance = ::GetModuleHandle(NULL);
        if (!messageWindowClass)
        {
            WNDCLASSEXW wc = {sizeof(WNDCLASSEXW), 0, completionWndProc, 0, 0,
                              instance, NULL, NULL, NULL, NULL, L"krDiff Async Message Window", NULL};
            messageWindowClass = ::RegisterClassExW(&wc);
            if (!messageWindowClass)
                return false;
        }
        messageWindow = ::CreateWindowExW(0, (LPCWSTR)MAKELONG(messageWindowClass, 0),
                                          L"krDiff Async Message", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, instance, NULL);
        return messageWindow != NULL;
    }

    static unsigned __stdcall diffThreadProc(void *data)
    {
        DiffTask *task = static_cast<DiffTask *>(data);
        try
        {
            diff_match_patch<wstring> dmp;
            task->patchText = dmp.patch_toText(dmp.patch_make(task->oldText, task->newText));
        }
        catch (const exception &)
        {
            task->errorText = L"patch generation failed";
        }
        catch (...)
        {
            task->errorText = L"unknown error while generating patch";
        }

        // patch 已经生成，后续等待主线程回调时不再需要输入全文。
        // swap 会同时释放字符串容量，clear 只会清空长度但通常保留内存。
        wstring().swap(task->oldText);
        wstring().swap(task->newText);

        // 工作线程只投递纯 C++ 数据，绝不调用或释放 TJS 对象。
        if (!::PostMessage(messageWindow, WM_KRDIFF_COMPLETE, 0, (LPARAM)task))
        {
            // 消息队列满时改为同步交给窗口所属的主线程，避免 task 泄漏。
            // completionWndProc 返回非零表示已经接管并释放 task。
            if (!messageWindow || !::SendMessageW(messageWindow, WM_KRDIFF_COMPLETE, 0, (LPARAM)task))
            {
                // 这里只能释放不涉及 TJS 的大块字符串；callback 与
                // callbackData 必须留给主线程销毁，避免跨线程调用 TJS。
                wstring().swap(task->patchText);
                wstring().swap(task->errorText);
            }
        }
        return 0;
    }

    static LRESULT WINAPI completionWndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
    {
        if (message == WM_KRDIFF_COMPLETE)
        {
            DiffTask *task = reinterpret_cast<DiffTask *>(lp);
            if (task)
                invokeCallbackAndDelete(task);
            return TRUE;
        }
        return ::DefWindowProc(hwnd, message, wp, lp);
    }

    static void invokeCallbackAndDelete(DiffTask *task)
    {
        // 即使 TJS 回调抛出异常，task 以及 callback/callbackData 也会释放。
        unique_ptr<DiffTask> ownedTask(task);
        try
        {
            tTJSVariant patch(tTJSString(task->patchText.c_str()));
            tTJSVariant error(tTJSString(task->errorText.c_str()));

            // TJS 参数已经拥有自己的字符串；先释放 C++ 结果缓冲区，
            // 避免执行回调期间同时保留两份大 patch。
            wstring().swap(task->patchText);
            wstring().swap(task->errorText);

            tTJSVariant *args[] = {&patch, &task->callbackData, &error};
            tTJSVariantClosure closure(task->callback.AsObjectClosureNoAddRef());

            // 直接调用保存的函数闭包，并保留其原始 this。
            (void)closure.FuncCall(0, NULL, NULL, NULL, 3, args, NULL);
        }
        catch (...)
        {
            ::OutputDebugStringW(L"krDiff: asynchronous callback threw an exception\n");
        }
    }
};

HWND krDiff::messageWindow = NULL;
ATOM krDiff::messageWindowClass = 0;
NCB_REGISTER_CLASS(krDiff)
{
    RawCallback("getDiff", &Class::getDiffPatch, 0);
    RawCallback("getDiffAsync", &Class::getDiffPatchAsync, 0);
    RawCallback("loadDiff", &Class::loadDiffPatch, 0);
};
