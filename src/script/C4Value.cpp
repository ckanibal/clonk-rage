// C4Value.cpp: Implementierung der Klasse C4Value.
//
//////////////////////////////////////////////////////////////////////

#include "C4Include.h"
#include "script/C4Value.h"
#include "script/C4StringTable.h"
#include "C4ValueList.h"

#ifndef BIG_C4INCLUDE
#include "game/C4Game.h"
#ifdef C4ENGINE
#include "object/C4Object.h"
#include "C4Log.h"
#endif
#endif

const C4Value C4VNull = C4Value();
const C4Value C4VTrue = C4VBool(true);
const C4Value C4VFalse = C4VBool(false);

C4Value::~C4Value() {
  // resolve all C4Values referencing this Value
  while (FirstRef) FirstRef->Set(*this);

  // delete contents
  DelDataRef(Data, Type, GetNextRef(), GetBaseArray());
}

C4Value &C4Value::operator=(const C4Value &nValue) {
  // set referenced value
  if (Type == C4V_pC4Value)
    GetRefVal().operator=(nValue);
  else
    Set(nValue.GetRefVal());

  return *this;
}

void C4Value::AddDataRef() {
  switch (Type) {
    case C4V_pC4Value:
      Data.Ref->AddRef(this);
      break;
    case C4V_Any:
      if (Data) {
        GuessType();
      }
      break;
#ifdef C4ENGINE
    case C4V_Array:
      Data.Array = Data.Array->IncRef();
      break;
    case C4V_String:
      Data.Str->IncRef();
      break;
    case C4V_C4Object:
      Data.Obj->AddRef(this);
#ifdef _DEBUG
      // check if the object actually exists
      if (!Game.Objects.ObjectNumber(Data.Obj)) {
        LogF("Warning: using wild object ptr %p!", Data.Obj);
      } else if (!Data.Obj->Status) {
        LogF("Warning: using ptr on deleted object %p (%s)!", Data.Obj,
             Data.Obj->GetName());
      }
#endif
      break;
#endif
    default:
      break;
  }
}

void C4Value::DelDataRef(C4V_Data Data, C4V_Type Type, C4Value *pNextRef,
                         C4ValueArray *pBaseArray) {
  // clean up
  switch (Type) {
    case C4V_pC4Value:
      // Save because AddDataRef does not set this flag
      HasBaseArray = false;
      Data.Ref->DelRef(this, pNextRef, pBaseArray);
      break;
#ifdef C4ENGINE
    case C4V_C4Object:
      Data.Obj->DelRef(this, pNextRef);
      break;
    case C4V_Array:
      Data.Array->DecRef();
      break;
    case C4V_String:
      Data.Str->DecRef();
      break;
#endif
    default:
      break;
  }
}

void C4Value::Set(C4V_Data nData, C4V_Type nType) {
  // Do not add this to the same linked list twice.
  if (Data == nData && Type == nType) return;

  C4V_Data oData = Data;
  C4V_Type oType = Type;
  C4Value *oNextRef = NextRef;
  C4ValueArray *oBaseArray = BaseArray;
  bool oHasBaseArray = HasBaseArray;

  // change
  Data = nData;
  Type = nData ? nType : C4V_Any;

  // hold
  AddDataRef();

  // clean up
  DelDataRef(oData, oType, oHasBaseArray ? NULL : oNextRef,
             oHasBaseArray ? oBaseArray : NULL);
}

void C4Value::Set0() {
  C4V_Data oData = Data;
  C4V_Type oType = Type;

  // change
  Data = 0;
  Type = C4V_Any;

  // clean up (save even if Data was 0 before)
  DelDataRef(oData, oType, HasBaseArray ? NULL : NextRef,
             HasBaseArray ? BaseArray : NULL);
}

void C4Value::Move(C4Value *nValue) {
  nValue->Set(*this);

  // change references
  for (C4Value *pVal = FirstRef; pVal; pVal = pVal->GetNextRef())
    pVal->Data.Ref = nValue;

  // copy ref list
  assert(!nValue->FirstRef);
  nValue->FirstRef = FirstRef;

  // delete usself
  FirstRef = NULL;
  Set(0);
}

#ifdef C4ENGINE
void C4Value::GetArrayElement(int32_t Index, C4Value &target,
                              C4AulContext *pctx, bool noref) {
  C4Value &Ref = GetRefVal();
  // No array (and no nullpointer because Data==0 => Type==any)
  if (Ref.Type != C4V_Array)
    throw new C4AulExecError(pctx->Obj, "Array access: array expected");
  if (noref) {
    // Get the item, but do not resize the array - it might be used more than
    // once
    if (Index < Ref.Data.Array->GetSize())
      target.Set(Ref.Data.Array->GetItem(Index));
    else
      target.Set(0);
  } else {
    // Is target the first ref?
    if (Index >= Ref.Data.Array->GetSize() ||
        !Ref.Data.Array->GetItem(Index).FirstRef) {
      Ref.Data.Array = Ref.Data.Array->IncElementRef();
      target.SetRef(&Ref.Data.Array->GetItem(Index));
      if (target.Type == C4V_pC4Value) {
        assert(!target.NextRef);
        target.BaseArray = Ref.Data.Array;
        target.HasBaseArray = true;
      }
      // else target apparently owned the last reference to the array
    } else {
      target.SetRef(&Ref.Data.Array->GetItem(Index));
    }
  }
}

void C4Value::SetArrayLength(int32_t size, C4AulContext *cthr) {
  C4Value &Ref = GetRefVal();
  // No array
  if (Ref.Type != C4V_Array)
    throw new C4AulExecError(cthr->Obj, "SetLength: array expected");
  Ref.Data.Array = Ref.Data.Array->SetLength(size);
}
#endif

const C4Value &C4Value::GetRefVal() const {
  const C4Value *pVal = this;
  while (pVal->Type == C4V_pC4Value) pVal = pVal->Data.Ref;
  return *pVal;
}

C4Value &C4Value::GetRefVal() {
  C4Value *pVal = this;
  while (pVal->Type == C4V_pC4Value) pVal = pVal->Data.Ref;
  return *pVal;
}

void C4Value::AddRef(C4Value *pRef) {
  pRef->NextRef = FirstRef;
  FirstRef = pRef;
}

void C4Value::DelRef(const C4Value *pRef, C4Value *pNextRef,
                     C4ValueArray *pBaseArray) {
  if (pRef == FirstRef)
    FirstRef = pNextRef;
  else {
    C4Value *pVal = FirstRef;
    while (pVal->NextRef != pRef) {
      // assert that pRef really was in the list
      assert(pVal->NextRef && !pVal->HasBaseArray);
      pVal = pVal->NextRef;
    }
    pVal->NextRef = pNextRef;
    if (pBaseArray) {
      pVal->HasBaseArray = true;
      pVal->BaseArray = pBaseArray;
    }
  }
// Was pRef the last ref to an array element?
#ifdef C4ENGINE
  if (pBaseArray && !FirstRef) {
    pBaseArray->DecElementRef();
  }
#endif
}

C4V_Type C4Value::GuessType() {
  // guaranteed by the caller
  assert(Data);

  if (Type != C4V_Any) return Type;

  // C4ID?
  if (LooksLikeID(Data.Int) && Data.Int >= 10000) return Type = C4V_C4ID;

#ifdef C4ENGINE
  // object?
  if (Game.Objects.ObjectNumber(Data.Obj)) {
    Type = C4V_C4Object;
    // With the type now known, the destructor will clean up the reference
    // which only works if the reference is added first
    AddDataRef();
    return Type;
  }

  // string?
  if (Game.ScriptEngine.Strings.FindString(Data.Str)) {
    Type = C4V_String;
    // see above
    AddDataRef();
    return Type;
  }
#endif

  // must be int now
  return Type = C4V_Int;
}

const char *GetC4VName(const C4V_Type Type) {
  switch (Type) {
    case C4V_Any:
      return "any";
    case C4V_Int:
      return "int";
    case C4V_Bool:
      return "bool";
    case C4V_C4Object:
      return "object";
    case C4V_C4ID:
      return "id";
    case C4V_String:
      return "string";
    case C4V_Array:
      return "array";
    case C4V_pC4Value:
      return "&";
    default:
      return "!Fehler!";
  }
}

char GetC4VID(const C4V_Type Type) {
  switch (Type) {
    case C4V_Any:
      return 'A';
    case C4V_Int:
      return 'i';
    case C4V_Bool:
      return 'b';
    case C4V_C4Object:
      return 'o';
    case C4V_C4ID:
      return 'I';
    case C4V_String:
      return 'S';
    case C4V_pC4Value:
      return 'V';  // should never happen
    case C4V_C4ObjectEnum:
      return 'O';
    case C4V_Array:
      return 'a';
  }
  return ' ';
}

C4V_Type GetC4VFromID(const char C4VID) {
  switch (C4VID) {
    case 'A':
      return C4V_Any;
    case 'i':
      return C4V_Int;
    case 'b':
      return C4V_Bool;
    case 'o':
      return C4V_C4Object;
    case 'I':
      return C4V_C4ID;
    case 'S':
      return C4V_String;
    case 'V':
      return C4V_pC4Value;
    case 'O':
      return C4V_C4ObjectEnum;
    case 'a':
      return C4V_Array;
  }
  return C4V_Any;
}

const char *C4Value::GetTypeInfo() { return GetC4VName(GetType()); }

// converter functions ----------------

static bool FnCnvDirectOld(C4Value *Val, C4V_Type toType, BOOL fStrict) {
  // new syntax: failure
  if (fStrict) return FALSE;
  // old syntax: do nothing
  return TRUE;
}

static bool FnCnvError(C4Value *Val, C4V_Type toType, BOOL fStrict) {
  // deny convert
  return FALSE;
}

static bool FnCnvDeref(C4Value *Val, C4V_Type toType, BOOL fStrict) {
  // resolve reference of Value
  Val->Deref();
  // retry to check convert
  return Val->ConvertTo(toType, fStrict);
}

bool C4Value::FnCnvGuess(C4Value *Val, C4V_Type toType, BOOL fStrict) {
  if (Val->Data) {
    // guess type (always possible because data is not 0)
    Val->GuessType();
    //  try to convert new type
    return Val->ConvertTo(toType, fStrict);
  } else {
    // 0 is every possible type except a reference at the same time
    return true;
  }
}

bool C4Value::FnCnvInt2Id(C4Value *Val, C4V_Type toType, BOOL fStrict) {
  // inside range?
  if (!Inside<long>(Val->Data.Int, 0, 9999)) return FALSE;
  // convert
  Val->Type = C4V_C4ID;
  return TRUE;
}

// Type conversion table
#define CnvOK 0, false  // allow conversion by same value
#define CnvError FnCnvError, true
#define CnvGuess C4Value::FnCnvGuess, false
#define CnvInt2Id C4Value::FnCnvInt2Id, false
#define CnvDirectOld FnCnvDirectOld, true
#define CnvDeref FnCnvDeref, false

C4VCnvFn C4Value::C4ScriptCnvMap[C4V_Last + 1][C4V_Last + 1] = {
    {             // C4V_Any - always try guess
     {CnvOK},     // any        same
     {CnvGuess},  // int
     {CnvGuess},  // Bool
     {CnvGuess},  // C4ID
     {CnvGuess},  // C4Object
     {CnvGuess},  // String
     {CnvGuess},  // Array
     {CnvError},  // pC4Value
    },
    {              // C4V_Int
     {CnvOK},      // any
     {CnvOK},      // int        same
     {CnvOK},      // Bool
     {CnvInt2Id},  // C4ID       numerical ID?
     {CnvError},   // C4Object   NEVER!
     {CnvError},   // String     NEVER!
     {CnvError},   // Array      NEVER!
     {CnvError},   // pC4Value
    },
    {                 // C4V_Bool
     {CnvOK},         // any
     {CnvOK},         // int        might be used
     {CnvOK},         // Bool       same
     {CnvDirectOld},  // C4ID       #strict forbid
     {CnvError},      // C4Object   NEVER!
     {CnvError},      // String     NEVER!
     {CnvError},      // Array      NEVER!
     {CnvError},      // pC4Value
    },
    {                 // C4V_C4ID
     {CnvOK},         // any
     {CnvDirectOld},  // int        #strict forbid
     {CnvOK},         // Bool
     {CnvOK},         // C4ID       same
     {CnvError},      // C4Object   NEVER!
     {CnvError},      // String     NEVER!
     {CnvError},      // Array      NEVER!
     {CnvError},      // pC4Value
    },
    {                 // C4V_Object
     {CnvOK},         // any
     {CnvDirectOld},  // int        #strict forbid
     {CnvOK},         // Bool
     {CnvError},      // C4ID       Senseless, thus error
     {CnvOK},         // C4Object   same
     {CnvError},      // String     NEVER!
     {CnvError},      // Array      NEVER!
     {CnvError},      // pC4Value
    },
    {                 // C4V_String
     {CnvOK},         // any
     {CnvDirectOld},  // int        #strict forbid
     {CnvOK},         // Bool
     {CnvError},      // C4ID       Sensless, thus error
     {CnvError},      // C4Object   NEVER!
     {CnvOK},         // String     same
     {CnvError},      // Array      NEVER!
     {CnvError},      // pC4Value
    },
    {             // C4V_Array
     {CnvOK},     // any
     {CnvError},  // int        NEVER!
     {CnvOK},     // Bool
     {CnvError},  // C4ID       NEVER!
     {CnvError},  // C4Object   NEVER!
     {CnvError},  // String     NEVER!
     {CnvOK},     // Array      same
     {CnvError},  // pC4Value   NEVER!
    },
    {             // C4V_pC4Value - resolve reference and retry type check
     {CnvDeref},  // any
     {CnvDeref},  // int
     {CnvDeref},  // Bool
     {CnvDeref},  // C4ID
     {CnvDeref},  // C4Object
     {CnvDeref},  // String
     {CnvDeref},  // Array
     {CnvOK},     // pC4Value   same
    },
};

#undef CnvOK
#undef CvnError
#undef CnvGuess
#undef CnvInt2Id
#undef CnvDirectOld
#undef CnvDeref

// Humanreadable debug output
StdStrBuf C4Value::GetDataString() {
  if (Type == C4V_pC4Value) return GetRefVal().GetDataString() + "*";

  // ouput by type info
  switch (GetType()) {
    case C4V_Int:
    case C4V_Any:
      return FormatString("%ld", Data.Int);
    case C4V_Bool:
      return StdStrBuf(Data ? "true" : "false");
    case C4V_C4ID:
      return StdCopyStrBuf(C4IdText(Data.Int));
#ifdef C4ENGINE
    case C4V_C4Object: {
      // obj exists?
      if (!Game.Objects.ObjectNumber(Data.Obj) &&
          !Game.Objects.InactiveObjects.ObjectNumber(Data.Obj))
        return FormatString("%ld", Data.Int);
      else if (Data.Obj)
        if (Data.Obj->Status == C4OS_NORMAL)
          return FormatString("%s #%d", Data.Obj->GetName(),
                              (int)Data.Obj->Number);
        else
          return FormatString("{%s #%d}", Data.Obj->GetName(),
                              (int)Data.Obj->Number);
      else
        return StdStrBuf("0");  // (impossible)
    }
    case C4V_String:
      return (Data.Str && Data.Str->Data.getData())
                 ? FormatString("\"%s\"", Data.Str->Data.getData())
                 : StdStrBuf("(nullstring)");
    case C4V_Array: {
      StdStrBuf DataString;
      DataString = "[";
      for (int32_t i = 0; i < Data.Array->GetSize(); i++) {
        if (i) DataString.Append(", ");
        DataString.Append(static_cast<const StdStrBuf &>(
            Data.Array->GetItem(i).GetDataString()));
      }
      DataString.AppendChar(']');
      return DataString;
    }
#endif  // C4ENGINE
    default:
      return StdStrBuf("-unknown type- ");
  }
}

C4Value C4VString(const char *strString) {
#ifdef C4ENGINE
  // safety
  if (!strString) return C4Value();
  return C4Value(new C4String(strString, &Game.ScriptEngine.Strings));
#else
  return C4Value();
#endif
}

C4Value C4VString(StdStrBuf Str) {
#ifdef C4ENGINE
  // safety
  if (Str.isNull()) return C4Value();
  return C4Value(new C4String(Str, &Game.ScriptEngine.Strings));
#else
  return C4Value();
#endif
}

void C4Value::DenumeratePointer() {
#ifdef C4ENGINE
  // array?
  if (Type == C4V_Array) {
    Data.Array->DenumeratePointers();
    return;
  }
  // object types only
  if (Type != C4V_C4ObjectEnum && Type != C4V_Any) return;
  // in range?
  if (Type != C4V_C4ObjectEnum &&
      !Inside(Data.Int, C4EnumPointer1, C4EnumPointer2))
    return;
  // get obj id, search object
  int iObjID =
      (Data.Int >= C4EnumPointer1 ? Data.Int - C4EnumPointer1 : Data.Int);
  C4Object *pObj = Game.Objects.ObjectPointer(iObjID);
  if (!pObj) pObj = Game.Objects.InactiveObjects.ObjectPointer(iObjID);
  if (pObj)
    // set
    SetObject(pObj);
  else {
    // any: guess type
    if (Type == C4V_Any) {
      if (Data) GuessType();
    }
    // object: invalid value - set to zero
    else
      Set(0);
  }
#endif
}

void C4Value::CompileFunc(StdCompiler *pComp) {
  // Type
  bool fCompiler = pComp->isCompiler();
  if (!fCompiler) {
    // Get type
    if (Type == C4V_Any && Data) GuessType();
    char cC4VID = GetC4VID(Type);
    // Object reference is saved enumerated
    if (Type == C4V_C4Object) cC4VID = GetC4VID(C4V_C4ObjectEnum);
    // Write
    pComp->Character(cC4VID);
  } else {
    // Clear
    Set(0);
    // Read type
    char cC4VID;
    try {
      pComp->Character(cC4VID);
    } catch (StdCompiler::NotFoundException *pExc) {
      delete pExc;
      cC4VID = 'A';
    }
    Type = GetC4VFromID(cC4VID);
  }
  // Data
  int32_t iTmp;
  switch (Type) {
    // simple data types: just save
    case C4V_Any:
    case C4V_Int:
    case C4V_Bool:
    case C4V_C4ID:

      // these are 32-bit integers
      iTmp = Data.Int;
      pComp->Value(iTmp);
      Data.Int = iTmp;

      break;

    // object: save object number instead
    case C4V_C4Object:
#ifdef C4ENGINE
      if (!fCompiler) iTmp = Game.Objects.ObjectNumber(getObj());
#else
      if (!fCompiler) iTmp = 0;
#endif
    case C4V_C4ObjectEnum:
      if (!fCompiler)
        if (Type == C4V_C4ObjectEnum) iTmp = Data.Int;
      pComp->Value(iTmp);
      if (fCompiler) {
        Type = C4V_C4ObjectEnum;
        Data.Int = iTmp;  // must be denumerated later
      }
      break;

// string: save string number
#ifdef C4ENGINE
    case C4V_String:
      if (!fCompiler) iTmp = getStr()->iEnumID;
      pComp->Value(iTmp);
      // search
      if (fCompiler) {
        C4String *pString = Game.ScriptEngine.Strings.FindString(iTmp);
        if (pString) {
          Data.Str = pString;
          pString->IncRef();
        } else
          Type = C4V_Any;
      }
      break;

    case C4V_Array:
      pComp->Seperator(StdCompiler::SEP_START2);
      pComp->Value(mkPtrAdapt(Data.Array, false));
      if (fCompiler) Data.Array = Data.Array->IncRef();
      pComp->Seperator(StdCompiler::SEP_END2);
      break;

    // shouldn't happen
    case C4V_pC4Value:

#endif  // C4ENGINE

    default:
      assert(false);
      break;
  }
}

bool C4Value::operator==(const C4Value &Value2) const {
  switch (Type) {
    case C4V_Any:
      assert(!Data);
      return Data == Value2.Data;
    case C4V_Int:
      switch (Value2.Type) {
        case C4V_Any:
          assert(!Value2.Data);
          return Data == Value2.Data;
        case C4V_Int:
        case C4V_Bool:
          return Data == Value2.Data;
        case C4V_C4ID:
          if (Inside<long>(Value2.Data.Int, 0, 9999))
            return Data == Value2.Data;
          return false;
        default:
          return false;
      }
    case C4V_Bool:
      switch (Value2.Type) {
        case C4V_Any:
          assert(!Value2.Data);
          return Data == Value2.Data;
        case C4V_Int:
        case C4V_Bool:
          return Data == Value2.Data;
        default:
          return false;
      }
    case C4V_C4ID:
      switch (Value2.Type) {
        case C4V_Any:
          assert(!Value2.Data);
          return Data == Value2.Data;
        case C4V_C4ID:
          return Data == Value2.Data;
        case C4V_Int:
          if (Inside<long>(Value2.Data.Int, 0, 9999))
            return Data == Value2.Data;
          return false;
        default:
          return false;
      }
    case C4V_C4Object:
      return Data == Value2.Data && Type == Value2.Type;
    case C4V_String:
      return Type == Value2.Type && Data.Str->Data == Value2.Data.Str->Data;
    case C4V_Array:
      return Type == Value2.Type && *(Data.Array) == *(Value2.Data.Array);
      break;
    default:
      // C4AulExec should have dereferenced both values, no need to implement
      // comparison here
      return Data == Value2.Data;
  }
  return GetData() == Value2.GetData();
}
bool C4Value::operator!=(const C4Value &Value2) const {
  // Fixme: implement faster
  return !(*this == Value2);
}
