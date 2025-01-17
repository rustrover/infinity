module;

#include <fstream>
#include <string>

module column_index_merger;

import stl;
import memory_pool;
import byte_slice;
import byte_slice_reader;
import file_reader;
import posting_decoder;
import posting_list_format;
import bitmap;
import index_defines;
import term_meta;
import index_full_text;
import column_index_iterator;
import segment_term_posting;
import fst;

namespace infinity {
ColumnIndexMerger::ColumnIndexMerger(const String &index_dir, optionflag_t flag, MemoryPool *memory_pool, RecyclePool *buffer_pool)
    : index_dir_(index_dir), flag_(flag), memory_pool_(memory_pool), buffer_pool_(buffer_pool) {}

ColumnIndexMerger::~ColumnIndexMerger() {}

SharedPtr<PostingMerger> ColumnIndexMerger::CreatePostingMerger() { return MakeShared<PostingMerger>(memory_pool_, buffer_pool_); }

void ColumnIndexMerger::Merge(const Vector<String> &base_names, const Vector<docid_t> &base_docids, const String &target_base_name) {
    Path path = Path(index_dir_) / target_base_name;
    String dict_file = path.string();
    String fst_file = dict_file + DICT_SUFFIX + ".fst";
    dict_file.append(DICT_SUFFIX);
    SharedPtr<FileWriter> dict_file_writer = MakeShared<FileWriter>(fs_, dict_file, 1024);
    TermMetaDumper term_meta_dumpler((PostingFormatOption(flag_)));
    String posting_file = path.string();
    posting_file.append(POSTING_SUFFIX);
    posting_file_ = MakeShared<FileWriter>(fs_, posting_file, 1024);

    std::ofstream ofs(fst_file.c_str(), std::ios::binary | std::ios::trunc);
    OstreamWriter wtr(ofs);
    FstBuilder fst_builder(wtr);

    SegmentTermPostingQueue term_posting_queue(index_dir_, base_names, base_docids, flag_);
    String term;
    TermMeta term_meta;
    SizeT term_meta_offset = 0;

    while (!term_posting_queue.Empty()) {
        const Vector<SegmentTermPosting *> &merging_term_postings = term_posting_queue.GetCurrentMerging(term);
        MergeTerm(term, term_meta, merging_term_postings);
        term_meta_dumpler.Dump(dict_file_writer, term_meta);
        fst_builder.Insert((u8 *)term.c_str(), term.length(), term_meta_offset);
        term_meta_offset = dict_file_writer->TotalWrittenBytes();
        term_posting_queue.MoveToNextTerm();
    }
    memory_pool_->Release();
    buffer_pool_->Release();
}

void ColumnIndexMerger::MergeTerm(const String &term, TermMeta &term_meta, const Vector<SegmentTermPosting *> &merging_term_postings) {
    SharedPtr<PostingMerger> posting_merger = CreatePostingMerger();
    posting_merger->Merge(merging_term_postings);
    posting_merger->Dump(posting_file_, term_meta);
    memory_pool_->Reset();
    buffer_pool_->Reset();
}

} // namespace infinity