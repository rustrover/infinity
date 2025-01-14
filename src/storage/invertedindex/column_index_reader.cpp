// Copyright(C) 2023 InfiniFlow, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

module;

#include <vector>

module column_index_reader;

import stl;
import memory_pool;
import segment_posting;
import index_segment_reader;
import posting_iterator;
import index_defines;
import disk_index_segment_reader;
import inmem_index_segment_reader;
import dict_reader;
import posting_list_format;

namespace infinity {
ColumnIndexReader::ColumnIndexReader() {}

void ColumnIndexReader::Open(const String &index_dir, const Vector<String> &base_names, const Vector<docid_t> &base_docids, optionflag_t flag) {
    flag_ = flag;
    for (SizeT i = 0; i < base_names.size(); i++) {
        SharedPtr<DiskIndexSegmentReader> segment_reader = CreateDiskSegmentReader(index_dir, base_names[i], base_docids[i], flag);
        segment_readers_.push_back(segment_reader);
        base_doc_ids_.push_back(base_docids[i]);
    }
    // TODO yzc: In memory segment
}

SharedPtr<DiskIndexSegmentReader>
ColumnIndexReader::CreateDiskSegmentReader(const String &index_dir, const String &base_name, docid_t base_doc_id, optionflag_t flag) {
    return MakeShared<DiskIndexSegmentReader>(index_dir, base_name, base_doc_id, flag);
}

PostingIterator *ColumnIndexReader::Lookup(const String &term, MemoryPool *session_pool) {
    SharedPtr<Vector<SegmentPosting>> seg_postings = MakeShared<Vector<SegmentPosting>>();
    for (u32 i = 0; i < segment_readers_.size(); ++i) {
        SegmentPosting seg_posting;
        auto ret = segment_readers_[i]->GetSegmentPosting(term, seg_posting, session_pool);
        if (ret) {
            seg_postings->push_back(seg_posting);
        }
    }
    if (seg_postings->empty())
        return nullptr;
    PostingIterator *iter = new ((session_pool)->Allocate(sizeof(PostingIterator))) PostingIterator(PostingFormatOption(flag_), session_pool);
    u32 state_pool_size = 0; // TODO
    iter->Init(seg_postings, state_pool_size);
    return iter;
}
} // namespace infinity
