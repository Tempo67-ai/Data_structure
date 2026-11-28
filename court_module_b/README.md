# court_module_b：索引与查询模块（B1 & B2）

本模块负责在 A1/A2 完成文书加载和代理人提取后，建立高效查询索引，向 **D1** 与 **E1/E2** 提供文书编号查询和主体枚举能力。

## 文件说明

- `CaseIndex.h` / `CaseIndex.cpp`：索引类 `CaseIndex` 的声明与实现。
- `tests/TestModuleB.cpp`：B1（案号/日期）与 B2（律师/律所）单元测试。
- `Makefile`：编译与运行测试。

## 数据结构与复杂度

| 功能 | 数据结构 / 算法 | 时间复杂度 | 说明 |
| --- | --- | --- | --- |
| B1 案号查询 `byCaseNo` | 哈希表（`std::unordered_map`）+ 线性表 | 建表 $O(N)$，查询均摊 $O(1)$ | 调用 A1 的 `normalizeCaseNo` 统一全角/半角括号，同案号多份文书全部保留 |
| B1 日期范围 `byDateRange` | 排序数组 + 二分查找（`lower_bound` / `upper_bound`） | 建表 $O(N \log N)$，查询 $O(\log N + K)$ | 按 `YYYY-MM-DD` 闭区间查询，跳过空日期 |
| B2 律师/律所查询 `byLawyer` / `byFirm` | 哈希倒排索引 + 去重有序 `vector<int>` | 建表 $O(N)$，查询均摊 $O(1)$ | 律师以 `姓名 + 律所` 唯一区分；同一主体与同一文书只返回一次 |
| E1 候选主体枚举 `allLawyers` / `allFirms` | 去重有序 `vector` | 查询 $O(1)$ | 直接返回去重后的律师列表与律所列表 |

## 编译与测试

在项目根目录（与 `court_module_a` 同级）执行：

```bash
make -C court_module_b all check
```

## 对外接入示例（供 D1 与 E1/E2 使用）

```cpp
#include "CsvLoader.h"
#include "AgentExtractor.h"
#include "CaseIndex.h"

// 1. 启动阶段（A1 -> A2 -> B）
std::vector<CaseDocument> documents = loadDocuments(csvPaths);
populateAgents(documents);
CaseIndex index(documents);

// 2. B1：按案号 / 日期范围查询
std::vector<int> caseDocs = index.byCaseNo("(2024)津0101民初1号");
std::vector<int> dateDocs = index.byDateRange("2024-01-10", "2024-01-11");

// 3. B2：按律师（姓名+律所）或律所查询，可传给 D1 的 findSharedCases
std::vector<int> lawyer1Docs = index.byLawyer("甲律师", "甲所");
std::vector<int> lawyer2Docs = index.byLawyer("乙律师", "乙所");
std::vector<int> firmDocs = index.byFirm("甲所");

// 4. E1：枚举所有已识别的律师或律所做画像与 Top-K 推荐
const std::vector<Agent>& lawyers = index.allLawyers();
const std::vector<std::string>& firms = index.allFirms();
```
