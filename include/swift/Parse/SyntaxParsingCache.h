//===----------- SyntaxParsingCache.h -================----------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_PARSE_SYNTAXPARSINGCACHE_H
#define SWIFT_PARSE_SYNTAXPARSINGCACHE_H

#include "swift/Syntax/SyntaxNodes.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

namespace {

/// A single edit to the original source file in which a continuous range of
/// characters have been replaced by a new string
struct SourceEdit {
  /// The byte offset from which on characters were replaced.
  size_t Start;

  /// The byte offset to which on characters were replaced.
  size_t End;

  /// The length of the string that replaced the range described above.
  size_t ReplacementLength;

  /// The length of the range that has been replaced
  size_t originalLength() { return End - Start; }

  /// Check if the characters replaced by this edit fall into the given range
  /// or are directly adjacent to it
  bool intersectsOrTouchesRange(size_t RangeStart, size_t RangeEnd) {
    return !(End <= RangeStart || Start >= RangeEnd);
  }
};

} // anonymous namespace

namespace swift {

using namespace swift::syntax;

struct SyntaxReuseRegion {
  /// The byte offset at which the range begins
  uintptr_t Start;
  /// The byte offset at which the end ends
  uintptr_t End;
};

class SyntaxParsingCache {
  /// The syntax tree prior to the edit
  SourceFileSyntax OldSyntaxTree;

  /// The edits that were made from the source file that created this cache to
  /// the source file that is now parsed incrementally
  llvm::SmallVector<SourceEdit, 4> Edits;

  /// Whether or not information about reused nodes shall be recored in
  /// \c ReusedRanges
  bool RecordReuseInformation = false;

  /// If \c RecordReuseInformation buffer offsets of ranges that have been
  /// successfully looked up in this cache are stored.
  std::vector<SyntaxReuseRegion> ReusedRanges;

public:
  SyntaxParsingCache(SourceFileSyntax OldSyntaxTree)
      : OldSyntaxTree(OldSyntaxTree) {}

  /// Add an edit that transformed the source file which created this cache into
  /// the source file that is now being parsed incrementally. The order in which
  /// the edits are added using this method needs to be the same order in which
  /// the edits were applied to the source file.
  void addEdit(size_t Start, size_t End, size_t ReplacementLength) {
    Edits.push_back({Start, End, ReplacementLength});
  }

  /// Check if a syntax node of the given kind at the given position can be
  /// reused for a new syntax tree.
  llvm::Optional<Syntax> lookUp(size_t NewPosition, SyntaxKind Kind);

  /// Turn recording of reused ranges on
  void setRecordReuseInformation() { RecordReuseInformation = true; }

  /// Return the ranges of the new source file that have been successfully
  /// looked up in this cache as a (start, end) pair of byte offsets in the
  /// post-edit file.
  std::vector<SyntaxReuseRegion> getReusedRanges() const {
    return ReusedRanges;
  }

private:
  llvm::Optional<Syntax> lookUpFrom(const Syntax &Node, size_t Position,
                                    SyntaxKind Kind);

  bool nodeCanBeReused(const Syntax &Node, size_t Position,
                       SyntaxKind Kind) const;
};

} // namespace swift

#endif // SWIFT_SYNTAX_PARSING_CACHE_H
