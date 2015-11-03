#include "web_util_http_parser.h"
#include "hphp/runtime/server/http-protocol.h"

#ifdef HAVE_HHVM_EXT_JSON_H
#include "hphp/runtime/ext/json/ext_json.h"
#else
namespace HPHP{
    Variant HHVM_FUNCTION(json_decode, const String& json, bool assoc = false, int64_t depth = 512, int64_t options = 0);
}
#endif

namespace HPHP {

#if HHVM_API_VERSION < 20140702L
    using JIT::VMRegAnchor;
#endif

    const static StaticString 
        s_header("Header"),
        s_request("Request"),
        s_response("Response"),
        s_query("Query"),
        s_content("Content"),
        s_content_parsed("Content-Parsed"),
        s_content_type("Content-Type"),
        s_cookie("Cookie"),
        s_parsedData("parsedData"),
        s_status_code("Status-Code"),
        s_protocol("Protocol"),
        s_protocol_version("Protocol-Version"),
        s_application_x_www_form_urlencode("application/x-www-form-urlencoded"),
        s_method("Method"),
        s_target("Target"),
        s_path("Path"),
        s_param("Param"),
        s_http("HTTP")
    ;   

    ALWAYS_INLINE int multipartCallback(http_parser_ext *parser, String content, int type){
        Variant return_value = 0;
        auto* data = Native::data<web_util_HttpParserData>(parser->http_parser_object_data);
        if(!data->onMultipartCallback.isNull()){
             return_value = vm_call_user_func(data->onMultipartCallback, make_packed_array(content, type));
        }
        return !return_value.toBoolean();
    }
    
    ALWAYS_INLINE int sendData(http_parser_ext *parser, String data){
        int64_t headerpos;
        if(parser->multipartHeader){
            parser->MultipartHeaderData += data;
            if((headerpos = parser->MultipartHeaderData.find("\r\n\r\n")) >= 0){
                if(multipartCallback(parser, parser->MultipartHeaderData.substr(0, headerpos), TYPE_MULTIPART_HEADER)){
                    return 1;
                }
                if(multipartCallback(parser, parser->MultipartHeaderData.substr(headerpos + 4), TYPE_MULTIPART_CONTENT)){
                    return 1;
                }
                parser->MultipartHeaderData.clear();
                parser->multipartHeader = false;
            }
            return 0;            
        }
        return multipartCallback(parser, data, TYPE_MULTIPART_CONTENT);
    }

    ALWAYS_INLINE int flushBufferData(http_parser_ext *parser) {
        int64_t pos, end_pos;
    
        if(parser->multipartEnd){
            parser->Body.clear();
            return 0;
        }
        
        while(true){
            if(parser->Body.size() < parser->DelimiterClose.size()) {
                return 0;
            }

            pos = parser->Body.find(parser->Delimiter);
            end_pos = parser->Body.find(parser->DelimiterClose);
            
            if(end_pos >= 0){
                parser->Body = parser->Body.substr(0, end_pos);
                parser->multipartEnd = true;
            }
            
            if(pos >= 0){
                if(pos > 0){
                    if(sendData(parser, parser->Body.substr(0, pos))){
                        return 1;
                    }
                }
                parser->Body = parser->Body.substr(pos + parser->Delimiter.size());
                parser->multipartHeader = true;
                if(parser->Body.find(parser->Delimiter) >= 0){
                    continue;
                }
            }
            
            if(parser->multipartEnd){
                 if(sendData(parser, parser->Body)){
                     return 1;
                 }
                 parser->Body.clear();
                 break;
            }
            
            if(pos < 0 && parser->Body.size() > parser->Delimiter.size()){
                if(sendData(parser, parser->Body.substr(0, parser->Body.size() - parser->Delimiter.size()))){
                    return 1;
                }
                parser->Body = parser->Body.substr(parser->Body.size() - parser->Delimiter.size());
            }
            break;
        }

        return 0;
    }
     
    ALWAYS_INLINE void parseContentType(http_parser_ext *parser){
        Array parsedData = parser->http_parser_object_data->o_get(s_parsedData, false, s_web_util_http_parser).toArray();
        Array parsedData_header = parsedData[s_header].toArray();
        String contentType = parsedData_header[s_content_type].toString();
        if(contentType == s_application_x_www_form_urlencode){
            parser->contentType = CONTENT_TYPE_URLENCODE;
            return;
        }
        if(contentType.find("application/json", 0, false) == 0){
            parser->contentType = CONTENT_TYPE_JSONENCODE;
            return;
        }
        if(contentType.find("multipart/form-data", 0, false) == 0){
            parser->contentType = CONTENT_TYPE_MULTIPART;
            int pos = contentType.find("boundary=", 0, false);
            if(pos>=0){
                parser->Delimiter = "\r\n--" + contentType.substr(pos + sizeof("boundary=") - 1) + "\r\n";
                parser->DelimiterClose = "\r\n--" + contentType.substr(pos + sizeof("boundary=") - 1) + StaticString("--");
            }
            parser->MultipartHeaderData.clear();
            parser->Body = "\r\n";
            parser->multipartEnd = false;
            parser->multipartHeader = false;
            return;
        }
    }

    ALWAYS_INLINE void parseResponse(http_parser_ext *parser){
        Array parsedData = parser->http_parser_object_data->o_get(s_parsedData, false, s_web_util_http_parser).toArray();
        Array parsedData_response = parsedData[s_response].toArray();
        parsedData_response.set(s_status_code, (int64_t)parser->status_code);
        parsedData_response.set(s_protocol, s_http); 
        parsedData_response.set(s_protocol_version, String(parser->http_major)+"."+String(parser->http_minor));
        parsedData.set(s_response, parsedData_response);
        parser->http_parser_object_data->o_set(s_parsedData, parsedData, s_web_util_http_parser);
    }

    ALWAYS_INLINE void parseRequest(http_parser_ext *parser){
        struct http_parser_url parser_url;
        Array parsedData = parser->http_parser_object_data->o_get(s_parsedData, false, s_web_util_http_parser).toArray();
        Array parsedData_request = parsedData[s_request].toArray();
        Array parsedData_query = parsedData[s_query].toArray();
        Array params;
        parsedData_request.set(s_method, http_method_str((enum http_method)parser->method));
        parsedData_request.set(s_target, parser->url);
        parsedData_request.set(s_protocol, s_http); 
        parsedData_request.set(s_protocol_version, String(parser->http_major)+"."+String(parser->http_minor));
        parsedData.set(s_request, parsedData_request);

        http_parser_parse_url(parser->url.data(),parser->url.size(), 0, &parser_url);
        parsedData_query.set(s_path, parser->url.substr(parser_url.field_data[UF_PATH].off, parser_url.field_data[UF_PATH].len));
        if(parser_url.field_data[UF_QUERY].len){
            String query;
            query = parser->url.substr(parser_url.field_data[UF_QUERY].off, parser_url.field_data[UF_QUERY].len);
            HttpProtocol::DecodeParameters(params, query.data(), query.size());
        }
        parsedData_query.set(s_param, params);
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
            parser->Delimiter.clear();
            parser->DelimiterClose.clear();
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

    ALWAYS_INLINE Variant parseBody(http_parser_ext *parser){
        Array parsedContent;
        switch(parser->contentType){
            case CONTENT_TYPE_URLENCODE:
                HttpProtocol::DecodeParameters(parsedContent, parser->Body.data(), parser->Body.size());
                break;
            case CONTENT_TYPE_JSONENCODE:
                parsedContent = HHVM_FN(json_decode)(parser->Body, true);
                break;
            default:
                return parser->Body;
        }
        return parsedContent;
    }
    
    static http_parser_settings parser_response_settings = {
        (http_cb) on_message_begin,    //on_message_begin
        (http_data_cb) on_url,    //on_url
        (http_data_cb) on_status,    //on_status
        (http_data_cb) on_header_field,    //on_header_field
        (http_data_cb) on_header_value,    //on_header_value
        (http_cb) on_headers_complete_response,    //on_headers_complete
        (http_data_cb) on_response_body,    //on_body      
        (http_cb) on_message_complete     //on_message_complete
    };

    static http_parser_settings parser_request_settings = {
        (http_cb) on_message_begin,    //on_message_begin
        (http_data_cb) on_url,    //on_url
        (http_data_cb) on_status,    //on_status
        (http_data_cb) on_header_field,    //on_header_field
        (http_data_cb) on_header_value,    //on_header_value
        (http_cb) on_headers_complete_request,    //on_headers_complete
        (http_data_cb) on_request_body,    //on_body      
        (http_cb) on_message_complete     //on_message_complete
    };
    
    void web_util_HttpParserData::reset(){
            parser->url.clear();
            parser->Header.clear();
            parser->Field.clear();
            parser->Body.clear();
            parser->Delimiter.clear();
            parser->DelimiterClose.clear();
            parser->headerEnd = false;
    }

    web_util_HttpParserData::web_util_HttpParserData(){
        onHeaderParsedCallback.setNull();
        onBodyParsedCallback.setNull();
        onContentPieceCallback.setNull();
        onMultipartCallback.setNull();
        parser = new http_parser_ext;
        reset();
    }
    
    web_util_HttpParserData::~web_util_HttpParserData(){
        sweep();
    }
    
    void web_util_HttpParserData::sweep() {
        if(parser){
            reset();
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
        Variant parsedBody = parseBody(parser);
        parsedData = parser->http_parser_object_data->o_get(s_parsedData, false, s_web_util_http_parser).toArray();
        if(!data->onBodyParsedCallback.isNull()){
             vm_call_user_func(data->onBodyParsedCallback, make_packed_array(parsedBody));
        }
        return 0;
    }
    
    static int on_headers_complete_response(http_parser_ext *parser){
        auto* data = Native::data<web_util_HttpParserData>(parser->http_parser_object_data);
        Array parsedData;
        resetHeaderParser(parser);
        parseResponse(parser);
        parseContentType(parser);
        parseCookie(parser);
        parsedData = parser->http_parser_object_data->o_get(s_parsedData, false, s_web_util_http_parser).toArray();
        if(!data->onHeaderParsedCallback.isNull()){
             vm_call_user_func(data->onHeaderParsedCallback, make_packed_array(parsedData));
        }
        return 0;
    }

    static int on_headers_complete_request(http_parser_ext *parser){
        auto* data = Native::data<web_util_HttpParserData>(parser->http_parser_object_data);
        Array parsedData;
        resetHeaderParser(parser);
        parseRequest(parser);
        parseContentType(parser);
        parseCookie(parser);
        parsedData = parser->http_parser_object_data->o_get(s_parsedData, false, s_web_util_http_parser).toArray();
        if(!data->onHeaderParsedCallback.isNull()){
             vm_call_user_func(data->onHeaderParsedCallback, make_packed_array(parsedData));
        }
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
    
    static int on_response_body(http_parser_ext *parser, const char *buf, size_t len){
        auto* data = Native::data<web_util_HttpParserData>(parser->http_parser_object_data);
        if(!data->onContentPieceCallback.isNull()){
             return !vm_call_user_func(data->onContentPieceCallback, make_packed_array(String(buf, len, CopyString))).toBoolean();
        }
        parser->Body += String(buf, len, CopyString);
        return 0;
    }

    
    static int on_request_body(http_parser_ext *parser, const char *buf, size_t len){
        auto* data = Native::data<web_util_HttpParserData>(parser->http_parser_object_data);
        if(!data->onContentPieceCallback.isNull()){
             return !vm_call_user_func(data->onContentPieceCallback, make_packed_array(String(buf, len, CopyString))).toBoolean();
        }

        parser->Body += String(buf, len, CopyString);

        if(parser->contentType == CONTENT_TYPE_MULTIPART){
            return flushBufferData(parser);
        }

        return 0;
    }

    static void HHVM_METHOD(WebUtil_HttpParser, feed, const String &feedData) {
        VMRegAnchor _;
        auto* data = Native::data<web_util_HttpParserData>(this_);
        http_parser_execute(data->parser, (data->parserType == HTTP_REQUEST ? &parser_request_settings:&parser_response_settings), feedData.data(), feedData.size());
    }
    
    static Variant HHVM_METHOD(WebUtil_HttpParser, setOnHeaderParsedCallback, const Variant &callback) {
        auto* data = Native::data<web_util_HttpParserData>(this_);
        Variant oldCallback = data->onHeaderParsedCallback;
        data->onHeaderParsedCallback = callback;
        return oldCallback;
    }

    static Variant HHVM_METHOD(WebUtil_HttpParser, setOnBodyParsedCallback, const Variant &callback) {
        auto* data = Native::data<web_util_HttpParserData>(this_);
        Variant oldCallback = data->onBodyParsedCallback;
        data->onBodyParsedCallback = callback;
        return oldCallback;
    }

    static Variant HHVM_METHOD(WebUtil_HttpParser, setOnContentPieceCallback, const Variant &callback) {
        auto* data = Native::data<web_util_HttpParserData>(this_);
        Variant oldCallback = data->onContentPieceCallback;
        data->onContentPieceCallback = callback;
        return oldCallback;
    }

    static Variant HHVM_METHOD(WebUtil_HttpParser, setOnMultipartCallback, const Variant &callback) {
        auto* data = Native::data<web_util_HttpParserData>(this_);
        Variant oldCallback = data->onMultipartCallback;
        data->onMultipartCallback = callback;
        return oldCallback;
    }

    static void HHVM_METHOD(WebUtil_HttpParser, __construct, int64_t type) {
        auto* data = Native::data<web_util_HttpParserData>(this_);
        data->parser->http_parser_object_data = this_;
        if(type == TYPE_REQUEST){
            data->parserType = HTTP_REQUEST;
        }
        else{
            data->parserType = HTTP_RESPONSE;
        }
        http_parser_init(data->parser, data->parserType);
    }

    static void HHVM_METHOD(WebUtil_HttpParser, reset) {
        auto* data = Native::data<web_util_HttpParserData>(this_);
        data->reset();
        http_parser_init(data->parser, data->parserType);
    }
    
    void web_utilExtension::initHttpParser() {
        HHVM_ME(WebUtil_HttpParser, __construct);
        HHVM_MALIAS(WebUtil\\Parser\\HttpParser, __construct, WebUtil_HttpParser, __construct);
        HHVM_ME(WebUtil_HttpParser, feed);
        HHVM_MALIAS(WebUtil\\Parser\\HttpParser, feed, WebUtil_HttpParser, feed);
        HHVM_ME(WebUtil_HttpParser, reset);
        HHVM_MALIAS(WebUtil\\Parser\\HttpParser, reset, WebUtil_HttpParser, reset);
        HHVM_ME(WebUtil_HttpParser, setOnHeaderParsedCallback);
        HHVM_MALIAS(WebUtil\\Parser\\HttpParser, setOnHeaderParsedCallback, WebUtil_HttpParser, setOnHeaderParsedCallback);
        HHVM_ME(WebUtil_HttpParser, setOnBodyParsedCallback);
        HHVM_MALIAS(WebUtil\\Parser\\HttpParser, setOnBodyParsedCallback, WebUtil_HttpParser, setOnBodyParsedCallback);
        HHVM_ME(WebUtil_HttpParser, setOnContentPieceCallback);
        HHVM_MALIAS(WebUtil\\Parser\\HttpParser, setOnContentPieceCallback, WebUtil_HttpParser, setOnContentPieceCallback);
        HHVM_ME(WebUtil_HttpParser, setOnMultipartCallback);
        HHVM_MALIAS(WebUtil\\Parser\\HttpParser, setOnMultipartCallback, WebUtil_HttpParser, setOnMultipartCallback);
        REGISTER_HTTP_PARSER_CONSTANT(TYPE_REQUEST);
        REGISTER_HTTP_PARSER_CONSTANT(TYPE_RESPONSE);
        Native::registerNativeDataInfo<web_util_HttpParserData>(s_web_util_http_parser.get());
    }
}
