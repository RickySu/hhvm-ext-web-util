#ifndef UTIL_H
#define	UTIL_H

#define REGISTER_WEB_UTIL_CONSTANT(class, name) \
    Native::registerClassConstant<KindOfInt64>(class.get(), \
                makeStaticString(#name), \
                            name)
namespace HPHP
{

    ALWAYS_INLINE ObjectData *makeObject(const String &ClassName, const Array arg, bool init){
        Class* cls = Unit::lookupClass(ClassName.get());
        ObjectData* ret;
        Object o = ret = ObjectData::newInstance(cls);        
        if(init){
            TypedValue dummy;
            g_context->invokeFunc(&dummy, cls->getCtor(), arg, ret);
        }
        return o.detach();
    }

    ALWAYS_INLINE ObjectData *makeObject(const String &ClassName, bool init = true){
        return makeObject(ClassName, Array::Create(), init);
    }

    ALWAYS_INLINE ObjectData *makeObject(const char *ClassName, const Array arg){
        return makeObject(String(ClassName), arg, true);
    }

    ALWAYS_INLINE ObjectData *makeObject(const char *ClassName, bool init = true){
        return makeObject(String(ClassName), Array::Create(), init);
    }
    
    ALWAYS_INLINE ObjectData* getThisOjectData(const Object &obj){
        return obj.get();
    }
    
    ALWAYS_INLINE ObjectData* getThisObjectData(ObjectData *objdata){
        return objdata;
    }    
}

#endif	/* UTIL_H */
