// This file has been automatically generated by the Unreal Header Implementation tool

#include "FGInputLibrary.h"

FInputActionKeyMapping UFGInputLibrary::GetKeyMappingForAction(APlayerController* playerController, FName inAction, bool getGamepadKey){ return FInputActionKeyMapping(); }
FText UFGInputLibrary::GetAbbreviatedKeyName(FKey key){ return FText(); }
FText UFGInputLibrary::FormatStringWithInputActionNames(APlayerController* playerController, FText textToFormat, bool abbreviateKeyNames){ return FText(); }
FText UFGInputLibrary::GetInputActionNameAsText(APlayerController* playerController, const FName& inActionName, bool abbreviateKeyNames){ return FText(); }
void UFGInputLibrary::GetPlayerRebindableMappingContexts(TArray<class UFGInputMappingContext*>& out_MappingContexts){ }
void UFGInputLibrary::FindAllChildMappingContexts(TMultiMap<TSoftObjectPtr<UFGInputMappingContext>, TSoftObjectPtr<UFGInputMappingContext>>& out_ParentToChildContexts){ }
TSoftObjectPtr< class UInputAction > UFGInputLibrary::FindInputActionByTag(const FGameplayTag& tag){ return TSoftObjectPtr<class UInputAction>(); }
TSoftObjectPtr< class UInputAction > UFGInputLibrary::FindInputActionByMappingName(APlayerController* playerController, const FName& mappingName){ return TSoftObjectPtr<class UInputAction>(); }
bool UFGInputLibrary::GetOverlappingEnhancedKeyMappings(APlayerController* playerController, const FName& inActionName, const FKey& primaryKey, const TArray<FKey>& modifierKeys, TArray<FEnhancedActionKeyMapping>& out_HardConflicts, TArray<FEnhancedActionKeyMapping>& out_SoftConflicts){ return bool(); }
void UFGInputLibrary::RebindEnhancedKeyMapping(APlayerController* playerController, const FName& inActionName, const FKey& primaryKey, const TArray<FKey>& modifierKeys){ }
void UFGInputLibrary::ResetAllEnhancedKeyBindings(APlayerController* playerController){ }
bool UFGInputLibrary::GetCurrentMappingForAction(APlayerController* playerController, const FName& inActionName, FKey& out_primaryKey, TArray<FKey>& out_modifierKeys){ return bool(); }
void UFGInputLibrary::ClearBindingsForWidget(UUserWidget* widget){ }
UTexture* UFGInputLibrary::GetTextureFromKey(const FKey& key){ return nullptr; }
UTexture* UFGInputLibrary::GetTextureFromTag(const FGameplayTag& tag){ return nullptr; }
bool UFGInputLibrary::GetTexturePaddingFromKey(const FKey& key, FMargin& out_Padding){ return bool(); }
FMargin UFGInputLibrary::GetReferenceKeyTexturePadding(){ return FMargin(); }
FMargin UFGInputLibrary::GetKeyVariantTexturePadding(){ return FMargin(); }
bool UFGInputLibrary::GetKeyTextureBinding(const FKey& key,  FFGKeyTextureBinding& out_Binding){ return bool(); }
