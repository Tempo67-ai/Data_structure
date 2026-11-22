# A 模块交接说明

本目录提供 A1 文书读取和 A2 律师及律所提取。其他模块可以直接使用 `vector<CaseDocument>` 和 `vector<Agent>`，各自按业务需要筛选。

## 文件清单

| 文件 | 用途 | 主要使用人 |
| --- | --- | --- |
| `CaseDocument.h` | 定义文书及律师和律所配对的公共结构 | 全组 |
| `CsvLoader.h`、`CsvLoader.cpp` | A1 接口和实现，从一份或多份 CSV 得到统一文书列表 | E、B1、C、D2 |
| `AgentExtractor.h`、`AgentExtractor.cpp` | A2 接口和实现，从正文识别律师及其对应律所 | E、B2 |
| `examples/sample.csv` | 四条虚构文书。含重复案号、正文换行、引号、空字段及多位律师 | 全组 |
| `examples/Demo.cpp` | 单独演示 A1 和 A2 的程序入口，打印总数及首份文书摘要 | 验收人 |
| `tests/TestModuleA.cpp` | 自动核对 CSV 规则、A2 套用规则和四条联调样例 | 验收人 |
| `Makefile` | 独立编译演示程序和验收程序 | 验收人 |
| `.gitignore` | 忽略本模块的构建目录和本地真实数据目录 | 提交人 |
| `代码讲解.md` | 逐步解释两个模块的实现、数据结构、验收和适用范围 | 汇报人、接手人 |

## 一分钟验收

在本目录打开终端。使用已有的 C++17 编译器。以 Mac 或 Linux 为例：

```bash
make
make check
./build/module_a_demo examples/sample.csv
```

样例应输出 4 份文书、3 份带律师配对的文书，配对总条数为 4。只验证 A1 时，在 CSV 路径前加 `--a1`。

也可以直接使用编译器：

```bash
c++ -std=c++17 -Wall -Wextra -I. CsvLoader.cpp AgentExtractor.cpp examples/Demo.cpp -o module_a_demo
./module_a_demo examples/sample.csv
```

将真实 CSV 放入本目录的 `data/`，每个月保留一份。按月份顺序传给程序。以下命令匹配交接时使用的原始文件名，带 `(1)` 的副本需使用实际文件名或在本地重命名。

```bash
./build/module_a_demo --a1 data/*01月裁判文书数据_天津.csv
./build/module_a_demo data/*月裁判文书数据_天津.csv
```

## E 怎样接入项目

将目录放入小组已有的仓库，保留下面三个公共头文件的字段名。把 `CsvLoader.cpp` 和 `AgentExtractor.cpp` 加入已有项目的编译目标，添加本目录的头文件搜索路径，并在项目已有的入口调用：

```cpp
#include "CsvLoader.h"
#include "AgentExtractor.h"

// csvPaths 由程序入口按约定顺序提供，每个文件只传一次。
std::vector<CaseDocument> documents = loadDocuments(csvPaths);
populateAgents(documents);
```

演示和测试各有自己的 `main()`，它们应各自编译成独立目标。E 可捕获 `std::exception`，在启动时显示具体错误信息。加载完成后，建议把 `documents` 以只读引用传入各模块。

| 接入方 | 使用方式 |
| --- | --- |
| B1 | 根据 `caseNo` 和 `judgeDate` 查询并返回 `docId`。输入案号调用 `normalizeCaseNo`，同案号的不同文书全部保留。 |
| B2 | 遍历每份文书的 `agents`，将「姓名＋律所」和 `docId` 关联。律所计数以需要的文书或案号口径为准。 |
| C | 从 `cause` 取案由，从 `content` 搜全文，结果返回 `docId`。 |
| D | D1 结合 B2 的主体索引。D2 直接用 `features` 和 `cause`。 |
| E | 启动时调用一次加载和一次提取。D1、D2、E1 按分工约定使用民事一审判决书范围。 |

在一次加载中，`docId` 从 0 开始按输入文件和文件内记录顺序连续编号，并与 `documents` 下标一致。展示或排序使用编号列表。跨月数据应一次传入，按顺序生成全局编号。`caseNo` 的全角括号和常见空白在 A1 中统一，`judgeDate` 沿用源文件中的日期值。空 `features` 表示源文书缺少案情特征，空 `agents` 表示 A2 没识别到足够明确的配对。

## GitHub 交接建议

以一个独立目录提交，避免覆盖仓库已有的入口或构建文件。PR 可以取名「[A] 文书 CSV 读取与律师律所提取」。把下面内容放在 PR 描述里：

> A1 提供 `loadDocuments`，保留同案号下的多份文书，并给每份文书连续编号。A2 提供 `populateAgents`，在文书中填入律师与律所的配对。进入 `court_module_a` 后运行 `make check` 验证样例。已用项目 12 个月数据验证，共读到 40,900 份文书。请 E 确认公共字段和启动调用，请 B2 核对 `agents` 的使用方式。真实数据沿用组内约定的数据路径。

首次对接可以让 B1、C、D2 先调用 `loadDocuments`，再由 E 调用 `populateAgents` 后交给 B2。样例 CSV 由虚构记录组成，适合放在仓库里给全组复用。
