#ifndef MACROHELPERS_HPP_PREPGEN
#define MACROHELPERS_HPP_PREPGEN

#define __PG_CMP___PARENTHESES__ ()

#define __PG_CMP___EXPAND_0__(__PG_CMP___ARGUMENTS__) __PG_CMP___ARGUMENTS__
#define __PG_CMP___EXPAND_1__(__PG_CMP___ARGUMENTS__)                          \
  __PG_CMP___EXPAND_0__(__PG_CMP___EXPAND_0__(                                 \
      __PG_CMP___EXPAND_0__(__PG_CMP___EXPAND_0__(__PG_CMP___ARGUMENTS__))))
#define __PG_CMP___EXPAND_2__(__PG_CMP___ARGUMENTS__)                          \
  __PG_CMP___EXPAND_1__(__PG_CMP___EXPAND_1__(                                 \
      __PG_CMP___EXPAND_1__(__PG_CMP___EXPAND_1__(__PG_CMP___ARGUMENTS__))))
#define __PG_CMP___EXPAND_3__(__PG_CMP___ARGUMENTS__)                          \
  __PG_CMP___EXPAND_2__(__PG_CMP___EXPAND_2__(                                 \
      __PG_CMP___EXPAND_2__(__PG_CMP___EXPAND_2__(__PG_CMP___ARGUMENTS__))))
#define __PG_CMP___EXPAND_4__(__PG_CMP___ARGUMENTS__)                          \
  __PG_CMP___EXPAND_3__(__PG_CMP___EXPAND_3__(                                 \
      __PG_CMP___EXPAND_3__(__PG_CMP___EXPAND_3__(__PG_CMP___ARGUMENTS__))))

#define __PG_CMP___EXPAND___(__PG_CMP___ARGUMENTS__)                           \
  __PG_CMP___EXPAND_4__(__PG_CMP___ARGUMENTS__)

#define __PG_CMP___REC_FN_makeLambda_A__(body, type, pName, pDesc, ...)        \
  [](work &tx) {                                                               \
    __PG_CMP___EXPAND_makeLambda__(__PG_CMP___REC_FN_makeLambda_B__(           \
        body, type, pName, pDesc, __VA_ARGS__)) body                           \
  }
#define __PG_CMP___REC_FN_makeLambda_B__(body, type, pName, pDesc, ...)        \
  type pName;                                                                  \
  std::cout << "Enter the " pDesc ":\t";                                       \
  readInto<type>(pName, pDesc);                                                \
  __VA_OPT__(__PG_CMP___REC_FN_makeLambda_C__ __PG_CMP___PARENTHESES__(        \
      type, __VA_ARGS__))
#define __PG_CMP___REC_FN_makeLambda_C__() __PG_CMP___REC_FN_makeLambda_B__
#define __PG_CMP___REC_FN_makeLambda__(body, type, pName, pDesc, ...)          \
  __PG_CMP___REC_FN_makeLambda_A__(body, type, pName, pDesc, __VA_ARGS__)
#define makeLambda(body, type, pName, pDesc, ...)                              \
  __PG_CMP___REC_FN_makeLambda__(body, type, pName, pDesc, __VA_ARGS__)

#define __PG_CMP___EXPAND_makeLambda__(__PG_CMP___ARGUMENTS__)                 \
  __PG_CMP___EXPAND___(__PG_CMP___ARGUMENTS__)

#define makeLambda(body, type, pName, pDesc, ...)                              \
  __PG_CMP___REC_FN_makeLambda__(body, type, pName, pDesc, __VA_ARGS__)

#define __PG_CMP___EXPAND_makeLambda__(__PG_CMP___ARGUMENTS__)                 \
  __PG_CMP___EXPAND___(__PG_CMP___ARGUMENTS__)

#endif // MACROHELPERS_HPP_PREPGENPGEN        \
  __PG_CMP___REC_FN_makeLambda_A__(body, type, pName, pDesc, __VA_ARGS__)
