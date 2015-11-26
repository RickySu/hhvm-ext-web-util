#ifndef WEB_UTIL_HTTP_PARSER_H
#define WEB_UTIL_HTTP_PARSER_H
#include "ext.h"
#include <http_parser.h>
#include <hphp/runtime/ext/std/ext_std_variable.h>
#define CONTENT_TYPE_NONE       0
#define CONTENT_TYPE_URLENCODE  1
#define CONTENT_TYPE_JSONENCODE 2
#define CONTENT_TYPE_MULTIPART  3
#define TYPE_REQUEST    0
#define TYPE_RESPONSE   1

#define TYPE_MULTIPART_HEADER   0
#define TYPE_MULTIPART_CONTENT  1

#define REGISTER_HTTP_PARSER_CONSTANT(name) \
    Native::registerClassConstant<KindOfInt64>(s_web_util_http_parser.get(), \
        makeStaticString(#name), \
        name)

namespace HPHP {
    
    typedef struct http_parser_ext_s: public http_parser{
        ObjectData *http_parser_object_data;
        String url;
        String Header;
        String Field;
        String Body;
        String Delimiter;
        String DelimiterClose;
        String MultipartHeaderData;
        int contentType = CONTENT_TYPE_NONE;
        bool headerEnd;
        bool multipartHeader;
        bool multipartEnd;
    } http_parser_ext;
                
    class web_util_HttpParserData {
        public:    
            enum http_parser_type parserType;
            http_parser_ext *parser = NULL;
            Variant onHeaderParsedCallback;
            Variant onBodyParsedCallback;
            Variant onContentPieceCallback;
            Variant onMultipartCallback;
            void reset();
            web_util_HttpParserData();
            void sweep();
            ~web_util_HttpParserData();
    };
    
    static int on_message_begin(http_parser_ext *parser);
    static int on_message_complete(http_parser_ext *parser);
    static int on_headers_complete_request(http_parser_ext *parser);
    static int on_headers_complete_response(http_parser_ext *parser);
    static int on_status(http_parser_ext *parser, const char *buf, size_t len);
    static int on_url(http_parser_ext *parser, const char *buf, size_t len);
    static int on_header_field(http_parser_ext *parser, const char *buf, size_t len);
    static int on_header_value(http_parser_ext *parser, const char *buf, size_t len);
    static int on_response_body(http_parser_ext *parser, const char *buf, size_t len);
    static int on_request_body(http_parser_ext *parser, const char *buf, size_t len);
}

#endif
