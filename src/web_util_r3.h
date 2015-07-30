#ifndef WEB_UTIL_R3_H
#define WEB_UTIL_R3_H
#include "ext.h"
#include <r3/r3.h>

namespace HPHP {
    class web_util_R3Data {
        private:
            node *n = NULL;
        public:
            ALWAYS_INLINE node *create(int count){
                n = r3_tree_create(count);
                return n;
            }
            ~web_util_R3Data(){
               if(n){
                   r3_tree_free(n);
                   n = NULL;
               }     
            }
            ALWAYS_INLINE node *getNode(){
                return n;
            }
    };
}

#endif