#include "web_util_r3.h"
#include "hphp/runtime/ext/array/ext_array.h"

namespace HPHP {

    static void findSlug(const char *pattern, int pattern_len, Array &names)
    {
        char *err = NULL; 
        char *placeholder;
        char *name;    
        int placeholder_len = 0, name_len = 0;
        int slug_cnt = r3_slug_count(pattern, pattern_len, &err);
        int i;

        if(slug_cnt <= 0){
            return;
        }
        
        placeholder = (char *)pattern;
        for(i=0; i<slug_cnt; i++){
            placeholder = r3_slug_find_placeholder(placeholder, &placeholder_len);
            name = r3_slug_find_name(placeholder, &name_len);
            names.append(String(name, name_len, CopyString)); 
            placeholder++;
        }
    }
  
    web_util_R3Data::web_util_R3Data(){
        n = NULL;
    }
    
    web_util_R3Data::~web_util_R3Data(){
        sweep();
    }
    void web_util_R3Data::sweep() {
        if(n){
            r3_tree_free(n);
            n = NULL;
        }
    }

    static bool HHVM_METHOD(WebUtil_R3, compile) {
        auto* data = Native::data<web_util_R3Data>(this_);
        Array routes = this_->o_get("routes", false, s_web_util_r3).toArray();
        Array names, route;
        node *n = data->create(routes.size());
        for (ArrayIter iter(routes); iter; ++iter) {
            Variant key(iter.first());
            route = routes[key].toArray();
            String pattern = route[0].toString();
            int64_t method = route[1].toInt64();
            names = Array();
            findSlug(pattern.c_str(), pattern.size(), names);
            if(names.size()){
                route.append(names);
                routes.set(key, route);
            }
            if(r3_tree_insert_routel(n, method, pattern.c_str(), pattern.size(), (void *) route.get()) == NULL) {
                return false;
            }
        }
        
        if(r3_tree_compile(n, NULL) != 0) {
            return false;
        }
        this_->o_set("routes", routes, s_web_util_r3);
        return true;
    }
    
    static Array HHVM_METHOD(WebUtil_R3, match, const String &uri, int64_t method) {
        auto* data = Native::data<web_util_R3Data>(this_);
        ArrayData *route_data;
        Array params;
        Variant r_params;
        node *n = data->getNode();
        Array ret;

        match_entry * entry = match_entry_create(uri.c_str());        
        entry->request_method = method;
        route *matched_route = r3_tree_match_route(n, entry);
                
        if(matched_route != NULL) {
            
            
            route_data = (ArrayData *) matched_route->data;
            for(int i=0;i<entry->vars->len;i++){
                params.append(Variant(entry->vars->tokens[i]));
            }
            
            if(params.size()){
                r_params = HHVM_FN(array_combine)(route_data->get(3), params);
                ret = make_packed_array(route_data->get(2), r_params);
            }
            else{
                ret = make_packed_array(route_data->get(2));
            }

        }
        
        match_entry_free(entry);
        //return Array(route_data);
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
        HHVM_ME(WebUtil_R3, compile);
        HHVM_ME(WebUtil_R3, match);
        HHVM_MALIAS(WebUtil\\R3, compile, WebUtil_R3, compile);
        HHVM_MALIAS(WebUtil\\R3, match, WebUtil_R3, match);        
        Native::registerNativeDataInfo<web_util_R3Data>(s_web_util_r3.get());
    }
}
