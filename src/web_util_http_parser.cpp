#include "web_util_http_parser.h"
#include "hphp/runtime/server/http-protocol.h"
#include "hphp/runtime/ext/json/ext_json.h"

namespace HPHP {

#if HHVM_API_VERSION < 20140702L
    using JIT::VMRegAnchor;
#endif

    const static StaticString 
        s_header("Header"),
        s_request("Request"),
        s_query("Query"),
        s_content("Content"),
        s_content_parsed("Content-Parsed"),
        s_cookie("Cookie"),
        s_parsedData("parsedData")
    ;   
     
    ALWAYS_INLINE void parseContentType(http_parser_ext *parser){
        Array parsedData = parser->http_parser_object_data->o_get(s_parsedData, false, s_web_util_http_parser).toArray();
        Array parsedData_header = parsedData[s_header].toArray();
        String contentType = parsedData_header[String("Content-Type")].toString();
        if(contentType == String("application/x-www-form-urlencoded")){
            parser->contentType = CONTENT_TYPE_URLENCODE;
            return;
        }
        if(contentType.find("application/json", 0, false) == 0){
            parser->contentType = CONTENT_TYPE_JSONENCODE;
            return;
        }
        if(contentType.find("multipart/form-data", 0, false) == 0){
            parser->contentType = CONTENT_TYPE_MULTIPART;
            return;
        }
    }

    ALWAYS_INLINE void parseRequest(http_parser_ext *parser){
        struct http_parser_url parser_url;
        Array parsedData = parser->http_parser_object_data->o_get(s_parsedData, false, s_web_util_http_parser).toArray();
        Array parsedData_request = parsedData[s_request].toArray();
        Array parsedData_query = parsedData[s_query].toArray();
        Array params;
        parsedData_request.set(String("Method"), http_method_str((enum http_method)parser->method));
        parsedData_request.set(String("Target"), parser->url);
        parsedData_request.set(String("Protocol"), "HTTP"); 
        parsedData_request.set(String("Protocol-Version"), String(parser->http_major)+"."+String(parser->http_minor));
        parsedData.set(s_request, parsedData_request);

        http_parser_parse_url(parser->url.data(),parser->url.size(), 0, &parser_url);
        parsedData_query.set(String("Path"), parser->url.substr(parser_url.field_data[UF_PATH].off, parser_url.field_data[UF_PATH].len));
        if(parser_url.field_data[UF_QUERY].len){
            String query;
            query = parser->url.substr(parser_url.field_data[UF_QUERY].off, parser_url.field_data[UF_QUERY].len);
            HttpProtocol::DecodeParameters(params, query.data(), query.size());
        }
        parsedData_query.set(String("Param"), params);
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

    ALWAYS_INLINE void parseCookie(http_parser_ext *parser){
        int token_equal_pos = 0, token_semi_pos = 0;
        int field_start = 0;
        int i = 0;
        const char *cookieString;
        int cookieString_len;
        Array parsedData = parser->http_parser_object_data->o_get(s_parsedData, false, s_web_util_http_parser).toArray();
        Array parsedData_header = parsedData[s_header].toArray();
        String parsedData_cookie = parsedData_header[s_cookie].toString();
        Array cookie;
        
        if(parsedData_cookie.size() == 0){
            return;
        }
        
        cookieString = parsedData_cookie.c_str();
        cookieString_len = parsedData_cookie.size();
        while(true){
            
            if(cookieString[i] == '='){
                if(token_equal_pos <= token_semi_pos){
                    token_equal_pos = i;
                }
            }
            
            if(cookieString[i] == ';' || i==cookieString_len-1){
                token_semi_pos = i;
                cookie.set(String(&cookieString[field_start], token_equal_pos - field_start, CopyString), String(&cookieString[token_equal_pos+1], token_semi_pos - token_equal_pos - 1, CopyString));
                field_start = i + 2;
            }
            
            if(++i>=cookieString_len){
                break;
            }
        }
        
        parsedData_header.set(s_cookie, cookie);
        parsedData.set(s_header, parsedData_header);
        parser->http_parser_object_data->o_set(s_parsedData, parsedData, s_web_util_http_parser);
    }    
    ALWAYS_INLINE void parseBody(http_parser_ext *parser){
        Array parsedData = parser->http_parser_object_data->o_get(s_parsedData, false, s_web_util_http_parser).toArray();
        Array params;
        parsedData.set(s_content, parser->Body);
        switch(parser->contentType){
            case CONTENT_TYPE_URLENCODE:
                HttpProtocol::DecodeParameters(params, parser->Body.data(), parser->Body.size());
                parsedData.set(s_content_parsed, params);
                break;
            case CONTENT_TYPE_JSONENCODE:
                parsedData.set(s_content_parsed, HHVM_FN(json_decode)(parser->Body, true));
                break;
            default:
                break;
        }
        parser->http_parser_object_data->o_set(s_parsedData, parsedData, s_web_util_http_parser);    
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
        return 0;
    }
    
    static int on_message_complete(http_parser_ext *parser){
        auto* data = Native::data<web_util_HttpParserData>(parser->http_parser_object_data);
        Array parsedData;
        parseBody(parser);
        parsedData = parser->http_parser_object_data->o_get(s_parsedData, false, s_web_util_http_parser).toArray();
        if(!data->onParsedCallback.isNull()){
             vm_call_user_func(data->onParsedCallback, make_packed_array(parsedData));
        }
        return 0;
    }
    
    static int on_headers_complete(http_parser_ext *parser){
        resetHeaderParser(parser);
        parseRequest(parser);
        parseContentType(parser);
        parseCookie(parser);
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
        parser->Body += String(buf, len, CopyString);
        return 0;
    }

    static void HHVM_METHOD(WebUtil_HttpParser, feed, const String &feedData) {
        VMRegAnchor _;
        auto* data = Native::data<web_util_HttpParserData>(this_);
        http_parser_execute(data->parser, &parser_settings, feedData.data(), feedData.size());
    }
    
    static Variant HHVM_METHOD(WebUtil_HttpParser, setOnParsedCallback, const Variant &callback) {
        auto* data = Native::data<web_util_HttpParserData>(this_);
        Variant oldCallback = data->onParsedCallback;
        data->onParsedCallback = callback;
        return oldCallback;
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
        HHVM_ME(WebUtil_HttpParser, setOnParsedCallback);
        HHVM_MALIAS(WebUtil\\Parser\\HttpParser, setOnParsedCallback, WebUtil_HttpParser, setOnParsedCallback);
        Native::registerNativeDataInfo<web_util_HttpParserData>(s_web_util_http_parser.get());
    }
}
