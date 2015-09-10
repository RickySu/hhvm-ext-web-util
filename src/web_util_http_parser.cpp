#include "web_util_http_parser.h"
#include "hphp/runtime/ext/array/ext_array.h"

namespace HPHP {
    const static StaticString 
        s_header("header"),
        s_request("request"),
        s_query("query"),
        s_parsedData("parsedData")
    ;    
    
    ALWAYS_INLINE void parseRequest(http_parser_ext *parser){
            struct http_parser_url parser_url;
            Array parsedData = parser->http_parser_object_data->o_get(s_parsedData, false, s_web_util_http_parser).toArray();
            Array parsedData_request = parsedData[s_request].toArray();
            Array parsedData_query = parsedData[s_query].toArray();
            parsedData_request.set(String("method"), http_method_str((enum http_method)parser->method));
            parsedData_request.set(String("target"), parser->url);
            parsedData_request.set(String("protocol"), "HTTP"); 
            parsedData_request.set(String("protocol-version"), String(parser->http_major)+"."+String(parser->http_minor));
            parsedData.set(s_request, parsedData_request);
            
            http_parser_parse_url(parser->url.data(),parser->url.size(), 0, &parser_url);
            printf("u:%s\n", parser->url.substr(parser_url.field_data[UF_QUERY].off, parser_url.field_data[UF_QUERY].len).c_str());
            parsedData.set(s_query, parsedData_query);
            parser->http_parser_object_data->o_set(s_parsedData, parsedData, s_web_util_http_parser);
    }
    
    ALWAYS_INLINE void resetHeaderParser(http_parser_ext *parser){
        if(parser->headerEnd){
            Array parsedData = parser->http_parser_object_data->o_get(s_parsedData, false, s_web_util_http_parser).toArray();
            Array parsedData_header = parsedData[s_header].toArray();
            parsedData_header.set(parser->Header, parser->Field);
            parsedData.set(s_header, parsedData_header);
            parser->http_parser_object_data->o_set(s_parsedData, parsedData, s_web_util_http_parser);
            parser->Header.clear();
            parser->Field.clear();
            parser->headerEnd = false;
        }    
    }
    
    static http_parser_settings parser_settings = {
        (http_cb) on_message_begin,    //on_message_begin
        (http_data_cb) on_url,    //on_url
        (http_data_cb) on_status,    //on_status
        (http_data_cb) on_header_field,    //on_header_field
        (http_data_cb) on_header_value,    //on_header_value
        (http_cb) on_headers_complete,    //on_headers_complete
        (http_data_cb) on_body,    //on_body      
        (http_cb) on_message_complete     //on_message_complete
    };

    web_util_HttpParserData::web_util_HttpParserData(){
        onParsedCallback.setNull();        
        parser = new http_parser_ext;
        parser->headerEnd = false;
        http_parser_init(parser, HTTP_REQUEST);
    }
    
    web_util_HttpParserData::~web_util_HttpParserData(){
        sweep();
    }
    
    void web_util_HttpParserData::sweep() {
        if(parser){
            delete parser;
            parser = NULL;
        }
    }
    
    static int on_message_begin(http_parser_ext *parser){
        printf("message begin\n");
        return 0;
    }
    
    static int on_message_complete(http_parser_ext *parser){
        printf("message complete\n");
        return 0;
    }
    
    static int on_headers_complete(http_parser_ext *parser){
        resetHeaderParser(parser);
        parseRequest(parser);
        return 0;
    }
    
    static int on_status(http_parser_ext *parser, const char *buf, size_t len){
        return 0;
    }
    
    static int on_url(http_parser_ext *parser, const char *buf, size_t len){
        parser->url += String(buf, len, CopyString);
        return 0;
    }
    
    static int on_header_field(http_parser_ext *parser, const char *buf, size_t len){
        resetHeaderParser(parser);
        parser->Header += String(buf, len, CopyString);        
        return 0;
    }
    
    static int on_header_value(http_parser_ext *parser, const char *buf, size_t len){
        parser->headerEnd = true;
        parser->Field += String(buf, len, CopyString);
        return 0;
    }
    
    static int on_body(http_parser_ext *parser, const char *buf, size_t len){
        return 0;
    }

    static void HHVM_METHOD(WebUtil_HttpParser, feed, const String &feedData) {
        auto* data = Native::data<web_util_HttpParserData>(this_);
        http_parser_execute(data->parser, &parser_settings, feedData.data(), feedData.size());
    }
    
    static void HHVM_METHOD(WebUtil_HttpParser, __construct) {
        auto* data = Native::data<web_util_HttpParserData>(this_);
        data->parser->http_parser_object_data = this_;
    }
    
    void web_utilExtension::initHttpParser() {
        HHVM_ME(WebUtil_HttpParser, __construct);
        HHVM_MALIAS(WebUtil\\Parser\\HttpParser, __construct, WebUtil_HttpParser, __construct);
        HHVM_ME(WebUtil_HttpParser, feed);
        HHVM_MALIAS(WebUtil\\Parser\\HttpParser, feed, WebUtil_HttpParser, feed);
        Native::registerNativeDataInfo<web_util_HttpParserData>(s_web_util_http_parser.get());
    }
}
