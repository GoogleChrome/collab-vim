// Copyright 2014 Google Inc.
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

/*
 * Contains utility functions for collaborate.
 * Most of these are here for the sake of testing and will not be included in
 * the rest of the vim project. Functions here aren't thread-safe unless noted.
 */

#ifndef VIM_COLLAB_UTIL_H_
#define VIM_COLLAB_UTIL_H_

#include "collab_structs.h"

/*
 * Dequeues a single collabedit_T.
 */
collabedit_T* collab_dequeue(editqueue_T *queue);

#endif // VIM_COLLAB_UTIL_H_

