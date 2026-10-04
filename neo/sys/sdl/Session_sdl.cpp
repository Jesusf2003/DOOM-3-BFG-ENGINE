#pragma hdrstop
#include "../../idlib/precompiled.h"
#include "Session_sdl.h"
#include "../sys_localuser.h"
#include "../sys_signin.h"
#include "../sys_achievements.h"
#include "../sys_savegame.h"
#include "../sys_voicechat.h"

namespace {

void WarnUnsupportedSDLSessionFeature( const char * feature ) {
	idLib::Warning( "SDL single-player session does not support %s", feature );
}

class idSessionLocalUserSDL : public idLocalUser {
public:
	virtual void PumpPlatform() {}
	virtual bool IsProfileReady() const {
		const idPlayerProfile * profile = const_cast<idSessionLocalUserSDL *>( this )->GetProfile();
		return profile != NULL && profile->GetState() == idPlayerProfile::IDLE;
	}
	virtual bool IsOnline() const { return false; }
	virtual uint32 GetOnlineCaps() const { return 0; }
	virtual int GetInputDevice() const { return 0; }
	virtual const char * GetGamerTag() const { return "Player"; }
	virtual bool IsInParty() const { return false; }
	virtual int GetPartyCount() const { return 1; }
};

class idSessionSignInManagerSDL : public idSignInManagerBase {
public:
	idSessionSignInManagerSDL() : registered( false ) {}

	virtual void Pump() {
		if ( !registered && minDesiredLocalUsers > 0 ) {
			RegisterLocalUser( 0 );
		}
		if ( registered ) {
			user.Pump();
		}
	}
	virtual int GetNumLocalUsers() const { return registered ? 1 : 0; }
	virtual idLocalUser * GetLocalUserByIndex( int index ) {
		return registered && index == 0 ? &user : NULL;
	}
	virtual const idLocalUser * GetLocalUserByIndex( int index ) const {
		return registered && index == 0 ? &user : NULL;
	}
	virtual void RemoveLocalUserByIndex( int index ) {
		if ( registered && index == 0 ) {
			registered = false;
		}
	}
	virtual void RegisterLocalUser( int inputDevice ) {
		if ( registered ) {
			return;
		}
		user.SetLocalUserHandle( localUserHandle_t( 1 ) );
		user.LoadProfileSettings();
		registered = true;
	}

private:
	idSessionLocalUserSDL user;
	bool registered;
};

class idSessionAchievementsSDL : public idAchievementSystem {
public:
	virtual void AchievementUnlock( idLocalUser * user, const int achievementID ) {
		WarnUnsupportedSDLSessionFeature( "platform achievements" );
	}
	virtual void Pump() {}
};

class idSessionVoiceChatSDL : public idVoiceChatMgr {
public:
	virtual bool GetLocalChatDataInternal( int talkerIndex, byte * data, int & dataSize ) {
		dataSize = 0;
		return false;
	}
	virtual void SubmitIncomingChatDataInternal( int talkerIndex, const byte * data, int dataSize ) {}
	virtual bool TalkerHasData( int talkerIndex ) { return false; }
	virtual bool RegisterTalkerInternal( int index ) { return false; }
	virtual void UnregisterTalkerInternal( int index ) {}
};

}

idSessionLocalSDL::idSessionLocalSDL() {
	voiceChat = new idSessionVoiceChatSDL();
}

idSessionLocalSDL::~idSessionLocalSDL() {
	delete voiceChat;
	voiceChat = NULL;
}

void idSessionLocalSDL::Initialize() {
	idSessionLocal::Initialize();
	if ( signInManager == NULL ) {
		signInManager = new idSessionSignInManagerSDL();
	}
	if ( saveGameManager == NULL ) {
		saveGameManager = new idSaveGameManager();
	}
	if ( achievementSystem == NULL ) {
		achievementSystem = new idSessionAchievementsSDL();
		achievementSystem->Init();
	}
	titleStorageVars.Set( "MAX_PLAYERS_ALLOWED", "1" );
	titleStorageLoaded = true;
	localState = STATE_IDLE;
	signInManager->SetDesiredLocalUsers( 1, 1 );
	signInManager->Pump();
}

void idSessionLocalSDL::Shutdown() {
	if ( voiceChat != NULL ) {
		voiceChat->Shutdown();
	}
	if ( achievementSystem != NULL ) {
		achievementSystem->Shutdown();
		delete achievementSystem;
		achievementSystem = NULL;
	}
	delete signInManager;
	signInManager = NULL;
	delete saveGameManager;
	saveGameManager = NULL;
	idSessionLocal::Shutdown();
}

void idSessionLocalSDL::InitializeSoundRelatedSystems() {
	if ( voiceChat != NULL ) {
		voiceChat->Init( NULL );
	}
}

void idSessionLocalSDL::ShutdownSoundRelatedSystems() {
	if ( voiceChat != NULL ) {
		voiceChat->Shutdown();
	}
}

void idSessionLocalSDL::PlatformPump() {
}

void idSessionLocalSDL::Pump() {
	if ( signInManager != NULL ) {
		signInManager->Pump();
	}
	if ( achievementSystem != NULL ) {
		achievementSystem->Pump();
	}
}

void idSessionLocalSDL::CreatePartyLobby( const idMatchParameters & parms ) {
	if ( parms.gameMode != GAME_MODE_SINGLEPLAYER || parms.gameMap != GAME_MAP_SINGLEPLAYER ) {
		WarnUnsupportedSDLSessionFeature( "multiplayer matches" );
		return;
	}
	stubLobby.SetMatchParms( parms );
	localState = STATE_PARTY_LOBBY_HOST;
}

void idSessionLocalSDL::CreateMatch( const idMatchParameters & parms ) {
	if ( parms.gameMode != GAME_MODE_SINGLEPLAYER || parms.gameMap != GAME_MAP_SINGLEPLAYER ) {
		WarnUnsupportedSDLSessionFeature( "multiplayer matches" );
		return;
	}
	stubLobby.SetMatchParms( parms );
	localState = STATE_GAME_LOBBY_HOST;
}

void idSessionLocalSDL::StartMatch() {
	if ( localState != STATE_GAME_LOBBY_HOST ) {
		idLib::Warning( "Cannot start a single-player match before its match state is created" );
		return;
	}
	localState = STATE_LOADING;
}

void idSessionLocalSDL::QuitMatchToTitle() {
	localState = STATE_IDLE;
}

void idSessionLocalSDL::LoadingFinished() {
	if ( localState != STATE_LOADING ) {
		idLib::Warning( "Cannot finish loading while the SDL session is not loading" );
		return;
	}
	localState = STATE_INGAME;
}

void idSessionLocalSDL::MoveToPressStart() {
	localState = STATE_IDLE;
}

void idSessionLocalSDL::EndMatch( bool premature ) {
	localState = STATE_GAME_LOBBY_HOST;
}

void idSessionLocalSDL::MatchFinished() {
	localState = STATE_GAME_LOBBY_HOST;
}

bool idSessionLocalSDL::ProcessInputEvent( const sysEvent_t * event ) {
	return signInManager != NULL && signInManager->ProcessInputEvent( event );
}

void idSessionLocalSDL::InviteFriends() { WarnUnsupportedSDLSessionFeature( "friend invites" ); }
void idSessionLocalSDL::InviteParty() { WarnUnsupportedSDLSessionFeature( "party invites" ); }
void idSessionLocalSDL::ShowPartySessions() { WarnUnsupportedSDLSessionFeature( "party sessions" ); }
void idSessionLocalSDL::ShowOnlineSignin() { WarnUnsupportedSDLSessionFeature( "online sign-in" ); }
void idSessionLocalSDL::UpdateRichPresence() { WarnUnsupportedSDLSessionFeature( "rich presence" ); }
void idSessionLocalSDL::CheckVoicePrivileges() { WarnUnsupportedSDLSessionFeature( "voice privileges" ); }
void idSessionLocalSDL::JoinAfterSwap( void * joinID ) { WarnUnsupportedSDLSessionFeature( "disc swapping" ); }
void idSessionLocalSDL::SetSystemUIShowing( bool show ) {
	if ( show ) {
		WarnUnsupportedSDLSessionFeature( "system UI" );
	}
}

int idSessionLocalSDL::NumServers() const { return 0; }
void idSessionLocalSDL::ListServers( const idCallback & callback ) { WarnUnsupportedSDLSessionFeature( "server browsing" ); }
void idSessionLocalSDL::CancelListServers() {}
void idSessionLocalSDL::ConnectToServer( int index ) { WarnUnsupportedSDLSessionFeature( "network connections" ); }
const serverInfo_t * idSessionLocalSDL::ServerInfo( int index ) const { return NULL; }
void idSessionLocalSDL::ShowServerGamerCardUI( int index ) { WarnUnsupportedSDLSessionFeature( "gamer cards" ); }
void idSessionLocalSDL::ShowLobbyUserGamerCardUI( lobbyUserID_t lobbyUserID ) { WarnUnsupportedSDLSessionFeature( "gamer cards" ); }

void idSessionLocalSDL::EnumerateDownloadableContent() {
	downloadedContent.Clear();
	marketplaceHasNewContent = false;
}
void idSessionLocalSDL::ShowSystemMarketplaceUI() const { WarnUnsupportedSDLSessionFeature( "marketplace UI" ); }
void idSessionLocalSDL::LeaderboardUpload( lobbyUserID_t lobbyUserID, const leaderboardDefinition_t * leaderboard, const column_t * stats, const idFile_Memory * attachment ) { WarnUnsupportedSDLSessionFeature( "leaderboard uploads" ); }
void idSessionLocalSDL::LeaderboardDownload( int sessionUserIndex, const leaderboardDefinition_t * leaderboard, int startingRank, int numRows, const idLeaderboardCallback & callback ) { WarnUnsupportedSDLSessionFeature( "leaderboard downloads" ); }
void idSessionLocalSDL::LeaderboardDownloadAttachment( int sessionUserIndex, const leaderboardDefinition_t * leaderboard, int64 attachmentID ) { WarnUnsupportedSDLSessionFeature( "leaderboard attachments" ); }
void idSessionLocalSDL::LeaderboardFlush() {}
void idSessionLocalSDL::SetLobbyUserRelativeScore( lobbyUserID_t lobbyUserID, int relativeScore, int team ) { WarnUnsupportedSDLSessionFeature( "online score submission" ); }
void idSessionLocalSDL::HandleBootableInvite( int64 lobbyId ) { WarnUnsupportedSDLSessionFeature( "platform invites" ); }
void idSessionLocalSDL::ClearBootableInvite() {}
void idSessionLocalSDL::ClearPendingInvite() {}
bool idSessionLocalSDL::HasPendingBootableInvite() { return false; }
void idSessionLocalSDL::SetDiscSwapMPInvite( void * parm ) { WarnUnsupportedSDLSessionFeature( "disc swapping" ); }
void * idSessionLocalSDL::GetDiscSwapMPInviteParms() { return NULL; }
void idSessionLocalSDL::HandleServerQueryRequest( lobbyAddress_t & remoteAddr, idBitMsg & msg, int msgType ) { WarnUnsupportedSDLSessionFeature( "server queries" ); }
void idSessionLocalSDL::HandleServerQueryAck( lobbyAddress_t & remoteAddr, idBitMsg & msg ) { WarnUnsupportedSDLSessionFeature( "server queries" ); }
idLobbyBackend * idSessionLocalSDL::CreateLobbyBackend( const idMatchParameters & parms, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType ) { WarnUnsupportedSDLSessionFeature( "network lobbies" ); return NULL; }
idLobbyBackend * idSessionLocalSDL::FindLobbyBackend( const idMatchParameters & parms, int numPartyUsers, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType ) { WarnUnsupportedSDLSessionFeature( "network lobbies" ); return NULL; }
idLobbyBackend * idSessionLocalSDL::JoinFromConnectInfo( const lobbyConnectInfo_t & connectInfo, idLobbyBackend::lobbyBackendType_t lobbyType ) { WarnUnsupportedSDLSessionFeature( "network lobbies" ); return NULL; }
void idSessionLocalSDL::DestroyLobbyBackend( idLobbyBackend * lobby ) {}
void idSessionLocalSDL::PumpLobbies() {}
bool idSessionLocalSDL::GetLobbyAddressFromNetAddress( const netadr_t & netAddress, lobbyAddress_t & outAddress ) const { return false; }
bool idSessionLocalSDL::GetNetAddressFromLobbyAddress( const lobbyAddress_t & lobbyAddress, netadr_t & outAddress ) const { return false; }

static idSessionLocalSDL sessionLocalSDL;
idSession * session = &sessionLocalSDL;
