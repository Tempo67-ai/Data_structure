#ifndef CSV_LOADER_H
#define CSV_LOADER_H

#include "CaseDocument.h"
#include <istream>

// Reads one record; skips a UTF-8 BOM at the start of a seekable stream.
// Returns false at clean EOF. Throws std::runtime_error on malformed CSV.
bool readCsvRecord(std::istream& input, std::vector<std::string>& fields);

// Use the same normalization for CSV case numbers and search input.
std::string normalizeCaseNo(const std::string& value);

// Appends records in path order and gives them consecutive docIds from zero.
// Reports file, header or record errors via std::runtime_error.
std::vector<CaseDocument> loadDocuments(const std::vector<std::string>& csvPaths);

#endif
