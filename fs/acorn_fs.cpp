// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "acorn_fs.hpp"

namespace acorn {

static size_t dir_count  {0U};
static size_t file_count {0U};

static Leaf *top {nullptr};
static int jobCount {0};

void recursive_tree_print(Leaf *top, int depth) {
  const int indent = (depth * 3);
  for (Leaf *leaf=top; leaf; leaf=leaf->next) {
    if (leaf->child) {
      printf("%*c-[ %s ]\n", indent, '+', leaf->name.c_str());
      recursive_tree_print(leaf->child, depth+1);
    } else
      printf("%*c %s (%llu)\n", indent, '+', leaf->name.c_str(), leaf->size);
    }
}

void recursive_fs_gather(fs::Disk_ptr disk, Leaf *node, const fs::Dirent &path, int depth) {
  jobCount++;
  disk->fs().ls(path, [disk, node, path, depth](fs::error_t err, const fs::FileSystem::dirvec_t &entries) {
    if( err ) {
      printf("cstat (recurse): ERROR: %s\n", err.to_string().c_str());
      return;
    }
    for (auto&& entry : *entries) {
      if (entry.name() not_eq "."  and entry.name() not_eq "..") {
        Leaf *leaf = new Leaf (entry.name(), entry.size());
        if (entry.is_dir()) {
          ++dir_count;
          recursive_fs_gather(disk, leaf, entry, depth+1);
        } else
          ++file_count;
        leaf->next = node->child;
        node->child = leaf;
      }
    }
    jobCount--;

    if (0 == jobCount)
    {
      printf("\n"
       "================================================================================\n"
       "STATIC CONTENT LISTING\n"
       "================================================================================\n");
      recursive_tree_print(top, 1);
      printf("\n%u directories, %u files\n", dir_count, file_count);
      printf("================================================================================\n");
      dir_count  = 0U;
      file_count = 0U;

      // ...and then destroy it.
      free_tree(top);
      top = nullptr;
    }
  });
}

void list_static_content(fs::Disk_ptr disk) {
  top = new Leaf("/", 0);
  disk->fs().ls("/", [disk](fs::error_t err, const fs::FileSystem::dirvec_t &entries) {
    if (err) {
      printf("cstat (init): ERROR: %s\n", err.to_string().c_str());
      return;
    }
    for (auto&& entry : *entries) {
      Leaf *node = new Leaf("/"+entry.name(), entry.size());
      node->next = top->child;
      top->child = node;
      recursive_fs_gather(disk, node, entry, 1);
    }
  });
}

void free_tree(Leaf *top) {
  Leaf *next;
  for (Leaf *leaf=top; leaf; leaf=next) {
    next = leaf->next;
    if (leaf->child)
      free_tree(leaf->child);
    delete leaf;
  }
}

} //< namespace acorn
