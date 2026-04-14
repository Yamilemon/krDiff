# krDiff

krkr（吉里吉里/Kirikiri）引擎的文本差异处理插件。

## 功能特性

- 生成两个文本之间的统一差异（unified diff）补丁
- 应用补丁还原文本内容
- 使用 Google 的 diff_match_patch 算法进行精确的文本比对

## 安装

1. 将编译好的 DLL 文件复制到插件目录
2. 在 TJS 脚本中加载插件：

```javascript
Plugins.link("diff.dll");
```

## 使用方法

### `getDiff(oldText, newText)`

生成两个文本之间的统一差异补丁字符串。

```javascript
var oldText = "Hello World";
var newText = "Hello krDiff";
var patch = krDiff.getDiff(oldText, newText);
// patch 包含统一差异格式的字符串
```

### `loadDiffPatch(sourceText, patchText)`

将补丁应用到源文本，返回修改后的文本。

```javascript
var sourceText = "Hello World";
var patch = "@@ -6,5 +6,7 @@\n World\n+krDiff\n";
var result = krDiff.loadDiffPatch(sourceText, patch);
// result = "Hello krDiff"
```

## API 参考

| 方法                                     | 参数                                                 | 返回值             |
| ---------------------------------------- | ---------------------------------------------------- | ------------------ |
| `getDiff(oldText, newText)`            | `oldText`: 原始文本 `newText`: 新文本            | 统一差异补丁字符串 |
| `loadDiffPatch(sourceText, patchText)` | `sourceText`: 待补丁文本 `patchText`: 补丁字符串 | 应用补丁后的文本   |

## 构建要求

- Visual Studio 2019 或更高版本
- ncbind 库（放到krkr插件源码目录下即可）

## 编译方法

1. 在 Visual Studio 中打开 `diff.sln`
2. 编译解决方案（Debug 或 Release 配置）
3. 编译好的 DLL 在输出目录中

## 许可证

本项目采用 Apache License 2.0 许可证。

本项目使用了 Google 的 [diff_match_patch](https://github.com/google/diff-match-patch) 库。

## 致谢

- [diff_match_patch](https://github.com/google/diff-match-patch) - Google 的文本差异库
- [ncbind](https://github.com/6t1s/ncbind) - Kirikiri 的 C++ 绑定库
