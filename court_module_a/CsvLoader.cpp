#include "CsvLoader.h"
#include <fstream>
#include <stdexcept>
#include <utility>

namespace {

int columnIndex(const std::vector<std::string>& header, const std::string& name) {
    for (std::size_t i = 0; i < header.size(); ++i) {
        if (header[i] == name) {
            return static_cast<int>(i);
        }
    }
    throw std::runtime_error("缺少列「" + name + "」");
}

bool isSpace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

std::string trimLabel(const std::string& label) {
    const std::string fullWidthSpace = "　";
    std::size_t first = 0;
    std::size_t last = label.size();
    while (first < last) {
        if (isSpace(label[first])) {
            ++first;
        } else if (label.compare(first, fullWidthSpace.size(), fullWidthSpace) == 0) {
            first += fullWidthSpace.size();
        } else {
            break;
        }
    }
    while (last > first) {
        if (isSpace(label[last - 1])) {
            --last;
        } else if (last - first >= fullWidthSpace.size() &&
                   label.compare(last - fullWidthSpace.size(), fullWidthSpace.size(),
                                 fullWidthSpace) == 0) {
            last -= fullWidthSpace.size();
        } else {
            break;
        }
    }
    return label.substr(first, last - first);
}

}  // namespace

std::string normalizeCaseNo(const std::string& value) {
    std::string result;
    for (std::size_t i = 0; i < value.size();) {
        if (value.compare(i, 3, "（") == 0) {
            result += '(';
            i += 3;
        } else if (value.compare(i, 3, "）") == 0) {
            result += ')';
            i += 3;
        } else if (value.compare(i, 3, "　") == 0) {
            i += 3;
        } else {
            if (!isSpace(value[i])) {
                result += value[i];
            }
            ++i;
        }
    }
    return result;
}

namespace {

std::vector<std::string> splitFeatures(const std::string& field) {
    std::vector<std::string> features;
    std::size_t start = 0;
    while (start < field.size()) {
        std::size_t end = field.find('\n', start);
        if (end == std::string::npos) {
            end = field.size();
        }
        std::string feature = trimLabel(field.substr(start, end - start));
        bool alreadyAdded = false;
        for (const std::string& existing : features) {
            if (existing == feature) {
                alreadyAdded = true;
                break;
            }
        }
        if (!feature.empty() && !alreadyAdded) {
            features.push_back(feature);
        }
        start = end + 1;
    }
    return features;
}

}  // namespace

bool readCsvRecord(std::istream& input, std::vector<std::string>& fields) {
    fields.clear();
    std::string field;
    bool inQuotes = false;
    bool afterClosingQuote = false;
    bool sawCharacter = false;

    // The source files are UTF-8. A BOM may appear before the first column.
    // Check the prefix only at the start of a seekable stream.
    if (input.tellg() == std::streampos(0) && input.peek() == 0xEF) {
        char marker[3];
        input.read(marker, 3);
        if (input.gcount() != 3) {
            throw std::runtime_error("CSV 开头的 UTF-8 字节不完整");
        }
        if (std::string(marker, 3) != "\xEF\xBB\xBF") {
            field.assign(marker, 3);
            sawCharacter = true;
        }
    }

    char ch;
    while (input.get(ch)) {
        sawCharacter = true;

        if (inQuotes) {
            if (ch != '"') {
                field += ch;
            } else if (input.peek() == '"') {
                input.get(ch);
                field += '"';
            } else {
                inQuotes = false;
                afterClosingQuote = true;
            }
            continue;
        }

        if (ch == ',') {
            fields.push_back(field);
            field.clear();
            afterClosingQuote = false;
        } else if (ch == '\n' || ch == '\r') {
            if (ch == '\r' && input.peek() == '\n') {
                input.get(ch);
            }
            fields.push_back(field);
            return true;
        } else if (afterClosingQuote) {
            throw std::runtime_error("结束引号之后应为逗号或换行");
        } else if (ch == '"') {
            if (!field.empty()) {
                throw std::runtime_error("字段中出现意外引号");
            }
            inQuotes = true;
        } else {
            field += ch;
        }
    }

    if (input.bad() || !input.eof()) {
        throw std::runtime_error("读取 CSV 失败");
    }
    if (inQuotes) {
        throw std::runtime_error("文件结束时仍有未闭合的引号");
    }
    if (!sawCharacter) {
        return false;
    }
    fields.push_back(field);
    return true;
}

std::vector<CaseDocument> loadDocuments(const std::vector<std::string>& csvPaths) {
    std::vector<CaseDocument> documents;

    for (const std::string& path : csvPaths) {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("打不开 CSV：" + path);
        }

        std::size_t recordNo = 1;  // header is record 1, regardless of embedded newlines
        try {
            std::vector<std::string> header;
            if (!readCsvRecord(input, header)) {
                throw std::runtime_error("CSV 为空，缺少表头");
            }

            const int title = columnIndex(header, "标题");
            const int caseNo = columnIndex(header, "案号");
            const int court = columnIndex(header, "法院");
            const int documentType = columnIndex(header, "文书类型");
            const int caseType = columnIndex(header, "案件类型");
            const int procedure = columnIndex(header, "审理程序");
            const int judgeDate = columnIndex(header, "判决时间");
            const int cause = columnIndex(header, "案由");
            const int result = columnIndex(header, "判决结果");
            const int features = columnIndex(header, "案情特征");
            const int content = columnIndex(header, "文书内容");

            std::vector<std::string> fields;
            ++recordNo;
            while (readCsvRecord(input, fields)) {
                if (fields.size() == 1 && fields[0].empty()) {
                    ++recordNo;
                    continue;
                }
                if (fields.size() != header.size()) {
                    throw std::runtime_error("字段数为 " + std::to_string(fields.size()) +
                                             "，表头列数为 " + std::to_string(header.size()));
                }

                CaseDocument document;
                document.docId = static_cast<int>(documents.size());
                document.title = fields[title];
                document.caseNo = normalizeCaseNo(fields[caseNo]);
                document.court = fields[court];
                document.documentType = fields[documentType];
                document.caseType = fields[caseType];
                document.procedure = fields[procedure];
                document.judgeDate = fields[judgeDate];
                document.cause = fields[cause];
                document.result = fields[result];
                document.features = splitFeatures(fields[features]);
                document.content = std::move(fields[content]);
                documents.push_back(std::move(document));
                ++recordNo;
            }
        } catch (const std::runtime_error& error) {
            throw std::runtime_error(path + "，CSV 第 " + std::to_string(recordNo) +
                                     " 条记录：" + error.what());
        }
    }

    return documents;
}
