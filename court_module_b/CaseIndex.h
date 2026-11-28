#ifndef CASE_INDEX_H
#define CASE_INDEX_H

#include "CaseDocument.h"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

// B1 & B2: 面向文书检索的索引模块
// - 案号索引 (B1): 哈希表，O(1) 均摊查询
// - 日期索引 (B1): 排序数组 + 二分查找，O(log N + K) 范围查询
// - 律师/律所倒排索引 (B2): 哈希表 + 去重有序 docId 列表，O(1) 均摊查询
class CaseIndex {
public:
    CaseIndex() = default;
    explicit CaseIndex(const std::vector<CaseDocument>& documents);

    // 根据 A1/A2 加载并提取后的文书列表构建全部索引
    void build(const std::vector<CaseDocument>& documents);
    void clear();

    // B1: 按案号查询（自动统一全角/半角括号及空白，保留同案号全部文书）
    std::vector<int> byCaseNo(const std::string& caseNo) const;

    // B1: 按判决日期闭区间 [startDate, endDate] 查询（格式 YYYY-MM-DD）
    std::vector<int> byDateRange(const std::string& startDate,
                                 const std::string& endDate) const;

    // B2: 按律师（姓名 + 律所）查询参与的文书 docId（已去重、升序）
    std::vector<int> byLawyer(const std::string& lawyerName,
                              const std::string& firmName) const;

    // B2: 按律所名称查询参与的文书 docId（已去重、升序）
    std::vector<int> byFirm(const std::string& firmName) const;

    // B2: 获取去重后的所有已识别律师与律所列表（供 E1 画像与推荐遍历）
    const std::vector<Agent>& allLawyers() const { return uniqueLawyers_; }
    const std::vector<std::string>& allFirms() const { return uniqueFirms_; }

    std::size_t documentCount() const { return documentCount_; }
    std::size_t lawyerCount() const { return uniqueLawyers_.size(); }
    std::size_t firmCount() const { return uniqueFirms_.size(); }

private:
    struct DateEntry {
        std::string judgeDate;
        int docId;
    };

    static std::string trimCopy(const std::string& text);
    static std::string makeLawyerKey(const std::string& lawyerName,
                                     const std::string& firmName);

    std::size_t documentCount_ = 0;

    // 哈希倒排索引
    std::unordered_map<std::string, std::vector<int>> caseNoIndex_;
    std::unordered_map<std::string, std::vector<int>> lawyerIndex_;
    std::unordered_map<std::string, std::vector<int>> firmIndex_;

    // 按日期排序的索引（用于二分范围查询）
    std::vector<DateEntry> dateEntries_;

    // 去重后的主体列表
    std::vector<Agent> uniqueLawyers_;
    std::vector<std::string> uniqueFirms_;
};

#endif
