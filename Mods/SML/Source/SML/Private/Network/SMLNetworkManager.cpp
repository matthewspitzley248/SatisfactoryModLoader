#include "Network/SMLConnection/SMLNetworkManager.h"
#include "FGPlayerController.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/NetConnection.h"
#include "Network/NetworkHandler.h"
#include "Player/SMLRemoteCallObject.h"
#include "GameFramework/GameModeBase.h"
#include "ModLoading/ModLoadingLibrary.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UObjectAnnotation.h"

static TAutoConsoleVariable CVarSkipRemoteModListCheck(
	TEXT("SML.SkipRemoteModListCheck"),
	GIsEditor,
	TEXT("1 to skip remote mod list check, 0 to enable it. Remote mod list checks are disabled by default in the editor."),
	ECVF_Default
);

static FUObjectAnnotationSparse<FConnectionMetadata, true> GModConnectionMetadata;

TSharedPtr<FMessageType> FSMLNetworkManager::MessageTypeModInit = NULL;

void FSMLNetworkManager::RegisterMessageTypeAndHandlers() {
    UModNetworkHandler* NetworkHandler = GEngine->GetEngineSubsystem<UModNetworkHandler>();
    MessageTypeModInit = MakeShareable(new FMessageType{TEXT("SML"), 1});
    
    FMessageEntry& MessageEntry = NetworkHandler->RegisterMessageType(*MessageTypeModInit);
    MessageEntry.bServerHandled = true;
    
    MessageEntry.MessageReceived.BindStatic(FSMLNetworkManager::HandleMessageReceived);
    NetworkHandler->OnClientInitialJoin().AddStatic(FSMLNetworkManager::HandleInitialClientJoin);
    NetworkHandler->OnWelcomePlayer().AddStatic(FSMLNetworkManager::HandleWelcomePlayer);
    FGameModeEvents::GameModePostLoginEvent.AddStatic(FSMLNetworkManager::HandleGameModePostLogin);
}

void FSMLNetworkManager::HandleMessageReceived(UNetConnection* Connection, FString Data) {
	FConnectionMetadata ConnectionMetadata{};
    ConnectionMetadata.bIsInitialized = true;
    if (!HandleModListObject(ConnectionMetadata, Data))
    {
        Connection->Close();
    }
	GModConnectionMetadata.AddAnnotation( Connection, ConnectionMetadata );
}

void FSMLNetworkManager::HandleInitialClientJoin(UNetConnection* Connection) {
    UModNetworkHandler* NetworkHandler = GEngine->GetEngineSubsystem<UModNetworkHandler>();
    const FString LocalModList = SerializeLocalModList(Connection);
    NetworkHandler->SendMessage(Connection, *MessageTypeModInit, LocalModList);
}

void FSMLNetworkManager::HandleWelcomePlayer(UWorld* World, UNetConnection* Connection) {
    ValidateSMLConnectionData(Connection);
}

void FSMLNetworkManager::HandleGameModePostLogin(AGameModeBase* GameMode, APlayerController* Controller) {

	UModNetworkHandler* NetworkHandler = GEngine->GetEngineSubsystem<UModNetworkHandler>();
    if (AFGPlayerController* CastedPlayerController = Cast<AFGPlayerController>(Controller)) {
        USMLRemoteCallObject* RemoteCallObject = CastedPlayerController->GetRemoteCallObjectOfClass<USMLRemoteCallObject>();

        if (CastedPlayerController->IsLocalController()) {
            //This is a local player, so installed mods are our local mod list
            UModLoadingLibrary* ModLoadingLibrary = GameMode->GetGameInstance()->GetSubsystem<UModLoadingLibrary>();
            const TArray<FModInfo> Mods = ModLoadingLibrary->GetLoadedMods();
            
            for (const FModInfo& ModInfo : Mods) {
                RemoteCallObject->ClientInstalledMods.Add(ModInfo.Name, ModInfo.Version);
            }
        } else {
            //This is remote player, retrieve installed mods from connection
            const UNetConnection* NetConnection = CastChecked<UNetConnection>(Controller->Player);
        	const FConnectionMetadata ConnectionMetadata = GModConnectionMetadata.GetAndRemoveAnnotation( NetConnection );
        	
            RemoteCallObject->ClientInstalledMods.Append(ConnectionMetadata.InstalledClientMods);
        }
    }
}

FString FSMLNetworkManager::SerializeLocalModList(UNetConnection* Connection)
{
	const TSharedRef<FJsonObject> ModListObject = MakeShareable(new FJsonObject());
	
	if (const UGameInstance* GameInstance = UModNetworkHandler::GetGameInstanceFromNetDriver( Connection->GetDriver() ) )
	{
		if ( UModLoadingLibrary* ModLoadingLibrary = GameInstance->GetSubsystem<UModLoadingLibrary>() )
		{
			const TArray<FModInfo> Mods = ModLoadingLibrary->GetLoadedMods();
    
			for (const FModInfo& ModInfo : Mods) {
				ModListObject->SetStringField(ModInfo.Name, ModInfo.Version.ToString());
			}
		}	
	}
	
	const TSharedRef<FJsonObject> MetadataObject = MakeShareable(new FJsonObject());
    MetadataObject->SetObjectField(TEXT("ModList"), ModListObject);
    
    FString ResultString;
    const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&ResultString);
    FJsonSerializer::Serialize(MetadataObject, Writer);
    
    return ResultString;
}

bool FSMLNetworkManager::HandleModListObject( FConnectionMetadata& Metadata, const FString& ModListString) {
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ModListString);
    TSharedPtr<FJsonObject> MetadataObject;
    
    if (!FJsonSerializer::Deserialize(Reader, MetadataObject)) {
        return false;
    }
    
    if (!MetadataObject->HasTypedField<EJson::Object>(TEXT("ModList"))) {
        return false;
    }
    
    const TSharedPtr<FJsonObject>& ModList = MetadataObject->GetObjectField(TEXT("ModList"));
    
    for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : ModList->Values) {
        FVersion ModVersion = FVersion{};
        FString ErrorMessage;
        if ( ModVersion.ParseVersion(Pair.Value->AsString(), ErrorMessage) ) {
            Metadata.InstalledClientMods.Add(Pair.Key, ModVersion);
        } else {
            return false;
        }
    }
    
    return true;
}

void FSMLNetworkManager::ValidateSMLConnectionData(UNetConnection* Connection)
{
	const bool bAllowMissingMods = CVarSkipRemoteModListCheck.GetValueOnGameThread();
	const FConnectionMetadata SMLMetadata = GModConnectionMetadata.GetAnnotation( Connection );
    TArray<FString> ClientMissingMods;
    
    if (!SMLMetadata.bIsInitialized && !bAllowMissingMods ) {
        UModNetworkHandler::CloseWithFailureMessage(Connection, TEXT("This server is running Satisfactory Mod Loader, and your client doesn't have it installed."));
        return;
    }

	if (const UGameInstance* GameInstance = UModNetworkHandler::GetGameInstanceFromNetDriver( Connection->GetDriver() ) )
	{
		if ( UModLoadingLibrary* ModLoadingLibrary = GameInstance->GetSubsystem<UModLoadingLibrary>() )
		{
			// TODO: Do we want to check that the client doesn't have any extra mods compared to the server?
			// Doing so would require the client passing the complete mod info to the server, rather than just the version

			const TArray<FModInfo> Mods = ModLoadingLibrary->GetLoadedMods();

			for (const FModInfo& ModInfo : Mods)
			{
				const FVersion* ClientVersion = SMLMetadata.InstalledClientMods.Find( ModInfo.Name );
				const FString ModName = FString::Printf( TEXT("%s (%s)"), *ModInfo.FriendlyName, *ModInfo.Name );
				if ( ClientVersion == nullptr )
				{
					if ( !ModInfo.bRequiredOnRemote )
					{
						continue; //Server-side only mod
					}
					ClientMissingMods.Add( ModName );
					continue;
				}
				const FVersionRange& RemoteVersion = ModInfo.RemoteVersionRange;

				if ( !RemoteVersion.Matches(*ClientVersion) )
				{
					const FString VersionText = FString::Printf( TEXT("required: %s, client: %s"), *RemoteVersion.ToString(), *ClientVersion->ToString() );
					ClientMissingMods.Add( FString::Printf( TEXT("%s: %s"), *ModName, *VersionText ) );
				}
			}
		}	
	}
    
    if ( ClientMissingMods.Num() > 0 && !bAllowMissingMods )
    {
        const FString JoinedModList = FString::Join( ClientMissingMods, TEXT("\n") );
        const FString Reason = FString::Printf( TEXT("Client missing mods: %s"), *JoinedModList );
        UModNetworkHandler::CloseWithFailureMessage( Connection, Reason );
    }
}
