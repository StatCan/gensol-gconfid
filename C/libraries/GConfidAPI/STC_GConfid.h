#ifndef _STC_GCONFID_H_
#define _STC_GCONFID_H_

#define STCM_CODE_FIRSTCHARACTER_CHARACTER_SET \
 "0123456789" \
 "abcdefghijklmnopqrstuvwxyz" \
 "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
 "зйиклбавднмопутфцъщыьэя" \
 "ЗЙИКЛБАВДНМОПУТФЦЪЩЫЬЭџ"

#define STCM_CODE_CHARACTER_SET \
 STCM_CODE_FIRSTCHARACTER_CHARACTER_SET \
 "-!@#$%?&(){}[]_+<>,.ґ=«»" \
 "\\\""   //list of characters that must be escaped

#endif
