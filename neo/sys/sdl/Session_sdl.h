#ifndef __SESSION_SDL_H__
#define __SESSION_SDL_H__

#include "../sys_session_local.h"

class idSessionLocalSDL : public idSessionLocal {
public:
	idSessionLocalSDL();
	virtual ~idSessionLocalSDL();

	virtual void Initialize();
	virtual void Shutdown();
	virtual void InitializeSoundRelatedSystems();
	virtual void ShutdownSoundRelatedSystems();
	virtual void PlatformPump();
	virtual void Pump();

	virtual void CreatePartyLobby( const idMatchParameters & parms );
	virtual void CreateMatch( const idMatchParameters & parms );
	virtual void StartMatch();
	virtual void QuitMatchToTitle();
	virtual void LoadingFinished();
	virtual void MoveToPressStart();
	virtual void EndMatch( bool premature = false );
	virtual void MatchFinished();

	virtual idLobbyBase & GetPartyLobbyBase() { return stubLobby; }
	virtual idLobbyBase & GetGameLobbyBase() { return stubLobby; }
	virtual idLobbyBase & GetActingGameStateLobbyBase() { return stubLobby; }
	virtual idLobbyBase & GetActivePlatformLobbyBase() { return stubLobby; }
	virtual idLobbyBase & GetLobbyFromLobbyUserID( lobbyUserID_t lobbyUserID ) { return stubLobby; }
	virtual bool IsPlatformPartyInLobby() { return false; }
	virtual bool IsAboutToLoad() const { return false; }
	virtual bool ProcessInputEvent( const sysEvent_t * event );

	virtual void InviteFriends();
	virtual void InviteParty();
	virtual void ShowPartySessions();
	virtual void ShowOnlineSignin();
	virtual void UpdateRichPresence();
	virtual void CheckVoicePrivileges();
	virtual void JoinAfterSwap( void * joinID );

	virtual int NumServers() const;
	virtual void ListServers( const idCallback & callback );
	virtual void CancelListServers();
	virtual void ConnectToServer( int index );
	virtual const serverInfo_t * ServerInfo( int index ) const;
	virtual void ShowServerGamerCardUI( int index );
	virtual void ShowLobbyUserGamerCardUI( lobbyUserID_t lobbyUserID );

	virtual void EnumerateDownloadableContent();
	virtual void ShowSystemMarketplaceUI() const;
	virtual void LeaderboardUpload( lobbyUserID_t lobbyUserID, const leaderboardDefinition_t * leaderboard, const column_t * stats, const idFile_Memory * attachment = NULL );
	virtual void LeaderboardDownload( int sessionUserIndex, const leaderboardDefinition_t * leaderboard, int startingRank, int numRows, const idLeaderboardCallback & callback );
	virtual void LeaderboardDownloadAttachment( int sessionUserIndex, const leaderboardDefinition_t * leaderboard, int64 attachmentID );
	virtual void LeaderboardFlush();
	virtual void SetLobbyUserRelativeScore( lobbyUserID_t lobbyUserID, int relativeScore, int team );

	virtual bool IsSystemUIShowing() const { return false; }
	virtual void SetSystemUIShowing( bool show );
	virtual void HandleBootableInvite( int64 lobbyId = 0 );
	virtual void ClearBootableInvite();
	virtual void ClearPendingInvite();
	virtual bool HasPendingBootableInvite();
	virtual void SetDiscSwapMPInvite( void * parm );
	virtual void * GetDiscSwapMPInviteParms();
	virtual void HandleServerQueryRequest( lobbyAddress_t & remoteAddr, idBitMsg & msg, int msgType );
	virtual void HandleServerQueryAck( lobbyAddress_t & remoteAddr, idBitMsg & msg );
	virtual idNetSessionPort & GetPort( bool dedicated = false ) { return port; }
	virtual idLobbyBackend * CreateLobbyBackend( const idMatchParameters & parms, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType );
	virtual idLobbyBackend * FindLobbyBackend( const idMatchParameters & parms, int numPartyUsers, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType );
	virtual idLobbyBackend * JoinFromConnectInfo( const lobbyConnectInfo_t & connectInfo, idLobbyBackend::lobbyBackendType_t lobbyType );
	virtual void DestroyLobbyBackend( idLobbyBackend * lobby );
	virtual void PumpLobbies();
	virtual bool GetLobbyAddressFromNetAddress( const netadr_t & netAddress, lobbyAddress_t & outAddress ) const;
	virtual bool GetNetAddressFromLobbyAddress( const lobbyAddress_t & lobbyAddress, netadr_t & outAddress ) const;

private:
	idNetSessionPort port;
};

#endif
