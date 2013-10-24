/*  
 * Copyright (c) 2009 Carnegie Mellon University. 
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://www.graphlab.ml.cmu.edu
 *
 */


#include <graphlab/util/lock_free_pool.hpp>
#include <graphlab/rpc/dc_compile_parameters.hpp>
#include <vector>
#include <utility>
namespace graphlab {
namespace dc_impl {
struct pool_manager {
  pthread_key_t thlocalkey;
  typedef std::pair<oarchive, bool> thlocal_type;
  static void pth_deleter(void* p) {
    if (p != NULL) {
      thlocal_type* arc = (thlocal_type*)(p);
      if (arc->first.buf) free(arc->first.buf);
      delete arc;
    }
  }
  pool_manager(){
    pthread_key_create(&thlocalkey, &pth_deleter);
  }

  ~pool_manager() {
    pthread_key_delete(thlocalkey);
  }
};

static pool_manager pool;

oarchive* oarchive_from_pool() {
  void* ptr = pthread_getspecific(pool.thlocalkey);
  pool_manager::thlocal_type* p = (pool_manager::thlocal_type*)(ptr);
  if (p == NULL) {
    p = new pool_manager::thlocal_type;
    p->second = false;
    p->first.buf = (char*)malloc(BUFFER_RELINQUISH_LIMIT);
    p->first.off = 0;
    p->first.len = BUFFER_RELINQUISH_LIMIT;
    pthread_setspecific(pool.thlocalkey, (void*)p);
  }
  if (p->second) return new oarchive;
  p->second = true;
  return &(p->first);
}

void release_oarchive_to_pool(oarchive* oarc) {
  void* ptr = pthread_getspecific(pool.thlocalkey);
  pool_manager::thlocal_type* p = (pool_manager::thlocal_type*)(ptr);
  if ((p != NULL) && &(p->first) == oarc) {
    oarc->off = 0;
    p->second = false;
  } else {
    if (oarc->buf) free(oarc->buf);
    delete oarc;
  }
}




} // dc_impl
} // graphlab
