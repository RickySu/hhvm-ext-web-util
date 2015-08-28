#ifndef WEB_UTIL_R3_H
#define WEB_UTIL_R3_H
#include "ext.h"
#include <r3/r3.h>

namespace HPHP {
    class web_util_R3Data {
        private:
            node *n;
        public:
            web_util_R3Data();
            ALWAYS_INLINE node *create(int count){
                sweep();
                n = r3_tree_create(count);
                return n;
            }
            void sweep();
            ~web_util_R3Data();
            ALWAYS_INLINE node *getNode(){
                return n;
            }
    };
}

#endif
