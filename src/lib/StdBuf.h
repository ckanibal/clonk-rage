
// Standard buffer classes

#ifndef STDBUF_H
#define STDBUF_H

#include <zlib.h>
#include "Standard.h"

#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

// debug memory management
#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

// Base buffer class. Either references or holds data.
class StdBuf {
 public:
  // *** Construction
  // Standard constructor
  StdBuf() : fRef(true), pData(NULL), iSize(0) {}

  // Constructor from other buffer (copy construction):
  // Will take over buffer ownership. Copies data if specified.
  // Note: Construct with Buf2.getRef() to construct a reference (This will work
  // for a constant Buf2, too)
  StdBuf(const StdBuf &Buf2) : fRef(true), pData(NULL), iSize(0) { Copy(Buf2); }

  StdBuf(StdBuf &&Buf2) : fRef(true), pData(NULL), iSize(0) { Take(Buf2); }

  // Set by constant data. Copies data if desired.
  StdBuf(const void *pData, size_t iSize, bool fCopy = false)
      : fRef(true), pData(pData), iSize(iSize) {
    if (fCopy) Copy();
  }

  ~StdBuf() { Clear(); }

 protected:
  // Reference? Otherwise, this object holds the data.
  bool fRef;
  // Data
  union {
    const void *pData;
    void *pMData;
#if defined(_DEBUG)
    char *szString;  // for debugger preview
#endif
  };
  size_t iSize;

 public:
  // *** Getters

  bool isNull() const { return !getData(); }
  const void *getData() const { return fRef ? pData : pMData; }
  void *getMData() {
    assert(!fRef);
    return pMData;
  }
  size_t getSize() const { return iSize; }
  bool isRef() const { return fRef; }

  const void *getPtr(size_t i) const {
    return reinterpret_cast<const char *>(getData()) + i;
  }
  void *getMPtr(size_t i) { return reinterpret_cast<char *>(getMData()) + i; }

  StdBuf getPart(size_t iStart, size_t inSize) const {
    assert(iStart + inSize <= iSize);
    return StdBuf(getPtr(iStart), inSize);
  }

  // *** Setters

  // * Direct setters

  // Reference given data
  void Ref(const void *pnData, size_t inSize) {
    Clear();
    fRef = true;
    pData = pnData;
    iSize = inSize;
  }
  // Take over data (hold it)
  void Take(void *pnData, size_t inSize) {
    Clear();
    if (pnData) {
      fRef = false;
      pMData = pnData;
      iSize = inSize;
    }
  }
  // Transfer puffer ownership to the caller
  void *GrabPointer() {
    if (isNull()) return NULL;
    // Do not give out a buffer which someone else will free
    if (fRef) Copy();
    void *pMData = getMData();
    pData = pMData;
    fRef = true;
    return pMData;
  }

  // * Buffer data operations

  // Create new buffer with given size
  void New(size_t inSize) {
    Clear();
    pMData = malloc(iSize = inSize);
    fRef = false;
  }
  // Write data into the buffer
  void Write(const void *pnData, size_t inSize, size_t iAt = 0) {
    assert(iAt + inSize <= iSize);
    if (pnData && inSize) memcpy(getMPtr(iAt), pnData, inSize);
  }
  // Move data around inside the buffer (checks overlap)
  void Move(size_t iFrom, size_t inSize, size_t iTo = 0) {
    assert(iFrom + inSize <= iSize);
    assert(iTo + inSize <= iSize);
    memmove(getMPtr(iTo), getPtr(iFrom), inSize);
  }
  // Compare to memory
  int Compare(const void *pCData, size_t iCSize, size_t iAt = 0) const {
    assert(iAt + iCSize <= getSize());
    return memcmp(getPtr(iAt), pCData, iCSize);
  }
  // Grow the buffer
  void Grow(size_t iGrow) {
    // Grow dereferences
    if (fRef) {
      Copy(iSize + iGrow);
      return;
    }
    if (!iGrow) return;
    // Realloc
    pMData = realloc(pMData, iSize += iGrow);
  }
  // Shrink the buffer
  void Shrink(size_t iShrink) {
    assert(iSize >= iShrink);
    // Shrink dereferences
    if (fRef) {
      Copy(iSize - iShrink);
      return;
    }
    if (!iShrink) return;
    // Realloc
    pMData = realloc(pMData, iSize -= iShrink);
  }
  // Clear buffer
  void Clear() {
    if (!fRef) free(pMData);
    pMData = NULL;
    fRef = true;
    iSize = 0;
  }
  // Free buffer that had been grabbed
  static void DeletePointer(void *data) { free(data); }

  // * Composed actions

  // Set buffer size (dereferences)
  void SetSize(size_t inSize) {
    if (inSize > iSize)
      Grow(inSize - iSize);
    else
      Shrink(iSize - inSize);
  }

  // Write buffer contents into the buffer
  void Write(const StdBuf &Buf2, size_t iAt = 0) {
    Write(Buf2.getData(), Buf2.getSize(), iAt);
  }

  // Compare (a part of) this buffer's contents to another's
  int Compare(const StdBuf &Buf2, size_t iAt = 0) const {
    return Compare(Buf2.getData(), Buf2.getSize(), iAt);
  }

  // Create a copy of the data (dereferences, obviously)
  void Copy(size_t inSize) {
    if (isNull() && !inSize) return;
    const void *pOldData = getData();
    size_t iOldSize = iSize;
    New(inSize);
    Write(pOldData, Min(iOldSize, inSize));
  }
  void Copy() { Copy(iSize); }
  // Copy data from address
  void Copy(const void *pnData, size_t inSize) {
    Ref(pnData, inSize);
    Copy();
  }
  // Copy from another buffer
  void Copy(const StdBuf &Buf2) { Copy(Buf2.getData(), Buf2.getSize()); }
  // Create a copy and return it
  StdBuf Duplicate() const {
    StdBuf Buf;
    Buf.Copy(*this);
    return Buf;
  }

  // Append data from address
  void Append(const void *pnData, int inSize) {
    Grow(inSize);
    Write(pnData, inSize, iSize - inSize);
  }
  // Append data from another buffer
  void Append(const StdBuf &Buf2) { Append(Buf2.getData(), Buf2.getSize()); }

  // Reference another buffer's contents
  void Ref(const StdBuf &Buf2) { Ref(Buf2.getData(), Buf2.getSize()); }
  // Create a reference to this buffer's contents
  StdBuf getRef() const { return StdBuf(getData(), getSize()); }
  // take over another buffer's contents
  void Take(StdBuf Buf2) { Take(Buf2.GrabPointer(), Buf2.getSize()); }

  // * File support
  bool LoadFromFile(const char *szFile);
  bool SaveToFile(const char *szFile) const;

  // *** Operators

  // Null check
  bool operator!() const { return isNull(); }

  // Appending
  StdBuf operator+=(const StdBuf &Buf2) {
    Append(Buf2);
    return *this;
  }
  StdBuf operator+(const StdBuf &Buf2) const {
    StdBuf Buf(getRef());
    Buf.Append(Buf2);
    return Buf;
  }

  // Compare
  bool operator==(const StdBuf &Buf2) const {
    return getSize() == Buf2.getSize() && !Compare(Buf2);
  }
  bool operator!=(const StdBuf &Buf2) const { return !operator==(Buf2); }

  // Set (as constructor: take if possible)
  StdBuf &operator=(StdBuf Buf2) {
    if (Buf2.isRef())
      Ref(Buf2);
    else
      Take(Buf2);
    return *this;
  }

  // build a simple hash
  int GetHash() const {
    if (isNull()) return 0;
    return crc32(0, reinterpret_cast<const Bytef *>(getData()), getSize());
  }

  // *** Compiling

  void CompileFunc(class StdCompiler *pComp, int iType = 0);
};

// Cast Hide Helpers - MSVC doesn't allow this as member template
template <class elem_t>
const elem_t *getBufPtr(const StdBuf &Buf, size_t iPos = 0) {
  // assert(iPos + sizeof(elem_t) <= Buf.getSize());
  const void *pPos = reinterpret_cast<const char *>(Buf.getData()) + iPos;
  return reinterpret_cast<const elem_t *>(pPos);
}
template <class elem_t>
elem_t *getMBufPtr(StdBuf &Buf, size_t iPos = 0) {
  // assert(iPos + sizeof(elem_t) <= Buf.getSize());
  void *pPos = reinterpret_cast<char *>(Buf.getMData()) + iPos;
  return reinterpret_cast<elem_t *>(pPos);
}

// Copy-Buffer - Just copies data in the copy constructor.
class StdCopyBuf : public StdBuf {
 public:
  StdCopyBuf() {}

  // Set by buffer. Copies data by default.
  StdCopyBuf(const StdBuf &Buf2) : StdBuf(Buf2.getRef()) {}

  // Set by buffer. Moves data by default.
  //  StdCopyBuf(StdCopyBuf &&Buf2) : StdBuf(Buf2.getRef()) {}

  // Set by constant data. Copies data by default.
  StdCopyBuf(const void *pData, size_t iSize, bool fCopy = true)
      : StdBuf(pData, iSize, fCopy) {}

  StdCopyBuf &operator=(const StdBuf &Buf2) {
    Copy(Buf2);
    return *this;
  }
  StdCopyBuf &operator=(const StdCopyBuf &Buf2) {
    Copy(Buf2);
    return *this;
  }
};

// Stringbuffer (operates on null-terminated character buffers)
class StdStrBuf : protected StdBuf {
 public:
  // *** Construction

  StdStrBuf() : StdBuf() {}

  // See StdBuf::StdBuf. Will take data if possible.
  StdStrBuf(const StdStrBuf &Buf2) : StdBuf(Buf2) {}
  StdStrBuf(StdStrBuf &&Buf2) : StdBuf(Buf2) {}

  // Set by constant data. References data by default, copies if specified.
  explicit StdStrBuf(const char *pData, bool fCopy = false)
      : StdBuf(pData, pData ? strlen(pData) + 1 : 0, fCopy) {}

  // As previous constructor, but set length manually.
  StdStrBuf(const char *pData, size_t iLength, bool fCopy = false)
      : StdBuf(pData, pData ? iLength + 1 : 0, fCopy) {}

 public:
  // *** Getters

  bool isNull() const { return StdBuf::isNull(); }
  const char *getData() const { return getBufPtr<char>(*this); }
  char *getMData() { return getMBufPtr<char>(*this); }
  size_t getSize() const { return StdBuf::getSize(); }
  size_t getLength() const { return getSize() ? getSize() - 1 : 0; }
  bool isRef() const { return StdBuf::isRef(); }

  const char *getPtr(size_t i) const { return getBufPtr<char>(*this, i); }
  char *getMPtr(size_t i) { return getMBufPtr<char>(*this, i); }

  // For convenience. Note that writing can't be allowed.
  char operator[](size_t i) const { return *getPtr(i); }

  // Analogous to StdBuf
  void Ref(const char *pnData) {
    StdBuf::Ref(pnData, pnData ? strlen(pnData) + 1 : 0);
  }
  void Ref(const char *pnData, size_t iLength) {
    assert((!pnData && !iLength) || strlen(pnData) == iLength);
    StdBuf::Ref(pnData, iLength + 1);
  }
  void Take(char *pnData) {
    StdBuf::Take(pnData, pnData ? strlen(pnData) + 1 : 0);
  }
  void Take(char *pnData, size_t iLength) {
    assert((!pnData && !iLength) || strlen(pnData) == iLength);
    StdBuf::Take(pnData, iLength + 1);
  }
  char *GrabPointer() {
    return reinterpret_cast<char *>(StdBuf::GrabPointer());
  }

  void Ref(const StdStrBuf &Buf2) {
    StdBuf::Ref(Buf2.getData(), Buf2.getSize());
  }
  StdStrBuf getRef() const { return StdStrBuf(getData(), getLength()); }
  void Take(StdStrBuf Buf2) { StdBuf::Take(Buf2); }

  void Clear() { StdBuf::Clear(); }
  void Copy() { StdBuf::Copy(); }
  void Copy(const char *pnData) {
    StdBuf::Copy(pnData, pnData ? strlen(pnData) + 1 : 0);
  }
  void Copy(const StdStrBuf &Buf2) { StdBuf::Copy(Buf2); }
  StdStrBuf Duplicate() const {
    StdStrBuf Buf;
    Buf.Copy(*this);
    return Buf;
  }
  void Move(size_t iFrom, size_t inSize, size_t iTo = 0) {
    StdBuf::Move(iFrom, inSize, iTo);
  }

  // Byte-wise compare (will compare characters up to the length of the second
  // string)
  int Compare(const StdStrBuf &Buf2, size_t iAt = 0) const {
    assert(iAt <= getLength());
    return StdBuf::Compare(Buf2.getData(), Buf2.getLength(), iAt);
  }
  int Compare_(const char *pCData, size_t iAt = 0) const {
    StdStrBuf str(pCData);  // GCC needs this, for some obscure reason
    return Compare(str, iAt);
  }

  // Grows the string to contain the specified number more/less characters.
  // Note: Will set the terminator, but won't initialize - use Append* instead.
  void Grow(size_t iGrow) {
    StdBuf::Grow(getSize() ? iGrow : iGrow + 1);
    *getMPtr(getLength()) = '\0';
  }
  void Shrink(size_t iShrink) {
    assert(iShrink <= getLength());
    StdBuf::Shrink(iShrink);
    *getMPtr(getLength()) = '\0';
  }
  void SetLength(size_t iLength) {
    if (iLength == getLength() && !isNull()) return;
    if (iLength >= getLength())
      Grow(iLength - getLength());
    else
      Shrink(getLength() - iLength);
  }

  // Append string
  void Append(const char *pnData, size_t iChars) {
    // assert(iChars <= strlen(pnData));
    Grow(iChars);
    Write(pnData, iChars, iSize - iChars - 1);
  }
  void Append(const char *pnData) { Append(pnData, strlen(pnData)); }
  void Append(const StdStrBuf &Buf2) {
    Append(Buf2.getData(), Buf2.getLength());
  }

  // Copy string
  void Copy(const char *pnData, size_t iChars) {
    Clear();
    Append(pnData, iChars);
  }

  // * File support
  bool LoadFromFile(const char *szFile);
  bool SaveToFile(const char *szFile) const;

  // * Operators

  bool operator!() const { return isNull(); }

  StdStrBuf &operator+=(const StdStrBuf &Buf2) {
    Append(Buf2);
    return *this;
  }
  StdStrBuf &operator+=(const char *szString) {
    Append(szString);
    return *this;
  }
  StdStrBuf operator+(const StdStrBuf &Buf2) const {
    StdStrBuf Buf = getRef();
    Buf.Append(Buf2);
    return Buf;
  }
  StdStrBuf operator+(const char *szString) const {
    StdStrBuf Buf = getRef();
    Buf.Append(szString);
    return Buf;
  }

  bool operator==(const StdStrBuf &Buf2) const {
    return getLength() == Buf2.getLength() && !Compare(Buf2);
  }
  bool operator!=(const StdStrBuf &Buf2) const { return !operator==(Buf2); }

  bool operator==(const char *szString) const {
    return StdStrBuf(szString) == *this;
  }
  bool operator!=(const char *szString) const { return !operator==(szString); }

  // Note this references the data.
  StdStrBuf &operator=(const StdStrBuf &Buf2) {
    Ref(Buf2);
    return *this;
  }
  StdStrBuf &operator=(const char *szString) {
    Ref(szString);
    return *this;
  }

  // conversion to "bool"
  operator const void *() const { return getData(); }

  // less-than operation for map
  inline bool operator<(const StdStrBuf &v2) {
    int iLen = getLength(), iLen2 = v2.getLength();
    if (iLen == iLen2)
      return iLen ? (strcmp(getData(), v2.getData()) < 0) : false;
    else
      return iLen < iLen2;
  }

  // * String specific

  void AppendChars(char cChar, size_t iCnt) {
    Grow(iCnt);
    for (size_t i = getLength() - iCnt; i < getLength(); i++)
      *getMPtr(i) = cChar;
  }
  void AppendChar(char cChar) { AppendChars(cChar, 1); }
  void InsertChar(char cChar, size_t insert_before) {
    assert(insert_before <= getLength());
    Grow(1);
    for (size_t i = getLength() - 1; i > insert_before; --i)
      *getMPtr(i) = *getPtr(i - 1);
    *getMPtr(insert_before) = cChar;
  }

  // Append data until given character (or string end) occurs.
  void AppendUntil(const char *szString, char cUntil) {
    const char *pPos = strchr(szString, cUntil);
    if (pPos)
      Append(szString, pPos - szString);
    else
      Append(szString);
  }
  // See above
  void CopyUntil(const char *szString, char cUntil) {
    Clear();
    AppendUntil(szString, cUntil);
  }
  // cut end after given char into another string. Return whether char was found
  // at all
  bool SplitAtChar(char cSplit, StdStrBuf *psSplit) {
    if (!getData()) return false;
    const char *pPos = strchr(getData(), cSplit);
    if (!pPos) return false;
    size_t iPos = pPos - getData();
    if (psSplit) psSplit->Take(copyPart(iPos + 1, getLength() - iPos - 1));
    Shrink(getLength() - iPos);
    return true;
  }

  void Format(const char *szFmt, ...) GNUC_FORMAT_ATTRIBUTE_O;
  void FormatV(const char *szFmt, va_list args);
  void AppendFormat(const char *szFmt, ...) GNUC_FORMAT_ATTRIBUTE_O;
  void AppendFormatV(const char *szFmt, va_list args);

  StdStrBuf copyPart(size_t iStart, size_t inSize) const {
    assert(iStart + inSize <= iSize);
    if (!inSize) return StdStrBuf();
    StdStrBuf sResult;
    sResult.Copy(getPtr(iStart), inSize);
    return sResult;
  }

  // replace all occurences of one string with another. Return number of
  // replacements.
  int Replace(const char *szOld, const char *szNew, size_t iStartSearch = 0);
  int ReplaceChar(char cOld, char cNew, size_t iStartSearch = 0);

  // replace the trailing part of a string with something else
  void ReplaceEnd(size_t iPos, const char *szNewEnd);

  // get an indexed section from the string like Section1;Section2;Section3
  bool GetSection(size_t idx, StdStrBuf *psOutSection,
                  char cSeparator = ';') const;

  // Checks wether the contents are valid UTF-8, and if not, convert them from
  // windows-1252 to UTF-8.
  void EnsureUnicode();

  // convert to lower case
  void ToLowerCase();

  // check if a string consists only of the given chars
  bool ValidateChars(const char *szInitialChars, const char *szMidChars);

  // build a simple hash
  int GetHash() const { return StdBuf::GetHash(); }

  void EscapeString() {
    Replace("\\", "\\\\");
    Replace("\"", "\\\"");
  }

  bool TrimSpaces();  // kill spaces at beginning and end. Return if changed.

  // * Compiling

  void CompileFunc(class StdCompiler *pComp, int iRawType = 0);
};

// Copy-Stringbuffer - Just copies data in the copy constructor.
class StdCopyStrBuf : public StdStrBuf {
 public:
  StdCopyStrBuf() {}

  explicit StdCopyStrBuf(const StdStrBuf &Buf2) : StdStrBuf(Buf2.getRef()) {}

  //  StdCopyStrBuf(StdCopyStrBuf &&Buf2) : StdStrBuf(Buf2) {}

  // Set by constant data. Copies data if desired.
  explicit StdCopyStrBuf(const char *pData, bool fCopy = true)
      : StdStrBuf(pData, fCopy) {}

  StdCopyStrBuf &operator=(const StdStrBuf &Buf2) {
    Copy(Buf2);
    return *this;
  }
  StdCopyStrBuf &operator=(const StdCopyStrBuf &Buf2) {
    Copy(Buf2);
    return *this;
  }
  StdCopyStrBuf &operator=(const char *szString) {
    Copy(szString);
    return *this;
  }
};

// Wrappers
extern StdStrBuf FormatString(const char *szFmt, ...) GNUC_FORMAT_ATTRIBUTE;
extern StdStrBuf FormatStringV(const char *szFmt, va_list args);

#endif
