#ifndef EXT_H_
#define EXT_H_
#include "hhvm_include.h"
#include "common.h"
#include "util.h"


namespace HPHP
{
    class web_utilExtension: public Extension
    {
        public:
            web_utilExtension(): Extension("web_util"){}
            virtual void moduleInit()
            {
                initR3();
                initHttpParser();
                loadSystemlib();
            }
        private:
            void initR3();
            void initHttpParser();
    };
}
#endif
