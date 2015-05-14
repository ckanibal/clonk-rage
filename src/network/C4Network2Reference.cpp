
#include "C4Include.h"
#ifdef C4ENGINE
#include "game/C4Game.h"
#endif
#include "C4Version.h"
#include "network/C4Network2Reference.h"

#include <fcntl.h>
#include <zlib.h>

// *** C4Network2Reference

C4Network2Reference::C4Network2Reference()
	: Icon(0), Time(0), Frame(0), StartTime(0), LeaguePerformance(0),
	JoinAllowed(true), ObservingAllowed(true), PasswordNeeded(false), OfficialServer(false),
	RegJoinOnly(false), iAddrCnt(0)
{

}

C4Network2Reference::~C4Network2Reference()
{

}

void C4Network2Reference::SetSourceIP(in_addr ip)
{
	for (int i = 0; i < iAddrCnt; i++)
		if (Addrs[i].isIPNull())
			Addrs[i].SetIP(ip);
}

#ifdef C4ENGINE
void C4Network2Reference::InitLocal(C4Game *pGame)
{
	// Copy all game parameters
	Parameters = pGame->Parameters;

	// Discard player resources (we don't want these infos in the reference)
	// Add league performance (but only after game end)
	C4ClientPlayerInfos *pClientInfos; C4PlayerInfo *pPlayerInfo;
	int32_t i, j;
	for (i = 0; pClientInfos = Parameters.PlayerInfos.GetIndexedInfo(i); i++)
		for (j = 0; pPlayerInfo = pClientInfos->GetPlayerInfo(j); j++)
		{
		pPlayerInfo->DiscardResource();
		if (pGame->GameOver)
			pPlayerInfo->SetLeaguePerformance(
			pGame->RoundResults.GetLeaguePerformance(pPlayerInfo->GetID()));
		}

	// Special additional information in reference 
	Icon = pGame->C4S.Head.Icon;
	Title.CopyValidated(pGame->ScenarioTitle);
	GameStatus = pGame->Network.Status;
	Time = pGame->Time;
	Frame = pGame->FrameCounter;
	StartTime = pGame->StartTime;
	LeaguePerformance = pGame->RoundResults.GetLeaguePerformance();
	Comment = Config.Network.Comment;
	JoinAllowed = pGame->Network.isJoinAllowed();
	ObservingAllowed = pGame->Network.isObservingAllowed();
	PasswordNeeded = pGame->Network.isPassworded();
	RegJoinOnly = pGame->RegJoinOnly;
	Game.Set();

	// Addresses
	C4Network2Client *pLocalNetClient = pGame->Clients.getLocal()->getNetClient();
	iAddrCnt = pLocalNetClient->getAddrCnt();
	for (i = 0; i < iAddrCnt; i++)
		Addrs[i] = pLocalNetClient->getAddr(i);

}
#endif

void C4Network2Reference::SortNullIPsBack()
{
	// Sort all addresses with zero IP to back of list
	int iSortAddrCnt = iAddrCnt;
	for (int i = 0; i < iSortAddrCnt; i++)
		if (Addrs[i].isIPNull())
		{
		C4Network2Address Addr = Addrs[i];
		for (int j = i + 1; j < iAddrCnt; j++)
			Addrs[j - 1] = Addrs[j];
		Addrs[iAddrCnt - 1] = Addr;
		// Correct position
		i--; iSortAddrCnt--;
		}
}

void C4Network2Reference::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Icon, "Icon", 0));
	pComp->Value(mkNamingAdapt(Title, "Title", "No title"));
	pComp->Value(mkParAdapt(GameStatus, true));
	pComp->Value(mkNamingAdapt(Time, "Time", 0));
	pComp->Value(mkNamingAdapt(Frame, "Frame", 0));
	pComp->Value(mkNamingAdapt(StartTime, "StartTime", 0));
	pComp->Value(mkNamingAdapt(LeaguePerformance, "LeaguePerformance", 0));
	pComp->Value(mkNamingAdapt(Comment, "Comment", ""));
	pComp->Value(mkNamingAdapt(JoinAllowed, "JoinAllowed", true));
	pComp->Value(mkNamingAdapt(ObservingAllowed, "ObservingAllowed", true));
	pComp->Value(mkNamingAdapt(PasswordNeeded, "PasswordNeeded", false));
	pComp->Value(mkNamingAdapt(RegJoinOnly, "RegJoinOnly", false));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iAddrCnt), "AddressCount", 0));
	iAddrCnt = Min<uint8_t>(C4ClientMaxAddr, iAddrCnt);
	pComp->Value(mkNamingAdapt(mkArrayAdapt(Addrs, iAddrCnt, C4Network2Address()), "Address"));
	pComp->Value(mkNamingAdapt(Game.sEngineName, "Game", "None"));
	pComp->Value(mkNamingAdapt(mkArrayAdaptDM(Game.iVer, 0), "Version"));
	pComp->Value(mkNamingAdapt(Game.iBuild, "Build", -1));
	pComp->Value(mkNamingAdapt(OfficialServer, "OfficialServer", false));

	pComp->Value(Parameters);
}

int32_t C4Network2Reference::getSortOrder() const // Don't go over 100, because that's for the masterserver...
{
	C4GameVersion verThis;
	int iOrder = 0;
	// Official server
	if (isOfficialServer() && !Config.Network.UseAlternateServer) iOrder += 50;
	// Joinable
	if (isJoinAllowed() && (getGameVersion() == verThis)) iOrder += 25;
	// League game
	if (Parameters.isLeague()) iOrder += 5;
	// In lobby
	if (getGameStatus().isLobbyActive()) iOrder += 3;
	// No password needed
	if (!isPasswordNeeded()) iOrder += 1;
	// Done
	return iOrder;
}


// *** C4Network2RefServer

C4Network2RefServer::C4Network2RefServer()
	: pReference(NULL)
{
}

C4Network2RefServer::~C4Network2RefServer()
{
	Clear();
}

void C4Network2RefServer::Clear()
{
	C4NetIOTCP::Close();
	delete pReference; pReference = NULL;
}

void C4Network2RefServer::SetReference(C4Network2Reference *pNewReference)
{
	CStdLock RefLock(&RefCSec);
	delete pReference; pReference = pNewReference;
}

void C4Network2RefServer::PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf)
{
	// Just append the packet
	rOutBuf.Append(rPacket);
}

size_t C4Network2RefServer::UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr)
{
	const char *pData = getBufPtr<char>(rInBuf);
	// Check for complete header
	const char *pHeaderEnd = strstr(pData, "\r\n\r\n");
	if (!pHeaderEnd)
		return 0;
	// Check method (only GET is allowed for now)
	if (!SEqual2(pData, "GET "))
	{
		RespondNotImplemented(addr, "Method not implemented");
		return rInBuf.getSize();
	}
	// Check target
	// TODO
	RespondReference(addr);
	return rInBuf.getSize();
}

void C4Network2RefServer::RespondNotImplemented(const C4NetIO::addr_t &addr, const char *szMessage)
{
	// Send the message
	StdStrBuf Data = FormatString("HTTP/1.0 501 %s\r\n\r\n", szMessage);
	Send(C4NetIOPacket(Data.getData(), Data.getLength(), false, addr));
	// Close the connection
	Close(addr);
}

void C4Network2RefServer::RespondReference(const C4NetIO::addr_t &addr)
{
	CStdLock RefLock(&RefCSec);
	// Pack
	StdStrBuf PacketData = DecompileToBuf<StdCompilerINIWrite>(mkNamingPtrAdapt(pReference, "Reference"));
	// Create header
	const char *szCharset = GetCharsetCodeName(Config.General.LanguageCharset);
	StdStrBuf Header = FormatString(
		"HTTP/1.1 200 Found\r\n"
		"Content-Length: %d\r\n"
		"Content-Type: text/plain; charset=%s\r\n"
		"Server: " C4ENGINENAME "/" C4VERSION "\r\n"
		"\r\n",
		PacketData.getLength(),
		szCharset);
	// Send back
	Send(C4NetIOPacket(Header, Header.getLength(), false, addr));
	Send(C4NetIOPacket(PacketData, PacketData.getLength(), false, addr));
	// Close the connection
	Close(addr);
}

// *** C4Network2RefClient

bool C4Network2RefClient::QueryReferences()
{
	// invalidate version
	fVerSet = false;
	// Perform an Query query
	return Query(NULL, false);
}

bool C4Network2RefClient::GetReferences(C4Network2Reference **&rpReferences, int32_t &rRefCount)
{
	// Sanity check
	if (isBusy() || !isSuccess()) return false;
	// Parse response
	MasterVersion.Set("", 0, 0, 0, 0, 0);
	fVerSet = false;
	// local update test
	try
	{
		// Create compiler
		StdCompilerINIRead Comp;
		Comp.setInput(ResultString);
		Comp.Begin();
		// get current version
		Comp.Value(mkNamingAdapt(mkInsertAdapt(mkInsertAdapt(mkInsertAdapt(
			mkNamingAdapt(mkParAdapt(MasterVersion, false), "Version"),
			mkNamingAdapt(mkParAdapt(MessageOfTheDay, StdCompiler::RCT_All), "MOTD", "")),
			mkNamingAdapt(mkParAdapt(MessageOfTheDayHyperlink, StdCompiler::RCT_All), "MOTDURL", "")),
			mkNamingAdapt(mkParAdapt(LeagueServerRedirect, StdCompiler::RCT_All), "LeagueServerRedirect", "")),
			C4ENGINENAME));
		// Read reference count
		Comp.Value(mkNamingCountAdapt(rRefCount, "Reference"));
		// Create reference array and initialize
		rpReferences = new C4Network2Reference *[rRefCount];
		for (int i = 0; i < rRefCount; i++)
			rpReferences[i] = NULL;
		// Get references
		Comp.Value(mkNamingAdapt(mkArrayAdaptMap(rpReferences, rRefCount, mkPtrAdaptNoNull<C4Network2Reference>), "Reference"));
		mkPtrAdaptNoNull<C4Network2Reference>(*rpReferences);
		// Done
		Comp.End();
	}
	catch (StdCompiler::Exception *pExc)
	{
		SetError(pExc->Msg.getData());
		return false;
	}
	// Set source ip
	for (int i = 0; i < rRefCount; i++)
		rpReferences[i]->SetSourceIP(getServerAddress().sin_addr);
	// validate version
	if (MasterVersion.iVer[0]) fVerSet = true;
	// Done
	ResetError();
	return true;
}

bool C4Network2RefClient::GetMasterVersion(C4GameVersion *piVerOut)
{
	// call only after GetReferences
	if (!fVerSet) return false;
	*piVerOut = MasterVersion;
	return true;
}

