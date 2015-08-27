// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintCompilerCppBackendModulePrivatePCH.h"
#include "BlueprintCompilerCppBackendUtils.h"
#include "EdGraphSchema_K2.h"

void FEmitHelper::ArrayToString(const TArray<FString>& Array, FString& OutString, const TCHAR* Separator)
{
	if (Array.Num())
	{
		OutString += Array[0];
	}
	for (int32 i = 1; i < Array.Num(); ++i)
	{
		OutString += TEXT(", ");
		OutString += Array[i];
	}
}

bool FEmitHelper::HasAllFlags(uint64 Flags, uint64 FlagsToCheck)
{
	return FlagsToCheck == (Flags & FlagsToCheck);
}

FString FEmitHelper::HandleRepNotifyFunc(const UProperty* Property)
{
	check(Property);
	if (HasAllFlags(Property->PropertyFlags, CPF_Net | CPF_RepNotify))
	{
		return FString::Printf(TEXT("ReplicatedUsing=%s"), *Property->RepNotifyFunc.ToString());
	}
	else if (HasAllFlags(Property->PropertyFlags, CPF_Net))
	{
		return TEXT("Replicated");
	}
	return FString();
}

bool FEmitHelper::MetaDataCanBeNative(const FName MetaDataName)
{
	if (MetaDataName == TEXT("ModuleRelativePath"))
	{
		return false;
	}
	return true;
}

FString FEmitHelper::HandleMetaData(const UField* Field, bool AddCategory, TArray<FString>* AdditinalMetaData)
{
	FString MetaDataStr;

	UPackage* Package = Field ? Field->GetOutermost() : nullptr;
	const UMetaData* MetaData = Package ? Package->GetMetaData() : nullptr;
	const TMap<FName, FString>* ValuesMap = MetaData ? MetaData->ObjectMetaDataMap.Find(Field) : nullptr;
	TArray<FString> MetaDataStrings;
	if (ValuesMap && ValuesMap->Num())
	{
		for (auto& Pair : *ValuesMap)
		{
			if (!MetaDataCanBeNative(Pair.Key))
			{
				continue;
			}
			if (!Pair.Value.IsEmpty())
			{
				FString Value = Pair.Value.Replace(TEXT("\n"), TEXT(""));
				MetaDataStrings.Emplace(FString::Printf(TEXT("%s=\"%s\""), *Pair.Key.ToString(), *Value));
			}
			else
			{
				MetaDataStrings.Emplace(Pair.Key.ToString());
			}
		}
	}
	if (AddCategory && (!ValuesMap || !ValuesMap->Find(TEXT("Category"))))
	{
		MetaDataStrings.Emplace(TEXT("Category"));
	}
	if (AdditinalMetaData)
	{
		MetaDataStrings.Append(*AdditinalMetaData);
	}
	MetaDataStrings.Remove(FString());
	if (MetaDataStrings.Num())
	{
		MetaDataStr += TEXT("meta=(");
		ArrayToString(MetaDataStrings, MetaDataStr, TEXT(", "));
		MetaDataStr += TEXT(")");
	}
	return MetaDataStr;
}

#ifdef HANDLE_CPF_TAG
	static_assert(false, "Macro HANDLE_CPF_TAG redefinition.");
#endif
#define HANDLE_CPF_TAG(TagName, CheckedFlags) if (HasAllFlags(Flags, (CheckedFlags))) { Tags.Emplace(TagName); }

TArray<FString> FEmitHelper::ProperyFlagsToTags(uint64 Flags)
{
	TArray<FString> Tags;

	// EDIT FLAGS
	if (HasAllFlags(Flags, (CPF_Edit | CPF_EditConst | CPF_DisableEditOnInstance)))
	{
		Tags.Emplace(TEXT("VisibleDefaultsOnly"));
	}
	else if (HasAllFlags(Flags, (CPF_Edit | CPF_EditConst | CPF_DisableEditOnTemplate)))
	{
		Tags.Emplace(TEXT("VisibleInstanceOnly"));
	}
	else if (HasAllFlags(Flags, (CPF_Edit | CPF_EditConst)))
	{
		Tags.Emplace(TEXT("VisibleAnywhere"));
	}
	else if (HasAllFlags(Flags, (CPF_Edit | CPF_DisableEditOnInstance)))
	{
		Tags.Emplace(TEXT("EditDefaultsOnly"));
	}
	else if (HasAllFlags(Flags, (CPF_Edit | CPF_DisableEditOnTemplate)))
	{
		Tags.Emplace(TEXT("EditInstanceOnly"));
	}
	else if (HasAllFlags(Flags, (CPF_Edit)))
	{
		Tags.Emplace(TEXT("EditAnywhere"));
	}

	// BLUEPRINT EDIT
	if (HasAllFlags(Flags, (CPF_BlueprintVisible | CPF_BlueprintReadOnly)))
	{
		Tags.Emplace(TEXT("BlueprintReadOnly"));
	}
	else if (HasAllFlags(Flags, (CPF_BlueprintVisible)))
	{
		Tags.Emplace(TEXT("BlueprintReadWrite"));
	}

	// CONFIG
	if (HasAllFlags(Flags, (CPF_GlobalConfig | CPF_Config)))
	{
		Tags.Emplace(TEXT("GlobalConfig"));
	}
	else if (HasAllFlags(Flags, (CPF_Config)))
	{
		Tags.Emplace(TEXT("Config"));
	}

	// OTHER
	HANDLE_CPF_TAG(TEXT("Transient"), CPF_Transient)
	HANDLE_CPF_TAG(TEXT("DuplicateTransient"), CPF_DuplicateTransient)
	HANDLE_CPF_TAG(TEXT("TextExportTransient"), CPF_TextExportTransient)
	HANDLE_CPF_TAG(TEXT("NonPIEDuplicateTransient"), CPF_NonPIEDuplicateTransient)
	HANDLE_CPF_TAG(TEXT("Export"), CPF_ExportObject)
	HANDLE_CPF_TAG(TEXT("NoClear"), CPF_NoClear)
	HANDLE_CPF_TAG(TEXT("EditFixedSize"), CPF_EditFixedSize)
	HANDLE_CPF_TAG(TEXT("NotReplicated"), CPF_RepSkip)
	HANDLE_CPF_TAG(TEXT("Interp"), CPF_Edit | CPF_BlueprintVisible | CPF_Interp)
	HANDLE_CPF_TAG(TEXT("NonTransactional"), CPF_NonTransactional)
	HANDLE_CPF_TAG(TEXT("BlueprintAssignable"), CPF_BlueprintAssignable)
	HANDLE_CPF_TAG(TEXT("BlueprintCallable"), CPF_BlueprintCallable)
	HANDLE_CPF_TAG(TEXT("BlueprintAuthorityOnly"), CPF_BlueprintAuthorityOnly)
	HANDLE_CPF_TAG(TEXT("AssetRegistrySearchable"), CPF_AssetRegistrySearchable)
	HANDLE_CPF_TAG(TEXT("SimpleDisplay"), CPF_SimpleDisplay)
	HANDLE_CPF_TAG(TEXT("AdvancedDisplay"), CPF_AdvancedDisplay)
	HANDLE_CPF_TAG(TEXT("SaveGame"), CPF_SaveGame)

	//TODO:
	//HANDLE_CPF_TAG(TEXT("Instanced"), CPF_PersistentInstance | CPF_ExportObject | CPF_InstancedReference)

	return Tags;
}

TArray<FString> FEmitHelper::FunctionFlagsToTags(uint64 Flags)
{
	TArray<FString> Tags;

	//Pointless: BlueprintNativeEvent, BlueprintImplementableEvent
	//Pointless: CustomThunk

	//TODO: SealedEvent
	//TODO: Unreliable
	//TODO: ServiceRequest, ServiceResponse

	HANDLE_CPF_TAG(TEXT("Exec"), FUNC_Exec)
	HANDLE_CPF_TAG(TEXT("Server"), FUNC_Net | FUNC_NetServer)
	HANDLE_CPF_TAG(TEXT("Client"), FUNC_Net | FUNC_NetClient)
	HANDLE_CPF_TAG(TEXT("NetMulticast"), FUNC_Net | FUNC_NetMulticast)
	HANDLE_CPF_TAG(TEXT("Reliable"), FUNC_NetReliable)
	HANDLE_CPF_TAG(TEXT("BlueprintCallable"), FUNC_BlueprintCallable)
	HANDLE_CPF_TAG(TEXT("BlueprintPure"), FUNC_BlueprintCallable | FUNC_BlueprintPure)
	HANDLE_CPF_TAG(TEXT("BlueprintAuthorityOnly"), FUNC_BlueprintAuthorityOnly)
	HANDLE_CPF_TAG(TEXT("BlueprintCosmetic"), FUNC_BlueprintCosmetic)
	HANDLE_CPF_TAG(TEXT("WithValidation"), FUNC_NetValidate)

	return Tags;
}

#undef HANDLE_CPF_TAG

bool FEmitHelper::IsBlueprintNativeEvent(uint64 FunctionFlags)
{
	return HasAllFlags(FunctionFlags, FUNC_Event | FUNC_BlueprintEvent | FUNC_Native);
}

bool FEmitHelper::IsBlueprintImplementableEvent(uint64 FunctionFlags)
{
	return HasAllFlags(FunctionFlags, FUNC_Event | FUNC_BlueprintEvent) && !HasAllFlags(FunctionFlags, FUNC_Native);
}

FString FEmitHelper::GenerateReplaceConvertedMD(UObject* Obj)
{
	FString Result;
	if (Obj)
	{
		Result = TEXT("ReplaceConverted=\"");

		// 1. Current object
		Result += Obj->GetPathName();

		// 2. Loaded Redirectors
		{
			struct FLocalHelper
			{
				static UObject* FindFinalObject(UObjectRedirector* Redirector)
				{
					auto DestObj = Redirector ? Redirector->DestinationObject : nullptr;
					auto InnerRedir = Cast<UObjectRedirector>(DestObj);
					return InnerRedir ? FindFinalObject(InnerRedir) : DestObj;
				}
			};

			TArray<UObject*> AllObjects;
			GetObjectsOfClass(UObjectRedirector::StaticClass(), AllObjects);
			for (auto LocalObj : AllObjects)
			{
				auto Redirector = CastChecked<UObjectRedirector>(LocalObj);
				if (Obj == FLocalHelper::FindFinalObject(Redirector))
				{
					Result += TEXT(",");
					Result += Redirector->GetPathName();
				}
			}
		}

		// 3. Unloaded Redirectors
		// TODO: It would be better to load all redirectors before compiling. Than checking here AssetRegistry.

		Result += TEXT("\"");
	}

	return Result;
}

FString FEmitHelper::EmitUFuntion(UFunction* Function, TArray<FString>* AdditinalMetaData)
{
	TArray<FString> Tags = FEmitHelper::FunctionFlagsToTags(Function->FunctionFlags);

	auto FunctionOwnerClass = Function->GetOwnerClass();
	if (FunctionOwnerClass->IsChildOf<UInterface>())
	{
		Tags.Emplace(TEXT("BlueprintNativeEvent"));
	}

	const bool bMustHaveCategory = (Function->FunctionFlags & (FUNC_BlueprintCallable | FUNC_BlueprintPure)) != 0;
	Tags.Emplace(FEmitHelper::HandleMetaData(Function, bMustHaveCategory, AdditinalMetaData));
	Tags.Remove(FString());

	FString AllTags;
	FEmitHelper::ArrayToString(Tags, AllTags, TEXT(", "));

	return FString::Printf(TEXT("UFUNCTION(%s)"), *AllTags);
}

int32 FEmitHelper::ParseDelegateDetails(UFunction* Signature, FString& OutParametersMacro, FString& OutParamNumberStr)
{
	int32 ParameterNum = 0;
	FStringOutputDevice Parameters;
	for (TFieldIterator<UProperty> PropIt(Signature); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
	{
		Parameters += ", ";
		PropIt->ExportCppDeclaration(Parameters, EExportedDeclaration::MacroParameter, NULL, EPropertyExportCPPFlags::CPPF_CustomTypeName);
		++ParameterNum;
	}

	FString ParamNumberStr;
	switch (ParameterNum)
	{
	case 0: ParamNumberStr = TEXT("");				break;
	case 1: ParamNumberStr = TEXT("_OneParam");		break;
	case 2: ParamNumberStr = TEXT("_TwoParams");	break;
	case 3: ParamNumberStr = TEXT("_ThreeParams");	break;
	case 4: ParamNumberStr = TEXT("_FourParams");	break;
	case 5: ParamNumberStr = TEXT("_FiveParams");	break;
	case 6: ParamNumberStr = TEXT("_SixParams");	break;
	case 7: ParamNumberStr = TEXT("_SevenParams");	break;
	case 8: ParamNumberStr = TEXT("_EightParams");	break;
	default: ParamNumberStr = TEXT("_TooMany");		break;
	}

	OutParametersMacro = Parameters;
	OutParamNumberStr = ParamNumberStr;
	return ParameterNum;
}

TArray<FString> FEmitHelper::EmitSinglecastDelegateDeclarations(const TArray<UDelegateProperty*>& Delegates)
{
	TArray<FString> Results;
	for (auto It : Delegates)
	{
		check(It);
		auto Signature = It->SignatureFunction;
		check(Signature);

		FString ParamNumberStr, Parameters;
		ParseDelegateDetails(Signature, Parameters, ParamNumberStr);

		Results.Add(*FString::Printf(TEXT("DECLARE_DYNAMIC_DELEGATE%s(%s%s)"),
			*ParamNumberStr, *It->GetCPPType(NULL, EPropertyExportCPPFlags::CPPF_CustomTypeName), *Parameters));
	}
	return Results;
}

TArray<FString> FEmitHelper::EmitMulticastDelegateDeclarations(UClass* SourceClass)
{
	TArray<FString> Results;
	for (TFieldIterator<UMulticastDelegateProperty> It(SourceClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		auto Signature = It->SignatureFunction;
		check(Signature);

		FString ParamNumberStr, Parameters;
		ParseDelegateDetails(Signature, Parameters, ParamNumberStr);

		Results.Add(*FString::Printf(TEXT("DECLARE_DYNAMIC_MULTICAST_DELEGATE%s(%s%s)"),
			*ParamNumberStr, *It->GetCPPType(NULL, EPropertyExportCPPFlags::CPPF_CustomTypeName), *Parameters));
	}

	return Results;
}

FString FEmitHelper::EmitLifetimeReplicatedPropsImpl(UClass* SourceClass, const FString& CppClassName, const TCHAR* InCurrentIndent)
{
	FString Result;
	bool bFunctionInitilzed = false;
	for (TFieldIterator<UProperty> It(SourceClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		if ((It->PropertyFlags & CPF_Net) != 0)
		{
			if (!bFunctionInitilzed)
			{
				Result += FString::Printf(TEXT("%svoid %s::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const\n"), InCurrentIndent, *CppClassName);
				Result += FString::Printf(TEXT("%s{\n"), InCurrentIndent);
				Result += FString::Printf(TEXT("%s\tSuper::GetLifetimeReplicatedProps(OutLifetimeProps);\n"), InCurrentIndent);
				bFunctionInitilzed = true;
			}
			Result += FString::Printf(TEXT("%s\tDOREPLIFETIME( %s, %s);\n"), InCurrentIndent, *CppClassName, *It->GetNameCPP());
		}
	}
	if (bFunctionInitilzed)
	{
		Result += FString::Printf(TEXT("%s}\n"), InCurrentIndent);
	}
	return Result;
}

FString FEmitHelper::LiteralTerm(FEmitterLocalContext& EmitterContext, const FEdGraphPinType& Type, const FString& CustomValue, UObject* LiteralObject)
{
	auto Schema = GetDefault<UEdGraphSchema_K2>();

	if (UEdGraphSchema_K2::PC_String == Type.PinCategory)
	{
		return FString::Printf(TEXT("TEXT(\"%s\")"), *CustomValue.ReplaceCharWithEscapedChar());
	}
	else if (UEdGraphSchema_K2::PC_Text == Type.PinCategory)
	{
		return FString::Printf(TEXT("FText::FromString(TEXT(\"%s\"))"), *CustomValue.ReplaceCharWithEscapedChar());
	}
	else if (UEdGraphSchema_K2::PC_Float == Type.PinCategory)
	{
		float Value = CustomValue.IsEmpty() ? 0.0f : FCString::Atof(*CustomValue);
		return FString::Printf(TEXT("%f"), Value);
	}
	else if (UEdGraphSchema_K2::PC_Int == Type.PinCategory)
	{
		int32 Value = CustomValue.IsEmpty() ? 0 : FCString::Atoi(*CustomValue);
		return FString::Printf(TEXT("%d"), Value);
	}
	else if ((UEdGraphSchema_K2::PC_Byte == Type.PinCategory) || (UEdGraphSchema_K2::PC_Enum == Type.PinCategory))
	{
		auto TypeEnum = Cast<UEnum>(Type.PinSubCategoryObject.Get());
		if (TypeEnum)
		{
			return FString::Printf(TEXT("%s::%s"), *TypeEnum->GetName(), CustomValue.IsEmpty() 
				? *TypeEnum->GetEnumName(TypeEnum->NumEnums()-1)
				: *CustomValue);
		}
		else
		{
			uint8 Value = CustomValue.IsEmpty() ? 0 : FCString::Atoi(*CustomValue);
			return FString::Printf(TEXT("%u"), Value);
		}
	}
	else if (UEdGraphSchema_K2::PC_Boolean == Type.PinCategory)
	{
		const bool bValue = CustomValue.ToBool();
		return bValue ? TEXT("true") : TEXT("false");
	}
	else if (UEdGraphSchema_K2::PC_Name == Type.PinCategory)
	{
		return CustomValue.IsEmpty() 
			? TEXT("FName()") 
			: FString::Printf(TEXT("FName(TEXT(\"%s\"))"), *(FName(*CustomValue).ToString().ReplaceCharWithEscapedChar()));
	}
	else if (UEdGraphSchema_K2::PC_Struct == Type.PinCategory)
	{
		auto StructType = Cast<UScriptStruct>(Type.PinSubCategoryObject.Get());
		ensure(StructType);
		if (StructType == TBaseStructure<FVector>::Get())
		{
			FVector Vect = FVector::ZeroVector;
			FDefaultValueHelper::ParseVector(CustomValue, /*out*/ Vect);
			return FString::Printf(TEXT("FVector(%f,%f,%f)"), Vect.X, Vect.Y, Vect.Z);
		}
		else if (StructType == TBaseStructure<FRotator>::Get())
		{
			FRotator Rot = FRotator::ZeroRotator;
			FDefaultValueHelper::ParseRotator(CustomValue, /*out*/ Rot);
			return FString::Printf(TEXT("FRotator(%f,%f,%f)"), Rot.Pitch, Rot.Yaw, Rot.Roll);
		}
		else if (StructType == TBaseStructure<FTransform>::Get())
		{
			FTransform Trans = FTransform::Identity;
			Trans.InitFromString(CustomValue);
			const FQuat Rot = Trans.GetRotation();
			const FVector Translation = Trans.GetTranslation();
			const FVector Scale = Trans.GetScale3D();
			return FString::Printf(TEXT("FTransform( FQuat(%f,%f,%f,%f), FVector(%f,%f,%f), FVector(%f,%f,%f) )"),
				Rot.X, Rot.Y, Rot.Z, Rot.W, Translation.X, Translation.Y, Translation.Z, Scale.X, Scale.Y, Scale.Z);
		}
		else if (StructType == TBaseStructure<FLinearColor>::Get())
		{
			FLinearColor LinearColor;
			LinearColor.InitFromString(CustomValue);
			return FString::Printf(TEXT("FLinearColor(%f,%f,%f,%f)"), LinearColor.R, LinearColor.G, LinearColor.B, LinearColor.A);
		}
		else if (StructType == TBaseStructure<FColor>::Get())
		{
			FColor Color;
			Color.InitFromString(CustomValue);
			return FString::Printf(TEXT("FColor(%d,%d,%d,%d)"), Color.R, Color.G, Color.B, Color.A);
		}
		if (StructType == TBaseStructure<FVector2D>::Get())
		{
			FVector2D Vect = FVector2D::ZeroVector;
			Vect.InitFromString(CustomValue);
			return FString::Printf(TEXT("FVector2D(%f,%f)"), Vect.X, Vect.Y);
		}
		else
		{
			//@todo:  This needs to be more robust, since import text isn't really proper for struct construction.
			const bool bEmptyCustomValue = CustomValue.IsEmpty() || (CustomValue == TEXT("()"));
			const FString StructName = FString(TEXT("F")) + StructType->GetName();
			if (bEmptyCustomValue)
			{
				return StructName + (StructType->IsA<UUserDefinedStruct>() ? TEXT("::GetDefaultValue()") : TEXT("{}"));
			}

			const FString LocalStructNativeName = EmitterContext.GenerateUniqueLocalName();
			EmitterContext.AddLine(FString::Printf(TEXT("auto %s = %s{};"), *LocalStructNativeName, *StructName)); // TODO: ?? should "::GetDefaultValue()" be called here?
			{
				FStructOnScope StructOnScope(StructType);

				FStringOutputDevice ImportError;
				const auto EndOfParsedBuff = UStructProperty::ImportText_Static(StructType, TEXT("FEmitHelper::LiteralTerm"), *CustomValue, StructOnScope.GetStructMemory(), 0, nullptr, &ImportError);
				if (!EndOfParsedBuff || !ImportError.IsEmpty())
				{
					UE_LOG(LogK2Compiler, Error, TEXT("FEmitHelper::LiteralTerm cannot parse struct \"%s\" error: %s"), *CustomValue, *ImportError);
				}

				for (auto LocalProperty : TFieldRange<const UProperty>(StructType))
				{
					FEmitDefaultValueHelper::OuterGenerate(EmitterContext, LocalProperty, LocalStructNativeName, StructOnScope.GetStructMemory(), nullptr, FEmitDefaultValueHelper::EPropertyAccessOperator::Dot);
				}
			}
			return LocalStructNativeName;
		}
	}
	else if (Type.PinSubCategory == UEdGraphSchema_K2::PSC_Self)
	{
		return TEXT("this");
	}
	else if (UEdGraphSchema_K2::PC_Class == Type.PinCategory)
	{
		if (auto FoundClass = Cast<const UClass>(LiteralObject))
		{
			const FString MappedObject = EmitterContext.FindGloballyMappedObject(LiteralObject);
			if (!MappedObject.IsEmpty())
			{
				return MappedObject;
			}
			return FString::Printf(TEXT("LoadClass<UObject>(nullptr, TEXT(\"%s\"), nullptr, 0, nullptr "), *(LiteralObject->GetPathName().ReplaceCharWithEscapedChar()));
		}
		return FString(TEXT("nullptr"));
	}
	else if((UEdGraphSchema_K2::PC_AssetClass == Type.PinCategory) || (UEdGraphSchema_K2::PC_Asset == Type.PinCategory))
	{
		if (LiteralObject)
		{
			const FString MappedObject = EmitterContext.FindGloballyMappedObject(LiteralObject);
			if (!MappedObject.IsEmpty())
			{
				return MappedObject;
			}
			return FString::Printf(TEXT("FStringAssetReference(TEXT(\"%s\"))"), *(LiteralObject->GetPathName().ReplaceCharWithEscapedChar()));
		}
		return FString(TEXT("nullptr"));
	}
	else if (UEdGraphSchema_K2::PC_Object == Type.PinCategory)
	{
		if (LiteralObject)
		{
			const FString MappedObject = EmitterContext.FindGloballyMappedObject(LiteralObject);
			if (!MappedObject.IsEmpty())
			{
				return MappedObject;
			}
			UClass* FoundClass = Cast<UClass>(Type.PinSubCategoryObject.Get());
			FString ClassString = FoundClass ? (FString(FoundClass->GetPrefixCPP()) + FoundClass->GetName()) : TEXT("UObject");
			return FString::Printf(TEXT("LoadObject<%s>(nullptr, TEXT(\"%s\"))"), *ClassString, *(LiteralObject->GetPathName().ReplaceCharWithEscapedChar()));
		}
		else
		{
			return FString(TEXT("nullptr"));
		}
	}
	/*
	else if (CoerceProperty->IsA(UInterfaceProperty::StaticClass()))
	{
		if (Term->Type.PinSubCategory == PSC_Self)
		{
			return TEXT("this");
		}
		else
		{
			ensureMsgf(false, TEXT("It is not possible to express this interface property as a literal value!"));
			return Term->Name;
		}
	}
	*/
	ensureMsgf(false, TEXT("It is not possible to express this type as a literal value!"));
	return CustomValue;
}

FString FEmitHelper::DefaultValue(FEmitterLocalContext& EmitterContext, const FEdGraphPinType& Type)
{
	return LiteralTerm(EmitterContext, Type, FString(), nullptr);
}

UFunction* FEmitHelper::GetOriginalFunction(UFunction* Function)
{
	check(Function);
	const FName FunctionName = Function->GetFName();

	UClass* Owner = Function->GetOwnerClass();
	check(Owner);
	for (auto& Inter : Owner->Interfaces)
	{
		if (UFunction* Result = Inter.Class->FindFunctionByName(FunctionName))
		{
			return GetOriginalFunction(Result);
		}
	}

	for (UClass* SearchClass = Owner->GetSuperClass(); SearchClass; SearchClass = SearchClass->GetSuperClass())
	{
		if (UFunction* Result = SearchClass->FindFunctionByName(FunctionName))
		{
			return GetOriginalFunction(Result);
		}
	}

	return Function;
}

bool FEmitHelper::ShouldHandleAsNativeEvent(UFunction* Function)
{
	check(Function);
	auto OriginalFunction = FEmitHelper::GetOriginalFunction(Function);
	check(OriginalFunction);
	if (OriginalFunction != Function)
	{
		const uint32 FlagsToCheckMask = EFunctionFlags::FUNC_Event | EFunctionFlags::FUNC_BlueprintEvent | EFunctionFlags::FUNC_Native;
		const uint32 FlagsToCheck = OriginalFunction->FunctionFlags & FlagsToCheckMask;

		auto OriginalOwner = OriginalFunction->GetOwnerClass();
		const bool bBPInterface = Cast<UBlueprintGeneratedClass>(OriginalOwner) && OriginalOwner->IsChildOf<UInterface>();
		return (FlagsToCheck == FlagsToCheckMask) || bBPInterface;
	}
	return false;
}

bool FEmitHelper::ShouldHandleAsImplementableEvent(UFunction* Function)
{
	check(Function);
	auto OriginalFunction = FEmitHelper::GetOriginalFunction(Function);
	check(OriginalFunction);
	if (OriginalFunction != Function)
	{
		const uint32 FlagsToCheckMask = EFunctionFlags::FUNC_Event | EFunctionFlags::FUNC_BlueprintEvent | EFunctionFlags::FUNC_Native;
		const uint32 FlagsToCheck = OriginalFunction->FunctionFlags & FlagsToCheckMask;
		return (FlagsToCheck == (EFunctionFlags::FUNC_Event | EFunctionFlags::FUNC_BlueprintEvent));
	}
	return false;
}

bool FEmitHelper::GenerateAutomaticCast(const FEdGraphPinType& LType, const FEdGraphPinType& RType, FString& OutCastBegin, FString& OutCastEnd)
{
	// BYTE to ENUM cast
	// ENUM to BYTE cast
	if ((LType.PinCategory == UEdGraphSchema_K2::PC_Byte) && (RType.PinCategory == UEdGraphSchema_K2::PC_Byte))
	{
		auto LTypeEnum = Cast<UEnum>(LType.PinSubCategoryObject.Get());
		auto RTypeEnum = Cast<UEnum>(RType.PinSubCategoryObject.Get());
		if (!RTypeEnum && LTypeEnum)
		{
			FString EnumCppType = !LTypeEnum->CppType.IsEmpty() ? LTypeEnum->CppType : LTypeEnum->GetName();
			OutCastBegin = FString::Printf(TEXT("static_cast<%s>("), *EnumCppType);
			OutCastEnd = TEXT(")");
			return true;
		}
		if (!LTypeEnum && RTypeEnum)
		{
			FString EnumCppType = !RTypeEnum->CppType.IsEmpty() ? RTypeEnum->CppType : RTypeEnum->GetName();
			OutCastBegin = FString::Printf(TEXT("EnumToByte<%s>("), *EnumCppType); 
			OutCastEnd = TEXT(")");
			return true;
		}
	}

	// OBJECT to OBJECT
	if (LType.PinCategory == UEdGraphSchema_K2::PC_Object)
	{
		UClass* LClass = Cast<UClass>(LType.PinSubCategoryObject.Get());
		UClass* RClass = Cast<UClass>(RType.PinSubCategoryObject.Get());
		if (LClass && RClass && LClass->IsChildOf(RClass) && !RClass->IsChildOf(LClass))
		{
			OutCastBegin = FString::Printf(TEXT("CastChecked<%s%s>("), LClass->GetPrefixCPP(), *LClass->GetName());
			OutCastEnd = TEXT(")");
			return true;
		}
	}

	return false;
}
