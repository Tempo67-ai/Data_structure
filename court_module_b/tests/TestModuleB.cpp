#include "../CaseIndex.h"
#include "CaseDocument.h"

#include <cassert>
#include <iostream>
#include <vector>

namespace {

std::vector<CaseDocument> buildSampleDocuments() {
    std::vector<CaseDocument> docs;

    CaseDocument d0;
    d0.docId = 0;
    d0.caseNo = "（2024）津0101民初1号";
    d0.documentType = "判决书";
    d0.caseType = "民事";
    d0.procedure = "一审";
    d0.judgeDate = "2024-01-10";
    d0.cause = "劳动争议";
    d0.features = {"工资", "加班"};
    d0.agents = {{"甲律师", "甲所"}, {"乙律师", "乙所"}};
    docs.push_back(d0);

    CaseDocument d1;
    d1.docId = 1;
    d1.caseNo = "(2024)津0101民初1号";  // 与 d0 同案号，半角括号
    d1.documentType = "裁定书";
    d1.caseType = "民事";
    d1.procedure = "一审";
    d1.judgeDate = "2024-01-10";
    d1.cause = "劳动争议";
    docs.push_back(d1);

    CaseDocument d2;
    d2.docId = 2;
    d2.caseNo = "(2024)津0101民初2号";
    d2.documentType = "判决书";
    d2.caseType = "民事";
    d2.procedure = "一审";
    d2.judgeDate = "2024-01-11";
    d2.cause = "劳动争议";
    d2.features = {"工资"};
    d2.agents = {{"甲律师", "甲所"}, {"丁律师", "甲所"}};  // 两位同所律师
    docs.push_back(d2);

    CaseDocument d3;
    d3.docId = 3;
    d3.caseNo = "(2024)津0101民初3号";
    d3.documentType = "判决书";
    d3.caseType = "民事";
    d3.procedure = "一审";
    d3.judgeDate = "2024-01-12";
    d3.cause = "劳动争议";
    d3.features = {"工资", "解除合同"};
    d3.agents = {{"丙律师", "丙所"}, {"甲律师", "戊所"}};  // 同名但不同律所的甲律师
    docs.push_back(d3);

    CaseDocument d4;
    d4.docId = 4;
    d4.caseNo = "(2024)津0101民初4号";
    d4.judgeDate = "";  // 缺失日期测试
    docs.push_back(d4);

    return docs;
}

void testByCaseNo(const CaseIndex& index) {
    // 1. 全角与半角括号统一，同案号返回两份文书
    std::vector<int> res1 = index.byCaseNo("(2024)津0101民初1号");
    assert((res1 == std::vector<int>{0, 1}));

    std::vector<int> res2 = index.byCaseNo("（2024）津0101民初1号");
    assert((res2 == std::vector<int>{0, 1}));

    // 2. 不存在的案号返回空
    assert(index.byCaseNo("(2024)津0101民初999号").empty());
    assert(index.byCaseNo("").empty());
}

void testByDateRange(const CaseIndex& index) {
    // 1. 起止日期相同（闭区间）
    std::vector<int> sameDay = index.byDateRange("2024-01-10", "2024-01-10");
    assert((sameDay == std::vector<int>{0, 1}));

    // 2. 跨日期范围查询
    std::vector<int> range = index.byDateRange("2024-01-10", "2024-01-11");
    assert((range == std::vector<int>{0, 1, 2}));

    // 3. 全部有效日期（不包含空日期 docId=4）
    std::vector<int> allValid = index.byDateRange("2024-01-01", "2024-12-31");
    assert((allValid == std::vector<int>{0, 1, 2, 3}));

    // 4. 倒置或无命中范围
    assert(index.byDateRange("2024-01-12", "2024-01-10").empty());
    assert(index.byDateRange("2025-01-01", "2025-01-31").empty());
}

void testByLawyerAndFirm(const CaseIndex& index) {
    // 1. 同名但不同律所的律师区分
    std::vector<int> jiaA = index.byLawyer("甲律师", "甲所");
    assert((jiaA == std::vector<int>{0, 2}));

    std::vector<int> jiaE = index.byLawyer("甲律师", "戊所");
    assert((jiaE == std::vector<int>{3}));

    // 2. 同文书多名同所律师，律所查询只返回该文书一次
    std::vector<int> firmA = index.byFirm("甲所");
    assert((firmA == std::vector<int>{0, 2}));

    std::vector<int> firmB = index.byFirm("乙所");
    assert((firmB == std::vector<int>{0}));

    // 3. 不存在的主体
    assert(index.byLawyer("不存在", "甲所").empty());
    assert(index.byFirm("不存在律所").empty());

    // 4. 去重后的主体列表（供 E1 使用）
    assert(index.lawyerCount() == 5);
    assert(index.firmCount() == 4);
}

}  // namespace

int main() {
    std::vector<CaseDocument> docs = buildSampleDocuments();
    CaseIndex index(docs);

    assert(index.documentCount() == 5);
    testByCaseNo(index);
    testByDateRange(index);
    testByLawyerAndFirm(index);

    std::cout << "[TestModuleB] All B1 & B2 unit tests passed!" << std::endl;
    return 0;
}
