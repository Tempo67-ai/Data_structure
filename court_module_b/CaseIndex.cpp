#include "CaseIndex.h"
#include "CsvLoader.h"

#include <algorithm>
#include <cctype>

namespace {

// 仅保留末尾追加升序 docId 时的去重逻辑
void appendUniqueSortedDocId(std::vector<int>& docIds, int docId) {
    if (docIds.empty() || docIds.back() != docId) {
        docIds.push_back(docId);
    }
}

void deduplicateAndSort(std::vector<int>& docIds) {
    std::sort(docIds.begin(), docIds.end());
    docIds.erase(std::unique(docIds.begin(), docIds.end()), docIds.end());
}

}  // namespace

CaseIndex::CaseIndex(const std::vector<CaseDocument>& documents) {
    build(documents);
}

void CaseIndex::clear() {
    documentCount_ = 0;
    caseNoIndex_.clear();
    lawyerIndex_.clear();
    firmIndex_.clear();
    dateEntries_.clear();
    uniqueLawyers_.clear();
    uniqueFirms_.clear();
}

std::string CaseIndex::trimCopy(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() &&
           std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    std::size_t end = text.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(start, end - start);
}

std::string CaseIndex::makeLawyerKey(const std::string& lawyerName,
                                     const std::string& firmName) {
    // 使用 ASCII 单元分隔符避免姓名与律所拼接产生歧义
    return trimCopy(lawyerName) + '\x1f' + trimCopy(firmName);
}

void CaseIndex::build(const std::vector<CaseDocument>& documents) {
    clear();
    documentCount_ = documents.size();
    dateEntries_.reserve(documents.size());

    for (std::size_t idx = 0; idx < documents.size(); ++idx) {
        const CaseDocument& doc = documents[idx];
        const int docId = (doc.docId >= 0) ? doc.docId : static_cast<int>(idx);

        // 1. 案号哈希索引 (B1)
        const std::string normCaseNo = normalizeCaseNo(doc.caseNo);
        if (!normCaseNo.empty()) {
            caseNoIndex_[normCaseNo].push_back(docId);
        }

        // 2. 日期排序索引收集 (B1)
        const std::string cleanDate = trimCopy(doc.judgeDate);
        if (!cleanDate.empty()) {
            dateEntries_.push_back(DateEntry{cleanDate, docId});
        }

        // 3. 律师与律所倒排索引 (B2)
        for (const Agent& agent : doc.agents) {
            const std::string lawyerName = trimCopy(agent.lawyerName);
            const std::string firmName = trimCopy(agent.firmName);

            if (!lawyerName.empty() && !firmName.empty()) {
                const std::string key = makeLawyerKey(lawyerName, firmName);
                auto it = lawyerIndex_.find(key);
                if (it == lawyerIndex_.end()) {
                    uniqueLawyers_.push_back(Agent{lawyerName, firmName});
                    lawyerIndex_.emplace(key, std::vector<int>{docId});
                } else {
                    appendUniqueSortedDocId(it->second, docId);
                }
            }

            if (!firmName.empty()) {
                auto it = firmIndex_.find(firmName);
                if (it == firmIndex_.end()) {
                    uniqueFirms_.push_back(firmName);
                    firmIndex_.emplace(firmName, std::vector<int>{docId});
                } else {
                    appendUniqueSortedDocId(it->second, docId);
                }
            }
        }
    }

    // 保证所有 docId 列表严格升序且无重复（防止输入 docId 非单调递增）
    for (auto& kv : caseNoIndex_) {
        deduplicateAndSort(kv.second);
    }
    for (auto& kv : lawyerIndex_) {
        deduplicateAndSort(kv.second);
    }
    for (auto& kv : firmIndex_) {
        deduplicateAndSort(kv.second);
    }

    // 对日期索引按 (judgeDate, docId) 升序排序，支持二分范围查找
    std::sort(dateEntries_.begin(), dateEntries_.end(),
              [](const DateEntry& a, const DateEntry& b) {
                  if (a.judgeDate != b.judgeDate) {
                      return a.judgeDate < b.judgeDate;
                  }
                  return a.docId < b.docId;
              });

    // 对主体列表排序，保证不同环境下枚举顺序稳定
    std::sort(uniqueLawyers_.begin(), uniqueLawyers_.end(),
              [](const Agent& a, const Agent& b) {
                  if (a.lawyerName != b.lawyerName) {
                      return a.lawyerName < b.lawyerName;
                  }
                  return a.firmName < b.firmName;
              });
    std::sort(uniqueFirms_.begin(), uniqueFirms_.end());
}

std::vector<int> CaseIndex::byCaseNo(const std::string& caseNo) const {
    const std::string normCaseNo = normalizeCaseNo(caseNo);
    if (normCaseNo.empty()) {
        return {};
    }
    auto it = caseNoIndex_.find(normCaseNo);
    if (it == caseNoIndex_.end()) {
        return {};
    }
    return it->second;
}

std::vector<int> CaseIndex::byDateRange(const std::string& startDate,
                                        const std::string& endDate) const {
    const std::string start = trimCopy(startDate);
    const std::string end = trimCopy(endDate);
    if (start.empty() || end.empty() || start > end) {
        return {};
    }

    auto lower = std::lower_bound(
        dateEntries_.begin(), dateEntries_.end(), start,
        [](const DateEntry& entry, const std::string& target) {
            return entry.judgeDate < target;
        });

    auto upper = std::upper_bound(
        dateEntries_.begin(), dateEntries_.end(), end,
        [](const std::string& target, const DateEntry& entry) {
            return target < entry.judgeDate;
        });

    std::vector<int> result;
    result.reserve(static_cast<std::size_t>(upper - lower));
    for (auto it = lower; it != upper; ++it) {
        result.push_back(it->docId);
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<int> CaseIndex::byLawyer(const std::string& lawyerName,
                                     const std::string& firmName) const {
    const std::string cleanLawyer = trimCopy(lawyerName);
    const std::string cleanFirm = trimCopy(firmName);
    if (cleanLawyer.empty() || cleanFirm.empty()) {
        return {};
    }
    auto it = lawyerIndex_.find(makeLawyerKey(cleanLawyer, cleanFirm));
    if (it == lawyerIndex_.end()) {
        return {};
    }
    return it->second;
}

std::vector<int> CaseIndex::byFirm(const std::string& firmName) const {
    const std::string cleanFirm = trimCopy(firmName);
    if (cleanFirm.empty()) {
        return {};
    }
    auto it = firmIndex_.find(cleanFirm);
    if (it == firmIndex_.end()) {
        return {};
    }
    return it->second;
}
