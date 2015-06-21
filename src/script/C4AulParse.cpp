/* by Sven2, 2001 */
// parses scripts

#include "C4Include.h"
#include "C4Aul.h"

#ifndef BIG_C4INCLUDE
#include "object/C4Def.h"
#include "game/C4Game.h"
#include "C4Wrappers.h"
#endif

#define DEBUG_BYTECODE_DUMP 0

#define C4AUL_Include "#include"
#define C4AUL_Strict "#strict"
#define C4AUL_Append "#appendto"

#define C4AUL_Func "func"

#define C4AUL_Private "private"
#define C4AUL_Protected "protected"
#define C4AUL_Public "public"
#define C4AUL_Global "global"
#define C4AUL_Const "const"

#define C4AUL_If "if"
#define C4AUL_Else "else"
#define C4AUL_While "while"
#define C4AUL_For "for"
#define C4AUL_In "in"
#define C4AUL_Return "return"
#define C4AUL_Var "Var"
#define C4AUL_Par "Par"
#define C4AUL_Goto "goto"
#define C4AUL_Break "break"
#define C4AUL_Continue "continue"
#define C4AUL_Inherited "inherited"
#define C4AUL_SafeInherited "_inherited"
#define C4AUL_this "this"

#define C4AUL_Image "Image"
#define C4AUL_Contents "Contents"
#define C4AUL_Condition "Condition"
#define C4AUL_Method "Method"
#define C4AUL_Desc "Desc"

#define C4AUL_MethodAll "All"
#define C4AUL_MethodNone "None"
#define C4AUL_MethodClassic "Classic"
#define C4AUL_MethodJumpAndRun "JumpAndRun"

#define C4AUL_GlobalNamed "static"
#define C4AUL_LocalNamed "local"
#define C4AUL_VarNamed "var"

#define C4AUL_TypeInt "int"
#define C4AUL_TypeBool "bool"
#define C4AUL_TypeC4ID "id"
#define C4AUL_TypeC4Object "object"
#define C4AUL_TypeString "string"
#define C4AUL_TypeArray "array"

#define C4AUL_True "true"
#define C4AUL_False "false"

#define C4AUL_CodeBufSize 16

// script token type
enum C4AulTokenType {
  ATT_INVALID,   // invalid token
  ATT_DIR,       // directive
  ATT_IDTF,      // identifier
  ATT_INT,       // integer constant
  ATT_BOOL,      // boolean constant
  ATT_STRING,    // string constant
  ATT_C4ID,      // C4ID constant
  ATT_COMMA,     // ","
  ATT_COLON,     // ":"
  ATT_DCOLON,    // "::"
  ATT_SCOLON,    // ";"
  ATT_BOPEN,     // "("
  ATT_BCLOSE,    // ")"
  ATT_BOPEN2,    // "["
  ATT_BCLOSE2,   // "]"
  ATT_BLOPEN,    // "{"
  ATT_BLCLOSE,   // "}"
  ATT_SEP,       // "|"
  ATT_CALL,      // "->"
  ATT_STAR,      // "*"
  ATT_AMP,       // "&"
  ATT_TILDE,     // '~'
  ATT_LDOTS,     // '...'
  ATT_OPERATOR,  // operator
  ATT_EOF        // end of file
};

class C4AulParseState {
 public:
  typedef enum { PARSER, PREPARSER } TypeType;
  C4AulParseState(C4AulScriptFunc *Fn, C4AulScript *a, TypeType Type)
      : Fn(Fn),
        a(a),
        SPos(Fn ? Fn->Script : a->Script.getData()),
        Done(false),
        Type(Type),
        fJump(false),
        iStack(0),
        pLoopStack(NULL) {}
  ~C4AulParseState() {
    while (pLoopStack) PopLoop();
  }
  C4AulScriptFunc *Fn;
  C4AulScript *a;
  const char *SPos;                 // current position in the script
  char Idtf[C4AUL_MAX_Identifier];  // current identifier
  C4AulTokenType TokenType;         // current token type
  long cInt;      // current int constant (long for compatibility with x86_64)
  bool Done;      // done parsing?
  TypeType Type;  // emitting bytecode?
  void Parse_Script();
  void Parse_FuncHead();
  void Parse_Desc();
  void Parse_Function();
  void Parse_Statement();
  void Parse_Block();
  int Parse_Params(int iMaxCnt, const char *sWarn, C4AulFunc *pFunc = 0);
  void Parse_Array();
  void Parse_While();
  void Parse_If();
  void Parse_For();
  void Parse_ForEach();
  void Parse_Expression(int iParentPrio = -1);
  void Parse_Expression2(int iParentPrio = -1);
  void Parse_Var();
  void Parse_Local();
  void Parse_Static();
  void Parse_Const();

  BOOL AdvanceSpaces();  // skip whitespaces; return whether script ended
  int GetOperator(const char *pScript);
  enum HoldStringsPolicy { Discard, Hold, Ref };
  C4AulTokenType GetNextToken(char *pToken, long *pInt,
                              HoldStringsPolicy HoldStrings,
                              bool bOperator);  // get next token of SPos

  void Shift(HoldStringsPolicy HoldStrings = Hold, bool bOperator = true);
  void Match(C4AulTokenType TokenType, const char *Message = NULL);
  void UnexpectedToken(const char *Expected);
  const char *GetTokenName(C4AulTokenType TokenType);

  void Warn(const char *pMsg, const char *pIdtf = 0);
  void StrictError(const char *pMsg, const char *pIdtf = 0);
  void Strict2Error(const char *pMsg, const char *pIdtf = 0);

 private:
  bool fJump;
  int iStack;

  void AddBCC(C4AulBCCType eType, intptr_t X = 0);

  void SetNoRef();  // Switches the bytecode to generate a value instead of a
                    // reference

  int JumpHere();  // Get position for a later jump to next instruction added
  void SetJumpHere(int iJumpOp);  // Use the next inserted instruction as jump
                                  // target for the given jump operation
  void SetJump(int iJumpOp, int iWhere);
  void AddJump(C4AulBCCType eType, int iWhere);

  // Keep track of loops and break/continue usages
  struct Loop {
    struct Control {
      bool Break;
      int Pos;
      Control *Next;
    };
    Control *Controls;
    int StackSize;
    Loop *Next;
  };
  Loop *pLoopStack;

  void PushLoop();
  void PopLoop();
  void AddLoopControl(bool fBreak);
};

void C4AulScript::Warn(const char *pMsg, const char *pIdtf) {
  // display error

  C4AulParseError warning(this, pMsg, pIdtf, TRUE);
  // display it
  warning.show();
  // count warnings
  ++Game.ScriptEngine.warnCnt;
}

void C4AulParseState::Warn(const char *pMsg, const char *pIdtf) {
  // do not show errors for System.c4g scripts that appear to be pure #appendto
  // scripts
  if (Fn && !Fn->Owner->Def && Fn->Owner->Appends) return;
  // script doesn't own function -> skip
  // (exception: global functions)
  // if(pFunc) if(pFunc->pOrgScript != pScript && pScript != (C4AulScript
  // *)&Game.ScriptEngine) return;
  // display error

  C4AulParseError warning(this, pMsg, pIdtf, TRUE);
  // display it
  warning.show();
  if (Fn && Fn->pOrgScript != a)
    DebugLogF("  (as #appendto/#include to %s)",
              Fn->Owner->ScriptName.getData());
  // count warnings
  ++Game.ScriptEngine.warnCnt;
}

void C4AulParseState::StrictError(const char *pMsg, const char *pIdtf) {
  if (Fn ? (Fn->pOrgScript->Strict < C4AulScript::STRICT1)
         : (a->Strict < C4AulScript::STRICT1))
    Warn(pMsg, pIdtf);
  else
    throw new C4AulParseError(this, pMsg, pIdtf);
}
void C4AulParseState::Strict2Error(const char *pMsg, const char *pIdtf) {
  if (Fn ? (Fn->pOrgScript->Strict < C4AulScript::STRICT2)
         : (a->Strict < C4AulScript::STRICT2))
    Warn(pMsg, pIdtf);
  else
    throw new C4AulParseError(this, pMsg, pIdtf);
}

C4AulParseError::C4AulParseError(C4AulParseState *state, const char *pMsg,
                                 const char *pIdtf, BOOL Warn)
    : C4AulError() {
  // compose error string
  sMessage.Format("%s: %s%s", Warn ? "WARNING" : "ERROR", pMsg,
                  pIdtf ? pIdtf : "");
  if (state->Fn && *(state->Fn->Name)) {
    // Show function name
    sMessage.AppendFormat(" (in %s", state->Fn->Name);

    // Exact position
    if (state->Fn->pOrgScript && state->SPos)
      sMessage.AppendFormat(
          ", %s:%d:%d)", state->Fn->pOrgScript->ScriptName.getData(),
          SGetLine(state->Fn->pOrgScript->Script.getData(), state->SPos),
          SLineGetCharacters(state->Fn->pOrgScript->Script.getData(),
                             state->SPos));
    else
      sMessage.AppendChar(')');
  } else if (state->a) {
    // Script name
    sMessage.AppendFormat(
        " (%s:%d:%d)", state->a->ScriptName.getData(),
        SGetLine(state->a->Script.getData(), state->SPos),
        SLineGetCharacters(state->a->Script.getData(), state->SPos));
  }
}

C4AulParseError::C4AulParseError(C4AulScript *pScript, const char *pMsg,
                                 const char *pIdtf, BOOL Warn) {
  // compose error string
  sMessage.Format("%s: %s%s", Warn ? "WARNING" : "ERROR", pMsg,
                  pIdtf ? pIdtf : "");
  if (pScript) {
    // Script name
    sMessage.AppendFormat(" (%s)", pScript->ScriptName.getData());
  }
}

void C4AulScriptFunc::ParseDesc() {
  // do nothing if no desc is given
  if (!Desc.getLength()) return;
  const char *DPos = Desc.getData();
  // parse desc
  while (*DPos) {
    const char *DPos0 = DPos;  // beginning of segment
    const char *DPos2 = NULL;  // pos of equal sign, if found
    // parse until end of segment
    while (*DPos && (*DPos != '|')) {
      // store break pos if found
      if (*DPos == '=')
        if (!DPos2) DPos2 = DPos;
      DPos++;
    }

    // if this was an assignment segment, get value to assign
    if (DPos2) {
      char Val[C4AUL_MAX_String + 1];
      SCopyUntil(DPos2 + 1, (char *)&Val, *DPos, C4AUL_MAX_String);
      // Image
      if (SEqual2(DPos0, C4AUL_Image)) {
        // image: special contents-image?
        if (SEqual((const char *)&Val, C4AUL_Contents))
          idImage = C4ID_Contents;
        else {
          // Find phase separator (:)
          char *colon;
          for (colon = Val; *colon != ':' && *colon != '\0'; ++colon)
            ;
          if (*colon == ':')
            *colon = '\0';
          else
            colon = NULL;
          // get image id
          idImage = C4Id((const char *)&Val);
          // get image phase
          if (colon) iImagePhase = atoi(colon + 1);
        }
      }
      // Condition
      else if (SEqual2(DPos0, C4AUL_Condition))
        // condition? get condition func
        Condition = Owner->GetFuncRecursive((const char *)&Val);
      // Method
      else if (SEqual2(DPos0, C4AUL_Method)) {
        if (SEqual2(Val, C4AUL_MethodAll))
          ControlMethod = C4AUL_ControlMethod_All;
        else if (SEqual2(Val, C4AUL_MethodNone))
          ControlMethod = C4AUL_ControlMethod_None;
        else if (SEqual2(Val, C4AUL_MethodClassic))
          ControlMethod = C4AUL_ControlMethod_Classic;
        else if (SEqual2(Val, C4AUL_MethodJumpAndRun))
          ControlMethod = C4AUL_ControlMethod_JumpAndRun;
        else
          // unrecognized: Default to all
          ControlMethod = C4AUL_ControlMethod_All;
      }
      // Long Description
      else if (SEqual2(DPos0, C4AUL_Desc)) {
        DescLong.CopyUntil(Val, '|');
      }
      // unrecognized? never mind
    }

    if (*DPos) DPos++;
  }
  assert(!Condition || !Condition->Owner->Def ||
         Condition->Owner->Def == Owner->Def);
  // copy desc text
  DescText.CopyUntil(Desc.getData(), '|');
}

BOOL C4AulParseState::AdvanceSpaces() {
  char C, C2 = (char)0;
  // defaultly, not in comment
  int InComment = 0;  // 0/1/2 = no comment/line comment/multi line comment
  // don't go past end
  while (C = *SPos) {
    // loop until out of comment and non-whitespace is found
    switch (InComment) {
      case 0:
        if (C == '/') {
          SPos++;
          switch (*SPos) {
            case '/':
              InComment = 1;
              break;
            case '*':
              InComment = 2;
              break;
            default:
              SPos--;
              return TRUE;
          }
        } else if ((BYTE)C > 32)
          return TRUE;
        break;
      case 1:
        if (((BYTE)C == 13) || ((BYTE)C == 10)) InComment = 0;
        break;
      case 2:
        if ((C == '/') && (C2 == '*')) InComment = 0;
        break;
    }
    // next char; store prev
    SPos++;
    C2 = C;
  }
  // end of script reached; return FALSE
  return FALSE;
}

//=========================== C4Script Operator Map
//===================================

C4ScriptOpDef C4ScriptOpMap[] = {
    // priority                     postfix  RetType
    // |  identifier                |  right-associative
    // |  |     Bytecode            |  |  no second id ParType1      ParType2
    // prefix
    {15, "++", AB_Inc1, 0, 1, 0, C4V_Int, C4V_pC4Value, C4V_Any},
    {15, "--", AB_Dec1, 0, 1, 0, C4V_Int, C4V_pC4Value, C4V_Any},
    {15, "~", AB_BitNot, 0, 1, 0, C4V_Int, C4V_Int, C4V_Any},
    {15, "!", AB_Not, 0, 1, 0, C4V_Bool, C4V_Bool, C4V_Any},
    {15, "+", AB_ERR, 0, 1, 0, C4V_Int, C4V_Int, C4V_Any},
    {15, "-", AB_Neg, 0, 1, 0, C4V_Int, C4V_Int, C4V_Any},

    // postfix (whithout second statement)
    {16, "++", AB_Inc1_Postfix, 1, 1, 1, C4V_Int, C4V_pC4Value, C4V_Any},
    {16, "--", AB_Dec1_Postfix, 1, 1, 1, C4V_Int, C4V_pC4Value, C4V_Any},

    // postfix
    {14, "**", AB_Pow, 1, 0, 0, C4V_Int, C4V_Int, C4V_Int},
    {13, "/", AB_Div, 1, 0, 0, C4V_Int, C4V_Int, C4V_Int},
    {13, "*", AB_Mul, 1, 0, 0, C4V_Int, C4V_Int, C4V_Int},
    {13, "%", AB_Mod, 1, 0, 0, C4V_Int, C4V_Int, C4V_Int},
    {12, "-", AB_Sub, 1, 0, 0, C4V_Int, C4V_Int, C4V_Int},
    {12, "+", AB_Sum, 1, 0, 0, C4V_Int, C4V_Int, C4V_Int},
    {11, "<<", AB_LeftShift, 1, 0, 0, C4V_Int, C4V_Int, C4V_Int},
    {11, ">>", AB_RightShift, 1, 0, 0, C4V_Int, C4V_Int, C4V_Int},
    {10, "<", AB_LessThan, 1, 0, 0, C4V_Bool, C4V_Int, C4V_Int},
    {10, "<=", AB_LessThanEqual, 1, 0, 0, C4V_Bool, C4V_Int, C4V_Int},
    {10, ">", AB_GreaterThan, 1, 0, 0, C4V_Bool, C4V_Int, C4V_Int},
    {10, ">=", AB_GreaterThanEqual, 1, 0, 0, C4V_Bool, C4V_Int, C4V_Int},
    {9, "==", AB_Equal, 1, 0, 0, C4V_Bool, C4V_Any, C4V_Any},
    {9, "!=", AB_NotEqual, 1, 0, 0, C4V_Bool, C4V_Any, C4V_Any},
    {9, "S=", AB_SEqual, 1, 0, 0, C4V_Bool, C4V_String, C4V_String},
    {9, "eq", AB_SEqual, 1, 0, 0, C4V_Bool, C4V_String, C4V_String},
    {9, "ne", AB_SNEqual, 1, 0, 0, C4V_Bool, C4V_String, C4V_String},
    {8, "&", AB_BitAnd, 1, 0, 0, C4V_Int, C4V_Int, C4V_Int},
    {6, "^", AB_BitXOr, 1, 0, 0, C4V_Int, C4V_Int, C4V_Int},
    {6, "|", AB_BitOr, 1, 0, 0, C4V_Int, C4V_Int, C4V_Int},
    {5, "&&", AB_And, 1, 0, 0, C4V_Bool, C4V_Bool, C4V_Bool},
    {4, "||", AB_Or, 1, 0, 0, C4V_Bool, C4V_Bool, C4V_Bool},
    {2, "*=", AB_MulIt, 1, 1, 0, C4V_Any, C4V_pC4Value, C4V_Int},
    {2, "/=", AB_DivIt, 1, 1, 0, C4V_Any, C4V_pC4Value, C4V_Int},
    {2, "%=", AB_ModIt, 1, 1, 0, C4V_Any, C4V_pC4Value, C4V_Int},
    {2, "+=", AB_Inc, 1, 1, 0, C4V_Any, C4V_pC4Value, C4V_Int},
    {2, "-=", AB_Dec, 1, 1, 0, C4V_Any, C4V_pC4Value, C4V_Int},
    {2, "&=", AB_AndIt, 1, 1, 0, C4V_Any, C4V_pC4Value, C4V_Int},
    {2, "|=", AB_OrIt, 1, 1, 0, C4V_Any, C4V_pC4Value, C4V_Int},
    {2, "^=", AB_XOrIt, 1, 1, 0, C4V_Any, C4V_pC4Value, C4V_Int},
    {2, "=", AB_Set, 1, 1, 0, C4V_Any, C4V_pC4Value, C4V_Any},

    {0, NULL, AB_ERR, 0, 0, 0, C4V_Any, C4V_Any, C4V_Any}};

int C4AulParseState::GetOperator(const char *pScript) {
  // return value:
  // >= 0: operator found. could be found in C4ScriptOfDef
  // -1:   isn't an operator

  unsigned int i;

  if (!*pScript) return 0;
  // text with > 2 characters or text and #strict 2?
  // then break (may be misinterpreted as operator
  if ((*pScript >= 'a' && *pScript <= 'z') ||
      (*pScript >= 'A' && *pScript <= 'Z')) {
    if (Fn ? (Fn->pOrgScript->Strict >= C4AulScript::STRICT2)
           : (a->Strict >= C4AulScript::STRICT2))
      return -1;
    if ((*(pScript + 1) >= 'a' && *(pScript + 1) <= 'z') ||
        (*(pScript + 1) >= 'A' && *(pScript + 1) <= 'Z'))
      if ((*(pScript + 2) >= 'a' && *(pScript + 2) <= 'z') ||
          (*(pScript + 2) >= 'A' && *(pScript + 2) <= 'Z') ||
          (*(pScript + 2) >= '0' && *(pScript + 2) <= '9') ||
          *(pScript + 2) == '_')
        return -1;
  }

  // it is a two-char-operator?
  for (i = 0; C4ScriptOpMap[i].Identifier; i++)
    if (SLen(C4ScriptOpMap[i].Identifier) == 2)
      if (SEqual2(pScript, C4ScriptOpMap[i].Identifier)) return i;

  // if not, a one-char one?
  for (i = 0; C4ScriptOpMap[i].Identifier; i++)
    if (SLen(C4ScriptOpMap[i].Identifier) == 1)
      if (SEqual2(pScript, C4ScriptOpMap[i].Identifier)) return i;

  return -1;
}

C4AulTokenType C4AulParseState::GetNextToken(char *pToken, long int *pInt,
                                             HoldStringsPolicy HoldStrings,
                                             bool bOperator) {
  // move to start of token
  if (!AdvanceSpaces()) return ATT_EOF;
  // store offset
  const char *SPos0 = SPos;
  int Len = 0;
  // token get state
  enum TokenGetState {
    TGS_None,    // just started
    TGS_Ident,   // getting identifier
    TGS_Int,     // getting integer
    TGS_IntHex,  // getting hexadecimal integer
    TGS_C4ID,    // getting C4ID
    TGS_String,  // getting string
    TGS_Dir      // getting directive
  };
  TokenGetState State = TGS_None;

  static char StrBuff[C4AUL_MAX_String + 1];
  char *pStrPos = StrBuff;

  // loop until finished
  while (TRUE) {
    // get char
    char C = *SPos;

    switch (State) {
      case TGS_None: {
        // get token type by first char (+/- are operators)
        if (((C >= '0') && (C <= '9')))
          State = TGS_Int;  // integer by 0-9
        else if (C == '"')
          State = TGS_String;  // string by "
        else if (C == '#')
          State = TGS_Dir;  // directive by "#"
        else if (C == ',') {
          SPos++;
          return ATT_COMMA;
        }  // ","
        else if (C == ';') {
          SPos++;
          return ATT_SCOLON;
        }  // ";"
        else if (C == '(') {
          SPos++;
          return ATT_BOPEN;
        }  // "("
        else if (C == ')') {
          SPos++;
          return ATT_BCLOSE;
        }  // ")"
        else if (C == '[') {
          SPos++;
          return ATT_BOPEN2;
        }  // "["
        else if (C == ']') {
          SPos++;
          return ATT_BCLOSE2;
        }  // "]"
        else if (C == '{') {
          SPos++;
          return ATT_BLOPEN;
        }  // "{"
        else if (C == '}') {
          SPos++;
          return ATT_BLCLOSE;
        }                   // "}"
        else if (C == ':')  // ":"
        {
          SPos++;
          // double-colon?
          if (*SPos == ':')  // "::"
          {
            SPos++;
            return ATT_DCOLON;
          } else  // ":"
            return ATT_COLON;
        } else if (C == '-' && *(SPos + 1) == '>')  // "->"
        {
          SPos += 2;
          return ATT_CALL;
        } else if (C == '.' && *(SPos + 1) == '.' &&
                   *(SPos + 2) == '.')  // "..."
        {
          SPos += 3;
          return ATT_LDOTS;
        } else {
          if (bOperator) {
            // may it be an operator?
            int iOpID;
            if ((iOpID = GetOperator(SPos)) != -1) {
              // store operator ID in pInt
              *pInt = iOpID;
              SPos += SLen(C4ScriptOpMap[iOpID].Identifier);
              return ATT_OPERATOR;
            }
          } else if (C == '*') {
            SPos++;
            return ATT_STAR;
          }  // "*"
          else if (C == '&') {
            SPos++;
            return ATT_AMP;
          }  // "&"
          else if (C == '~') {
            SPos++;
            return ATT_TILDE;
          }  // "~"

          // identifier by all non-special chars
          if (C >= '@') {
            // Old Scripts could have wacky identifier
            if (a->Strict < C4AulScript::STRICT2) {
              State = TGS_Ident;
              break;
            }
            // But now only the alphabet and '_' is allowed
            else if ((C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z') ||
                     (C == '_')) {
              State = TGS_Ident;
              break;
            }
            // unrecognized char
            // make sure to skip the invalid char so the error won't be output
            // forever
            ++SPos;
          } else {
            // examine next char
            ++SPos;
            ++Len;
            // no operator expected and '-' or '+' found?
            // this could be an int const; parse it directly
            if (!bOperator && (C == '-' || C == '+')) {
              // skip spaces between sign and int constant
              if (AdvanceSpaces()) {
                // continue parsing int, if a numeral follows
                C = *SPos;
                if (((C >= '0') && (C <= '9'))) {
                  State = TGS_Int;
                  break;
                }
              }
            }
            // special char and/or error getting it as a signed int
          }
          // unrecognized char
          // show appropriate error message
          if (C >= '!' && C <= '~')
            throw new C4AulParseError(
                this, FormatString("unexpected character '%c' found",
                                   (int)(unsigned char) C).getData());
          else
            throw new C4AulParseError(
                this, FormatString("unexpected character 0x%x found",
                                   (int)(unsigned char) C).getData());
        }
        break;
      }
      case TGS_Ident:  // ident and directive: parse until non ident-char is
                       // found
      case TGS_Dir:
        if (!Inside(C, '0', '9') && !Inside(C, 'a', 'z') &&
            !Inside(C, 'A', 'Z') && C != '_') {
          // return ident/directive
          Len = Min(Len, C4AUL_MAX_Identifier);
          SCopy(SPos0, pToken, Len);
          // check if it's a C4ID (and NOT a label)
          BOOL fllid = LooksLikeID(pToken);
          if ((C != '(') && (C != ':' || *(SPos + 1) == ':') && fllid) {
            // will be parsed next time
            State = TGS_C4ID;
            SPos--;
            Len--;
          } else {
            // warn if using C4ID as func label
            if (fllid) Strict2Error("stupid func label: ", pToken);
            // directive?
            if (State == TGS_Dir) return ATT_DIR;
            // check reserved names
            if (SEqual(pToken, C4AUL_False)) {
              *pInt = false;
              return ATT_BOOL;
            }
            if (SEqual(pToken, C4AUL_True)) {
              *pInt = true;
              return ATT_BOOL;
            }
            // everything else is an identifier
            return ATT_IDTF;
          }
        }
        break;

      case TGS_Int:  // integer: parse until non-number is found
      case TGS_IntHex:
        if ((C < '0') || (C > '9')) {
          const char *szScanCode;
          if (State == TGS_Int) {
            // turn to hex mode?
            if (*SPos0 == '0' && C == 'x' && Len == 1) {
              State = TGS_IntHex;
              break;
            }
            // some strange C4ID?
            if (((C >= 'A') && (C <= 'Z')) || (C == '_')) {
              State = TGS_C4ID;
              break;
            }
            // parse as decimal int
            szScanCode = "%ld";
          } else {
            // parse as hexadecimal int: Also allow 'a' to 'f' and 'A' to 'F'
            if ((C >= 'A' && C <= 'F') || (C >= 'a' && C <= 'f')) break;
            szScanCode = "%lx";
          }
          // return integer
          Len = Min(Len, C4AUL_MAX_Identifier);
          SCopy(SPos0, pToken, Len);
          // or is it a func label?
          if ((C == '(') || (C == ':')) {
            Strict2Error("stupid func label: ", pToken);
            return ATT_IDTF;
          }
          // it's not, so return the int
          sscanf(pToken, szScanCode, pInt);
          return ATT_INT;
        }
        break;

      case TGS_C4ID:  // c4id: parse until non-ident char is found
        if (!Inside(C, '0', '9') && !Inside(C, 'a', 'z') &&
            !Inside(C, 'A', 'Z')) {
          // return C4ID string
          Len = Min(Len, C4AUL_MAX_Identifier);
          SCopy(SPos0, pToken, Len);
          // another stupid label identifier?
          if ((C == '(') || (C == ':' && *(SPos + 1) != ':')) {
            Strict2Error("stupid func label: ", pToken);
            return ATT_IDTF;
          }
          // check if valid
          if (!LooksLikeID(pToken))
            throw new C4AulParseError(this, "erroneous Ident: ", pToken);
          // get id of it
          *pInt = (int)C4Id(pToken);
          return ATT_C4ID;
        }
        break;

      case TGS_String:  // string: parse until '"'; check for eof!

        // string end
        if (C == '"') {
          *pStrPos = 0;
          SPos++;
          // no string expected?
          if (HoldStrings == Discard) return ATT_STRING;
          // reg string (if not already done so)
          C4String *pString;
          if (!(pString = a->Engine->Strings.FindString(StrBuff)))
            pString = a->Engine->Strings.RegString(StrBuff);
          if (HoldStrings == Hold) pString->Hold = 1;
          // return pointer on string object
          *pInt = (long)pString;
          return ATT_STRING;
        }
        // check: enough buffer space?
        if (pStrPos - StrBuff >= C4AUL_MAX_String)
          Warn("string too long", NULL);
        else {
          if (C == '\\')  // escape
            switch (*(SPos + 1)) {
              case '"':
                SPos++;
                *(pStrPos++) = '"';
                break;
              case '\\':
                SPos++;
                *(pStrPos++) = '\\';
                break;
              default: {
                // just insert "\"
                *(pStrPos++) = '\\';
                // show warning
                char strEscape[2] = {*(SPos + 1), 0};
                Warn("unknown escape: ", strEscape);
              }
            }
          else if (C == 0 || C == 10 || C == 13)  // line break / feed
            throw new C4AulParseError(this, "string not closed");

          else
            // copy character
            *(pStrPos++) = C;
        }
        break;
    }
    // next char
    SPos++;
    Len++;
  }
}

static const char *GetTTName(C4AulBCCType e) {
  switch (e) {
    case AB_ARRAYA_R:
      return "AB_ARRAYA_R";  // array access
    case AB_ARRAYA_V:
      return "AB_ARRAYA_V";  // not creating a reference
    case AB_VARN_R:
      return "AB_VARN_R";  // a named var
    case AB_VARN_V:
      return "AB_VARN_V";
    case AB_PARN_R:
      return "AB_PARN_R";  // a named parameter
    case AB_PARN_V:
      return "AB_PARN_V";
    case AB_LOCALN_R:
      return "AB_LOCALN_R";  // a named local
    case AB_LOCALN_V:
      return "AB_LOCALN_V";
    case AB_GLOBALN_R:
      return "AB_GLOBALN_R";  // a named global
    case AB_GLOBALN_V:
      return "AB_GLOBALN_V";
    case AB_VAR_R:
      return "AB_VAR_R";  // Var statement
    case AB_VAR_V:
      return "AB_VAR_V";
    case AB_PAR_R:
      return "AB_PAR_R";  // Par statement
    case AB_PAR_V:
      return "AB_PAR_V";
    case AB_FUNC:
      return "AB_FUNC";  // function

    // prefix
    case AB_Inc1:
      return "AB_Inc1";  // ++
    case AB_Dec1:
      return "AB_Dec1";  // --
    case AB_BitNot:
      return "AB_BitNot";  // ~
    case AB_Not:
      return "AB_Not";  // !
                        // +
    case AB_Neg:
      return "AB_Neg";  // -

    // postfix (whithout second statement)
    case AB_Inc1_Postfix:
      return "AB_Inc1_Postfix";  // ++
    case AB_Dec1_Postfix:
      return "AB_Dec1_Postfix";  // --

    // postfix
    case AB_Pow:
      return "AB_Pow";  // **
    case AB_Div:
      return "AB_Div";  // /
    case AB_Mul:
      return "AB_Mul";  // *
    case AB_Mod:
      return "AB_Mod";  // %
    case AB_Sub:
      return "AB_Sub";  // -
    case AB_Sum:
      return "AB_Sum";  // +
    case AB_LeftShift:
      return "AB_LeftShift";  // <<
    case AB_RightShift:
      return "AB_RightShift";  // >>
    case AB_LessThan:
      return "AB_LessThan";  // <
    case AB_LessThanEqual:
      return "AB_LessThanEqual";  // <=
    case AB_GreaterThan:
      return "AB_GreaterThan";  // >
    case AB_GreaterThanEqual:
      return "AB_GreaterThanEqual";  // >=
    case AB_EqualIdent:
      return "AB_EqualIdent";  // old ==
    case AB_Equal:
      return "AB_Equal";  // new ==
    case AB_NotEqualIdent:
      return "AB_NotEqualIdent";  // old !=
    case AB_NotEqual:
      return "AB_NotEqual";  // new !=
    case AB_SEqual:
      return "AB_SEqual";  // S=, eq
    case AB_SNEqual:
      return "AB_SNEqual";  // ne
    case AB_BitAnd:
      return "AB_BitAnd";  // &
    case AB_BitXOr:
      return "AB_BitXOr";  // ^
    case AB_BitOr:
      return "AB_BitOr";  // |
    case AB_And:
      return "AB_And";  // &&
    case AB_Or:
      return "AB_Or";  // ||
    case AB_MulIt:
      return "AB_MulIt";  // *=
    case AB_DivIt:
      return "AB_DivIt";  // /=
    case AB_ModIt:
      return "AB_ModIt";  // %=
    case AB_Inc:
      return "AB_Inc";  // +=
    case AB_Dec:
      return "AB_Dec";  // -=
    case AB_AndIt:
      return "AB_AndIt";  // &=
    case AB_OrIt:
      return "AB_OrIt";  // |=
    case AB_XOrIt:
      return "AB_XOrIt";  // ^=
    case AB_Set:
      return "AB_Set";  // =

    case AB_CALL:
      return "AB_CALL";  // direct object call
    case AB_CALLFS:
      return "AB_CALLFS";  // failsafe direct call
    case AB_CALLNS:
      return "AB_CALLNS";  // direct object call: namespace operator
    case AB_STACK:
      return "AB_STACK";  // push nulls / pop
    case AB_INT:
      return "AB_INT";  // constant: int
    case AB_BOOL:
      return "AB_BOOL";  // constant: bool
    case AB_STRING:
      return "AB_STRING";  // constant: string
    case AB_C4ID:
      return "AB_C4ID";  // constant: C4ID
    case AB_ARRAY:
      return "AB_ARRAY";  // semi-constant: array
    case AB_IVARN:
      return "AB_IVARN";  // initialization of named var
    case AB_JUMP:
      return "AB_JUMP";  // jump
    case AB_JUMPAND:
      return "AB_JUMPAND";
    case AB_JUMPOR:
      return "AB_JUMPOR";
    case AB_CONDN:
      return "AB_CONDN";  // conditional jump (negated, pops stack)
    case AB_FOREACH_NEXT:
      return "AB_FOREACH_NEXT";  // foreach: next element
    case AB_RETURN:
      return "AB_RETURN";  // return statement
    case AB_ERR:
      return "AB_ERR";  // parse error at this position
    case AB_EOFN:
      return "AB_EOFN";  // end of function
    case AB_EOF:
      return "AB_EOF";

    default:
      return "?";
  }
}

void C4AulScript::AddBCC(C4AulBCCType eType, intptr_t X, const char *SPos) {
  // range check
  if (CodeSize >= CodeBufSize) {
    // create new buffer
    CodeBufSize = CodeBufSize ? 2 * CodeBufSize : C4AUL_CodeBufSize;
    C4AulBCC *nCode = new C4AulBCC[CodeBufSize];
    // copy data
    memcpy(nCode, Code, sizeof(*Code) * CodeSize);
    // replace buffer
    delete[] Code;
    Code = nCode;
    // adjust pointer
    CPos = Code + CodeSize;
  }
  // store chunk
  CPos->bccType = eType;
  CPos->bccX = X;
  CPos->SPos = SPos;
  CPos++;
  CodeSize++;
}

BOOL C4AulScript::Preparse() {
  // handle easiest case first
  if (State < ASS_NONE) return FALSE;
  if (!Script) {
    State = ASS_PREPARSED;
    return TRUE;
  }

  // clear stuff
  /* simply setting Includes to NULL will waste some space in the associative
     list
          however, this is just a few bytes per updated definition in developer
     mode, which
          seems acceptable for me. The mem will be released when destroying the
     list */
  Includes = NULL;
  Appends = NULL;
  CPos = Code;
  while (Func0) {
    // belongs to this script?
    if (Func0->SFunc())
      if (Func0->SFunc()->pOrgScript == this)
        // then desroy linked funcs, too
        Func0->DestroyLinked();
    // destroy func
    delete Func0;
  }

  C4AulParseState state(0, this, C4AulParseState::PREPARSER);
  state.Parse_Script();

  // no #strict? we don't like that :(
  if (!Strict) {
    Engine->nonStrictCnt++;
  }

  // done, reset state var
  Preparsing = FALSE;

  // #include will have to be resolved now...
  IncludesResolved = false;

  // return success
  C4AulScript::State = ASS_PREPARSED;
  return TRUE;
}

void C4AulParseState::AddBCC(C4AulBCCType eType, intptr_t X) {
  if (Type != PARSER) return;
  // Track stack size
  switch (eType) {
    case AB_INT:
    case AB_BOOL:
    case AB_STRING:
    case AB_C4ID:
    case AB_VARN_R:
    case AB_VARN_V:
    case AB_PARN_R:
    case AB_PARN_V:
    case AB_LOCALN_R:
    case AB_LOCALN_V:
    case AB_GLOBALN_R:
    case AB_GLOBALN_V:
      iStack++;
      break;

    case AB_Pow:
    case AB_Div:
    case AB_Mul:
    case AB_Mod:
    case AB_Sub:
    case AB_Sum:
    case AB_LeftShift:
    case AB_RightShift:
    case AB_LessThan:
    case AB_LessThanEqual:
    case AB_GreaterThan:
    case AB_GreaterThanEqual:
    case AB_Equal:
    case AB_EqualIdent:
    case AB_NotEqual:
    case AB_NotEqualIdent:
    case AB_SEqual:
    case AB_SNEqual:
    case AB_BitAnd:
    case AB_BitXOr:
    case AB_BitOr:
    case AB_And:
    case AB_Or:
    case AB_MulIt:
    case AB_DivIt:
    case AB_ModIt:
    case AB_Inc:
    case AB_Dec:
    case AB_AndIt:
    case AB_OrIt:
    case AB_XOrIt:
    case AB_Set:
    case AB_ARRAYA_R:
    case AB_ARRAYA_V:
    case AB_CONDN:
    case AB_IVARN:
    case AB_RETURN:
    // JUMPAND/JUMPOR are special: They either jump over instructions adding one
    // to the stack
    // or decrement the stack. Thus, for stack counting purposes, they
    // decrement.
    case AB_JUMPAND:
    case AB_JUMPOR:
      iStack--;
      break;

    case AB_FUNC:
      iStack -= reinterpret_cast<C4AulFunc *>(X)->GetParCount() - 1;
      break;

    case AB_CALL:
    case AB_CALLFS:
      iStack -= C4AUL_MAX_Par;
      break;

    case AB_Inc1:
    case AB_Dec1:
    case AB_BitNot:
    case AB_Not:
    case AB_Neg:
    case AB_Inc1_Postfix:
    case AB_Dec1_Postfix:
    case AB_VAR_R:
    case AB_VAR_V:
    case AB_PAR_R:
    case AB_PAR_V:
    case AB_FOREACH_NEXT:
    case AB_ERR:
    case AB_EOFN:
    case AB_EOF:
    case AB_JUMP:
    case AB_CALLNS:
      break;

    case AB_STACK:
      iStack += X;
      break;

    case AB_ARRAY:
      iStack -= X - 1;
      break;

    default:
      assert(false);
  }

  // Use stack operation instead of 0-Int (enable optimization)
  if ((eType == AB_INT || eType == AB_BOOL) && !X) {
    eType = AB_STACK;
    X = 1;
  }

  // Join checks only if it's not a jump target
  if (!fJump) {
    // Join together stack operations
    if (eType == AB_STACK && a->CPos > a->Code &&
        (a->CPos - 1)->bccType == AB_STACK &&
        (X <= 0 || (a->CPos - 1)->bccX >= 0)) {
      (a->CPos - 1)->bccX += X;
      // Empty? Remove it.
      if (!(a->CPos - 1)->bccX) {
        a->CPos--;
        a->CodeSize--;
      }
      return;
    }
  }

  // Add
  a->AddBCC(eType, X, SPos);

  // Reset jump flag
  fJump = false;
}

void C4AulParseState::SetNoRef() {
  if (Type != PARSER) return;
  C4AulBCC *CPos = a->CPos - 1;
  switch (CPos->bccType) {
    case AB_ARRAYA_R:
      CPos->bccType = AB_ARRAYA_V;
      break;
    case AB_PAR_R:
      CPos->bccType = AB_PAR_V;
      break;
    case AB_VAR_R:
      CPos->bccType = AB_VAR_V;
      break;
    case AB_PARN_R:
      CPos->bccType = AB_PARN_V;
      break;
    case AB_VARN_R:
      CPos->bccType = AB_VARN_V;
      break;
    case AB_LOCALN_R:
      CPos->bccType = AB_LOCALN_V;
      break;
    case AB_GLOBALN_R:
      CPos->bccType = AB_GLOBALN_V;
      break;
  }
}

int C4AulParseState::JumpHere() {
  // Set flag so the next generated code chunk won't get joined
  fJump = true;
  return a->GetCodePos();
}

void C4AulParseState::SetJumpHere(int iJumpOp) {
  if (Type != PARSER) return;
  // Set target
  C4AulBCC *pBCC = a->GetCodeByPos(iJumpOp);
  assert(pBCC->bccType == AB_JUMP || pBCC->bccType == AB_JUMPAND ||
         pBCC->bccType == AB_JUMPOR || pBCC->bccType == AB_CONDN);
  pBCC->bccX = a->GetCodePos() - iJumpOp;
  // Set flag so the next generated code chunk won't get joined
  fJump = true;
}

void C4AulParseState::SetJump(int iJumpOp, int iWhere) {
  if (Type != PARSER) return;
  // Set target
  C4AulBCC *pBCC = a->GetCodeByPos(iJumpOp);
  assert(pBCC->bccType == AB_JUMP || pBCC->bccType == AB_JUMPAND ||
         pBCC->bccType == AB_JUMPOR || pBCC->bccType == AB_CONDN);
  pBCC->bccX = iWhere - iJumpOp;
}

void C4AulParseState::AddJump(C4AulBCCType eType, int iWhere) {
  AddBCC(eType, iWhere - a->GetCodePos());
}

void C4AulParseState::PushLoop() {
  if (Type != PARSER) return;
  Loop *pNew = new Loop();
  pNew->StackSize = iStack;
  pNew->Controls = NULL;
  pNew->Next = pLoopStack;
  pLoopStack = pNew;
}

void C4AulParseState::PopLoop() {
  if (Type != PARSER) return;
  // Delete loop controls
  Loop *pLoop = pLoopStack;
  while (pLoop->Controls) {
    // Unlink
    Loop::Control *pCtrl = pLoop->Controls;
    pLoop->Controls = pCtrl->Next;
    // Delete
    delete pCtrl;
  }
  // Unlink & delete
  pLoopStack = pLoop->Next;
  delete pLoop;
}

void C4AulParseState::AddLoopControl(bool fBreak) {
  if (Type != PARSER) return;
  Loop::Control *pNew = new Loop::Control();
  pNew->Break = fBreak;
  pNew->Pos = a->GetCodePos();
  pNew->Next = pLoopStack->Controls;
  pLoopStack->Controls = pNew;
}

const char *C4AulParseState::GetTokenName(C4AulTokenType TokenType) {
  switch (TokenType) {
    case ATT_INVALID:
      return "invalid token";
    case ATT_DIR:
      return "directive";
    case ATT_IDTF:
      return "identifier";
    case ATT_INT:
      return "integer constant";
    case ATT_BOOL:
      return "boolean constant";
    case ATT_STRING:
      return "string constant";
    case ATT_C4ID:
      return "id constant";
    case ATT_COMMA:
      return "','";
    case ATT_COLON:
      return "':'";
    case ATT_DCOLON:
      return "'::'";
    case ATT_SCOLON:
      return "';'";
    case ATT_BOPEN:
      return "'('";
    case ATT_BCLOSE:
      return "')'";
    case ATT_BOPEN2:
      return "'['";
    case ATT_BCLOSE2:
      return "']'";
    case ATT_BLOPEN:
      return "'{'";
    case ATT_BLCLOSE:
      return "'}'";
    case ATT_SEP:
      return "'|'";
    case ATT_CALL:
      return "'->'";
    case ATT_STAR:
      return "'*'";
    case ATT_AMP:
      return "'&'";
    case ATT_TILDE:
      return "'~'";
    case ATT_LDOTS:
      return "'...'";
    case ATT_OPERATOR:
      return "operator";
    case ATT_EOF:
      return "end of file";
    default:
      return "unrecognized token";
  }
}

void C4AulParseState::Shift(HoldStringsPolicy HoldStrings, bool bOperator) {
  TokenType = GetNextToken(Idtf, &cInt, HoldStrings, bOperator);
}
void C4AulParseState::Match(C4AulTokenType RefTokenType, const char *Message) {
  if (TokenType != RefTokenType)
    // error
    throw new C4AulParseError(
        this,
        Message ? Message : FormatString("%s expected, but found %s",
                                         GetTokenName(RefTokenType),
                                         GetTokenName(TokenType)).getData());
  Shift();
}
void C4AulParseState::UnexpectedToken(const char *Expected) {
  throw new C4AulParseError(this,
                            FormatString("%s expected, but found %s", Expected,
                                         GetTokenName(TokenType)).getData());
}

void C4AulScript::ParseFn(C4AulScriptFunc *Fn, bool fExprOnly) {
  // check if fn overloads other fn (all func tables are built now)
  // *MUST* check Fn->Owner-list, because it may be the engine (due to linked
  // globals)
  if (Fn->OwnerOverloaded = Fn->Owner->GetOverloadedFunc(Fn))
    if (Fn->Owner == Fn->OwnerOverloaded->Owner)
      Fn->OwnerOverloaded->OverloadedBy = Fn;
  // reset pointer to next same-named func (will be set in AfterLink)
  Fn->NextSNFunc = NULL;
  // store byte code pos
  // (relative position to code start; code pointer may change while
  //  parsing)
  Fn->Code = (C4AulBCC *)(CPos - Code);
  // parse
  C4AulParseState state(Fn, this, C4AulParseState::PARSER);
  // get first token
  state.Shift();
  if (!fExprOnly)
    state.Parse_Function();
  else {
    state.Parse_Expression();
    AddBCC(AB_RETURN, 0, state.SPos);
  }
  // done
  return;
}

void C4AulParseState::Parse_Script() {
  int IncludeCount = 0;
  bool fDone = false;
  const char *SPos0 = SPos;
  bool all_ok = true;
  while (!fDone) try {
      // Go to the next token if the current token could not be processed or no
      // token has yet been parsed
      if (SPos == SPos0) {
        Shift();
      }
      SPos0 = SPos;
      switch (TokenType) {
        case ATT_DIR: {
          // check for include statement
          if (SEqual(Idtf, C4AUL_Include)) {
            Shift();
            // get id of script to include
            if (TokenType != ATT_C4ID) UnexpectedToken("id constant");
            C4ID Id = (C4ID)cInt;
            Shift();
            // add to include list
            C4AListEntry *e = a->Engine->itbl.push(Id);
            if (!a->Includes) a->Includes = e;
            IncludeCount++;
          } else if (SEqual(Idtf, C4AUL_Append)) {
            // for #appendto * '*' needs to be ATT_STAR, not an operator.
            Shift(Hold, false);
            // get id of script to include/append
            C4ID Id;
            switch (TokenType) {
              case ATT_C4ID:
                Id = (C4ID)cInt;
                Shift();
                break;
              case ATT_STAR:  // "*"
                Id = ~0;
                Shift();
                break;
              default:
                // -> ID expected
                UnexpectedToken("id constant");
            }
            // add to append list
            C4AListEntry *e = a->Engine->atbl.push(Id);
            if (!a->Appends) a->Appends = e;
          } else if (SEqual(Idtf, C4AUL_Strict)) {
            // declare it as strict
            a->Strict = C4AulScript::STRICT1;
            Shift();
            if (TokenType == ATT_INT) {
              if (cInt == 2)
                a->Strict = C4AulScript::STRICT2;
              else
                throw new C4AulParseError(this, "unknown strict level");
              Shift();
            }
          } else
            // -> unknown directive
            throw new C4AulParseError(this, "unknown directive: ", Idtf);
          break;
        }
        case ATT_IDTF: {
          if (SEqual(Idtf, C4AUL_For)) {
            throw new C4AulParseError(this, "unexpected for outside function");
          }
          // check for variable definition (var)
          else if (SEqual(Idtf, C4AUL_VarNamed)) {
            throw new C4AulParseError(
                this, "unexpected variable definition outside function");
          }
          // check for object-local variable definition (local)
          else if (SEqual(Idtf, C4AUL_LocalNamed)) {
            Shift();
            Parse_Local();
            Match(ATT_SCOLON);
            break;
          }
          // check for variable definition (static)
          else if (SEqual(Idtf, C4AUL_GlobalNamed)) {
            Shift();
            // constant?
            if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_Const)) {
              Shift();
              Parse_Const();
            } else
              Parse_Static();
            Match(ATT_SCOLON);
            break;
          } else
            Parse_FuncHead();
          break;
        }
        case ATT_EOF:
          fDone = true;
          break;
        default:
          UnexpectedToken("declaration");
      }
      all_ok = true;
    } catch (C4AulError *err) {
      // damn! something went wrong, print it out
      // but only one error per function
      if (all_ok) err->show();
      all_ok = false;
      delete err;
    }

  // includes were added?
  if (a->Includes) {
    // reverse include order, for compatiblity with the C4Script syntax
    if (IncludeCount > 1) {
      C4AListEntry *i = a->Includes;
      while (IncludeCount > 1) {
        C4AListEntry *i2 = i;
        for (int cnt = IncludeCount - 1; cnt; cnt--) i2 = i2->next();
        C4AListEntry xchg = *i;
        *i = *i2;
        *i2 = xchg;
        i = i->next();
        IncludeCount -= 2;
      }
    }
    // push stop entry for include list
    a->Engine->itbl.push();
  }

  // appends were added?
  if (a->Appends) {
    // push stop entry for append list
    a->Engine->atbl.push();
  }
}

void C4AulParseState::Parse_FuncHead() {
  C4AulAccess Acc = AA_PUBLIC;
  // Access?
  if (SEqual(Idtf, C4AUL_Private)) {
    Acc = AA_PRIVATE;
    Shift();
  } else if (SEqual(Idtf, C4AUL_Protected)) {
    Acc = AA_PROTECTED;
    Shift();
  } else if (SEqual(Idtf, C4AUL_Public)) {
    Acc = AA_PUBLIC;
    Shift();
  } else if (SEqual(Idtf, C4AUL_Global)) {
    Acc = AA_GLOBAL;
    Shift();
  }
  // check for func declaration
  if (SEqual(Idtf, C4AUL_Func)) {
    Shift(Discard, false);
    bool bReturnRef = false;
    // get next token, must be func name or "&"
    if (TokenType == ATT_AMP) {
      bReturnRef = true;
      Shift(Discard, false);
    }
    if (TokenType != ATT_IDTF) UnexpectedToken("function name");
    // check: symbol already in use?
    switch (Acc) {
      case AA_PRIVATE:
      case AA_PROTECTED:
      case AA_PUBLIC:
        if (a->LocalNamed.GetItemNr(Idtf) != -1)
          throw new C4AulParseError(
              this,
              "function definition: name already in use (local variable)");
        if (a->Def) break;
      // func in global context: fallthru
      case AA_GLOBAL:
        if (a->Engine->GlobalNamedNames.GetItemNr(Idtf) != -1)
          throw new C4AulParseError(
              this,
              "function definition: name already in use (global variable)");
        if (a->Engine->GlobalConstNames.GetItemNr(Idtf) != -1)
          Strict2Error(
              "function definition: name already in use (global variable)", 0);
    }
    // create script fn
    if (Acc == AA_GLOBAL) {
      // global func
      Fn = new C4AulScriptFunc(a->Engine, Idtf);
      C4AulFunc *FnLink = new C4AulFunc(a, NULL);
      FnLink->LinkedTo = Fn;
      Fn->LinkedTo = FnLink;
      Acc = AA_PUBLIC;
    } else {
      // normal, local func
      Fn = new C4AulScriptFunc(a, Idtf);
    }
    // set up func (in the case we got an error)
    Fn->Script = SPos;  // temporary
    Fn->Access = Acc;
    Fn->pOrgScript = a;
    Fn->bNewFormat = true;
    Fn->bReturnRef = bReturnRef;
    Shift(Discard, false);
    // expect an opening bracket now
    if (TokenType != ATT_BOPEN) UnexpectedToken("'('");
    Shift(Discard, false);
    // get pars
    Fn->ParNamed.Reset();  // safety :)
    int cpar = 0;
    while (1) {
      // closing bracket?
      if (TokenType == ATT_BCLOSE) {
        Fn->Script = SPos;
        Shift();
        // end of params
        break;
      }
      // too many parameters?
      if (cpar >= C4AUL_MAX_Par)
        throw new C4AulParseError(
            this, "'func' parameter list: too many parameters (max 10)");
      // must be a name or type now
      if (TokenType != ATT_IDTF && TokenType != ATT_AMP) {
        UnexpectedToken("parameter or closing bracket");
      }
      char TypeIdtf[C4AUL_MAX_Identifier] = "";  // current identifier
      if (TokenType == ATT_IDTF) {
        SCopy(Idtf, TypeIdtf);
      }
      // type identifier?
      if (SEqual(Idtf, C4AUL_TypeInt)) {
        Fn->ParType[cpar] = C4V_Int;
        Shift(Discard, false);
      } else if (SEqual(Idtf, C4AUL_TypeBool)) {
        Fn->ParType[cpar] = C4V_Bool;
        Shift(Discard, false);
      } else if (SEqual(Idtf, C4AUL_TypeC4ID)) {
        Fn->ParType[cpar] = C4V_C4ID;
        Shift(Discard, false);
      } else if (SEqual(Idtf, C4AUL_TypeC4Object)) {
        Fn->ParType[cpar] = C4V_C4Object;
        Shift(Discard, false);
      } else if (SEqual(Idtf, C4AUL_TypeString)) {
        Fn->ParType[cpar] = C4V_String;
        Shift(Discard, false);
      } else if (SEqual(Idtf, C4AUL_TypeArray)) {
        Fn->ParType[cpar] = C4V_Array;
        Shift(Discard, false);
      }
      // ampersand?
      if (TokenType == ATT_AMP) {
        Fn->ParType[cpar] = C4V_pC4Value;
        Shift(Discard, false);
      }
      if (TokenType != ATT_IDTF) {
        if (SEqual(TypeIdtf, ""))
          UnexpectedToken("parameter or closing bracket");
        // A parameter with the same name as a type
        Strict2Error("parameter has the same name as type ", TypeIdtf);
        Fn->ParType[cpar] = C4V_Any;
        Fn->ParNamed.AddName(TypeIdtf);
      } else {
        Fn->ParNamed.AddName(Idtf);
        Shift();
      }
      // end of params?
      if (TokenType == ATT_BCLOSE) {
        Fn->Script = SPos;
        Shift();
        break;
      }
      // must be a comma now
      if (TokenType != ATT_COMMA) UnexpectedToken("comma or closing bracket");
      Shift(Discard, false);
      cpar++;
    }
    // ok, expect opening block
    if (TokenType != ATT_BLOPEN) {
      // warn
      Strict2Error(
          "'func': expecting opening block ('{') after func declaration", 0);
      // not really new syntax (a sort of legacy mode)
      Fn->bNewFormat = false;
    } else {
      Fn->Script = SPos;
      Shift();
    }
    Parse_Desc();
    Parse_Function();
    Match(ATT_BLCLOSE);
    return;
  }
  // Must be old-style function declaration now
  if (a->Strict >= C4AulScript::STRICT2)
    throw new C4AulParseError(
        this, "Declaration expected, but found identifier ", Idtf);
  // check: symbol already in use?
  switch (Acc) {
    case AA_PRIVATE:
    case AA_PROTECTED:
    case AA_PUBLIC:
      if (a->LocalNamed.GetItemNr(Idtf) != -1)
        throw new C4AulParseError(
            this, "function definition: name already in use (local variable)");
      if (a->Def) break;
    // func in global context: fallthru
    case AA_GLOBAL:
      if (a->Engine->GlobalNamedNames.GetItemNr(Idtf) != -1)
        throw new C4AulParseError(
            this, "function definition: name already in use (global variable)");
  }
  char FuncIdtf[C4AUL_MAX_Identifier] = "";  // current identifier
  SCopy(Idtf, FuncIdtf);
  Shift();
  if (TokenType != ATT_COLON)
    throw new C4AulParseError(
        this, FormatString("declaration expected, but found identifier '%s'",
                           FuncIdtf).getData());
  // create script fn
  if (Acc == AA_GLOBAL) {
    // global func
    Fn = new C4AulScriptFunc(a->Engine, FuncIdtf);
    C4AulFunc *FnLink = new C4AulFunc(a, NULL);
    FnLink->LinkedTo = Fn;
    Fn->LinkedTo = FnLink;
    Acc = AA_PUBLIC;
  } else {
    // normal, local func
    Fn = new C4AulScriptFunc(a, FuncIdtf);
  }
  Fn->Script = SPos;
  Fn->Access = Acc;
  Fn->pOrgScript = a;
  Fn->bNewFormat = false;
  Fn->bReturnRef = false;
  Shift();
  Parse_Desc();
  const char *SPos0 = SPos;
  while (1) switch (TokenType) {
      // end of function
      case ATT_EOF:
      case ATT_DIR:
        return;
      case ATT_IDTF: {
        // check for func declaration
        if (SEqual(Idtf, C4AUL_Private))
          return;
        else if (SEqual(Idtf, C4AUL_Protected))
          return;
        else if (SEqual(Idtf, C4AUL_Public))
          return;
        else if (SEqual(Idtf, C4AUL_Global))
          return;
        else if (SEqual(Idtf, C4AUL_Func))
          return;
        // check for variable definition (var)
        else if (SEqual(Idtf, C4AUL_VarNamed)) {
          Shift();
          Parse_Var();
        }
        // check for variable definition (local)
        else if (SEqual(Idtf, C4AUL_LocalNamed)) {
          if (a->Def) {
            Shift();
            Parse_Local();
          } else
            throw new C4AulParseError(
                this, "'local' variable declaration in global script");
        }
        // check for variable definition (static)
        else if (SEqual(Idtf, C4AUL_GlobalNamed)) {
          Shift();
          Parse_Static();
        }
        // might be old style declaration
        else {
          const char *SPos0_ = SPos;
          Shift();
          if (TokenType == ATT_COLON) {
            // Reset source position to the point before the label
            SPos = SPos0;
            Shift();
            return;
          } else {
            // The current token might be a label
            // In that case the next round of the loop will need to reset the
            // position to what's in SPos0_ now
            SPos0 = SPos0_;
          }
        }
        break;
      }
      default: {
        SPos0 = SPos;
        Shift();
        break;
      }
    }
}

void C4AulParseState::Parse_Desc() {
  // check for function desc
  if (TokenType == ATT_BOPEN2) {
    // parse for end of desc
    const char *SPos0 = SPos;
    int Len = 0;
    int iBracketsOpen = 1;
    while (true) {
      // another bracket open
      if (*SPos == '[') iBracketsOpen++;
      // a bracket closed
      if (*SPos == ']') iBracketsOpen--;
      // last bracket closed: at end of desc block
      if (iBracketsOpen == 0) break;
      // check for eof
      if (!*SPos)
        // -> function desc not closed
        throw new C4AulParseError(this, "function desc not closed");
      // next char
      SPos++;
      Len++;
    }
    SPos++;
    // extract desc
    Fn->Desc.Copy(SPos0, Len);
    Fn->Script = SPos;
    Shift();
  } else
    Fn->Desc.Clear();
}

void C4AulParseState::Parse_Function() {
  iStack = 0;
  Done = false;
  while (!Done) switch (TokenType) {
      // a block end?
      case ATT_BLCLOSE: {
        // new-form func?
        if (Fn->bNewFormat) {
          // all ok, insert a return
          C4AulBCC *CPos = a->GetCodeByPos(Max(a->GetCodePos() - 1, 0));
          if (!CPos || CPos->bccType != AB_RETURN || fJump) {
            AddBCC(AB_INT);
            AddBCC(AB_RETURN);
          }
          // and break
          Done = TRUE;
          // Do not blame this function for script errors between functions
          Fn = 0;
          return;
        }
        throw new C4AulParseError(this, "no '{' found for '}'");
      }
      case ATT_EOF: {
        Done = TRUE;
        return;
      }
      default: {
        Parse_Statement();
        assert(!iStack);
      }
    }
}

void C4AulParseState::Parse_Block() {
  Match(ATT_BLOPEN);
  // insert block in byte code
  while (1) switch (TokenType) {
      case ATT_BLCLOSE:
        Shift();
        return;
      default: {
        Parse_Statement();
        break;
      }
    }
}
void C4AulParseState::Parse_Statement() {
  switch (TokenType) {
    // do we have a block start?
    case ATT_BLOPEN: {
      Parse_Block();
      return;
    }
    case ATT_BOPEN:
    case ATT_BOPEN2:
    case ATT_OPERATOR:
    case ATT_INT:     // constant in cInt
    case ATT_BOOL:    // constant in cInt
    case ATT_STRING:  // reference in cInt
    case ATT_C4ID:    // converted ID in cInt
    {
      Parse_Expression();
      SetNoRef();
      AddBCC(AB_STACK, -1);
      Match(ATT_SCOLON);
      return;
    }
    // additional function seperator
    case ATT_SCOLON: {
      Shift();
      break;
    }
    case ATT_IDTF: {
      // check for variable definition (var)
      if (SEqual(Idtf, C4AUL_VarNamed)) {
        Shift();
        Parse_Var();
      }
      // check for variable definition (local)
      else if (SEqual(Idtf, C4AUL_LocalNamed)) {
        Shift();
        Parse_Local();
      }
      // check for variable definition (static)
      else if (SEqual(Idtf, C4AUL_GlobalNamed)) {
        Shift();
        Parse_Static();
      }
      // check for parameter
      else if (Fn->ParNamed.GetItemNr(Idtf) != -1) {
        Parse_Expression();
        SetNoRef();
        AddBCC(AB_STACK, -1);
      }
      // check for variable (var)
      else if (Fn->VarNamed.GetItemNr(Idtf) != -1) {
        Parse_Expression();
        SetNoRef();
        AddBCC(AB_STACK, -1);
      }
      // check for objectlocal variable (local)
      else if (a->LocalNamed.GetItemNr(Idtf) != -1) {
        // global func?
        if (Fn->Owner == &Game.ScriptEngine)
          throw new C4AulParseError(this,
                                    "using local variable in global function!");
        // insert variable by id
        Parse_Expression();
        AddBCC(AB_STACK, -1);
      }
      // check for global variable (static)
      else if (a->Engine->GlobalNamedNames.GetItemNr(Idtf) != -1) {
        Parse_Expression();
        AddBCC(AB_STACK, -1);
      }
      // check new-form func begin
      else if (SEqual(Idtf, C4AUL_Func)) {
        // break parsing: start of next func
        Done = TRUE;
        break;
      }
      // get function by identifier: first check special functions
      else if (SEqual(Idtf, C4AUL_If))  // if
      {
        Shift();
        Parse_If();
        break;
      } else if (SEqual(Idtf, C4AUL_Else))  // else
      {
        throw new C4AulParseError(this, "misplaced 'else'");
      } else if (SEqual(Idtf, C4AUL_While))  // while
      {
        Shift();
        Parse_While();
        break;
      } else if (SEqual(Idtf, C4AUL_For))  // for
      {
        Shift();
        // Look if it's the for([var] foo in array)-form
        const char *SPos0 = SPos;
        // must be followed by a bracket
        Match(ATT_BOPEN);
        // optional var
        if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_VarNamed)) Shift();
        // variable and "in"
        if (TokenType ==
                ATT_IDTF /*&& (iVarID = Fn->VarNamed.GetItemNr(Idtf)) != -1*/
            &&
            GetNextToken(Idtf, &cInt, Discard, true) == ATT_IDTF &&
            SEqual(Idtf, C4AUL_In)) {
          // reparse the stuff in the brackets like normal statements
          SPos = SPos0;
          Shift();
          Parse_ForEach();
        } else {
          // reparse the stuff in the brackets like normal statements
          SPos = SPos0;
          Shift();
          Parse_For();
        }
        break;
      } else if (SEqual(Idtf, C4AUL_Return))  // return
      {
        bool multi_params_hack = false;
        Shift();
        if (TokenType == ATT_BOPEN &&
            Fn->pOrgScript->Strict < C4AulScript::STRICT2) {
          // parse return(retvals) - return(retval, unused, parameters, ...)
          // allowed for backwards compatibility
          if (Parse_Params(1, NULL) == 1) {
            // return (1 + 1) * 3 returns 6, not 2
            Parse_Expression2();
          } else
            multi_params_hack = true;
        } else if (TokenType == ATT_SCOLON) {
          // allow return; without return value (implies 0)
          AddBCC(AB_INT);
        } else {
          // return retval;
          Parse_Expression();
        }
        if (!Fn->bReturnRef) SetNoRef();
        AddBCC(AB_RETURN);
        if (multi_params_hack && TokenType != ATT_SCOLON) {
          // return (1, 1) * 3 returns 1
          // but warn about it, the * 3 won't get executed and a stray ',' could
          // lead to unexpected results
          Warn("';' expected, but found ", GetTokenName(TokenType));
          AddBCC(AB_STACK, +1);
          Parse_Expression2();
          AddBCC(AB_STACK, -1);
        }
      } else if (SEqual(Idtf, C4AUL_this))  // this on top level
      {
        Parse_Expression();
        SetNoRef();
        AddBCC(AB_STACK, -1);
      } else if (SEqual(Idtf, C4AUL_Break))  // break
      {
        Shift();
        if (Type == PARSER) {
          // Must be inside a loop
          if (!pLoopStack) {
            Strict2Error("'break' is only allowed inside loops");
          } else {
            // Insert code
            if (pLoopStack->StackSize != iStack)
              AddBCC(AB_STACK, pLoopStack->StackSize - iStack);
            AddLoopControl(true);
            AddBCC(AB_JUMP);
          }
        }
      } else if (SEqual(Idtf, C4AUL_Continue))  // continue
      {
        Shift();
        if (Type == PARSER) {
          // Must be inside a loop
          if (!pLoopStack) {
            Strict2Error("'continue' is only allowed inside loops");
          } else {
            // Insert code
            if (pLoopStack->StackSize != iStack)
              AddBCC(AB_STACK, pLoopStack->StackSize - iStack);
            AddLoopControl(false);
            AddBCC(AB_JUMP);
          }
        }
      } else if (SEqual(Idtf, C4AUL_Var))  // Var
      {
        Parse_Expression();
        SetNoRef();
        AddBCC(AB_STACK, -1);
      } else if (SEqual(Idtf, C4AUL_Par))  // Par
      {
        Parse_Expression();
        SetNoRef();
        AddBCC(AB_STACK, -1);
      } else if (SEqual(Idtf, C4AUL_Inherited) ||
                 SEqual(Idtf, C4AUL_SafeInherited)) {
        Parse_Expression();
        SetNoRef();
        AddBCC(AB_STACK, -1);
      }
      // now this may be the end of the function: first of all check for access
      // directives
      else if (SEqual(Idtf, C4AUL_Private) || SEqual(Idtf, C4AUL_Protected) ||
               SEqual(Idtf, C4AUL_Public) || SEqual(Idtf, C4AUL_Global)) {
        Shift();
        // check if it's followed by a function declaration
        // 'func' idtf?
        if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_Func)) {
          // ok, break here
          Done = TRUE;
        } else {
          // expect function name and colon
          Match(ATT_IDTF);
          Match(ATT_COLON);
          // break here
          Done = TRUE;
        }
        break;
      } else {
        bool gotohack = false;
        // none of these? then it's a function
        // if it's a label, it will be missinterpreted here, which will be
        // corrected later
        // it may be the first goto() found? (old syntax only!)
        if (SEqual(Idtf, C4AUL_Goto) && !Fn->pOrgScript->Strict)
          // add AB_RETURN later on
          gotohack = true;
        C4AulFunc *FoundFn;
        // old syntax: do not allow recursive calls in overloaded functions
        if (!Fn->pOrgScript->Strict && Fn->OwnerOverloaded &&
            SEqual(Idtf, Fn->Name))
          FoundFn = Fn->OwnerOverloaded;
        else
            // get regular function
            if (Fn->Owner == &Game.ScriptEngine)
          FoundFn = a->Owner->GetFuncRecursive(Idtf);
        else
          FoundFn = a->GetFuncRecursive(Idtf);
        // ignore func-not-found errors in the preparser, because the function
        // tables are not built yet
        if (!FoundFn && Type == PARSER) {
          // the function could not be found
          // this *could* be because it's a label to the next function, which,
          // however, is not visible in the current
          // context (e.g., global functions) [Soeren]
          // parsing would have to be aborted anyway, so have a short look at
          // the next token
          if (GetNextToken(Idtf, &cInt, Discard, true) == ATT_COLON) {
            // OK, next function found. abort
            Done = TRUE;
            break;
          }
          // -> func not found
          throw new C4AulParseError(this, "unknown identifier: ", Idtf);
        }
        Shift();
        // check if it's a label - labels like OCF_Living are OK (ugh...)
        if (TokenType == ATT_COLON) {
          // break here
          Done = true;
          return;
        }
        // The preparser assumes the syntax is correct
        if (TokenType == ATT_BOPEN || Type == PARSER)
          Parse_Params(FoundFn ? FoundFn->GetParCount() : 10,
                       FoundFn ? FoundFn->Name : Idtf, FoundFn);
        AddBCC(AB_FUNC, (long)FoundFn);
        if (gotohack) {
          AddBCC(AB_RETURN);
          AddBCC(AB_STACK, +1);
        }
        Parse_Expression2();
        SetNoRef();
        AddBCC(AB_STACK, -1);
      }
      Match(ATT_SCOLON);
      break;
    }
    default: {
      // -> unexpected token
      UnexpectedToken("statement");
    }
  }
}

int C4AulParseState::Parse_Params(int iMaxCnt, const char *sWarn,
                                  C4AulFunc *pFunc) {
  int size = 0;
  // so it's a regular function; force "("
  Match(ATT_BOPEN);
  bool fDone = false;
  do
    switch (TokenType) {
      case ATT_BCLOSE: {
        Shift();
        // () -> size 0, (*,) -> size 2, (*,*,) -> size 3
        if (size > 0) {
          AddBCC(AB_INT, 0);
          ++size;
        }
        fDone = true;
        break;
      }
      case ATT_COMMA: {
        // got no parameter before a ","? then push a 0-constant
        AddBCC(AB_INT, 0);
        Shift();
        ++size;
        break;
      }
      case ATT_LDOTS: {
        Shift();
        // Push all unnamed parameters of the current function as parameters
        int i = Fn->ParNamed.iSize;
        while (size < iMaxCnt && i < C4AUL_MAX_Par) {
          AddBCC(AB_PARN_R, i);
          ++i;
          ++size;
        }
        // Do not allow more parameters even if there is place left
        fDone = true;
        Match(ATT_BCLOSE);
        break;
      }
      default: {
        // get a parameter
        Parse_Expression();
        if (pFunc) {
          bool anyfunctakesref = (pFunc->GetParType()[size] == C4V_pC4Value);
          // pFunc either was the return value from a GetFuncFast-Call or
          // pFunc is the only function that could be called, so this loop is
          // superflous
          C4AulFunc *pFunc2 = pFunc;
          while (pFunc2 = a->Engine->GetNextSNFunc(pFunc2))
            if (pFunc2->GetParType()[size] == C4V_pC4Value)
              anyfunctakesref = true;
          // Change the bytecode to the equivalent that does not produce a
          // reference.
          if (!anyfunctakesref) SetNoRef();
          /*C4V_Type from = C4V_Any;
          switch((a->CPos-1)->bccType)
                  {
                  case AB_C4ID: from = C4V_C4ID; break;
                  case AB_INT: from = C4V_Int; break;
                  case AB_STRING: from = C4V_String; break;
                  case AB_ARRAY: from = C4V_Array; break;
                  case AB_BOOL: from = C4V_Bool; break;
                  case AB_UNOP: case AB_BINOP: from =
          C4ScriptOpMap[(a->CPos-1)->bccX].RetType; break;
                  case AB_FUNC: case AB_CALL: case AB_CALLFS:
                          if((a->CPos-1)->bccX) from =
          reinterpret_cast<C4AulFunc *>((a->CPos-1)->bccX)->GetRetType(); break;
                  case AB_ARRAYA_R: case AB_PAR_R: case AB_VAR_R: case
          AB_PARN_R: case AB_VARN_R: case AB_LOCALN_R: case AB_GLOBALN_R:
                          from = C4V_pC4Value; break;
                  }
          C4V_Type to = pFunc->GetParType()[size];
          // TODO: Check wether from could be converted to to, but take every
          pFunc2 into account
          */
        }
        ++size;
        // end of parameter list?
        if (TokenType == ATT_COMMA)
          Shift();
        else if (TokenType == ATT_BCLOSE) {
          Shift();
          fDone = true;
        } else
          UnexpectedToken("',' or ')'");
        break;
      }
    }
  while (!fDone);
  // too many parameters?
  if (sWarn && size > iMaxCnt && Type == PARSER)
    Warn(FormatString("%s: passing %d parameters, but only %d are used", sWarn,
                      size, iMaxCnt).getData(),
         NULL);
  // Balance stack
  if (size != iMaxCnt) AddBCC(AB_STACK, iMaxCnt - size);
  return size;
}

void C4AulParseState::Parse_Array() {
  // force "["
  Match(ATT_BOPEN2);
  // Create an array
  int size = 0;
  bool fDone = false;
  do
    switch (TokenType) {
      case ATT_BCLOSE2: {
        Shift();
        // [] -> size 0, [*,] -> size 2, [*,*,] -> size 3
        if (size > 0) {
          AddBCC(AB_INT);
          ++size;
        }
        fDone = true;
        break;
      }
      case ATT_COMMA: {
        // got no parameter before a ","? then push a 0-constant
        AddBCC(AB_INT);
        Shift();
        ++size;
        break;
      }
      default: {
        Parse_Expression();
        ++size;
        if (TokenType == ATT_COMMA)
          Shift();
        else if (TokenType == ATT_BCLOSE2) {
          Shift();
          fDone = true;
          break;
        } else
          UnexpectedToken("',' or ']'");
      }
    }
  while (!fDone);
  // add terminator
  AddBCC(AB_ARRAY, size);
}

void C4AulParseState::Parse_While() {
  // Save position for later jump back
  int iStart = JumpHere();
  // Execute condition
  if (Fn->pOrgScript->Strict >= C4AulScript::STRICT2) {
    Match(ATT_BOPEN);
    Parse_Expression();
    Match(ATT_BCLOSE);
  } else
    Parse_Params(1, C4AUL_While);
  SetNoRef();
  // Check condition
  int iCond = a->GetCodePos();
  AddBCC(AB_CONDN);
  // We got a loop
  PushLoop();
  // Execute body
  Parse_Statement();
  if (Type != PARSER) return;
  // Jump back
  AddJump(AB_JUMP, iStart);
  // Set target for conditional jump
  SetJumpHere(iCond);
  // Set targets for break/continue
  for (Loop::Control *pCtrl = pLoopStack->Controls; pCtrl; pCtrl = pCtrl->Next)
    if (pCtrl->Break)
      SetJumpHere(pCtrl->Pos);
    else
      SetJump(pCtrl->Pos, iStart);
  PopLoop();
}

void C4AulParseState::Parse_If() {
  if (Fn->pOrgScript->Strict >= C4AulScript::STRICT2) {
    Match(ATT_BOPEN);
    Parse_Expression();
    Match(ATT_BCLOSE);
  } else
    Parse_Params(1, C4AUL_If);
  // create bytecode, remember position
  int iCond = a->GetCodePos();
  AddBCC(AB_CONDN);
  // parse controlled statement
  Parse_Statement();
  if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_Else)) {
    // add jump
    int iJump = a->GetCodePos();
    AddBCC(AB_JUMP);
    // set condition jump target
    SetJumpHere(iCond);
    Shift();
    // expect a command now
    Parse_Statement();
    // set jump target
    SetJumpHere(iJump);
  } else
    // set condition jump target
    SetJumpHere(iCond);
}

void C4AulParseState::Parse_For() {
  // Initialization
  if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_VarNamed)) {
    Shift();
    Parse_Var();
  } else if (TokenType != ATT_SCOLON) {
    Parse_Expression();
    SetNoRef();
    AddBCC(AB_STACK, -1);
  }
  // Consume first semicolon
  Match(ATT_SCOLON);
  // Condition
  int iCondition = -1, iJumpBody = -1, iJumpOut = -1;
  if (TokenType != ATT_SCOLON) {
    // Add condition code
    iCondition = JumpHere();
    Parse_Expression();
    SetNoRef();
    // Jump out
    iJumpOut = a->GetCodePos();
    AddBCC(AB_CONDN);
  }
  // Consume second semicolon
  Match(ATT_SCOLON);
  // Incrementor
  int iIncrementor = -1;
  if (TokenType != ATT_BCLOSE) {
    // Must jump over incrementor
    iJumpBody = a->GetCodePos();
    AddBCC(AB_JUMP);
    // Add incrementor code
    iIncrementor = JumpHere();
    Parse_Expression();
    SetNoRef();
    AddBCC(AB_STACK, -1);
    // Jump to condition
    if (iCondition != -1) AddJump(AB_JUMP, iCondition);
  }
  // Consume closing bracket
  Match(ATT_BCLOSE);
  // Allow break/continue from now on
  PushLoop();
  // Body
  int iBody = JumpHere();
  if (iJumpBody != -1) SetJumpHere(iJumpBody);
  Parse_Statement();
  if (Type != PARSER) return;
  // Where to jump back?
  int iJumpBack;
  if (iIncrementor != -1)
    iJumpBack = iIncrementor;
  else if (iCondition != -1)
    iJumpBack = iCondition;
  else
    iJumpBack = iBody;
  AddJump(AB_JUMP, iJumpBack);
  // Set target for condition
  if (iJumpOut != -1) SetJumpHere(iJumpOut);
  // Set targets for break/continue
  for (Loop::Control *pCtrl = pLoopStack->Controls; pCtrl; pCtrl = pCtrl->Next)
    if (pCtrl->Break)
      SetJumpHere(pCtrl->Pos);
    else
      SetJump(pCtrl->Pos, iJumpBack);
  PopLoop();
}

void C4AulParseState::Parse_ForEach() {
  if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_VarNamed)) {
    Shift();
  }
  // get variable name
  if (TokenType != ATT_IDTF) UnexpectedToken("variable name");
  if (Type == PREPARSER) {
    // insert variable
    Fn->VarNamed.AddName(Idtf);
  }
  // search variable (fail if not found)
  int iVarID = Fn->VarNamed.GetItemNr(Idtf);
  if (iVarID < 0)
    throw new C4AulParseError(
        this,
        "internal error: var definition: var not found in variable table");
  Shift();
  if (TokenType != ATT_IDTF || !SEqual(Idtf, C4AUL_In)) UnexpectedToken("'in'");
  Shift();
  // get expression for array
  Parse_Expression();
  Match(ATT_BCLOSE);
  // push initial position (0)
  AddBCC(AB_INT);
  // get array element
  int iStart = a->GetCodePos();
  AddBCC(AB_FOREACH_NEXT, iVarID);
  // jump out (FOREACH_NEXT will jump over this if
  // we're not at the end of the array yet)
  int iCond = a->GetCodePos();
  AddBCC(AB_JUMP);
  // got a loop...
  PushLoop();
  // loop body
  Parse_Statement();
  if (Type != PARSER) return;
  // jump back
  AddJump(AB_JUMP, iStart);
  // set condition jump target
  SetJumpHere(iCond);
  // set jump targets for break/continue
  for (Loop::Control *pCtrl = pLoopStack->Controls; pCtrl; pCtrl = pCtrl->Next)
    if (pCtrl->Break)
      SetJumpHere(pCtrl->Pos);
    else
      SetJump(pCtrl->Pos, iStart);
  PopLoop();
  // remove array and counter from stack
  AddBCC(AB_STACK, -2);
}

void C4AulParseState::Parse_Expression(int iParentPrio) {
  switch (TokenType) {
    case ATT_IDTF: {
      // check for parameter (par)
      if (Fn->ParNamed.GetItemNr(Idtf) != -1) {
        // insert variable by id
        AddBCC(AB_PARN_R, Fn->ParNamed.GetItemNr(Idtf));
        Shift();
      }
      // check for variable (var)
      else if (Fn->VarNamed.GetItemNr(Idtf) != -1) {
        // insert variable by id
        AddBCC(AB_VARN_R, Fn->VarNamed.GetItemNr(Idtf));
        Shift();
      }
      // check for variable (local)
      else if (a->LocalNamed.GetItemNr(Idtf) != -1) {
        // global func?
        if (Fn->Owner == &Game.ScriptEngine)
          throw new C4AulParseError(this,
                                    "using local variable in global function!");
        // insert variable by id
        AddBCC(AB_LOCALN_R, a->LocalNamed.GetItemNr(Idtf));
        Shift();
      }
      // check for global variable (static)
      else if (a->Engine->GlobalNamedNames.GetItemNr(Idtf) != -1) {
        // insert variable by id
        AddBCC(AB_GLOBALN_R, a->Engine->GlobalNamedNames.GetItemNr(Idtf));
        Shift();
      }
      // function identifier: check special functions
      else if (SEqual(Idtf, C4AUL_If))
        // -> if is not a valid parameter
        throw new C4AulParseError(this, "'if' may not be used as a parameter");
      else if (SEqual(Idtf, C4AUL_While))
        // -> while is not a valid parameter
        throw new C4AulParseError(this,
                                  "'while' may not be used as a parameter");
      else if (SEqual(Idtf, C4AUL_Else))
        // -> else is not a valid parameter
        throw new C4AulParseError(this, "misplaced 'else'");
      else if (SEqual(Idtf, C4AUL_For))
        // -> for is not a valid parameter
        throw new C4AulParseError(this, "'for' may not be used as a parameter");
      else if (SEqual(Idtf, C4AUL_Return)) {
        // return: treat as regular function with special byte code
        Strict2Error("return used as a parameter", 0);
        Shift();
        Parse_Params(1, NULL);
        AddBCC(AB_RETURN);
        AddBCC(AB_STACK, +1);
      } else if (SEqual(Idtf, C4AUL_Par)) {
        // and for Par
        Shift();
        Parse_Params(1, C4AUL_Par);
        AddBCC(AB_PAR_R);
      } else if (SEqual(Idtf, C4AUL_Var)) {
        // same for Var
        Shift();
        Parse_Params(1, C4AUL_Var);
        AddBCC(AB_VAR_R);
      } else if (SEqual(Idtf, C4AUL_Inherited) ||
                 SEqual(Idtf, C4AUL_SafeInherited)) {
        Shift();
        // inherited keyword: check strict syntax
        if (!Fn->pOrgScript->Strict)
          throw new C4AulParseError(this,
                                    "inherited disabled; use #strict syntax!");
        // get function
        if (Fn->OwnerOverloaded) {
          // add direct call to byte code
          Parse_Params(Fn->OwnerOverloaded->GetParCount(), NULL,
                       Fn->OwnerOverloaded);
          AddBCC(AB_FUNC, (long)Fn->OwnerOverloaded);
        } else
            // not found? raise an error, if it's not a safe call
            if (SEqual(Idtf, C4AUL_Inherited) && Type == PARSER)
          throw new C4AulParseError(
              this,
              "inherited function not found, use _inherited to call failsafe");
        else {
          // otherwise, parse parameters, but discard them
          Parse_Params(0, NULL);
          // Push a null as return value
          AddBCC(AB_STACK, 1);
        }
      } else if (!Fn->pOrgScript->Strict && Fn->OwnerOverloaded &&
                 SEqual(Idtf, Fn->Name)) {
        // old syntax: do not allow recursive calls in overloaded functions
        Shift();
        Parse_Params(Fn->OwnerOverloaded->GetParCount(), Fn->Name, Fn);
        AddBCC(AB_FUNC, (long)Fn->OwnerOverloaded);
      } else {
        C4AulFunc *FoundFn;
        // get regular function
        if (Fn->Owner == &Game.ScriptEngine)
          FoundFn = Fn->Owner->GetFuncRecursive(Idtf);
        else
          FoundFn = a->GetFuncRecursive(Idtf);
        if (Type == PREPARSER) {
          Shift();
          // The preparser just assumes that the syntax is correct: if no '('
          // follows, it must be a constant
          if (TokenType == ATT_BOPEN)
            Parse_Params(FoundFn ? FoundFn->GetParCount() : 10, Idtf, FoundFn);
        } else if (FoundFn) {
          Shift();
          // Function parameters for all functions except "this", which can be
          // used without
          if (!SEqual(FoundFn->Name, C4AUL_this) || TokenType == ATT_BOPEN)
            Parse_Params(FoundFn->GetParCount(), FoundFn->Name, FoundFn);
          else
            AddBCC(AB_STACK, FoundFn->GetParCount());
          AddBCC(AB_FUNC, (long)FoundFn);
        } else {
          // -> func not found
          // check for global constant (static const)
          // global constants have lowest priority for backwards compatibility
          // it is now allowed to have functional overloads of these constants
          C4Value val;
          if (a->Engine->GetGlobalConstant(Idtf, &val)) {
            // store as direct constant
            switch (val.GetType()) {
              case C4V_Int:
                AddBCC(AB_INT, val.GetData().Int);
                break;
              case C4V_Bool:
                AddBCC(AB_BOOL, val.GetData().Int);
                break;
              case C4V_String:
                AddBCC(AB_STRING,
                       reinterpret_cast<intptr_t>(val.GetData().Str));
                break;
              case C4V_C4ID:
                AddBCC(AB_C4ID, val.GetData().Int);
                break;
              case C4V_Any:
                // any: allow zero; add it as int
                if (!val.GetData()) {
                  AddBCC(AB_INT, 0);
                  break;
                }
              // otherwise: fall through to error
              default: {
                throw new C4AulParseError(
                    this,
                    FormatString(
                        "internal error: constant %s has undefined type %d",
                        Idtf, val.GetType()).getData());
              }
            }
            Shift();
            // now let's check whether it used old- or new-style
            if (TokenType == ATT_BOPEN &&
                Fn->pOrgScript->Strict < C4AulScript::STRICT2) {
              // old-style usage: ignore function call
              // must not use parameters here (although generating the byte code
              // for that would be possible)
              Shift();
              Match(ATT_BCLOSE,
                    "parameters not allowed in functional usage of constants");
            }
          } else {
            // identifier could not be resolved
            throw new C4AulParseError(this, "unknown identifier: ", Idtf);
          }
        }
      }
      break;
    }
    case ATT_INT:  // constant in cInt
    {
      AddBCC(AB_INT, cInt);
      Shift();
      break;
    }
    case ATT_BOOL:  // constant in cInt
    {
      AddBCC(AB_BOOL, cInt);
      Shift();
      break;
    }
    case ATT_STRING:  // reference in cInt
    {
      AddBCC(AB_STRING, cInt);
      Shift();
      break;
    }
    case ATT_C4ID:  // converted ID in cInt
    {
      AddBCC(AB_C4ID, cInt);
      Shift();
      break;
    }
    case ATT_OPERATOR: {
      // -> must be a prefix operator
      // get operator ID
      int OpID = cInt;
      // postfix?
      if (C4ScriptOpMap[OpID].Postfix)
        // oops. that's wrong
        throw new C4AulParseError(this,
                                  "postfix operator without first expression");
      Shift();
      // generate code for the following expression
      Parse_Expression(C4ScriptOpMap[OpID].Priority);
      // ignore?
      if (SEqual(C4ScriptOpMap[OpID].Identifier, "+")) break;
      // negate constant?
      if (Type == PARSER && SEqual(C4ScriptOpMap[OpID].Identifier, "-"))
        if ((a->CPos - 1)->bccType == AB_INT) {
          (a->CPos - 1)->bccX = -(a->CPos - 1)->bccX;
          break;
        }
      // write byte code
      AddBCC(C4ScriptOpMap[OpID].Code, OpID);
      break;
    }
    case ATT_BOPEN: {
      // parse it like a function...
      Shift();
      Parse_Expression();
      Match(ATT_BCLOSE);
      break;
    }
    case ATT_BOPEN2: {
      // Arrays are not tested in non-strict mode at all
      if (!Fn->pOrgScript->Strict)
        throw new C4AulParseError(this, "unexpected '['");
      Parse_Array();
      break;
    }
    default: {
      // -> unexpected token
      UnexpectedToken("expression");
    }
  }
  Parse_Expression2(iParentPrio);
}

void C4AulParseState::Parse_Expression2(int iParentPrio) {
  while (1) switch (TokenType) {
      case ATT_OPERATOR: {
        // expect postfix operator
        int OpID = cInt;
        if (!C4ScriptOpMap[OpID].Postfix) {
          // does an operator with the same name exist?
          // when it's a postfix-operator, it can be used instead.
          int nOpID;
          for (nOpID = OpID + 1; C4ScriptOpMap[nOpID].Identifier; nOpID++)
            if (SEqual(C4ScriptOpMap[OpID].Identifier,
                       C4ScriptOpMap[nOpID].Identifier))
              if (C4ScriptOpMap[nOpID].Postfix) break;
          // not found?
          if (!C4ScriptOpMap[nOpID].Identifier) {
            throw new C4AulParseError(this, "unexpected prefix operator: ",
                                      C4ScriptOpMap[OpID].Identifier);
          }
          // otherwise use the new-found correct postfix operator
          OpID = nOpID;
        }
        // lower priority?
        if (C4ScriptOpMap[OpID].RightAssociative
                ? C4ScriptOpMap[OpID].Priority < iParentPrio
                : C4ScriptOpMap[OpID].Priority <= iParentPrio)
          return;
        // If the operator does not modify the first argument, no reference is
        // necessary
        if (C4ScriptOpMap[OpID].Type1 != C4V_pC4Value) SetNoRef();
        Shift();

        if ((C4ScriptOpMap[OpID].Code == AB_And ||
             C4ScriptOpMap[OpID].Code == AB_Or) &&
            Fn->pOrgScript->Strict >= C4AulScript::STRICT2) {
          // create bytecode, remember position
          int iCond = a->GetCodePos();
          // Jump or discard first parameter
          AddBCC(C4ScriptOpMap[OpID].Code == AB_And ? AB_JUMPAND : AB_JUMPOR);
          // parse second expression
          Parse_Expression(C4ScriptOpMap[OpID].Priority);
          // set condition jump target
          SetJumpHere(iCond);
          break;
        } else {
          // expect second parameter for operator
          if (!C4ScriptOpMap[OpID].NoSecondStatement) {
            switch (TokenType) {
              case ATT_IDTF:
              case ATT_INT:
              case ATT_BOOL:
              case ATT_STRING:
              case ATT_C4ID:
              case ATT_OPERATOR:
              case ATT_BOPEN:
              case ATT_BOPEN2:
                Parse_Expression(C4ScriptOpMap[OpID].Priority);
                // If the operator does not modify the second argument, no
                // reference is necessary
                if (C4ScriptOpMap[OpID].Type2 != C4V_pC4Value) SetNoRef();
                break;
              default:
                // Stuff like foo(42+,1) used to silently work
                Strict2Error(
                    FormatString(
                        "Operator %s: Second expression expected, but %s found",
                        C4ScriptOpMap[OpID].Identifier,
                        GetTokenName(TokenType)).getData(),
                    NULL);
                AddBCC(AB_INT, 0);
                break;
            }
          }
          // write byte code, with a few backward compat changes
          if (C4ScriptOpMap[OpID].Code == AB_Equal &&
              Fn->pOrgScript->Strict < C4AulScript::STRICT2)
            AddBCC(AB_EqualIdent, OpID);
          else if (C4ScriptOpMap[OpID].Code == AB_NotEqual &&
                   Fn->pOrgScript->Strict < C4AulScript::STRICT2)
            AddBCC(AB_NotEqualIdent, OpID);
          else
            AddBCC(C4ScriptOpMap[OpID].Code, OpID);
        }
        break;
      }
      case ATT_BOPEN2: {
        // Arrays are not tested in non-strict mode at all
        if (!Fn->pOrgScript->Strict)
          throw new C4AulParseError(this, "unexpected '['");
        // Access the array
        Shift();
        Parse_Expression();
        Match(ATT_BCLOSE2);
        AddBCC(AB_ARRAYA_R);
        break;
      }
      case ATT_CALL: {
        // Here, a '~' is not an operator, but a token
        Shift(Discard, false);
        // C4ID -> namespace given
        C4AulFunc *pFunc = NULL;
        C4AulBCCType eCallType = AB_CALL;
        C4ID idNS = 0;
        if (TokenType == ATT_C4ID) {
          // from now on, stupid func names must stay outside ;P
          idNS = (C4ID)cInt;
          Shift();
          // expect namespace-operator now
          Match(ATT_DCOLON);
          // next, we need a function name
          if (TokenType != ATT_IDTF) UnexpectedToken("function name");
          if (Type == PARSER) {
            // get def from id
            C4Def *pDef = C4Id2Def(idNS);
            if (!pDef) {
              throw new C4AulParseError(
                  this, "direct object call: def not found: ", C4IdText(idNS));
            }
            // search func
            if (!(pFunc = pDef->Script.GetSFunc(Idtf))) {
              throw new C4AulParseError(
                  this,
                  FormatString("direct object call: function %s::%s not found",
                               C4IdText(idNS), Idtf).getData());
            }
            // write namespace chunk to byte code
            AddBCC(AB_CALLNS, (int)idNS);
          }
        } else {
          // may it be a failsafe call?
          if (TokenType == ATT_TILDE) {
            // store this and get the next token
            eCallType = AB_CALLFS;
            Shift();
          }
          // expect identifier of called function now
          if (TokenType != ATT_IDTF)
            throw new C4AulParseError(this, "expecting func name after '->'");
          // search a function with the given name
          if (!(pFunc = a->Engine->GetFirstFunc(Idtf))) {
            // not failsafe?
            if (eCallType != AB_CALLFS && Type == PARSER) {
              throw new C4AulParseError(
                  this,
                  FormatString("direct object call: function %s not found",
                               Idtf).getData());
            }
            // otherwise: nothing to call - just execute parameters and discard
            // them
            Shift();
            Parse_Params(0, NULL);
            // remove target from stack, push a zero value as result
            AddBCC(AB_STACK, -1);
            AddBCC(AB_STACK, +1);
            // done
            break;
          }
        }
        // add call chunk
        Shift();
        Parse_Params(C4AUL_MAX_Par, pFunc ? pFunc->Name : 0, pFunc);
        if (idNS != 0) AddBCC(AB_CALLNS, (long)idNS);
        AddBCC(eCallType, (long)pFunc);
        break;
      }
      default: { return; }
    }
}

void C4AulParseState::Parse_Var() {
  while (1) {
    // get desired variable name
    if (TokenType != ATT_IDTF) UnexpectedToken("variable name");
    if (Type == PREPARSER) {
      // insert variable
      Fn->VarNamed.AddName(Idtf);
    }
    // search variable (fail if not found)
    int iVarID = Fn->VarNamed.GetItemNr(Idtf);
    if (iVarID < 0)
      throw new C4AulParseError(
          this,
          "internal error: var definition: var not found in variable table");
    Shift();
    if (TokenType == ATT_OPERATOR) {
      // only accept "="
      int iOpID = cInt;
      if (SEqual(C4ScriptOpMap[iOpID].Identifier, "=")) {
        // insert initialization in byte code
        Shift();
        Parse_Expression();
        AddBCC(AB_IVARN, iVarID);
      } else
        throw new C4AulParseError(this, "unexpected operator");
    }
    switch (TokenType) {
      case ATT_COMMA: {
        Shift();
        break;
      }
      case ATT_SCOLON: {
        return;
      }
      default: { UnexpectedToken("',' or ';'"); }
    }
  }
}

void C4AulParseState::Parse_Local() {
  while (1) {
    if (Type == PREPARSER) {
      // get desired variable name
      if (TokenType != ATT_IDTF) UnexpectedToken("variable name");
      // check: symbol already in use?
      if (a->GetFunc(Idtf))
        throw new C4AulParseError(this,
                                  "variable definition: name already in use");
      // insert variable
      a->LocalNamed.AddName(Idtf);
    }
    Match(ATT_IDTF);
    switch (TokenType) {
      case ATT_COMMA: {
        Shift();
        break;
      }
      case ATT_SCOLON: {
        return;
      }
      default: { UnexpectedToken("',' or ';'"); }
    }
  }
}

void C4AulParseState::Parse_Static() {
  while (1) {
    if (Type == PREPARSER) {
      // get desired variable name
      if (TokenType != ATT_IDTF) UnexpectedToken("variable name");
      // global variable definition
      // check: symbol already in use?
      if (a->Engine->GetFuncRecursive(Idtf))
        throw new C4AulParseError(this,
                                  "variable definition: name already in use");
      if (a->Engine->GetGlobalConstant(Idtf, NULL))
        Strict2Error("constant and variable with name ", Idtf);
      // insert variable if not defined already
      if (a->Engine->GlobalNamedNames.GetItemNr(Idtf) == -1)
        a->Engine->GlobalNamedNames.AddName(Idtf);
    }
    Match(ATT_IDTF);
    switch (TokenType) {
      case ATT_COMMA: {
        Shift();
        break;
      }
      case ATT_SCOLON: {
        return;
      }
      default: { UnexpectedToken("',' or ';'"); }
    }
  }
}

void C4AulParseState::Parse_Const() {
  // get global constant definition(s)
  while (1) {
    char Name[C4AUL_MAX_Identifier] = "";  // current identifier
    // get desired variable name
    if (TokenType != ATT_IDTF) UnexpectedToken("constant name");
    SCopy(Idtf, Name);
    // check func lists - functions of same name are allowed for backwards
    // compatibility
    // (e.g., for overloading constants such as OCF_Living() in chaos scenarios)
    // it is not encouraged though, so better warn
    if (a->Engine->GetFuncRecursive(Idtf))
      Strict2Error("definition of constant hidden by function ", Idtf);
    if (a->Engine->GlobalNamedNames.GetItemNr(Idtf) != -1)
      Strict2Error("constant and variable with name ", Idtf);
    Match(ATT_IDTF);
    // expect '='
    if (TokenType != ATT_OPERATOR ||
        !SEqual(C4ScriptOpMap[cInt].Identifier, "="))
      UnexpectedToken("'='");
    // expect value. Theoretically, something like C4AulScript::ExecOperator
    // could be used here
    // this would allow for definitions like "static const OCF_Edible = 1<<23"
    // However, such stuff should better be generalized, so the preparser (and
    // parser)
    // can evaluate any constant expression, including functions with constant
    // retval (e.g. Sqrt)
    // So allow only direct constants for now.
    // Do not set a string constant to "Hold" (which would delete it in the next
    // UnLink())
    Shift(Ref, false);
    if (Type == PREPARSER) {
      C4Value vGlobalValue;
      switch (TokenType) {
        case ATT_INT:
          vGlobalValue.SetInt(cInt);
          break;
        case ATT_BOOL:
          vGlobalValue.SetBool(!!cInt);
          break;
        case ATT_STRING:
          vGlobalValue.SetString(reinterpret_cast<C4String *>(cInt));
          break;  // increases ref count of C4String in cInt to 1
        case ATT_C4ID:
          vGlobalValue.SetC4ID(cInt);
          break;
        case ATT_IDTF:
          // identifier is only OK if it's another constant
          if (!a->Engine->GetGlobalConstant(Idtf, &vGlobalValue))
            UnexpectedToken("constant value");
          break;
        default:
          UnexpectedToken("constant value");
      }
      // register as constant
      a->Engine->RegisterGlobalConstant(Name, vGlobalValue);
    }
    // expect ',' (next global) or ';' (end of definition) now
    Shift();
    switch (TokenType) {
      case ATT_COMMA: {
        Shift();
        break;
      }
      case ATT_SCOLON: {
        return;
      }
      default: { UnexpectedToken("',' or ';'"); }
    }
  }
}

BOOL C4AulScript::Parse() {
  if (DEBUG_BYTECODE_DUMP) {
    C4ScriptHost *scripthost = 0;
    if (Def) scripthost = &Def->Script;
    if (scripthost)
      LogSilentF("parsing %s...\n", scripthost->GetFilePath());
    else
      LogSilentF("parsing unknown...\n");
  }
  // parse children
  C4AulScript *s = Child0;
  while (s) {
    s->Parse();
    s = s->Next;
  }
  // check state
  if (State != ASS_LINKED) return FALSE;
  // don't parse global funcs again, as they're parsed already through links
  if (this == Engine) return FALSE;
  // delete existing code
  delete[] Code;
  CodeSize = CodeBufSize = 0;
  // reset code and script pos
  CPos = Code;

  // parse script funcs
  C4AulFunc *f;
  for (f = Func0; f; f = f->Next) {
    // check whether it's a script func, or linked to one
    C4AulScriptFunc *Fn;
    if (!(Fn = f->SFunc())) {
      if (f->LinkedTo) Fn = f->LinkedTo->SFunc();
      // do only parse global funcs, because otherwise, the #append-links get
      // parsed (->code overflow)
      if (Fn)
        if (Fn->Owner != Engine) Fn = NULL;
    }
    if (Fn) {
      // parse function
      try {
        ParseFn(Fn);
      } catch (C4AulError *err) {
        // do not show errors for System.c4g scripts that appear to be pure
        // #appendto scripts
        if (Fn->Owner->Def || !Fn->Owner->Appends) {
          // show
          err->show();
          // show a warning if the error is in a remote script
          if (Fn->pOrgScript != this)
            DebugLogF("  (as #appendto/#include to %s)",
                      Fn->Owner->ScriptName.getData());
          // and count (visible only ;) )
          ++Game.ScriptEngine.errCnt;
        }
        delete err;
        // make all jumps that don't have their destination yet jump here
        // intptr_t to make it work on 64bit
        for (intptr_t i = reinterpret_cast<intptr_t>(Fn->Code); i < CPos - Code;
             i++) {
          C4AulBCC *pBCC = Code + i;
          if (pBCC->bccType == AB_JUMP || pBCC->bccType == AB_JUMPAND ||
              pBCC->bccType == AB_JUMPOR || pBCC->bccType == AB_CONDN)
            if (!pBCC->bccX) pBCC->bccX = CPos - Code - i;
        }
        // add an error chunk
        AddBCC(AB_ERR);
      }

      // add seperator
      AddBCC(AB_EOFN);
    }
  }

  // add eof chunk
  AddBCC(AB_EOF);

  // calc absolute code addresses for script funcs
  for (f = Func0; f; f = f->Next) {
    C4AulScriptFunc *Fn;
    if (!(Fn = f->SFunc())) {
      if (f->LinkedTo) Fn = f->LinkedTo->SFunc();
      if (Fn)
        if (Fn->Owner != Engine) Fn = NULL;
    }
    if (Fn) Fn->Code = Code + (long)Fn->Code;
  }

  // save line count
  Engine->lineCnt +=
      SGetLine(Script.getData(), Script.getPtr(Script.getLength()));

  // dump bytecode
  if (DEBUG_BYTECODE_DUMP)
    for (f = Func0; f; f = f->Next) {
      C4AulScriptFunc *Fn;
      if (!(Fn = f->SFunc())) {
        if (f->LinkedTo) Fn = f->LinkedTo->SFunc();
        if (Fn)
          if (Fn->Owner != Engine) Fn = NULL;
      }
      if (Fn) {
        LogSilentF("%s:", Fn->Name);
        for (C4AulBCC *pBCC = Fn->Code;; pBCC++) {
          C4AulBCCType eType = pBCC->bccType;
          long X = pBCC->bccX;
          switch (eType) {
            case AB_FUNC:
            case AB_CALL:
            case AB_CALLFS:
              LogSilentF("%s\t'%s'\n", GetTTName(eType),
                         X ? ((C4AulFunc *)X)->Name : "");
              break;
            case AB_STRING:
              LogSilentF("%s\t'%s'\n", GetTTName(eType),
                         X ? ((C4String *)X)->Data.getData() : "");
              break;
            default:
              LogSilentF("%s\t%ld\n", GetTTName(eType), X);
              break;
          }
          if (eType == AB_EOFN) break;
        }
      }
    }

  // finished
  State = ASS_PARSED;

  return TRUE;
}

void C4AulScript::ParseDescs() {
  // parse children
  C4AulScript *s = Child0;
  while (s) {
    s->ParseDescs();
    s = s->Next;
  }
  // check state
  if (State < ASS_LINKED) return;
  // parse descs of all script funcs
  for (C4AulFunc *f = Func0; f; f = f->Next)
    if (C4AulScriptFunc *Fn = f->SFunc()) Fn->ParseDesc();
}

C4AulScript *C4AulScript::FindFirstNonStrictScript() {
  // self is not #strict?
  if (Script && !Strict) return this;
  // search children
  C4AulScript *pNonStrScr;
  for (C4AulScript *pScr = Child0; pScr; pScr = pScr->Next)
    if (pNonStrScr = pScr->FindFirstNonStrictScript()) return pNonStrScr;
  // nothing found
  return NULL;
}

#undef DEBUG_BYTECODE_DUMP
