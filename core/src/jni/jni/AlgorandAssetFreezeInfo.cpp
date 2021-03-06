// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from idl.djinni

#include "AlgorandAssetFreezeInfo.hpp"  // my header
#include "Marshal.hpp"

namespace djinni_generated {

AlgorandAssetFreezeInfo::AlgorandAssetFreezeInfo() = default;

AlgorandAssetFreezeInfo::~AlgorandAssetFreezeInfo() = default;

auto AlgorandAssetFreezeInfo::fromCpp(JNIEnv* jniEnv, const CppType& c) -> ::djinni::LocalRef<JniType> {
    const auto& data = ::djinni::JniClass<AlgorandAssetFreezeInfo>::get();
    auto r = ::djinni::LocalRef<JniType>{jniEnv->NewObject(data.clazz.get(), data.jconstructor,
                                                           ::djinni::get(::djinni::String::fromCpp(jniEnv, c.assetId)),
                                                           ::djinni::get(::djinni::String::fromCpp(jniEnv, c.frozenAddress)),
                                                           ::djinni::get(::djinni::Bool::fromCpp(jniEnv, c.frozen)))};
    ::djinni::jniExceptionCheck(jniEnv);
    return r;
}

auto AlgorandAssetFreezeInfo::toCpp(JNIEnv* jniEnv, JniType j) -> CppType {
    ::djinni::JniLocalScope jscope(jniEnv, 4);
    assert(j != nullptr);
    const auto& data = ::djinni::JniClass<AlgorandAssetFreezeInfo>::get();
    return {::djinni::String::toCpp(jniEnv, (jstring)jniEnv->GetObjectField(j, data.field_assetId)),
            ::djinni::String::toCpp(jniEnv, (jstring)jniEnv->GetObjectField(j, data.field_frozenAddress)),
            ::djinni::Bool::toCpp(jniEnv, jniEnv->GetBooleanField(j, data.field_frozen))};
}

}  // namespace djinni_generated
