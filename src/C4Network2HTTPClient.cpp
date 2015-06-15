#include "C4Include.h"
#ifdef C4ENGINE
#include "game/C4Game.h"
#endif
#include "C4Version.h"
#include "C4Network2HTTPClient.h"

C4Network2HTTPClient::C4Network2HTTPClient()
	: fBusy(false), fSuccess(false), fConnected(false), iDownloadedSize(0), iTotalSize(0), fBinary(false), iDataOffset(0),
	pNotify(NULL) {
	C4NetIOTCP::SetCallback(this);
}

C4Network2HTTPClient::~C4Network2HTTPClient() {
}

void C4Network2HTTPClient::PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf) {
	// Just append the packet
	rOutBuf.Append(rPacket);
}

size_t C4Network2HTTPClient::UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr) {
	// since new data arrived, increase timeout time
	ResetRequestTimeout();
	// Check for complete header
	if (!iDataOffset)
	{
		// Copy data into string buffer (terminate)
		StdStrBuf Data; Data.Copy(getBufPtr<char>(rInBuf), rInBuf.getSize());
		const char *pData = Data.getData();
		// Header complete?
		const char *pContent = SSearch(pData, "\r\n\r\n");
		if (!pContent)
			return 0;
		// Read the header
		if (!ReadHeader(Data))
		{
			fBusy = fSuccess = false;
			Close(addr);
			return rInBuf.getSize();
		}
	}
	iDownloadedSize = rInBuf.getSize() - iDataOffset;
	// Check if the packet is complete
	if (iTotalSize > iDownloadedSize)
	{
		return 0;
	}
	// Get data, uncompress it if needed
	StdBuf Data = rInBuf.getPart(iDataOffset, iTotalSize);
	if (fCompressed)
		if (!Decompress(&Data))
		{
			fBusy = fSuccess = false;
			Close(addr);
			return rInBuf.getSize();
		}
	// Save the result
	if (fBinary)
		ResultBin.Copy(Data);
	else
		ResultString.Copy(getBufPtr<char>(Data), Data.getSize());
	fBusy = false; fSuccess = true;
	// Callback
    OnPacket(C4NetIOPacket(Data, addr), this);
	// Done
	Close(addr);
	return rInBuf.getSize();
}

bool C4Network2HTTPClient::ReadHeader(StdStrBuf Data) {
	const char *pData = Data.getData();
	const char *pContent = SSearch(pData, "\r\n\r\n");
	if (!pContent)
		return 0;
	// Parse header line
	int iHTTPVer1, iHTTPVer2, iResponseCode, iStatusStringPtr;
	if (sscanf(pData, "HTTP/%d.%d %d %n", &iHTTPVer1, &iHTTPVer2, &iResponseCode, &iStatusStringPtr) != 3)
	{
		Error = "Invalid status line!";
		return false;
	}
	// Check HTTP version
	if (iHTTPVer1 != 1)
	{
		Error.Format("Unsupported HTTP version: %d.%d!", iHTTPVer1, iHTTPVer2);
		return false;
	}
	// Check code
	if (iResponseCode != 200)
	{
		// Get status string	
		StdStrBuf StatusString; StatusString.CopyUntil(pData + iStatusStringPtr, '\r');
		// Create error message
		Error.Format("HTTP server responded %d: %s", iResponseCode, StatusString.getData());
		return false;
	}
	// Get content length
	const char *pContentLength = SSearch(pData, "\r\nContent-Length:");
	int iContentLength;
	if (!pContentLength || pContentLength > pContent ||
		sscanf(pContentLength, "%d", &iContentLength) != 1)
	{
		Error.Format("Invalid server response: Content-Length is missing!");
		return false;
	}
	iTotalSize = iContentLength;
	iDataOffset = (pContent - pData);
	// Get content encoding
	const char *pContentEncoding = SSearch(pData, "\r\nContent-Encoding:");
	if (pContentEncoding)
	{
		while (*pContentEncoding == ' ') pContentEncoding++;
		StdStrBuf Encoding; Encoding.CopyUntil(pContentEncoding, '\r');
		if (Encoding == "gzip")
			fCompressed = true;
		else
			fCompressed = false;
	}
	else
		fCompressed = false;
	// Okay
	return true;
}

bool C4Network2HTTPClient::Decompress(StdBuf *pData) {
	size_t iSize = pData->getSize();
	// Create buffer
	uint32_t iOutSize = *getBufPtr<uint32_t>(*pData, pData->getSize() - sizeof(uint32_t));
	iOutSize = Min<uint32_t>(iOutSize, iSize * 1000);
	StdBuf Out; Out.New(iOutSize);
	// Prepare stream
	z_stream zstrm;
	ZeroMem(&zstrm, sizeof(zstrm));
	zstrm.next_in = const_cast<Byte *>(getBufPtr<Byte>(*pData));
	zstrm.avail_in = pData->getSize();
	zstrm.next_out = getMBufPtr<Byte>(Out);
	zstrm.avail_out = Out.getSize();
	// Inflate...
	if (inflateInit2(&zstrm, 15 + 16) != Z_OK)
	{
		Error.Format("Could not decompress data!");
		return false;
	}
	// Inflate!
	if (inflate(&zstrm, Z_FINISH) != Z_STREAM_END)
	{
		inflateEnd(&zstrm);
		Error.Format("Could not decompress data!");
		return false;
	}
	// Return the buffer
	Out.SetSize(zstrm.total_out);
	pData->Take(Out);
	// Okay
	inflateEnd(&zstrm);
	return true;
}

bool C4Network2HTTPClient::OnConn(const C4NetIO::addr_t &AddrPeer, const C4NetIO::addr_t &AddrConnect, const C4NetIO::addr_t *pOwnAddr, C4NetIO *pNetIO) {

	// Make sure we're actually waiting for this connection
	if (!AddrEqual(AddrConnect, ServerAddr))
		return false;
	// Save pack peer address
	PeerAddr = AddrPeer;
	// Send the request
	Send(C4NetIOPacket(Request, AddrPeer));
	Request.Clear();
	fConnected = true;
	return true;
}

void C4Network2HTTPClient::OnDisconn(const C4NetIO::addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason) {
	// Got no complete packet? Failure...
	if (!fSuccess && Error.isNull())
	{
		fBusy = false;
		Error.Format("Unexpected disconnect: %s", szReason);
	}
	fConnected = false;
	// Notify
	if (pNotify)
		pNotify->PushEvent(Ev_HTTP_Response, this);
}

void C4Network2HTTPClient::OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO) {
	// Everything worthwhile was already done in UnpackPacket. Only do notify callback
	if (pNotify)
		pNotify->PushEvent(Ev_HTTP_Response, this);
}

bool C4Network2HTTPClient::Execute(int iMaxTime) {
	// Check timeout
	if (fBusy && time(NULL) > iRequestTimeout)
	{
		Cancel("Request timeout");
		return true;
	}
	// Execute normally
	return C4NetIOTCP::Execute(iMaxTime);
}

int C4Network2HTTPClient::GetTimeout() {
	if (!fBusy)
		return C4NetIOTCP::GetTimeout();
	return MaxTimeout(C4NetIOTCP::GetTimeout(), 1000 * Max<int>(time(NULL) - iRequestTimeout, 0));
}

bool C4Network2HTTPClient::Query(const StdBuf &Data, bool fBinary) {
	if (Server.isNull()) return false;
	// Cancel previous request
	if (fBusy)
		Cancel("Cancelled");
	// No result known yet
	ResultString.Clear();
	// store mode
	this->fBinary = fBinary;
	// Create request
	const char *szCharset = GetCharsetCodeName(Config.General.LanguageCharset);
	StdStrBuf Header;
	if (Data.getSize())
		Header.Format(
		"POST %s HTTP/1.0\r\n"
		"Host: %s\r\n"
		"Connection: Close\r\n"
		"Content-Length: %d\r\n"
		"Content-Type: text/plain; encoding=%s\r\n"
		"Accept-Charset: %s\r\n"
		"Accept-Encoding: gzip\r\n"
		"Accept-Language: %s\r\n"
		"User-Agent: " C4ENGINENAME "/" C4VERSION "\r\n"
		"\r\n",
		RequestPath.getData(),
		Server.getData(),
		Data.getSize(),
		szCharset,
		szCharset,
		Config.General.LanguageEx);
	else
		Header.Format(
		"GET %s HTTP/1.0\r\n"
		"Host: %s\r\n"
		"Connection: Close\r\n"
		"Accept-Charset: %s\r\n"
		"Accept-Encoding: gzip\r\n"
		"Accept-Language: %s\r\n"
		"User-Agent: " C4ENGINENAME "/" C4VERSION "\r\n"
		"\r\n",
		RequestPath.getData(),
		Server.getData(),
		szCharset,
		Config.General.LanguageEx);
	// Compose query
	Request.Take(Header.GrabPointer(), Header.getLength());
	Request.Append(Data);
	// Start connecting
	if (!Connect(ServerAddr))
		return false;
	// Okay, request will be performed when connection is complete
	fBusy = true;
	iDataOffset = 0;
	ResetRequestTimeout();
	ResetError();
	return true;
}

void C4Network2HTTPClient::ResetRequestTimeout() {
	// timeout C4Network2HTTPQueryTimeout seconds from this point
	iRequestTimeout = time(NULL) + C4Network2HTTPQueryTimeout;
}

void C4Network2HTTPClient::Cancel(const char *szReason) {
	// Close connection - and connection attempt
	Close(ServerAddr); Close(PeerAddr);
	// Reset flags
	fBusy = fSuccess = fConnected = fBinary = false;
	iDownloadedSize = iTotalSize = iDataOffset = 0;
	Error = szReason;
}

void C4Network2HTTPClient::Clear() {
	fBusy = fSuccess = fConnected = fBinary = false;
	iDownloadedSize = iTotalSize = iDataOffset = 0;
	ResultBin.Clear();
	ResultString.Clear();
	Error.Clear();
}

bool C4Network2HTTPClient::SetServer(const char *szServerAddress) {
	// Split address
	const char *pRequestPath;
	if (pRequestPath = strchr(szServerAddress, '/'))
	{
		Server.CopyUntil(szServerAddress, '/');
		RequestPath = pRequestPath;
	}
	else
	{
		Server = szServerAddress;
		RequestPath = "/";
	}
	// Resolve address
	if (!ResolveAddress(Server.getData(), &ServerAddr, GetDefaultPort()))
	{
		SetError(FormatString("Could not resolve server address %s!", Server.getData()).getData());
		return false;
	}
	// Remove port
	const char *pColon = strchr(Server.getData(), ':');
	if (pColon)
		Server.SetLength(pColon - Server.getData());
	// Done
	ResetError();
	return true;
}