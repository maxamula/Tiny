#pragma once
#include "entt.hpp"

#define META(Type)                                                                  \
    static void META_register_##Type(decltype(entt::meta_factory<Type>()) meta);    \
                                                                                    \
    static const bool META_dummy_##Type = []() {                                    \
        META_register_##Type(entt::meta_factory<Type>().type(#Type##_hs));           \
        return true;                                                                \
    }();                                                                            \
                                                                                    \
    static void META_register_##Type(decltype(entt::meta_factory<Type>()) meta)
