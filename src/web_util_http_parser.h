#ifndef WEB_UTIL_R3_H
#define WEB_UTIL_R3_H
#include "ext.h"
#include <http_parser.h>

namespace HPHP {

    typedef struct http_parser_ext_s: public http_parser{
        ObjectData *http_parser_object_data;
        String url;
        String Header;
        String Field;
        bool headerEnd;
    } http_parser_ext;
                
    class web_util_HttpParserData {
        public:    
            http_parser_ext *parser = NULL;
            Variant onParsedCallback;
            web_util_HttpParserData();
            void sweep();
            ~web_util_HttpParserData();
    };
    
    static int on_message_begin(http_parser_ext *parser);
    static int on_message_complete(http_parser_ext *parser);
    static int on_headers_complete(http_parser_ext *parser);
    static int on_status(http_parser_ext *parser, const char *buf, size_t len);
    static int on_url(http_parser_ext *parser, const char *buf, size_t len);
    static int on_header_field(http_parser_ext *parser, const char *buf, size_t len);
    static int on_header_value(http_parser_ext *parser, const char *buf, size_t len);
    static int on_body(http_parser_ext *parser, const char *buf, size_t len);
}

#endif
