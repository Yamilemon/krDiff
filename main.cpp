#define NOMINMAX
#include "ncbind.hpp"
#include "diff_match_patch.h"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;

class krDiff {

	public:
		// 生成差异化 patch 字符串
		// 参数 1: 原始文本 (oldText)
		// 参数 2: 新文本 (newText)
		// 返回：patch 字符串（统一差异格式）
		static tjs_error TJS_INTF_METHOD getDiffPatch(tTJSVariant* result, tjs_int numparams, tTJSVariant** param, iTJSDispatch2* objthis) {
			
			if (numparams < 2 || !param[0] || !param[1]) {
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
		static tjs_error TJS_INTF_METHOD loadDiffPatch(tTJSVariant* result, tjs_int numparams, tTJSVariant** param, iTJSDispatch2* objthis) {
			
			if (numparams < 2 || !param[0] || !param[1]) {
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
};



NCB_REGISTER_CLASS(krDiff)
{
	RawCallback("getDiff", &Class::getDiffPatch, 0);
	RawCallback("loadDiff", &Class::loadDiffPatch, 0);
};
