#include "web_util_r3.h"

namespace HPHP {
    
    static Variant HHVM_METHOD(WebUtil_R3, _compile, const Array &routes) {
        auto* data = Native::data<web_util_R3Data>(this_);
        node *n = data->create(routes.size());

        for (ArrayIter iter(routes); iter; ++iter) {
            Variant key(iter.first());
            String pattern = routes.rvalAt(key).toArray().rvalAt(0).toString();
            int64_t method = routes.rvalAt(key).toArray().rvalAt(1).toInt64();
            int64_t idx = key.toInt64Val();
            if(r3_tree_insert_routel(n, method, pattern.c_str(), pattern.size(), (void *) idx) == NULL) {
                return idx;
            }
        }
        
        if(r3_tree_compile(n, NULL) != 0) {
            return -1;
        }
        return true;
    }
    
    static Array HHVM_METHOD(WebUtil_R3, _match, const String &uri, int64_t method) {
        auto* data = Native::data<web_util_R3Data>(this_);
        node *n = data->getNode();
        Array ret;

        match_entry * entry = match_entry_create(uri.c_str());        
        entry->request_method = method;
        route *matched_route = r3_tree_match_route(n, entry);
                
        if(matched_route != NULL) {
            Array params;
            int64_t matched = (int64_t) matched_route->data;
            for(int i=0;i<entry->vars->len;i++){
                params.append(Variant(entry->vars->tokens[i]));
            }
            ret = make_packed_array(matched, params);
        }
        
        match_entry_free(entry);
        return ret;
    }
    
    void web_utilExtension::initR3() {
        REGISTER_WEB_UTIL_CONSTANT(s_web_util_r3, METHOD_GET);
        REGISTER_WEB_UTIL_CONSTANT(s_web_util_r3, METHOD_POST);
        REGISTER_WEB_UTIL_CONSTANT(s_web_util_r3, METHOD_PUT);
        REGISTER_WEB_UTIL_CONSTANT(s_web_util_r3, METHOD_DELETE);
        REGISTER_WEB_UTIL_CONSTANT(s_web_util_r3, METHOD_PATCH);
        REGISTER_WEB_UTIL_CONSTANT(s_web_util_r3, METHOD_HEAD);
        REGISTER_WEB_UTIL_CONSTANT(s_web_util_r3, METHOD_OPTIONS);
        HHVM_ME(WebUtil_R3, _compile);
        HHVM_ME(WebUtil_R3, _match);
        HHVM_MALIAS(WebUtil\\R3, _compile, WebUtil_R3, _compile);
        HHVM_MALIAS(WebUtil\\R3, _match, WebUtil_R3, _match);        
        Native::registerNativeDataInfo<web_util_R3Data>(s_web_util_r3.get());
    }
}
