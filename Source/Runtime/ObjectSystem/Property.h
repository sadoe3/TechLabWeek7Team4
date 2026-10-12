#pragma once

#include <initializer_list>
#include <type_traits>
#include "Container/Array.h"
#include "Core/EngineString.h"
#include "Math/EngineMath.h"
#include "Math/Transform.h"

class UMaterial;
class UFont;

enum class EPropertyType { Unknown, Float, Int, String, Bool, Vector, Vector4, Color, Rotator, Transform, Object, Enum};

template <typename T>
constexpr EPropertyType GetPropertyType()
{
    if constexpr (std::is_enum_v<T>)
    {
        return EPropertyType::Enum;
    }
    else if constexpr (std::is_pointer_v<T> &&
        std::is_base_of_v<UObject, std::remove_pointer_t<T>>)
    {
        return EPropertyType::Object;
    }
    else
    {
        static_assert(sizeof(T) == 0, "Not Valid Type");
        return EPropertyType::Unknown;
    }
}

#define DEFINE_PROPERTY_TYPE(CppType, EnumValue)                    \
    template <> constexpr EPropertyType GetPropertyType<CppType>()  \
    { return EPropertyType::EnumValue; }

DEFINE_PROPERTY_TYPE(int, Int)
DEFINE_PROPERTY_TYPE(uint32, Int)
DEFINE_PROPERTY_TYPE(float, Float)
DEFINE_PROPERTY_TYPE(bool, Bool)
DEFINE_PROPERTY_TYPE(FString, String)
DEFINE_PROPERTY_TYPE(FVector, Vector)
DEFINE_PROPERTY_TYPE(FVector4, Vector4)
DEFINE_PROPERTY_TYPE(FTransform, Transform)
DEFINE_PROPERTY_TYPE(FRotator, Rotator)

struct FEnumEntry
{
    int64 Value;
    FString DisplayName;

    template <typename T>
    FEnumEntry(T InValue, const FString& InDisplayName)
        : Value(static_cast<int64>(InValue)), DisplayName(InDisplayName)
    {
        static_assert(std::is_enum_v<T>, "Enum entry requires an enum value");
    }
};

struct FProperty
{
    FString Name;
    EPropertyType Type;
    size_t Offset;
    size_t Size;
    UClass* Class = nullptr;
	float MinValue = 0.0f;
	float MaxValue = 0.0f;
	bool bHasRange = false;
    TArray<FEnumEntry> EnumEntries;
    int64 (*GetEnumValue)(const void*) = nullptr;
    void (*SetEnumValue)(void*, int64) = nullptr;
};
