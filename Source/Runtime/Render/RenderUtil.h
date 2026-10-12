#pragma once

#include "RenderEnums.h"

struct FShaderByteCode
{
	ComPtr<ID3DBlob> Blob;

	const void* GetData() const { return Blob ? Blob->GetBufferPointer() : nullptr; }
	SIZE_T      GetSize() const { return Blob ? Blob->GetBufferSize() : 0; }
	bool        IsValid() const { return Blob != nullptr; }
};

struct FShaderMacro
{
	FString Name;
	FString Value;
};

class RenderUtil
{
public:
	static FShaderByteCode CompileShader(FString Path, const char* EntryPoint, EShaderType ShaderType, const TArray<FShaderMacro>& Macros = TArray<FShaderMacro>());
	static void SaveByteCode(FString TargetPath, const FShaderByteCode& ByteCode);
	static FShaderByteCode LoadByteCode(const FString& CsoPath);

	static FString MakeMacroKey(const TArray<FShaderMacro>& Macros);
	static FString GetCsoPath(const FString& ShaderPath, EShaderType ShaderType, const TArray<FShaderMacro>& Macros = TArray<FShaderMacro>());
	static bool IsCsoUpToDate(const FString& ShaderPath, const FString& CsoPath);
	static FShaderByteCode GetOrCompile(const FString& ShaderPath, const char* EntryPoint, EShaderType ShaderType, FString& OutCSOPath, const TArray<FShaderMacro>& Macros = TArray<FShaderMacro>());
};